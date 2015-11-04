#ifndef _HELLO_ANDROID_H_  
#define _HELLO_ANDROID_H_  
  
#include <linux/cdev.h>  
#include <linux/semaphore.h>  
  
#define PN54X_DEVICE_NODE_NAME  "pn54x"  
#define PN54X_DEVICE_FILE_NAME  "pn54x"  
#define PN54X_DEVICE_PROC_NAME  "pn54x"  
#define PN54X_DEVICE_CLASS_NAME "pn54x"  
  
struct pn54x_android_dev {  
    int               val;
    char              data[300];
    struct semaphore  sem;  
    struct cdev       dev;
    wait_queue_head_t read_wq;
	  struct mutex      read_mutex;
	  int               is_data_ready;
};  
  
#endif  
