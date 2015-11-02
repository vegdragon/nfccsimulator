    #include <linux/init.h>  
    #include <linux/module.h>  
    #include <linux/types.h>  
    #include <linux/fs.h>  
    #include <linux/proc_fs.h>  
    #include <linux/device.h>  
    #include <asm/uaccess.h>  

    #include "pn54x.h"
    #include "nci_engine.h"

    /* major/minor dev version no. */  
    static int pn54x_major = 0;  
    static int pn54x_minor = 0;  
      
    /* class & device */  
    static struct class* pn54x_class = NULL;  
    static struct pn54x_android_dev* pn54x_dev = NULL;  
      
    /* device file operations */  
    static int pn54x_open(struct inode* inode, struct file* filp);  
    static int pn54x_release(struct inode* inode, struct file* filp);  
    static ssize_t pn54x_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos);  
    static ssize_t pn54x_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos);  
      
static long  pn54x_dev_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg);
    /*设备文件操作方法表*/  
    static struct file_operations pn54x_fops = {  
        .owner = THIS_MODULE,  
        .open = pn54x_open,  
        .release = pn54x_release,  
        .read = pn54x_read,  
        .write = pn54x_write,
        .unlocked_ioctl = pn54x_dev_ioctl,
    };  
      
    /*访问设置属性方法*/  
    static ssize_t pn54x_val_show(struct device* dev, struct device_attribute* attr,  char* buf);  
    static ssize_t pn54x_val_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);  
      
    /*定义设备属性*/  
    static DEVICE_ATTR(val, S_IRUGO | S_IWUSR, pn54x_val_show, pn54x_val_store);  

    /*打开设备方法*/  
    static int pn54x_open(struct inode* inode, struct file* filp) {  
        struct pn54x_android_dev* dev;          
          
        /*将自定义设备结构体保存在文件指针的私有数据域中，以便访问设备时拿来用*/  
        dev = container_of(inode->i_cdev, struct pn54x_android_dev, dev);  
        filp->private_data = dev;

        nci_kfifo_init();  
          
        return 0;  
    }  
      
    /*设备文件释放时调用，空实现*/  
    static int pn54x_release(struct inode* inode, struct file* filp) {  
        return 0;  
    }  
      
    /*读取设备的寄存器val的值*/  
    static ssize_t pn54x_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos) 
    {  
        ssize_t err = 0;  
        struct pn54x_android_dev* dev = filp->private_data;    
      
        /*同步访问*/  
        if(down_interruptible(&(dev->sem))) {  
            return -ERESTARTSYS;  
        }  
      
        if(count < sizeof(dev->val)) {  
            goto out;  
        }          
      
        /*将寄存器val的值拷贝到用户提供的缓冲区*/  
        if(copy_to_user(buf, &(dev->val), sizeof(dev->val))) {  
            err = -EFAULT;  
            goto out;  
        }  
      
        err = sizeof(dev->val);  
     
    out:  
        up(&(dev->sem));  
        return err;  
    }  
      
    /*写设备的寄存器值val*/  
    static ssize_t pn54x_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos) {  
        struct pn54x_android_dev* dev = filp->private_data;  
        ssize_t err = 0;          
      
        /*同步访问*/  
        if(down_interruptible(&(dev->sem))) {  
            return -ERESTARTSYS;          
        }          
      
        if(count != sizeof(dev->val)) {  
            goto out;          
        }          
      
        /*将用户提供的缓冲区的值写到设备寄存器去*/  
        if(copy_from_user(&(dev->val), buf, count)) {  
            err = -EFAULT;  
            goto out;  
        }  
      
        err = sizeof(dev->val);  
      
    out:  
        up(&(dev->sem));  
        return err;  
    }  


    /*读取寄存器val的值到缓冲区buf中，内部使用*/  
    static ssize_t __pn54x_get_val(struct pn54x_android_dev* dev, char* buf) {  
        int val = 0;          
      
        /*同步访问*/  
        if(down_interruptible(&(dev->sem))) {                  
            return -ERESTARTSYS;          
        }          
      
        val = dev->val;          
        up(&(dev->sem));          
      
        return snprintf(buf, PAGE_SIZE, "%d\n", val);  
    }  
      
    /*把缓冲区buf的值写到设备寄存器val中去，内部使用*/  
    static ssize_t __pn54x_set_val(struct pn54x_android_dev* dev, const char* buf, size_t count) {  
        int val = 0;          
      
        /*将字符串转换成数字*/          
        val = simple_strtol(buf, NULL, 10);          
      
        /*同步访问*/          
        if(down_interruptible(&(dev->sem))) {                  
            return -ERESTARTSYS;          
        }          
      
        dev->val = val;          
        up(&(dev->sem));  
      
        return count;  
    }  
      
    /*读取设备属性val*/  
    static ssize_t pn54x_val_show(struct device* dev, struct device_attribute* attr, char* buf) {  
        struct pn54x_android_dev* hdev = (struct pn54x_android_dev*)dev_get_drvdata(dev);          
      
        return __pn54x_get_val(hdev, buf);  
    }  
      
    /*写设备属性val*/  
    static ssize_t pn54x_val_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count) {   
        struct pn54x_android_dev* hdev = (struct pn54x_android_dev*)dev_get_drvdata(dev);    
          
        return __pn54x_set_val(hdev, buf, count);  
    }  

    /*读取设备寄存器val的值，保存在page缓冲区中*/  
    static ssize_t pn54x_proc_read(char* page, char** start, off_t off, int count, int* eof, void* data) {  
        if(off > 0) {  
            *eof = 1;  
            return 0;  
        }  
      
        return __pn54x_get_val(pn54x_dev, page);  
    }  
      
    /*把缓冲区的值buff保存到设备寄存器val中去*/  
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
      
        /*先把用户提供的缓冲区值拷贝到内核缓冲区中去*/  
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
      
    /*创建/proc/pn54x文件*/  
    static void pn54x_create_proc(void) {  
        struct proc_dir_entry* entry;  
          
        entry = create_proc_entry(PN54X_DEVICE_PROC_NAME, 0, NULL);  
        if(entry) {  
            // entry->owner = THIS_MODULE;  
            entry->read_proc = pn54x_proc_read;  
            entry->write_proc = pn54x_proc_write;  
        }  
    }  
      
    /*删除/proc/pn54x文件*/  
    static void pn54x_remove_proc(void) {  
        remove_proc_entry(PN54X_DEVICE_PROC_NAME, NULL);  
    }  

