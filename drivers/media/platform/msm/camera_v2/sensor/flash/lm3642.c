/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Created by ZTE_JIA_20130925 jia.jia
 */

#include "msm_led_flash_zte.h"

/*#define CONFIG_MSMB_CAMERA_DEBUG*/
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

DEFINE_MSM_MUTEX(lm3642_mut);

#define LM3642_FLAGS_REG  (0x0B)

static struct msm_led_flash_ctrl_t lm3642_fctrl;
static struct msm_flash_fn_t lm3642_func_tbl;

static struct msm_camera_i2c_reg_conf lm3642_init_setting[] = {
	{ 0x0A, 0x00 }, // enable register, standby mode, strobe pin disabled
};

static struct msm_camera_i2c_reg_conf lm3642_low_setting[] = {
	{ 0x06, 0x00 }, // torch time, ramp-up time 16ms, ramp-down time 16ms
	{ 0x09, 0x28 }, // current control, torch current 140.63mA, flash current 843.75mA
	{ 0x01, 0x10 }, // IVFM mode, UVLO disabled, IVM-D threshold 3.3v
	{ 0x0A, 0x42 }, // enable register, IVFM disabled, TX pin enabled, strobe pin disabled, torch pin disabled, torch mode, 
};

static struct msm_camera_i2c_reg_conf lm3642_high_setting[] = {
	{ 0x08, 0x57 }, // flash time, ramp time 1.024ms, time-out time 800ms
	{ 0x09, 0x29 }, // current control, torch current 140.63mA, flash current 937.5mA
	{ 0x01, 0x10 }, // IVFM mode, UVLO disabled, IVM-D threshold 3.3v
	{ 0x0A, 0xC3 }, // enable register, IVFM enabled, TX pin enabled, strobe pin disabled, torch pin disabled, flash mode
};

static struct msm_camera_i2c_reg_conf lm3642_off_setting[] = {
	{ 0x0A, 0x00 }, // enable register, standby mode, strobe pin disabled
};

static struct msm_camera_i2c_reg_conf lm3642_release_setting[] = {
	{ 0x0A, 0x00 }, // enable register, standby mode, strobe pin disabled
};

static int32_t lm3642_led_trigger_i2c_write_table(struct msm_led_flash_ctrl_t *fctrl,
																							 					 struct msm_camera_i2c_reg_conf *table,
																							 					 int32_t num);
static int32_t lm3642_led_trigger_init(struct msm_led_flash_ctrl_t *fctrl);
static int32_t lm3642_led_trigger_low(struct msm_led_flash_ctrl_t *fctrl);
static int32_t lm3642_led_trigger_high(struct msm_led_flash_ctrl_t *fctrl);
static int32_t lm3642_led_trigger_off(struct msm_led_flash_ctrl_t *fctrl);
static int32_t lm3642_led_trigger_release(struct msm_led_flash_ctrl_t *fctrl);

static int32_t lm3642_platform_probe(struct platform_device *pdev);
static void lm3642_platform_shutdown(struct platform_device *pdev);

static int32_t lm3642_led_trigger_i2c_write_table(struct msm_led_flash_ctrl_t *fctrl,
																												 struct msm_camera_i2c_reg_conf *table,
																												 int32_t num)
{
	int32_t i = 0, rc = 0;

	for (i = 0; i < num; ++i) {
		rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_write(fctrl->flash_i2c_client,
																													table->reg_addr,
																													table->reg_data,
																													fctrl->reg_setting->default_data_type);
		if (rc < 0) {
			break;
		}

		table++;
	}

	return rc;
}

static int32_t lm3642_led_trigger_init(struct msm_led_flash_ctrl_t *fctrl)
{
	uint16_t flags = 0;
	int32_t rc = 0;

	CDBG("%s: E\n", __func__);

	rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_util(fctrl->flash_i2c_client, MSM_CCI_INIT);
	if (rc < 0) {
		pr_err("%s: cci_init failed\n", __func__);
		(void)fctrl->flash_i2c_client->i2c_func_tbl->i2c_util(fctrl->flash_i2c_client, MSM_CCI_RELEASE);
		return rc;
	}

	/*
	 * Faults require a read-back of the "Flags Register" to resume operation.
	 * Flags report an event occurred, but do not inhibit future functionality.
	 * A read-back of the Flags Register will only get updated again if the fault or flags
	 * is still present upon a restart.
	 */
	rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_read(fctrl->flash_i2c_client,
																											 LM3642_FLAGS_REG,
																											 &flags,
																											 fctrl->reg_setting->default_data_type);
	if (rc < 0) {
		pr_err("%s: read flags failed\n", __func__);
		return rc;
	}
	CDBG("%s: flags: 0x%x\n", __func__, flags);

/*
 * Disused here
 * Fix run-time error of I2C write
 */
#if 0
	rc = lm3642_led_trigger_i2c_write_table(fctrl, fctrl->reg_setting->init_setting, fctrl->reg_setting->init_setting_size);
	if (rc < 0) {
		pr_err("%s: i2c write table failed\n", __func__);
		return rc;
	}
#endif

	/*
	 * Fix exceptional status of flash led in scenario of Auto LED + Touch AF + Capture
	 */
	rc = msm_camera_request_gpio_table(fctrl->board_info.gpio_conf->flash_gpio_req_tbl,
																		 fctrl->board_info.gpio_conf->flash_gpio_req_tbl_size,
																		 1);
	if (rc < 0) {
		pr_err("%s: request gpio failed\n", __func__);
		return rc;
	}
	gpio_set_value_cansleep(fctrl->board_info.gpio_conf->gpio_num_info->gpio_num[FLASH_GPIO_STROBE], GPIO_OUT_LOW);

	CDBG("%s: X\n", __func__);

	return 0;
}

