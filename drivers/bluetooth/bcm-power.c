/* Copyright (c) 2009-2010, 2013 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/*
 * Bluetooth Power Switch Module
 * controls power to external Bluetooth device
 * with interface to power management device
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/bluetooth-power.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>

#define BT_PWR_DBG(fmt, arg...)  pr_debug("%s: " fmt "\n" , __func__ , ## arg)
#define BT_PWR_INFO(fmt, arg...) pr_info("%s: " fmt "\n" , __func__ , ## arg)
#define BT_PWR_ERR(fmt, arg...)  pr_err("%s: " fmt "\n" , __func__ , ## arg)


static struct of_device_id bt_power_match_table[] = {
	{	.compatible = "brcm,4339" },
	{}
};

static struct bluetooth_power_platform_data *bt_power_pdata;
static struct platform_device *btpdev;
static bool previous;



static int bt_configure_gpios(int on)
{
	int rc = 0;
	int bt_reset_gpio = bt_power_pdata->bt_gpio_sys_rst;

	BT_PWR_DBG("%s  bt_gpio= %d on: %d", __func__, bt_reset_gpio, on);
printk("%s  bt_gpio= %d on: %d", __func__, bt_reset_gpio, on);

	if (on) {
		rc = gpio_request(bt_reset_gpio, "bt_sys_rst_n");
		if (rc) {
			BT_PWR_ERR("unable to request gpio %d (%d)\n",
					bt_reset_gpio, rc);
			return rc;
		}

		rc = gpio_direction_output(bt_reset_gpio, 0);
		if (rc) {
			BT_PWR_ERR("Unable to set direction\n");
			return rc;
		}
       msleep(100); 

		rc = gpio_direction_output(bt_reset_gpio, 1);
		if (rc) {
			BT_PWR_ERR("Unable to set direction\n");
			return rc;
		}
		//msleep(100);
	} else {
		gpio_set_value(bt_reset_gpio, 0);

		rc = gpio_direction_input(bt_reset_gpio);
		if (rc)
			BT_PWR_ERR("Unable to set direction\n");

		//msleep(100);
	}
	return rc;
}
#if 1
static unsigned bt_config_power_on[] = {
	GPIO_CFG(78, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),	/* WAKE */ //gsbi11 guoyuhua
	//GPIO_CFG(41, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* RFR */
	//GPIO_CFG(40, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* CTS */
	//GPIO_CFG(39, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* Rx */
	//GPIO_CFG(38, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* Tx */
    //   GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),          /*BT_RST*/
    //   GPIO_CFG(BT_REG_ON, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),  /* BT_REG_ON */
	GPIO_CFG(77, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_UP, GPIO_CFG_2MA),	/* HOST_WAKE */
	GPIO_CFG(65, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),	/* WAKE */ //gsbi11 guoyuhua
};


static unsigned bt_config_power_off[] = {
	GPIO_CFG(78, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* WAKE */ //gsbi11 guoyuhua
    	//GPIO_CFG(41, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),  /* RFR */
    	//GPIO_CFG(40, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),  /* CTS */
    	//GPIO_CFG(39, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),  /* Rx */
    	//GPIO_CFG(38, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),  /* Tx */
    	//GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* reset */
    //	GPIO_CFG(BT_REG_ON, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* BT_REG_ON */
	GPIO_CFG(77, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* HOST_WAKE */
	GPIO_CFG(65, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* WAKE */ //gsbi11 guoyuhua
};
#endif
uint8_t read_zte_hw_ver_byte(void);
 int get_brcm_bt_dev_wake(void){
   int brcm_bt_dev_wake = 65; 
   uint8_t hw_ver;
   hw_ver = read_zte_hw_ver_byte(); 
   printk("%s: dm^_^ hw_ver 0x%2.2x\n",__func__,hw_ver);  
      if ( 0 == hw_ver ){
            brcm_bt_dev_wake = 78;

         }else {
            brcm_bt_dev_wake = 65;

         }

   return  brcm_bt_dev_wake; 
}
static int bluetooth_power(int on)
{
	int rc = 0;
    int pin;
        int ext_wake_num =65;
	BT_PWR_DBG("on: %d", on);
        ext_wake_num = get_brcm_bt_dev_wake();
        printk("%s: ^_^ dm ext_wake_num %d\n ",__func__,ext_wake_num);        
printk("%s: on=%d  bt_power_pdata->bt_gpio_sys_rst=%d\n",__func__,on,bt_power_pdata->bt_gpio_sys_rst);
	if (on) {
#if 1
            if( ext_wake_num== 78)
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on)-1; pin++) 
            if( ext_wake_num== 65)
		for (pin = 1; pin < ARRAY_SIZE(bt_config_power_on); pin++) 
                {
			rc = gpio_tlmm_config(bt_config_power_on[pin],
					      GPIO_CFG_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_on[pin], rc);
				return -EIO;
			}
		}
  #if 0
        rc = gpio_request(ext_wake_num, "bt_dev_wake");
		if (rc) {
			BT_PWR_ERR("unable to request gpio %d (%d)\n",
					78, rc);
			return rc;
		}

	rc = gpio_direction_output( ext_wake_num,1);
		if (rc) {
			BT_PWR_ERR("Unable to set direction\n");
			return rc;
		}
	gpio_free(ext_wake_num);
	#endif
#endif		
		if (bt_power_pdata->bt_gpio_sys_rst) {
			rc = bt_configure_gpios(on);
			if (rc < 0) {
				BT_PWR_ERR("bt_power gpio config failed");
				goto gpio_fail;
			}
		}
	} else {
		bt_configure_gpios(on);
#if 1
            if(ext_wake_num== 78)
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on)-1; pin++) 
            if(ext_wake_num== 65)
		for (pin = 1; pin < ARRAY_SIZE(bt_config_power_on); pin++) 
	        {	rc = gpio_tlmm_config(bt_config_power_off[pin],
					      GPIO_CFG_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_off[pin], rc);
				return -EIO;
			}
		}
