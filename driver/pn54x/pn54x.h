#ifndef _HELLO_ANDROID_H_  
#define _HELLO_ANDROID_H_  
  
#include <linux/cdev.h>  
#include <linux/semaphore.h>  
  
#define PN54X_DEVICE_NODE_NAME  "pn54x"  
#define PN54X_DEVICE_FILE_NAME  "pn54x"  
#define PN54X_DEVICE_PROC_NAME  "pn54x"  
#define PN54X_DEVICE_CLASS_NAME "pn54x"  

#define PN544_MAGIC 0xE9

/*
 * PN544 power control via ioctl
 * PN544_SET_PWR(0): power off
 * PN544_SET_PWR(1): power on
 * PN544_SET_PWR(2): reset and power on with firmware download enabled
 */

#define PWR_OFF 0
#define PWR_ON  1
#define PWR_FW  2

#define CLK_OFF 0
#define CLK_ON  1

#define PN544_SET_PWR   _IOW(PN544_MAGIC, 0x01, unsigned int)
#define PN54X_CLK_REQ   _IOW(PN544_MAGIC, 0x02, unsigned int)


typedef struct pn54x_android_dev {  
    int               val;
    char              data[300];
    struct semaphore  sem;  
    struct cdev       dev;
    wait_queue_head_t read_wq;
    wait_queue_head_t write_wq;
	struct mutex      read_mutex;
	int               is_read_data_ready;
	int               is_write_data_ready;
} pn54x_android_dev_t;  
  
#endif  
