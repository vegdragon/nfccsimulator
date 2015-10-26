#ifndef _NCI_ENGINE_H_ 
#define _NCI_ENGINE_H_  

#include<linux/init.h>
#include<linux/slab.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/kfifo.h>
#include<linux/time.h>

#define CMD_NCI_FIFO_INIT	0
#define CMD_NCI_FIFO_PUSH	1
#define CMD_NCI_FIFO_RELEASE	2


#define TRACE_FUNC_ENTER printk(KERN_ALERT "%s: entering...\n", __func__);
#define TRACE_FUNC_EXIT printk(KERN_ALERT "%s: exiting...\n", __func__);

typedef struct nci_data
{
    long		timestamp;
    char		data[300];
    int                 len;
} nci_data_t;

int nci_kfifo_init(void);
int nci_kfifo_release(void);
int nci_kfifo_push(nci_data_t * pNciData);

#endif
