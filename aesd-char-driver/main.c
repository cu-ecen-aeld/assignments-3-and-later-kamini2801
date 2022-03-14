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
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd-circular-buffer.h"

#define TRUE 1
#define FALSE 0

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Kamini Budke");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;


int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev;

	PDEBUG("open");
	
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
	
	filp->private_data = dev;

	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");

	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t no_of_bytes_read = 0;
	size_t entry_offset_byte_rtn = 0;

	struct aesd_dev* device;
	struct aesd_buffer_entry* entry;
	
	printk(KERN_ALERT "read %zu bytes with offset %lld",count,*f_pos);
	device = (struct aesd_dev*)filp->private_data;
	entry= device->entry;
	
	/**
	 * TODO: handle read
	 * privatee_data member has aesd_dev
	 * use copy_to_user to access buffer
	 * count -> max number of writes0
	 * f_pos pointer to the read offset which has value char_offset (assignment 7)
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
	
	//Acquire lock before accessing buffer
	if (mutex_lock_interruptible(&device->aesdchar_mutex) != 0){

		printk(KERN_ALERT "Mutex lock failed\n");
		return -ERESTARTSYS;

	}
	
	entry = aesd_circular_buffer_find_entry_offset_for_fpos(&device->circular_buffer, *f_pos, &entry_offset_byte_rtn);

	//Release lock
	mutex_unlock(&device->aesdchar_mutex);

	if( entry == NULL )
	{
		printk(KERN_ALERT "queue is empty\n");
		buf = NULL;
		return 0;
	}
	
	no_of_bytes_read = entry->size - entry_offset_byte_rtn;
	
	if(copy_to_user( buf, entry->buffptr + entry_offset_byte_rtn, no_of_bytes_read )!= 0)
	{
		printk(KERN_ALERT "copy_to_user failed\n");
		return -EFAULT;
	}

	*f_pos += no_of_bytes_read;

	return no_of_bytes_read;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	
	struct aesd_dev* device = NULL;
	struct aesd_buffer_entry* entry = NULL;
	struct aesd_buffer_entry ret_ptr;
	ssize_t retval = -ENOMEM;
	int i = 0;
	
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

	device = (struct aesd_dev*)filp->private_data;
	
	entry = device->entry;

	device->total_count += count;
	if(!device->write_pending)
	{
		entry->buffptr = (char*)kmalloc(count , GFP_KERNEL);
		if (entry->buffptr == NULL)
		{
			printk(KERN_ALERT "kmalloc failed\n");
			return -ENOMEM;
		}
		PDEBUG("malloced: %p\n", entry->buffptr);
		
		//memset to keep clean string
		memset(entry->buffptr, '\0' , count);

		if(copy_from_user(entry->buffptr, buf, count )!=0)
			return -EFAULT;
		
	}	
	else
	{
		entry->buffptr = (char*)krealloc(entry->buffptr, (device->total_count), GFP_KERNEL);
		
		if (entry->buffptr == NULL)
		{
			printk(KERN_ALERT "krealloc failed\n");
			return -ENOMEM;
		}

		if(copy_from_user(entry->buffptr + device->total_count - count , buf, count)!=0)
		{
			printk(KERN_ALERT "krealloc copy from user failed\n");
			return -EFAULT;
		}
		
	}


	/**
	 * TODO: optimize index
	 */
	for ( i=0; i < device->total_count; i++)
	{

		if(entry->buffptr[i] == '\n')
		{
			entry->size = device->total_count;
			
			//Acquire lock before accessing buffer
			if (mutex_lock_interruptible(&device->aesdchar_mutex) != 0){

				printk(KERN_ALERT "Mutex lock failed\n");
				return -ERESTARTSYS;

			}
			ret_ptr = aesd_circular_buffer_add_entry(&device->circular_buffer, entry);

			mutex_unlock(&device->aesdchar_mutex);

			if(ret_ptr.buffptr != NULL)
			{
				PDEBUG("Freed %p\n", ret_ptr.buffptr);
				kfree(ret_ptr.buffptr);
			}		
			retval = device->total_count; 
			device->total_count = 0;
			device->write_pending = FALSE;
			break;
		}
		else
		{
			device->write_pending = TRUE;
			retval = device->total_count;
		}
	}
	
	#ifdef AESD_DEBUG
		print_buffer(&device->circular_buffer);
	#endif
	
	return count;
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

	PDEBUG("AESD CHAR INIT\n");

	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));
	
	//Initiliaze circular buffer
	aesd_circular_buffer_init(&aesd_device.circular_buffer);

	aesd_device.entry = kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
	if (aesd_device.entry == NULL)
	{
		printk(KERN_ALERT "kmalloc failed\n");
		return -ENOMEM;
	}
	// PDEBUG("malloced at init: %p\n", aesd_device.entry);

	aesd_device.total_count = 0;
	aesd_device.write_pending = FALSE;

	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	uint8_t index;
	struct aesd_buffer_entry *entry;

	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	//free circular buffer
 	AESD_CIRCULAR_BUFFER_FOREACH(entry,&aesd_device.circular_buffer,index) {
 		if(entry->buffptr == NULL)
		 	break;
		kfree(entry->buffptr);
 	}

	//freedevice entry allocated at init
	kfree(aesd_device.entry);

	//destroy mutex
	mutex_destroy(&aesd_device.aesdchar_mutex);

	PDEBUG("AESD CHAR EXIT\n");

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
