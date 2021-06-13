/*
 * Driver for zte battery switch
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>

struct zte_misc_platform_data {
	int ftl_gpio; /*Battery Switch OFF pin*/
	int count;
};
static struct zte_misc_platform_data alt_pdata;

/*Battery Switch Function*/
static int bs_test = 0;
static int poweroff_bs = 0;
static int factory_mode = 0;
int is_factory_mode = 0;
static int bs_test_set(const char *val, struct kernel_param *kp)
{
	int ret;

	ret = param_set_int(val, kp);

	if (ret)
		return ret;
	
	if (bs_test > 1)
		bs_test=1;
	gpio_direction_output(alt_pdata.ftl_gpio,bs_test);
	pr_info("%s:set to%d\n",__func__,bs_test);

	return 0;
}
#define MAX_CHAR	32
static int bs_test_get(char *buf, struct kernel_param *kp)
{
	bs_test = gpio_get_value_cansleep(alt_pdata.ftl_gpio);
	return snprintf(buf, MAX_CHAR, "%d", bs_test);
}

module_param_call(bs_test, bs_test_set, bs_test_get,
			&bs_test, 0644);

/*
 *hardware will poweroff after 7.3second automatically when pin from
 *low to high (rising edge triggered) and holding it high for at least 1 ms
 */
static int poweroff_bs_set(const char *val, struct kernel_param *kp)
{
	int ret;

	ret = param_set_int(val, kp);

	if (ret)
		return ret;
	
	if (poweroff_bs != 1)
		return 0 ;
	pr_info("%s:switch down",__func__);
	gpio_direction_output(alt_pdata.ftl_gpio,0);
	msleep(10);
	gpio_direction_output(alt_pdata.ftl_gpio,1);
	return 0;
}
module_param_call(poweroff_bs, poweroff_bs_set, NULL,
			&poweroff_bs, 0644);

static int factory_mode_set(const char *val, struct kernel_param *kp)
{
	int ret;

	ret = param_set_int(val, kp);

	if (ret)
		return ret;
	
	if (factory_mode == 0)
		is_factory_mode = 0;
	else
		is_factory_mode = 1;

	return 0;
}

static int factory_mode_get(char *buffer, struct kernel_param *kp)
{
	return	sprintf(buffer,"%d",is_factory_mode);
}

module_param_call(factory_mode, factory_mode_set, factory_mode_get,
			&factory_mode, 0644);

void set_bs_poweroff(void)
{
	gpio_direction_output(alt_pdata.ftl_gpio,0);
	msleep(10);
	gpio_direction_output(alt_pdata.ftl_gpio,1);
	
}
EXPORT_SYMBOL(set_bs_poweroff);



static struct of_device_id zte_misc_of_match[] = {
	{ .compatible = "zte-misc", },
	{ },
};
MODULE_DEVICE_TABLE(of, zte_misc_of_match);

static int get_devtree_pdata(struct device *dev,
			    struct zte_misc_platform_data *pdata)
{
	struct device_node *node, *pp;
	const char *desc;

	node = dev->of_node;
	if (node == NULL)
		return -ENODEV;

	memset(pdata, 0, sizeof *pdata);

	/* First count the subnodes */
	pdata->count = 0;
	pp = NULL;
	while ((pp = of_get_next_child(node, pp)))
		pdata->count++;

	if (pdata->count == 0)
		return -ENODEV;

	pp = NULL;
	while ((pp = of_get_next_child(node, pp))) {
		if (!of_find_property(pp, "label", NULL)) {
			pdata->count--;
			dev_warn(dev, "Found without labels\n");
			continue;
		}

		desc = of_get_property(pp, "label", NULL);
		pr_info("%s:1 ftl_gpio=%d desc=%s",__func__,pdata->ftl_gpio,desc);
		if (!strcmp(desc,"battery_switch")) {
			pdata->ftl_gpio=of_get_gpio(pp, 0);
			pr_info("%s:2 ftl_gpio=%d desc=%s",__func__,pdata->ftl_gpio,desc);
		}

	}
	return 0;
}

static int __devinit zte_misc_probe(struct platform_device *pdev)
{
	const struct zte_misc_platform_data *pdata = pdev->dev.platform_data;
	struct device *dev = &pdev->dev;
	int error;
	
	pr_info("%s +++++\n",__func__);
	if (!pdata) {
		error = get_devtree_pdata(dev, &alt_pdata);
		if (error)
			return error;
		pdata = &alt_pdata;
	}
	
	if (pdata->ftl_gpio) {
		int rc=gpio_request(pdata->ftl_gpio,"battery_switch");
		 if (rc) {
    			pr_info("%s: unable to request gpio %d (%d)\n",
    					__func__, pdata->ftl_gpio, rc);
    			 return -EIO;
    	}
	}

	pr_info("%s ----\n",__func__);
	return 0;
}

static int __devexit zte_misc_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver zte_misc_device_driver = {
	.probe		= zte_misc_probe,
	.remove		= __devexit_p(zte_misc_remove),
	.driver		= {
		.name	= "zte-misc",
		.owner	= THIS_MODULE,
		.of_match_table = zte_misc_of_match,
	}
};

static int __init zte_misc_init(void)
{
	return platform_driver_register(&zte_misc_device_driver);
}

static void __exit zte_misc_exit(void)
{
	platform_driver_unregister(&zte_misc_device_driver);
}

late_initcall(zte_misc_init);
module_exit(zte_misc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Misc driver for zte");
MODULE_ALIAS("platform:zte-misc");
