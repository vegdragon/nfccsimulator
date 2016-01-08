#include <linux/init.h> 
#include <linux/module.h>  
#include <linux/types.h>  
#include <linux/fs.h>  
#include <linux/proc_fs.h>  
#include <linux/device.h>  
#include <linux/sched.h>
#include <linux/delay.h>

#include <asm/uaccess.h>  

#include "pn54x.h"
#include "nci_engine.h"

/* major/minor dev version no. */  
static int pn54x_major = 0;  
static int pn54x_minor = 0;  
  
/* class & device */  
static struct class* pn54x_class = NULL;  
static pn54x_android_dev_t* pn54x_dev = NULL;  
  
/* device file operations */  
static int pn54x_open(struct inode* inode, struct file* filp);  
static int pn54x_release(struct inode* inode, struct file* filp);  
static ssize_t pn54x_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos);  
static ssize_t pn54x_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos);  
      
static long  pn54x_dev_ioctl(struct file *filp, unsigned int cmd,
        unsigned long arg);

/* device file operation func table */  
static struct file_operations pn54x_fops = {  
    .owner = THIS_MODULE,  
    .open = pn54x_open,  
    .release = pn54x_release,  
    .read = pn54x_read,  
    .write = pn54x_write,
    .unlocked_ioctl = pn54x_dev_ioctl,
};  
  
/* access dev attribute */  
static ssize_t pn54x_val_show(struct device* dev, struct device_attribute* attr,  char* buf);  
static ssize_t pn54x_val_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);  
  
/* define device properties */  
static DEVICE_ATTR(val, S_IRUGO | S_IWUSR, pn54x_val_show, pn54x_val_store);  

/* open the device */  
static int pn54x_open(struct inode* inode, struct file* filp) 
{  
    pn54x_android_dev_t* dev;          

    TRACE_FUNC_ENTER
    /* save dev struct into private data field for further usage */  
    dev = container_of(inode->i_cdev, pn54x_android_dev_t, dev);  
    filp->private_data = dev;

    TRACE_FUNC_EXIT

    return 0;  
}  
  
/* release device file */  
static int pn54x_release(struct inode* inode, struct file* filp) 
{  
    pn54x_android_dev_t* pn54x_dev = filp->private_data; 
    TRACE_FUNC_ENTER

    mutex_destroy(&pn54x_dev->read_mutex);
    // nci_kfifo_release();

    TRACE_FUNC_EXIT
    return 0;  
}  
  
/* read nci response from the queue */  
static ssize_t pn54x_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos) 
{  
    ssize_t err = 0;  
    pn54x_android_dev_t* pn54x_dev = filp->private_data;    
    nci_data_t * pNciData = NULL;
    int ret = 0;

    TRACE_FUNC_ENTER

    if (NULL==pn54x_dev || !pn54x_dev->is_ready_to_go)
    {
        goto exit;
    }
        
    /* sync the access */  
    // mutex_lock(&pn54x_dev->read_mutex);  
    pn54x_dev->is_reading = true;
	wake_up(&pn54x_dev->is_reading_wq);

    while (1)
    {
        pr_warning("%s: waiting... is_read_data_ready=%d\n", __func__, pn54x_dev->is_read_data_ready);
        
		ret = wait_event_interruptible(
				pn54x_dev->read_wq,
				pn54x_dev->is_read_data_ready
				);
        pn54x_dev->is_read_data_ready = false;
        pr_warning("%s: wake up. is_read_data_ready=%d\n", __func__, pn54x_dev->is_read_data_ready);
        /*
		if (ret)
        {
            printk(KERN_ALERT "wait_event_interruptible returned error, ret=%d\n", ret);
			goto exit;
        }
        */

        /* pop data from nci queue */
        pNciData = getNciReadData();

        if (NULL == pNciData)
        {
            printk(KERN_ALERT "getNciReadData empty!\n");
            err = -1;
            goto exit;
        }
        else
        {
            printk(KERN_ALERT "clearNciReadData!\n");
            clearNciReadData();
            break;
        }
    }
    
    /* copy data to user buffer */  
    ret = copy_to_user(buf, pNciData->data, pNciData->len);
    if(ret) 
    {  
        printk(KERN_ALERT "pn54x_read: copy_to_user failed: ret=%d.\n", ret);
        err = -EFAULT;  
        goto exit;  
    }

    printk(KERN_ALERT "read nci data: delay=%ld, len=%d\n", pNciData->delay, pNciData->len);
    //print_nci_data(pNciData);
    err = pNciData->len;
    
    /* simulate the delay nfcc response to the mw */
    msleep (pNciData->delay);
 
exit: 
    // mutex_unlock(&pn54x_dev->read_mutex); 

    TRACE_FUNC_EXIT_VALUE(err);
        
    return err;  
}  
  
