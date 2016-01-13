#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/spinlock_types.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include "nci_engine.h"

#define KFF_LEN 512*256 
#define TAG "NCI_ENGINE: "

DECLARE_KFIFO_PTR(nci_fifo, nci_data_t *);

static int _is_engine_working = 0;
static nci_data_t * _pNciReadData = NULL;
/* kernel thread task */
static struct task_struct * _task_nci_engine = NULL;

static int nci_engine_thread (void * data);

nci_data_t * getNciReadData()
{
  return _pNciReadData;
}

void clearNciReadData()
{
  _pNciReadData = NULL;
}

int nci_engine_start (pn54x_android_dev_t * pn54x_dev)
{
    int err = 0;

    TRACE_FUNC_ENTER
        
    /* start engin thread */
    _task_nci_engine = kthread_create(nci_engine_thread, pn54x_dev, "nci_engine_task");
    if (IS_ERR(_task_nci_engine))
    {
        printk (KERN_ALERT"Unable to start nci engine thread. \n");
        err = PTR_ERR (_task_nci_engine);
        _task_nci_engine = NULL;
    }
    wake_up_process (_task_nci_engine);

    /* reset data ready flag so that thread can wait before next round of r/w request coming */
    pn54x_dev->is_read_data_ready = false;
	pn54x_dev->is_write_data_ready = false;
	pn54x_dev->is_reading = false;
	pn54x_dev->is_write_complete = false; /* set to true to make write go through at the first time */
    pn54x_dev->is_ready_to_go = true;

    TRACE_FUNC_EXIT
        
    return err;
}


int nci_engine_stop(pn54x_android_dev_t * pn54x_dev)
{
    int err = 0;

	pn54x_dev->is_reading = true;
	wake_up(&pn54x_dev->is_reading_wq);
	pn54x_dev->is_read_data_ready= true;
	wake_up(&pn54x_dev->read_wq);
	pn54x_dev->is_write_data_ready= true;
	wake_up(&pn54x_dev->write_wq);
	pn54x_dev->is_write_complete = true;
	wake_up(&pn54x_dev->write_complete_wq);

    /* exit engine_start thread */
    if (_task_nci_engine)
    {
        err = kthread_stop (_task_nci_engine);
        _task_nci_engine = NULL;
    }
    _is_engine_working = 0;

    pn54x_dev->is_ready_to_go = false;
	
    return err;
}

void print_current_time(int is_new_line)
{
    struct timeval *tv;
    struct tm *t;
    tv = kmalloc(sizeof(struct timeval), GFP_KERNEL);
    t = kmalloc(sizeof(struct tm), GFP_KERNEL);

    do_gettimeofday(tv);
    time_to_tm(tv->tv_sec, 0, t);

    printk(KERN_ALERT "%s%ld-%d-%d %d:%d:%d", TAG,
           t->tm_year + 1900,
           t->tm_mon + 1,
           t->tm_mday,
           (t->tm_hour + 8) % 24,
           t->tm_min,
           t->tm_sec);

    if (is_new_line == 1)
        printk(KERN_ALERT "\n");
    
    kfree(tv);
    kfree(t);
}


/**************************** Other functions *********************************/

/*******************************************************************************
**
** Function         phNxpNciHal_print_packet
**
** Description      Print packet
**
** Returns          None
**
*******************************************************************************/
void print_nci_data(char * data, int len)
{
    uint32_t i, j;
    char print_buffer[len * 3 + 1];

    memset (print_buffer, 0, sizeof(print_buffer));
    for (i = 0; i < len; i++) {
        snprintf(&print_buffer[i * 2], 3, "%02X", data[i]);
    }
    printk("[[==len=%3d > %s==]]\n", len, print_buffer);

    return;
}

int nci_kfifo_init(void)
{
    int ret=0;

    TRACE_FUNC_ENTER;

    // init kfifo
    ret = kfifo_alloc(&nci_fifo, 500, GFP_KERNEL);

    if (ret != 0)
    {
      printk(KERN_ALERT "%skfifo_alloc failed!\n", TAG);
      return ret;
    }

    TRACE_FUNC_EXIT;

    return 0;
}



int nci_kfifo_release(void)
{
    TRACE_FUNC_ENTER;
    kfifo_free(&nci_fifo);
    TRACE_FUNC_EXIT;
    return 0;
}


int nci_kfifo_push(nci_data_t * pNciData)
{
    int ret = 0;

    TRACE_FUNC_ENTER;

    kfifo_put(&nci_fifo, &pNciData);

    printk(KERN_ALERT "%s nci_kfifo_push: current &fifo length is : %d, idx[%d]\n", TAG, kfifo_len(&nci_fifo), pNciData->index);
    print_nci_data(pNciData->data, pNciData->len);

        // ret = kfifo_get(&nci_fifo, &nci_data_tmp);
        // WARN_ON(!ret);

    TRACE_FUNC_EXIT;
    return ret;
}

