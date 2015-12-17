#ifndef _NCI_ENGINE_H_ 
#define _NCI_ENGINE_H_  

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/time.h>
#include "pn54x.h"

#define CMD_NCI_FIFO_INIT	    0
#define CMD_NCI_FIFO_PUSH	    1
#define CMD_NCI_FIFO_RELEASE	2
#define CMD_NCI_ENGINE_START    3
#define CMD_NCI_ENGINE_STOP		4


#define TRACE_FUNC_ENTER printk(KERN_ALERT "%s: entering...\n", __func__);
#define TRACE_FUNC_EXIT printk(KERN_ALERT "%s: exiting...\n", __func__);

typedef struct nci_data
{
    int         index;
    long		timestamp;
    long        delay;
    char        direction;    // 'X': HOST->NFCC    'R':NFCC->HOST
    char		data[300];
    int     	len;
} nci_data_t;

int nci_kfifo_init(void);
int nci_kfifo_release(void);
int nci_kfifo_push(nci_data_t * pNciData);
int nci_kfifo_get(nci_data_t ** ppNciData);

void print_nci_data(nci_data_t * pNci);


nci_data_t * getNciReadData (void);
void clearNciReadData (void);

int nci_engine_fill (nci_data_t * pNciData);
int nci_engine_start (pn54x_android_dev_t * pn54x_dev);
int nci_engine_stop (void);

#endif