/* write to pn54x driver */  
static ssize_t pn54x_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos) 
{  
    pn54x_android_dev_t* pn54x_dev = filp->private_data;  
    int ret = 0; 
    TRACE_FUNC_ENTER

    if (NULL==pn54x_dev || !pn54x_dev->is_ready_to_go)
    {
        goto exit;
    }
    
    /* sync the access */  
    // mutex_lock(&pn54x_dev->read_mutex);            
    
    /* get nci cmd sent from nfc mw */ 
    if(copy_from_user(&(pn54x_dev->data), buf, count)) 
    {  
        ret = -EFAULT;  
        goto exit;
    }

    pn54x_dev->is_write_data_ready = true;
    wake_up (&pn54x_dev->write_wq);

	pr_warning("%s: waiting... is_write_complete=%d\n", __func__, pn54x_dev->is_write_complete);
	ret = wait_event_interruptible(
			pn54x_dev->write_complete_wq,
			pn54x_dev->is_write_complete
			);
    pn54x_dev->is_write_complete = false;
    pr_warning("%s: wake up. is_write_complete=%d\n", __func__, pn54x_dev->is_write_complete);

    ret = count;
  
exit:
    // mutex_unlock(&pn54x_dev->read_mutex);  
    TRACE_FUNC_EXIT_VALUE(ret);
        
    return ret;  
}  


/* read val to buffer */  
static ssize_t __pn54x_get_val(pn54x_android_dev_t* dev, char* buf) 
{  
    int val = 0;          

    if(down_interruptible(&(dev->sem))) {                  
        return -ERESTARTSYS;          
    }          
  
    val = dev->val;          
    up(&(dev->sem));          
  
    return snprintf(buf, PAGE_SIZE, "%d\n", val);  
}  
  
/* write value in buf to dev register (val). internal usage */  
static ssize_t __pn54x_set_val(pn54x_android_dev_t* dev, const char* buf, size_t count) 
{  
    int val = 0;          
  
    /* convert str to number */          
    val = simple_strtol(buf, NULL, 10);          
  
    /* sync */          
    if(down_interruptible(&(dev->sem))) {                  
        return -ERESTARTSYS;          
    }
  
    dev->val = val;          
    up(&(dev->sem));  
  
    return count;  
}  
  
/* read val */  
static ssize_t pn54x_val_show(struct device* dev, struct device_attribute* attr, char* buf) 
{  
    pn54x_android_dev_t* hdev = (pn54x_android_dev_t*)dev_get_drvdata(dev);          
  
    return __pn54x_get_val(hdev, buf);  
}  
  
/* write val*/  
static ssize_t pn54x_val_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count) 
{   
    pn54x_android_dev_t* hdev = (pn54x_android_dev_t*)dev_get_drvdata(dev);    
      
    return __pn54x_set_val(hdev, buf, count);  
}  

/* read val and save it in page buffer */  
static ssize_t pn54x_proc_read(char* page, char** start, off_t off, int count, int* eof, void* data) 
{  
    if(off > 0) {  
        *eof = 1;  
        return 0;  
    }  
  
    return __pn54x_get_val(pn54x_dev, page);  
}  
  
