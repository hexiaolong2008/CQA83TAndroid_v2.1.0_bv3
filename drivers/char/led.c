#include <linux/types.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/input/matrix_keypad.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/miscdevice.h>
#include <linux/printk.h>
#include <linux/kernel.h>

#define LED_IOCTL_SET_ON		1
#define LED_IOCTL_SET_OFF			0
static script_item_u			led_val[5];
static script_item_value_type_e		led_type;
static struct semaphore lock;


static int led_open(struct inode *inode, struct file *file)
{
	if (!down_trylock(&lock))
		return 0;
	else
		return -EBUSY;
}

static int  led_close(struct inode *inode, struct file *file)
{
	up(&lock);
	return 0;
}

static long  led_ioctl(struct file *filep, unsigned int cmd,
		unsigned long arg)
{
	unsigned int n;
	n = (unsigned int)arg;
	switch (cmd) {
		case LED_IOCTL_SET_ON:
			if (n < 1)
				return -EINVAL;
			if(led_val[n-1].gpio.gpio != -1) {
				__gpio_set_value(led_val[n-1].gpio.gpio, 1);
				printk("led%d on !\n", n);
			}
			break;

		case LED_IOCTL_SET_OFF:
		default:
			if (n < 1)
				return -EINVAL;
			if(led_val[n-1].gpio.gpio != -1) {
				__gpio_set_value(led_val[n-1].gpio.gpio, 0);
				printk("led%d off !\n", n);
			}
			break;
	}

	return 0;
}
static int __devinit led_gpio(void)
{
	int i = 0;
	char gpio_num[10];

	for(i =1 ; i < 6; i++) {
		sprintf(gpio_num, "led_gpio%d", i);

		led_type= script_get_item("led_para", gpio_num, &led_val[i-1]);
		if(SCIRPT_ITEM_VALUE_TYPE_PIO != led_type) {
			printk("led_gpio type fail !");
//			gpio_free(led_val[i-1].gpio.gpio);
			led_val[i-1].gpio.gpio	= -1;
			continue;
		}
	
		if(0 != gpio_request(led_val[i-1].gpio.gpio, NULL)) {
			printk("led_gpio gpio_request fail !");		
			led_val[i-1].gpio.gpio = -1;
			continue;
		}

		if (0 != gpio_direction_output(led_val[i-1].gpio.gpio, 0)) {
			printk("led_gpio gpio_direction_output fail !");
//			gpio_free(led_val[i-1].gpio.gpio);
			led_val[i-1].gpio.gpio = -1;
			continue;
		}	
	}

	return 0;
}

static struct file_operations leds_ops = {
	.owner			= THIS_MODULE,
	.open			= led_open,
	.release		= led_close, 
	.unlocked_ioctl		= led_ioctl,
};

static struct miscdevice leds_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "led",
	.fops = &leds_ops,
};

static int __devexit led_remove(struct platform_device *pdev)
{
	return 0;
}

static int __devinit led_probe(struct platform_device *pdev)
{
	int led_used;
	script_item_u	val;
	script_item_value_type_e  type;
	
	int err;
	
    printk("led_para!\n");
	type = script_get_item("led_para", "led_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("%s script_get_item \"led_para\" led_used = %d\n",
				__FUNCTION__, val.val);
		return -1;
	}
	led_used = val.val;
	printk("%s script_get_item \"led_para\" led_used = %d\n",
				__FUNCTION__, val.val);

	if(!led_used) {
		printk("%s led_used is not used in config,  led_used=%d\n", __FUNCTION__,led_used);
		return -1;
	}

	err = led_gpio();
	if (err)
		return -1;

	sema_init(&lock, 1);
	err = misc_register(&leds_dev);
	printk("======= cqa83 led initialized ================\n");
	
	return err;
}


struct platform_device led_device = {
	.name		= "led",	
};
static struct platform_driver led_driver = {
	.probe		= led_probe,
	.remove		= __devexit_p(led_remove),
	.driver		= {
		.name	= "led",
		.owner	= THIS_MODULE,
	},
};


static int __init led_init(void)
{	
	
     if (platform_device_register(&led_device)) {
        printk("%s: register gpio device failed\n", __func__);
    }
    if (platform_driver_register(&led_driver)) {
        printk("%s: register gpio driver failed\n", __func__);
    }

	return 0;
}

static void __exit led_exit(void)
{
	platform_driver_unregister(&led_driver);	
}
module_init(led_init);
module_exit(led_exit);

MODULE_DESCRIPTION("Led Driver");
MODULE_LICENSE("GPL v2");