static int32_t lm3642_led_trigger_low(struct msm_led_flash_ctrl_t *fctrl)
{
	uint16_t flags = 0;
	int32_t rc = 0;

	CDBG("%s: E\n", __func__);

	/*
	 * Faults require a read-back of the "Flags Register" to resume operation.
	 * Flags report an event occurred, but do not inhibit future functionality.
	 * A read-back of the Flags Register will only get updated again if the fault or flags
	 * is still present upon a restart.
	 */
	rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_read(fctrl->flash_i2c_client,
																											 LM3642_FLAGS_REG,
																											 &flags,
																											 fctrl->reg_setting->default_data_type);
	if (rc < 0) {
		pr_err("%s: read flags failed\n", __func__);
		return rc;
	}
	CDBG("%s: flags: 0x%x\n", __func__, flags);

	rc = lm3642_led_trigger_i2c_write_table(fctrl, fctrl->reg_setting->low_setting, fctrl->reg_setting->low_setting_size);
	if (rc < 0) {
		pr_err("%s: i2c write table failed\n", __func__);
		return rc;
	}

	CDBG("%s: X\n", __func__);

	return 0;
}

static int32_t lm3642_led_trigger_high(struct msm_led_flash_ctrl_t *fctrl)
{
	uint16_t flags = 0;
	int32_t rc = 0;

	CDBG("%s: E\n", __func__);

	/*
	 * Faults require a read-back of the "Flags Register" to resume operation.
	 * Flags report an event occurred, but do not inhibit future functionality.
	 * A read-back of the Flags Register will only get updated again if the fault or flags
	 * is still present upon a restart.
	 */
	rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_read(fctrl->flash_i2c_client,
																											 LM3642_FLAGS_REG,
																											 &flags,
																											 fctrl->reg_setting->default_data_type);
	if (rc < 0) {
		pr_err("%s: read flags failed\n", __func__);
		return rc;
	}
	CDBG("%s: flags: 0x%x\n", __func__, flags);

	rc = lm3642_led_trigger_i2c_write_table(fctrl, fctrl->reg_setting->high_setting, fctrl->reg_setting->high_setting_size);
	if (rc < 0) {
		pr_err("%s: i2c write table failed\n", __func__);
		return rc;
	}

	/*
	 * Fix exceptional status of flash led in scenario of Auto LED + Touch AF + Capture
	 */
	gpio_set_value_cansleep(fctrl->board_info.gpio_conf->gpio_num_info->gpio_num[FLASH_GPIO_STROBE], GPIO_OUT_HIGH);

	CDBG("%s: X\n", __func__);

	return 0;
}

static int32_t lm3642_led_trigger_off(struct msm_led_flash_ctrl_t *fctrl)
{
	uint16_t flags = 0;

	CDBG("%s: E\n", __func__);

	/*
	 * Faults require a read-back of the "Flags Register" to resume operation.
	 * Flags report an event occurred, but do not inhibit future functionality.
	 * A read-back of the Flags Register will only get updated again if the fault or flags
	 * is still present upon a restart.
	 */
	(void)fctrl->flash_i2c_client->i2c_func_tbl->i2c_read(fctrl->flash_i2c_client,
																											  LM3642_FLAGS_REG,
																											  &flags,
																											  fctrl->reg_setting->default_data_type);
	CDBG("%s: flags: 0x%x\n", __func__, flags);

	(void)lm3642_led_trigger_i2c_write_table(fctrl, fctrl->reg_setting->off_setting, fctrl->reg_setting->off_setting_size);

	/*
	 * Fix exceptional status of flash led in scenario of Auto LED + Touch AF + Capture
	 */
	gpio_set_value_cansleep(fctrl->board_info.gpio_conf->gpio_num_info->gpio_num[FLASH_GPIO_STROBE], GPIO_OUT_LOW);

	CDBG("%s: X\n", __func__);

	return 0;
}

