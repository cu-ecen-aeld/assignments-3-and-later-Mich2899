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
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/kernel.h>
#include <linux/slab.h>
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Michelle Joslin Christian"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev; /* device information */
	PDEBUG("open");
	/**
	 * TODO: handle open
	 */

	/*  Find the device */
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);

	/* and use filp->private_data to point to the device data */
	filp->private_data = dev;

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
	ssize_t retval = 0, bytes_to_read = 0;
	struct aesd_dev *dev = filp->private_data;
	const char* buffer_entry_value=NULL;
	struct aesd_buffer_entry* kernel_read_buff = NULL;
	size_t read_pos=0;
	unsigned long read_ret = 0;

	/**
	 * TODO: handle read
	 */
	if(mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

	kernel_read_buff = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &read_pos);

	if(kernel_read_buff == NULL){
		retval = 0;
		goto out;
	}

	if(kernel_read_buff!=NULL){
		bytes_to_read = count + read_pos;

		if(bytes_to_read <= kernel_read_buff->size){
			retval = count;
		}

		else{
			retval = kernel_read_buff->size - read_pos;
		}

	buffer_entry_value = kernel_read_buff->buffptr + read_pos;

		read_ret = copy_to_user(buf, buffer_entry_value, retval);
		if(read_ret!=0){
			retval = -EFAULT;
			goto out;
		}

		*f_pos += retval;
	}

out:
	mutex_unlock(&dev->lock);
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	/********************************************************************************************/
	struct aesd_dev *dev = filp->private_data;
	char* kernel_buff = NULL;
	char* newline_check = NULL;
	const char* aesd_entry_return = NULL;
	unsigned long ret;
	/********************************************************************************************/
	ssize_t retval = -ENOMEM;
	
	/**
	 * TODO: handle write
	 */

	if(mutex_lock_interruptible(&dev->lock))
			return -ERESTARTSYS;

	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	
	if(dev->newline){
		kernel_buff = kmalloc(count, GFP_KERNEL);

		if(kernel_buff == NULL){
			retval = -ENOMEM;
			goto out;
		}

		ret = copy_from_user(kernel_buff, buf, count);
		if(ret!=0){
			PDEBUG("copy from user fail!!!");
			goto out;
		}

		dev->entry.size = count;
	}

	else{
		kernel_buff = kmalloc((dev->entry.size) + count, GFP_KERNEL);
		
		if(kernel_buff == NULL){
			retval = -ENOMEM;
			goto out;
		}

		memcpy(kernel_buff, dev->entry.buffptr, dev->entry.size);

		kfree(dev->entry.buffptr);

		ret = copy_from_user((kernel_buff+ dev->entry.size), buf, count);
		if(ret!=0){
			PDEBUG("copy from user fail!!!");
			goto out;
		}

		dev->entry.size += count;

	}

	dev->entry.buffptr = kernel_buff;

	newline_check = strchr(kernel_buff, '\n');
	if(newline_check == NULL){
		dev->newline = 0;
	}
	else{
		dev->newline = 1;
		aesd_entry_return = aesd_circular_buffer_add_entry(&dev->buffer, &dev->entry);
		if(aesd_entry_return){
			kfree(aesd_entry_return);
		}

		newline_check = NULL;
	}

	retval = count;

out:
	mutex_unlock(&dev->lock);
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
	 */
	aesd_circular_buffer_init(&aesd_device.buffer);

	mutex_init(&aesd_device.lock);
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
	 */

	unregister_chrdev_region(devno, 1);
	aesd_circular_buffer_clean(&aesd_device.buffer);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);