/*初始化设备*/  
static int  __pn54x_setup_dev(struct pn54x_android_dev* dev) {  
    int err;  
    dev_t devno = MKDEV(pn54x_major, pn54x_minor);  
  
    memset(dev, 0, sizeof(struct pn54x_android_dev));  
  
    cdev_init(&(dev->dev), &pn54x_fops);  
    dev->dev.owner = THIS_MODULE;  
    dev->dev.ops = &pn54x_fops;          
  
    /*注册字符设备*/  
    err = cdev_add(&(dev->dev),devno, 1);  
    if(err) {  
        return err;  
    }          
  
    /*初始化信号量和寄存器val的值*/  
    sema_init(&(dev->sem), 1);  
    dev->val = 0;  
  
    return 0;  
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
    pn54x_dev = kmalloc(sizeof(struct pn54x_android_dev), GFP_KERNEL);  
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
}  


static long  pn54x_dev_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	// struct pn54x_dev *pn54x_dev = filp->private_data;
        nci_data_t * pNciData = NULL;
	int ret = 0;

	pr_info("%s, cmd=%d, arg=%lu\n", __func__, cmd, arg);
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
			nci_kfifo_push(pNciData);
		}
		break;
	case CMD_NCI_FIFO_RELEASE:
		nci_kfifo_release();
		break;

	default:
		pr_err("%s bad ioctl %u\n", __func__, cmd);
		return -EINVAL;
	}

	return 0;
}

MODULE_LICENSE("GPL");  
MODULE_DESCRIPTION("First Android Driver");  
  
module_init(pn54x_init);  
module_exit(pn54x_exit); 
