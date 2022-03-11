 /**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd-circular-buffer.h"

#define TRUE 1
#define FALSE 0

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Your Name Here"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

struct aesd_circular_buffer history_buffer;

int aesd_open(struct inode *inode, struct file *filp)
{
	PDEBUG("open");
	/**
	 * TODO: handle open
	 * set filp->private_data with aesd_dev device struct
	 * inode->i_cdev with container_of ------- locate within aesd_dev
	 */
	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t no_of_bytes_read = 0;
	size_t *entry_offset_byte_rtn;
	struct aesd_buffer_entry* entry;
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	
	entry = (struct aesd_buffer_entry*)kmalloc(sizeof(struct aesd_buffer_entry*), GFP_KERNEL);
	/**
	 * TODO: handle read
	 * privatee_data member has aesd_dev
	 * use copy_to_user to access buffer
	 * count -> max number of writes0
	 * f_pos pointer to the read offset which has value char_offset (assignment 7)
	 * 
	 * if(return == count) req number of bytes transferred
	 * 
	 * if(0 < return < count) partialread
	 * 
	 * if(return == 0) EOF
	 * 
	 * if(return < 0) Error  ERESTARTSYS. EINTR, EFAULT
	 * 
	 * 
	 * 
	 * PARTIAL READ RULE:
	 *  
	 * Returns a single aesd_circular_buffer_entry and incerement file
	 * offset 
	 * 
	 * User application can retry read to read all the data until all 
	 * available data is read  
	 * (implementation handled by  hhigher level funcs like fread and cat)
	 */
	entry = (struct aesd_buffer_entry*)kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
	if (entry == NULL)
	{
		printk(KERN_ALERT "kmalloc failed\n");
		kfree(entry);
		return ENOMEM;
	}

	entry = aesd_circular_buffer_find_entry_offset_for_fpos(&history_buffer, *f_pos, entry_offset_byte_rtn);
	//////////no_of_bytes_read = entry->size;
	if( entry == NULL )
	{
		kfree(entry);
		return EFAULT;
	}
	
	if( entry->size < count )
	{
		if(copy_to_user( buf, entry->buffptr, no_of_bytes_read )!= 0)
		{
			kfree(entry);
			return EFAULT;
		}
		no_of_bytes_read = entry->size;
	}
	else
	{
		if(copy_to_user( buf, entry->buffptr, count )!= 0)
		{
			kfree(entry);
			return EFAULT;
		}
		no_of_bytes_read = count;
	}

	kfree(entry);
	return no_of_bytes_read;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	
	static uint8_t write_pending;
	static size_t total_count;
	//static char * temp;
	struct aesd_buffer_entry* entry;
	//ssize_t retval = -ENOMEM;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

	
	entry = (struct aesd_buffer_entry*)kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
	if (entry == NULL)
	{
		printk(KERN_ALERT "kmalloc failed\n");
		return ENOMEM;
	}
	total_count += count;
	if(!write_pending)
	{
		entry->buffptr = (char*)kmalloc(count, GFP_KERNEL);
		if (entry->buffptr == NULL)
		{
			printk(KERN_ALERT "kmalloc failed\n");
			kfree(entry);
			return ENOMEM;
		}
		if(copy_from_user(entry->buffptr, buf, count)!=0)
		{
			kfree(entry);
			return EFAULT;
		}
		strncpy(entry->buffptr + count, "\0", 1);
	}	
	else
	{
		entry->buffptr = (char*)krealloc(entry->buffptr, total_count, GFP_KERNEL);
		if (entry->buffptr == NULL)
		{
			printk(KERN_ALERT "krealloc failed\n");
			kfree(entry);
			return ENOMEM;
		}
		if(copy_from_user(entry->buffptr + total_count - count, buf, count)!=0)
		{
			kfree(entry);
			return EFAULT;
		}
		strncpy(entry->buffptr + total_count, "\0", 1);
	}

	if(strchr(entry->buffptr, '\n') != NULL)
	{
		//entry = (struct aesd_buffer_entry*)kmalloc(sizeof(struct aesd_buffer_entry));
		//entry->buffptr = temp;
		entry->size = total_count;
		aesd_circular_buffer_add_entry(&history_buffer, entry);
		total_count = 0;
		write_pending = FALSE;
	}
	else
	{
		write_pending = TRUE;
	}

	/**
	 * TODO: handle write
	 * 
	 * ignore f_pos
	 * 
	 
	 * if(!flag)

	 * 
	 * 
	 * else: 
	 * 
	 * if(return == count) req number of bytes written
	 * 
	 * if(0 < return < count partial write
	 * 
	 * if(return == 0) nothing written
	 * 
	 * if(return < 0) Error  ERESTARTSYS. EINTR, EFAULT
	 * 
	 */
	return total_count;
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{

	
	dev_t dev = 0; 
	int result;

	printk(KERN_ALERT "AESD CHAR INIT\n");

	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 * Initialize members of the structure (locking)
	 */

	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 * 
	 * kfree memeory
	 * unlock
	 */
	printk(KERN_ALERT "AESD CHAR EXIT\n");

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
