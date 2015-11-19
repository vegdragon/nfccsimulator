#include <linux/kfifo.h>
#include <linux/spinlock_types.h>
#include "nci_engine.h"

#define KFF_LEN 512*256 
#define TAG "NCI_ENGINE: "

DECLARE_KFIFO_PTR(nci_fifo, nci_data_t *);

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