static int32_t lm3642_led_trigger_release(struct msm_led_flash_ctrl_t *fctrl)
{
	uint16_t flags = 0;

	CDBG("%s: E\n", __func__);

	(void)msm_led_trigger_power_up(fctrl);

	(void)fctrl->flash_i2c_client->i2c_func_tbl->i2c_read(fctrl->flash_i2c_client,
																											 	LM3642_FLAGS_REG,
																											 	&flags,
																											 	fctrl->reg_setting->default_data_type);

	(void)lm3642_led_trigger_i2c_write_table(fctrl, fctrl->reg_setting->release_setting, fctrl->reg_setting->release_setting_size);

	(void)msm_led_trigger_power_down(fctrl);

	(void)fctrl->flash_i2c_client->i2c_func_tbl->i2c_util(fctrl->flash_i2c_client, MSM_CCI_RELEASE);

	/*
	 * Fix exceptional status of flash led in scenario of Auto LED + Touch AF + Capture
	 */
	gpio_set_value_cansleep(fctrl->board_info.gpio_conf->gpio_num_info->gpio_num[FLASH_GPIO_STROBE], GPIO_OUT_LOW);
	(void)msm_camera_request_gpio_table(fctrl->board_info.gpio_conf->flash_gpio_req_tbl,
																			fctrl->board_info.gpio_conf->flash_gpio_req_tbl_size,
																		 	0);

	CDBG("%s: X\n", __func__);

	return 0;
}

static struct msm_camera_i2c_client lm3642_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static struct msm_flash_fn_t lm3642_func_tbl = {
	.flash_get_subdev_id = msm_led_trigger_get_subdev_id,
	.flash_led_config = msm_led_trigger_config,
	.flash_led_init = lm3642_led_trigger_init,
	.flash_led_low = lm3642_led_trigger_low,
	.flash_led_high = lm3642_led_trigger_high,
	.flash_led_off = lm3642_led_trigger_off,
	.flash_led_release = lm3642_led_trigger_release,
};

static struct msm_flash_reg_t lm3642_reg_setting = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.init_setting = lm3642_init_setting,
	.init_setting_size = ARRAY_SIZE(lm3642_init_setting),
	.low_setting = lm3642_low_setting,
	.low_setting_size = ARRAY_SIZE(lm3642_low_setting),
	.high_setting = lm3642_high_setting,
	.high_setting_size = ARRAY_SIZE(lm3642_high_setting),
	.off_setting = lm3642_off_setting,
	.off_setting_size = ARRAY_SIZE(lm3642_off_setting),
	.release_setting = lm3642_release_setting,
	.release_setting_size = ARRAY_SIZE(lm3642_release_setting),
};

static struct msm_led_flash_ctrl_t lm3642_fctrl = {
	.flash_i2c_client = &lm3642_i2c_client,
	.func_tbl = &lm3642_func_tbl,
	.reg_setting = &lm3642_reg_setting,
	.msm_led_flash_mutex = &lm3642_mut,
};

static const struct of_device_id lm3642_dt_match[] = {
	{.compatible = "qcom,lm3642", .data = &lm3642_fctrl},
	{ }
};
MODULE_DEVICE_TABLE(of, lm3642_dt_match);

static struct platform_driver lm3642_platform_driver = {
	.shutdown = lm3642_platform_shutdown,
	.driver = {
		.name = "qcom,lm3642",
		.owner = THIS_MODULE,
		.of_match_table = lm3642_dt_match,
	},
};

static int32_t lm3642_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	match = of_match_device(lm3642_dt_match, &pdev->dev);
	rc = msm_led_trigger_probe(pdev, match->data);
	return rc;
}

/*
 * Add shutdown implementation to fix potential exceptional status of flash IC
 * in power-down or reboot procedure.
 */
static void lm3642_platform_shutdown(struct platform_device *pdev)
{
	(void)msm_led_trigger_power_up(&lm3642_fctrl);
	(void)lm3642_fctrl.func_tbl->flash_led_init(&lm3642_fctrl);
	(void)lm3642_fctrl.func_tbl->flash_led_off(&lm3642_fctrl);
	(void)msm_led_trigger_power_down(&lm3642_fctrl);

	(void)msm_led_trigger_free_data(&lm3642_fctrl);
}

static int __init lm3642_init_module(void)
{
	return platform_driver_probe(&lm3642_platform_driver, lm3642_platform_probe);
}

static void __exit lm3642_exit_module(void)
{
	if (lm3642_fctrl.pdev) {
		platform_driver_unregister(&lm3642_platform_driver);
	}
}

module_init(lm3642_init_module);
module_exit(lm3642_exit_module);

MODULE_DESCRIPTION("LM3642 Flash Trigger");
MODULE_LICENSE("GPL v2");
