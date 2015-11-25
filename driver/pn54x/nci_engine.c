#include <linux/kfifo.h>
#include <linux/spinlock_types.h>
#include "nci_engine.h"

#define KFF_LEN 512*256 
#define TAG "NCI_ENGINE: "

DECLARE_KFIFO_PTR(nci_fifo, nci_data_t *);

static int g_is_engine_working = 0;
static nci_data_t * _pNciReadData = NULL;

nci_data_t * getNciReadData()
{
  return _pNciReadData;
}

void clearNciReadData()
{
  _pNciReadData = NULL;
}

void nci_engine_stop()
{
  g_is_engine_working = 0;
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

void print_nci_data(nci_data_t * pNciData)
{
    int i;
    if (pNciData != NULL)
    {
      printk(KERN_ALERT "%s=========================\n", TAG);
      // print_current_time(0);
      printk(KERN_ALERT "timestamp=%ld\tdirection=%c\ndata=\n", pNciData->timestamp, pNciData->direction);
      //for (i=0;i<pNci->len;i++)
	  //  printk(KERN_ALERT "%02x", pNciData->data[i]);
	  
      printk("print_nci_data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
              pNciData->data[0],pNciData->data[1],pNciData->data[2],pNciData->data[3],pNciData->data[4],
              pNciData->data[5],pNciData->data[6],pNciData->data[7],pNciData->data[8],pNciData->data[9]);

      printk(KERN_ALERT "%s=========================\n", TAG);
    }
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

    printk(KERN_ALERT "%s nci_kfifo_push: current &fifo length is : %d\n", TAG, kfifo_len(&nci_fifo));

        // ret = kfifo_get(&nci_fifo, &nci_data_tmp);
        // WARN_ON(!ret);
        
    // print_nci_data(pNciData);

    TRACE_FUNC_EXIT;
    return ret;
}

int nci_kfifo_get(nci_data_t ** ppNciData)
{
    int ret = 0;

    TRACE_FUNC_ENTER;

    ret = kfifo_get(&nci_fifo, ppNciData);
    WARN_ON(!ret);
    printk(KERN_ALERT "%s nci_kfifo_get: current &fifo length is : %d\n", TAG, kfifo_len(&nci_fifo));
        
    // print_nci_data(*ppNciData);

    TRACE_FUNC_EXIT;
    return ret;
}

int nci_engine_fill (nci_data_t * pNciData)
{
    int ret = 0;
    ret = nci_kfifo_push(pNciData);

    return ret;
}

int nci_engine_start (pn54x_android_dev* pn54x_dev)
{
    int ret = 0;  
    nci_data_t * pNciData = NULL;  

    g_is_engine_working = 1;
    while (is_engine_working)
    {
      // read a nci data
      ret = nci_kfifo_get (&pNciData);
      if (ret == 0)
      {
        printk(KERN_ALERT"nci_kfifo_get failed: ret=%d.\n", ret);
        ret = EFAULT;
        goto out;
      }

      // check R or X

      // if R, set is_data_ready=true, wait (delay), wakeup, then read the next nci data
      if ('R' == pNciData->direction)
      {
        msleep (pNciData->delay);
        pn54x_dev->is_read_data_ready = true;
        _nciReadData = pNciData;
        wakeup (pn54x_dev->read_wq);
      }
      else if ('X' == pNciData->direction)
      {
        // if X, waiting for nci_engine_write, set is_data_ready=false, then read the next nci data
        ret = wait_event_interruptible(
            pn54x_dev->write_wq,
            pn54x_dev->is_write_data_ready
            );
        pNciData->is_write_data_ready = false;

        printk("from usr(%d): %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", count,
        pn54x_dev->data[0],pn54x_dev->data[1],pn54x_dev->data[2],pn54x_dev->data[3],pn54x_dev->data[4],
        pn54x_dev->data[5],pn54x_dev->data[6],pn54x_dev->data[7],pn54x_dev->data[8],pn54x_dev->data[9]);
    
    
        /* compare received data with cmd in the queue */
        if (memcmp(pn54x_dev->data, pNciData->data, pNciData->len) != 0)
        {
            printk(KERN_ALERT"Nci cmd data received is not as we expected!.\n");
            printk("from queue(%d): %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", pNciData->len,
              pNciData->data[0],pNciData->data[1],pNciData->data[2],pNciData->data[3],pNciData->data[4],
              pNciData->data[5],pNciData->data[6],pNciData->data[7],pNciData->data[8],pNciData->data[9]);

            ret = -EFAULT;
            goto out;
        }
        else
        {
            /* notify data received, so we can release data to be read */
            // to notifiy in the next start of loop, if ('R' == pNciData->direction);
        }
      }
    }

    return ret;
}
