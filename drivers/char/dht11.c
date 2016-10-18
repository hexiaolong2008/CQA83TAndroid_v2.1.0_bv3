#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/input/matrix_keypad.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

#define DEVICE_NAME			"dht11"
static script_item_u     		dht11_val;
static script_item_value_type_e         dht11_type;

static char dht11_read_byte(void)
{
	char dht11_byte ;
	unsigned char i ;
	unsigned char temp ;
	
	dht11_byte = 0 ;
	for( i = 0 ; i < 8 ; i ++ )
	{
		temp = 0 ;
		while (!(__gpio_get_value(dht11_val.gpio.gpio)))
		{
			temp ++ ;
			if ( temp > 12 )
				return 1 ;
			udelay (5) ;
		}
		temp = 0 ;
		while (__gpio_get_value(dht11_val.gpio.gpio))
		{
			temp ++ ;
			if ( temp > 20 )
				return 1 ;
			udelay (5) ;
		}
		if (temp > 6)
		{
			dht11_byte <<= 1 ;
			dht11_byte |= 1 ;
		} 
		else
		{
			dht11_byte <<= 1 ;
			dht11_byte |= 0 ;
		}
	}
	return dht11_byte ;
}

static ssize_t dht11_read(struct file* filp, char __user* buf, size_t count, loff_t* f_pos)
{
	unsigned char DataTemp;
	unsigned char i;
	unsigned char err;
	char tempBuf[5];
	
	err = 0 ;
	gpio_direction_output(dht11_val.gpio.gpio , 0);
	msleep (18);
	gpio_direction_output(dht11_val.gpio.gpio , 1);
	udelay (40);
	gpio_direction_input(dht11_val.gpio.gpio);
	if (!err)
	{
		DataTemp = 10 ;
		while (!(__gpio_get_value(dht11_val.gpio.gpio)) && DataTemp)
		{
			DataTemp --;
			udelay (10);
		}
		if(!DataTemp)
		{
			err = 1;
			count = -EFAULT;
		}
	}
	if(!err)
	{
		DataTemp = 10 ;
		while ((__gpio_get_value(dht11_val.gpio.gpio)) && DataTemp)
		{
			DataTemp --;
			udelay ( 10 );
		}
		if(!DataTemp)
		{
			err = 1;
			count = -EFAULT;
		}
	}
	if (!err)
	{
		for(i = 0; i < 5; i ++)
		{
			tempBuf[i] = dht11_read_byte () ;
		}
		DataTemp = 0 ;
		for(i = 0; i < 4; i ++)
		{
			DataTemp += tempBuf[i] ;
		}
		if(DataTemp != tempBuf[4])
		{
			count = -EFAULT;
		}
		if(count > 5)
		{
			count = 5 ;
		}
		if(copy_to_user(buf , tempBuf , count))
		{
			count = -EFAULT ;
		}
	}
	gpio_direction_output(dht11_val.gpio.gpio , 1);
	return count;
}

static struct file_operations dev_fops = {
	.owner	= THIS_MODULE,
	.read	= dht11_read,
};
static struct miscdevice misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= DEVICE_NAME,
	.fops	= &dev_fops,
};

static int __devinit dht11_probe(struct platform_device *pdev)
{
        int err = -EINVAL;
        script_item_value_type_e  type;
	int dht11_used;
        script_item_u   val;

        type = script_get_item("dht11_para", "dht11_used", &val);
        if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
                printk("%s script_get_item \"dht11_para\" dht11_used = %d\n",
                                __FUNCTION__, val.val);
                return -1;
        }
        dht11_used = val.val;
        printk("%s script_get_item \"dht11_para\" dht11_used = %d\n",
                                __FUNCTION__, val.val);
        if(!dht11_used) {
                printk("%s dht11_used is not used in config,  dht11_used=%d\n", __FUNCTION__,dht11_used);
                return -1;
        }

        dht11_type= script_get_item("dht11_para", "gpio1", &dht11_val);
        if(SCIRPT_ITEM_VALUE_TYPE_PIO != dht11_type) {
                printk("get dht11 gpio fail");
                gpio_free(dht11_val.gpio.gpio);
                return err;
        }
        if(0 != gpio_request(dht11_val.gpio.gpio, NULL)) {
                printk("gpio1 request is fail\n");
                return err;
        }
        err = misc_register(&misc);
	if(0 != err) {
        	printk (DEVICE_NAME "\tinitialized fail\n" );
		return err;
        }

	return 0;
}

static int __devexit dht11_remove(struct platform_device *pdev)
{
	return misc_deregister(&misc);;
}

static struct platform_driver dht11_device_driver = {
        .probe          = dht11_probe,
        .remove         = __devexit_p(dht11_remove),
        .driver         = {
                .name   = "dht11",
                .owner  = THIS_MODULE,
        }
};

static struct platform_device dht11_device = {
        .name           = "dht11",
};

static int __init dht11_init_module ( void )
{
	platform_device_register(&dht11_device);
	platform_driver_register(&dht11_device_driver);
	return 0;
}

static void __exit dht11_exit_module ( void )
{
	platform_driver_unregister(&dht11_device_driver);
}

module_init (dht11_init_module);
module_exit (dht11_exit_module);
MODULE_LICENSE ("GPL") ;
