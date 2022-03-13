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

//struct aesd_aesd_device.circular_buffer aesd_device.circular_buffer;

int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev;

	PDEBUG("open");
	/**
	 * TODO: handle open
	 * set filp->private_data with aesd_dev device struct
	 * inode->i_cdev with container_of ------- locate within aesd_dev
	 */

	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
	filp->private_data = dev;

	
	PDEBUG("open end");
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
	size_t entry_offset_byte_rtn = 0;
	//struct aesd_buffer_device->entry* entry;

	struct aesd_dev* device;
	struct aesd_buffer_entry* entry;
	
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
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
	

	entry = aesd_circular_buffer_find_entry_offset_for_fpos(&device->circular_buffer, *f_pos, &entry_offset_byte_rtn);
	if( entry == NULL )
	{
		printk(KERN_ALERT "find_entry_offset_for_fpos failed\n");
		return -EFAULT;
	}
	
	no_of_bytes_read = entry->size - entry_offset_byte_rtn;

	// if( no_of_bytes_read < count )
	// {
	// 	if(copy_to_user( buf, entry->buffptr, no_of_bytes_read )!= 0)
	// 	{
	// 		//kfree(entry);
	// 		return -EFAULT;
	// 	}
	// 	no_of_bytes_read = entry->size;
	// }
	// else
	// {
	// 	if(copy_to_user( buf, entry->buffptr, count )!= 0)
	// 	{
	// 		//kfree(entry);
	// 		return EFAULT;
	// 	}
	// 	no_of_bytes_read = count;
	// }
	
	if(copy_to_user( buf, entry->buffptr, no_of_bytes_read )!= 0)
	{
		printk(KERN_ALERT "copy_to_user failed\n");
		return -EFAULT;
	}

	device->circular_buffer.full = 	FALSE;
	device->circular_buffer.out_offs++;

	if(device->circular_buffer.out_offs > AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
		device->circular_buffer.out_offs=0;
	
	printk(KERN_ALERT "Read end on reading %d bytes\n", no_of_bytes_read);
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
		entry->buffptr = (char*)kmalloc(count, GFP_KERNEL);
		if (entry->buffptr == NULL)
		{
			printk(KERN_ALERT "kmalloc failed\n");
			//kfree(entry);
			return -ENOMEM;
		}
		printk(KERN_ALERT "malloced: %p\n", entry->buffptr);
		if(copy_from_user(entry->buffptr, buf, count)!=0)
		{
			//kfree(entry);
			return -EFAULT;
		}
		//strncpy(entry->buffptr + count, "\0", 1);
	}	
	else
	{
		entry->buffptr = (char*)krealloc(entry->buffptr, device->total_count, GFP_KERNEL);
		if (entry->buffptr == NULL)
		{
			printk(KERN_ALERT "krealloc failed\n");
			//kfree(entry);
			return -ENOMEM;
		}
		if(copy_from_user(entry->buffptr + device->total_count - count, buf, count)!=0)
		{
			//kfree(entry);
			return -EFAULT;
		}
		//strncpy(entry->buffptr + device->total_count, "\0", 1);
	}



	//if(strchr(entry->buffptr, '\n') != NULL)

	/**
	 * TODO: optimize index
	 */
	for ( i=0; i < device->total_count; i++)
	{

		if(entry->buffptr[i] == '\n')
		{
			//entry->size = device->total_count;
			entry->size = i;
			ret_ptr = aesd_circular_buffer_add_entry(&device->circular_buffer, entry);
			if(ret_ptr.buffptr != NULL)
			{
				printk(KERN_ALERT "Freed %p\n", ret_ptr.buffptr);
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
		}
	}
	
	print_buffer(&device->circular_buffer);
	PDEBUG("Write end\n");
	return retval;
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
	
	//Initiliaze circular buffer
	aesd_circular_buffer_init(&aesd_device.circular_buffer);

	aesd_device.entry = kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);

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