/* copy buffer value to val */  
static ssize_t pn54x_proc_write(struct file* filp, const char __user *buff, unsigned long len, void* data) {  
    int err = 0;  
    char* page = NULL;  
  
    if(len > PAGE_SIZE) {  
        printk(KERN_ALERT"The buff is too large: %lu.\n", len);
        return -EFAULT;  
    }  
  
    page = (char*)__get_free_page(GFP_KERNEL);  
    if(!page) {                  
        printk(KERN_ALERT"Failed to alloc page.\n");  
        return -ENOMEM;  
    }          
  
    if(copy_from_user(page, buff, len)) {  
        printk(KERN_ALERT"Failed to copy buff from user.\n");                  
        err = -EFAULT;  
        goto out;  
    }  
  
    err = __pn54x_set_val(pn54x_dev, page, len);  
  
out:  
    free_page((unsigned long)page);  
    return err;  
}  
  
/* create /proc/pn54x */  
static void pn54x_create_proc(void) 
{  
    struct proc_dir_entry* entry;  
      
    entry = create_proc_entry(PN54X_DEVICE_PROC_NAME, 0, NULL);  
    if(entry) {  
        // entry->owner = THIS_MODULE;  
        entry->read_proc = pn54x_proc_read;  
        entry->write_proc = pn54x_proc_write;  
    }  
}  
  
/* remove /proc/pn54x */  
static void pn54x_remove_proc(void) {  
    remove_proc_entry(PN54X_DEVICE_PROC_NAME, NULL);  
}  

/* init the device */  
static int  __pn54x_setup_dev(pn54x_android_dev_t* pn54x_dev) {  
    int err;  
    dev_t devno = MKDEV(pn54x_major, pn54x_minor);  
  
    memset(pn54x_dev, 0, sizeof(pn54x_android_dev_t));  
  
    cdev_init(&(pn54x_dev->dev), &pn54x_fops);  
    pn54x_dev->dev.owner = THIS_MODULE;  
    pn54x_dev->dev.ops = &pn54x_fops;          
  
    /* register char device */  
    err = cdev_add(&(pn54x_dev->dev),devno, 1);  
    if(err) 
    {  
        goto exit;
    }          
  
    /* init semphore & dev register(val) */  
    sema_init(&(pn54x_dev->sem), 1);  
    pn54x_dev->val = 0;

    pn54x_dev->is_read_data_ready = false;
    pn54x_dev->is_write_data_ready = false;
    pr_warning("%s: setup. is_read_data_ready=%d\n", __func__, pn54x_dev->is_read_data_ready);

exit:
    return err;  
}
  
/* init driver module */  
static int __init pn54x_init(void){   
    int err = -1;  
    dev_t dev = 0;  
    struct device* temp = NULL;  
  
    printk(KERN_ALERT"Initializing pn54x device.\n");          
  
    /* alloc major/minor verison no. */  
    err = alloc_chrdev_region(&dev, 0, 1, PN54X_DEVICE_NODE_NAME);  
    if(err < 0) {  
        printk(KERN_ALERT"Failed to alloc char dev region.\n");  
        goto fail;  
    }  
  
    pn54x_major = MAJOR(dev);  
    pn54x_minor = MINOR(dev);          
  
    /* alloc dev data structure */  
    pn54x_dev = kmalloc(sizeof(pn54x_android_dev_t), GFP_KERNEL);  
    if(!pn54x_dev) {  
        err = -ENOMEM;  
        printk(KERN_ALERT"Failed to alloc pn54x_dev.\n");  
        goto unregister;  
    }          
  
    /* init device */  
    err = __pn54x_setup_dev(pn54x_dev);  
    if(err) {  
        printk(KERN_ALERT"Failed to setup dev: %d.\n", err);  
        goto cleanup;  
    }          
  
    /*在/sys/class/目录下创建设备类别目录pn54x*/  
    pn54x_class = class_create(THIS_MODULE, PN54X_DEVICE_CLASS_NAME);  
    if(IS_ERR(pn54x_class)) {  
        err = PTR_ERR(pn54x_class);  
        printk(KERN_ALERT"Failed to create pn54x class.\n");  
        goto destroy_cdev;  
    }          
  
    /*在/dev/目录和/sys/class/pn54x目录下分别创建设备文件pn54x*/  
    temp = device_create(pn54x_class, NULL, dev, "%s", PN54X_DEVICE_FILE_NAME);  
    if(IS_ERR(temp)) {  
        err = PTR_ERR(temp);  
        printk(KERN_ALERT"Failed to create pn54x device.");  
        goto destroy_class;  
    }          
  
    /*在/sys/class/pn54x/pn54x目录下创建属性文件val*/  
    err = device_create_file(temp, &dev_attr_val);  
    if(err < 0) {  
        printk(KERN_ALERT"Failed to create attribute val.");                  
        goto destroy_device;  
    }  
  
    dev_set_drvdata(temp, pn54x_dev);          
  
    /*创建/proc/pn54x文件*/  
    pn54x_create_proc();  
  
    /* init mutex and queues */
	init_waitqueue_head(&pn54x_dev->is_reading_wq);
  	init_waitqueue_head(&pn54x_dev->read_wq);
    init_waitqueue_head(&pn54x_dev->write_wq);
	init_waitqueue_head(&pn54x_dev->write_complete_wq);
  	mutex_init(&pn54x_dev->read_mutex);

    pn54x_dev->is_ready_to_go = false;

    /* init fifo for nci commands */
    nci_kfifo_init(); 
    
    printk(KERN_ALERT"Succedded to initialize pn54x device.\n");  
    return 0;  
  
destroy_device:  
    device_destroy(pn54x_class, dev);  
  
destroy_class:  
    class_destroy(pn54x_class);  
  
destroy_cdev:  
    cdev_del(&(pn54x_dev->dev));  
  
cleanup:  
    kfree(pn54x_dev);  
  
unregister:  
    unregister_chrdev_region(MKDEV(pn54x_major, pn54x_minor), 1);  
  
fail:  
    return err;  
}  
  