#endif
gpio_fail:
		if (bt_power_pdata->bt_gpio_sys_rst)
			gpio_free(bt_power_pdata->bt_gpio_sys_rst);
	}


	return rc;
}

static int bluetooth_toggle_radio(void *data, bool blocked)
{
	int ret = 0;
	int (*power_control)(int enable);

	power_control =
		((struct bluetooth_power_platform_data *)data)->bt_power_setup;

	if (previous != blocked)
		ret = (*power_control)(!blocked);
	if (!ret)
		previous = blocked;
	return ret;
}

static const struct rfkill_ops bluetooth_power_rfkill_ops = {
	.set_block = bluetooth_toggle_radio,
};

static int bluetooth_power_rfkill_probe(struct platform_device *pdev)
{
	struct rfkill *rfkill;
	int ret;

	rfkill = rfkill_alloc("bt_power", &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			      &bluetooth_power_rfkill_ops,
			      pdev->dev.platform_data);

	if (!rfkill) {
		dev_err(&pdev->dev, "rfkill allocate failed\n");
		return -ENOMEM;
	}

	/* force Bluetooth off during init to allow for user control */
	rfkill_init_sw_state(rfkill, 1);
	previous = 1;

	ret = rfkill_register(rfkill);
	if (ret) {
		dev_err(&pdev->dev, "rfkill register failed=%d\n", ret);
		rfkill_destroy(rfkill);
		return ret;
	}

	platform_set_drvdata(pdev, rfkill);

	return 0;
}

static void bluetooth_power_rfkill_remove(struct platform_device *pdev)
{
	struct rfkill *rfkill;

	dev_dbg(&pdev->dev, "%s\n", __func__);

	rfkill = platform_get_drvdata(pdev);
	if (rfkill)
		rfkill_unregister(rfkill);
	rfkill_destroy(rfkill);
	platform_set_drvdata(pdev, NULL);
}


static int bt_power_populate_dt_pinfo(struct platform_device *pdev)
{

	BT_PWR_DBG("");

	if (!bt_power_pdata)
		return -ENOMEM;

	if (pdev->dev.of_node) {
		bt_power_pdata->bt_gpio_sys_rst =
			of_get_named_gpio(pdev->dev.of_node,
						"brcm,bt-reset-gpio", 0);
		if (bt_power_pdata->bt_gpio_sys_rst < 0) {
			BT_PWR_ERR("bt-reset-gpio not provided in device tree");
			return bt_power_pdata->bt_gpio_sys_rst;
		}


	}

	bt_power_pdata->bt_power_setup = bluetooth_power;

	return 0;
}

static int __devinit bt_power_probe(struct platform_device *pdev)
{
	int ret = 0;

	dev_dbg(&pdev->dev, "%s\n", __func__);

	bt_power_pdata =
		kzalloc(sizeof(struct bluetooth_power_platform_data),
			GFP_KERNEL);

	if (!bt_power_pdata) {
		BT_PWR_ERR("Failed to allocate memory");
		return -ENOMEM;
	}

	if (pdev->dev.of_node) {
		ret = bt_power_populate_dt_pinfo(pdev);
		if (ret < 0) {
			BT_PWR_ERR("Failed to populate device tree info");
			goto free_pdata;
		}
		pdev->dev.platform_data = bt_power_pdata;
	} else if (pdev->dev.platform_data) {
		/* Optional data set to default if not provided */
		if (!((struct bluetooth_power_platform_data *)
			(pdev->dev.platform_data))->bt_power_setup)
			((struct bluetooth_power_platform_data *)
				(pdev->dev.platform_data))->bt_power_setup =
						bluetooth_power;

		memcpy(bt_power_pdata, pdev->dev.platform_data,
			sizeof(struct bluetooth_power_platform_data));
	} else {
		BT_PWR_ERR("Failed to get platform data");
		goto free_pdata;
	}

	if (bluetooth_power_rfkill_probe(pdev) < 0)
		goto free_pdata;

	btpdev = pdev;

	return 0;

free_pdata:
	kfree(bt_power_pdata);
	return ret;
}

static int __devexit bt_power_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "%s\n", __func__);

	bluetooth_power_rfkill_remove(pdev);
	kfree(bt_power_pdata);

	return 0;
}

static struct platform_driver bt_power_driver = {
	.probe = bt_power_probe,
	.remove = __devexit_p(bt_power_remove),
	.driver = {
		.name = "bt_power",
		.owner = THIS_MODULE,
		.of_match_table = bt_power_match_table,
	},
};

static int __init bluetooth_power_init(void)
{
	int ret;

	ret = platform_driver_register(&bt_power_driver);
	return ret;
}

static void __exit bluetooth_power_exit(void)
{
	platform_driver_unregister(&bt_power_driver);
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MSM Bluetooth power control driver");
MODULE_VERSION("1.40");

module_init(bluetooth_power_init);
module_exit(bluetooth_power_exit);