int nci_kfifo_get(nci_data_t ** ppNciData)
{
    int ret = 0;

    TRACE_FUNC_ENTER;

    ret = kfifo_get(&nci_fifo, ppNciData);
    WARN_ON(!ret);
    printk(KERN_ALERT "%s nci_kfifo_get: current &fifo length is : %d, idx[%d], ret=%d\n", TAG, kfifo_len(&nci_fifo), (*ppNciData)->index, ret);
    print_nci_data((*ppNciData)->data, (*ppNciData)->len);

    TRACE_FUNC_EXIT;
    return kfifo_len(&nci_fifo);
}

int nci_engine_fill (nci_data_t * pNciData)
{
    int ret = 0;
    ret = nci_kfifo_push(pNciData);

    return ret;
}

int nci_engine_thread (void * data)
{
    int ret = 0;  
    nci_data_t * pNciData = NULL;  
    pn54x_android_dev_t * pn54x_dev = (pn54x_android_dev_t*)data;

    TRACE_FUNC_ENTER

    _is_engine_working = 1;
    while (_is_engine_working)
    {
      set_current_state (TASK_UNINTERRUPTIBLE);
      if (kthread_should_stop()) break;

      // read a nci data
      ret = nci_kfifo_get (&pNciData);
      
      if (ret == 0)
      {
        printk(KERN_ALERT"nci_kfifo_get empty: ret=%d.\n", ret);
		pNciData = NULL;
        ret = EFAULT;
        goto exit;
      }

      // check R or X

      // if R, set is_data_ready=true, wait (delay), wakeup, then read the next nci data
      if ('R' == pNciData->direction)
      {
		pr_warning("%s: waiting... is_reading=%d\n", __func__, pn54x_dev->is_reading);
        
		ret = wait_event_interruptible(
				pn54x_dev->is_reading_wq,
				pn54x_dev->is_reading
				);
        pn54x_dev->is_reading = false;
        pr_warning("%s: wake up. is_reading=%d\n", __func__, pn54x_dev->is_reading);
		
        _pNciReadData = pNciData;
        
        if (_pNciReadData != NULL)
            print_nci_data(pNciData->data, pNciData->len);
        else
            pr_warning("%s: _pNciReadData == NULL!!!\n", __func__);
        
		pn54x_dev->is_read_data_ready = true;
        wake_up (&pn54x_dev->read_wq);
      }
      else if ('X' == pNciData->direction)
      {
        // if X, waiting for nci_engine_write, set is_data_ready=false, then read the next nci data
        printk(KERN_ALERT"wait_event_interruptible: is_write_data_ready=%d.\n", pn54x_dev->is_write_data_ready);
        ret = wait_event_interruptible(
            pn54x_dev->write_wq,
            pn54x_dev->is_write_data_ready
            );
        pn54x_dev->is_write_data_ready = false;

        printk("[from usr: ]\n");
        print_nci_data(pn54x_dev->data, pn54x_dev->len);
        
        /* compare received data with cmd in the queue */
        if (memcmp(pn54x_dev->data, pNciData->data, pNciData->len) != 0)
        {
            /* check if it is HCI session id */
            char HCI_SET_SESSION_ID[] = {0x03,0x00,0x0B,0x81,0x01,0x01};
            if (memcmp (pn54x_dev->data, HCI_SET_SESSION_ID, sizeof(HCI_SET_SESSION_ID))==0
                && memcmp (pNciData->data, HCI_SET_SESSION_ID, sizeof(HCI_SET_SESSION_ID))==0)
            {
                printk(KERN_ALERT"Nci cmd data: HCI_SET_SESSION_ID.\n");
            }
            else
            {
                printk(KERN_ALERT"Nci cmd data received is not as we expected!.\n");
                printk("[from queue: idx=%d]\n", pNciData->index);
                print_nci_data(pNciData->data, pNciData->len);

                ret = -EFAULT;
                goto exit;
            }
        }

        /* notify data received, so we can release data to be read */
        // to notifiy in the next start of loop, if ('R' == pNciData->direction);
        printk(KERN_ALERT "%s: pn54x_dev->is_write_complete = true.\n", __FUNCTION__);
        pn54x_dev->is_write_complete = true;
        wake_up (&pn54x_dev->write_complete_wq);

      }
    }

exit:
	pn54x_dev->is_read_data_ready = true;
    wake_up (&pn54x_dev->read_wq);
    pn54x_dev->is_write_complete = true;
    wake_up (&pn54x_dev->write_complete_wq);
	clearNciReadData();

    TRACE_FUNC_EXIT

    return ret;
}