/* exit driver module */  
static void __exit pn54x_exit(void) {  
    dev_t devno = MKDEV(pn54x_major, pn54x_minor);  
  
    printk(KERN_ALERT"Destroy pn54x device.\n");          
  
    /* remove /proc/pn54x file*/  
    pn54x_remove_proc();          
  
    /* destroy device & class */  
    if(pn54x_class) {  
        device_destroy(pn54x_class, MKDEV(pn54x_major, pn54x_minor));  
        class_destroy(pn54x_class);  
    }          
  
    /* delete char device & release dev memory */  
    if(pn54x_dev) {  
        cdev_del(&(pn54x_dev->dev));  
        kfree(pn54x_dev);  
    }          
  
    /* release device no. */  
    unregister_chrdev_region(devno, 1);  

    nci_engine_stop(pn54x_dev);
}  


static long  pn54x_dev_ioctl(struct file *filp, unsigned int cmd,
        unsigned long arg)
{
  pn54x_android_dev_t * pn54x_dev = filp->private_data;
  nci_data_t * pNciData = NULL;
  int ret = 0;
  
  printk(KERN_ALERT "%s, cmd=%d, arg=%lu\n", __func__, cmd, arg);      

  switch (cmd) {
  case CMD_NCI_FIFO_INIT:
    nci_kfifo_init();
    break;
  case CMD_NCI_FIFO_PUSH:
    pNciData = (nci_data_t*)kmalloc(sizeof(nci_data_t), GFP_KERNEL);
    if (pNciData)
    {
      ret = copy_from_user(pNciData, (nci_data_t *)arg, sizeof(nci_data_t));
      if (ret) break;
      ret = nci_engine_fill (pNciData);
    }
    break;
  case CMD_NCI_ENGINE_START:
    ret = nci_engine_start(pn54x_dev);
    break;
  case CMD_NCI_ENGINE_STOP:
    ret = nci_engine_stop(pn54x_dev);
    break;
  case CMD_NCI_FIFO_RELEASE:
    nci_kfifo_release();
    break;
  case CMD_NCI_FIFO_GETALL:
    while (nci_kfifo_get(&pNciData)>0)
    {
      print_nci_data(pNciData);
    }
    break;
  case PN544_SET_PWR:
  case PN54X_CLK_REQ:
    ret = 0;
    break;

  default:
    pr_err("%s bad ioctl %u\n", __func__, cmd);
    ret = -EINVAL;
  }

  pr_info("%s: ret=%d, exit.\n", __func__, ret);
  
  return ret;
}

MODULE_LICENSE("GPL");  
MODULE_DESCRIPTION("First Android Driver");  
  
module_init(pn54x_init);  
module_exit(pn54x_exit); 
