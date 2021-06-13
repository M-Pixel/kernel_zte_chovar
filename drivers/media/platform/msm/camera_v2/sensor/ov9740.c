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
 * Added for camera sensor by ZTE_JIA_20130422 jia.jia lijing
 */

#include "msm_sensor.h"
#include "msm_cci.h"
#include "msm_camera_io_util.h"
#define OV9740_SENSOR_NAME "ov9740"
#define PLATFORM_DRIVER_NAME "msm_camera_ov9740"
#define ov9740_obj ov9740_##obj

#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

#define OV9740_MODE_SELECT  (0x0100)

DEFINE_MSM_MUTEX(ov9740_mut);

static struct msm_sensor_ctrl_t ov9740_s_ctrl;

static struct msm_sensor_power_setting ov9740_power_setting[] = {
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_LDO,
		.config_val = GPIO_OUT_HIGH,
		.sleep_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.sleep_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_LOW,
		.sleep_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.sleep_val = GPIO_OUT_HIGH,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.sleep_val = GPIO_OUT_HIGH,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.sleep_val = GPIO_OUT_HIGH,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 100,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};

static struct msm_camera_i2c_reg_conf ov9740_prev_settings[] = 
{
	//Orientation
	{0x0101 ,0x02},//N9100-01

	//PLL setting
	{0x3104 ,0x20},// PLL mode control
	{0x0305 ,0x03},// PLL control
	{0x0307 ,0x4c},// PLL control
	{0x0303 ,0x01},// PLL control
	{0x0301 ,0x08},// PLL control
	{0x3010 ,0x01},// PLL control
	//Timing setting
	{0x0340 ,0x03},//VTS
	{0x0341 ,0x07},//VTS
	{0x0342 ,0x06},//HTS
	{0x0343 ,0x62},//HTS
	{0x0344 ,0x00},// X start
	{0x0345 ,0x08},//X start
	{0x0346 ,0x00},//Y start
	//{0x0347 ,0x04},//Y start 2012/11/05 yanwei
	{0x0347 ,0x05},//Y start	
	{0x0348 ,0x05},//X end
	{0x0349 ,0x0c},//X end
	{0x034a ,0x02},//Y end
	//{0x034b ,0xd8},// Y end 2012/11/05 yanwei
	{0x034b ,0xd9},// Y end	
	{0x034c ,0x05},// H output size
	{0x034d ,0x00},//H output size
	{0x034e ,0x02},//V output size
	{0x034f ,0xd0},//V output size
	//Output select 
	{0x3002 ,0x00},// IO control
	{0x3004 ,0x00},// IO control
	{0x3005 ,0x00},// IO control
	{0x3012 ,0x70},// MIPI control
	{0x3013 ,0x60},//MIPI control
	{0x3014 ,0x01},//MIPI control
	{0x301f ,0x03},// MIPI control
	{0x3026 ,0x00},// Output select
	{0x3027 ,0x00},// Output select
	//Analog control
	{0x3601 ,0x40},// Analog control
	{0x3602 ,0x16},// Analog control
	{0x3603 ,0xaa},// Analog control
	{0x3604 ,0x0c},// Analog control
	{0x3610 ,0xa1},// Analog control
	{0x3612 ,0x24},// Analog control
	{0x3620 ,0x66},// Analog control
	{0x3621 ,0xc0},// Analog control
	{0x3622 ,0x9f},// Analog control
	{0x3630 ,0xd2},// Analog control
	{0x3631 ,0x5e},// Analog control
	{0x3632 ,0x27},// Analog control
	{0x3633 ,0x50},// Analog control
	// Sensor control
	{0x3703 ,0x42},// Sensor control 
	{0x3704 ,0x10},// Sensor control 
	{0x3705 ,0x45},// Sensor control 
	{0x3707 ,0x11},// Sensor control 
	//Timing control
	{0x3817 ,0x94},// Internal timing control
	{0x3819 ,0x6e},// Internal timing control
	{0x3831 ,0x40},// Digital gain enable
	{0x3833 ,0x04},// Internal timing control
	{0x3835 ,0x04},// Internal timing control
	{0x3837 ,0x01},
	//Internal timing control;; AEC/AGC control
	{0x3503 ,0x10},//AEC/AGC control
	{0x3a18 ,0x01},//Gain ceiling
	{0x3a19 ,0xB5},//Gain ceiling
	{0x3a1a ,0x05},//Max diff
	{0x3a11 ,0x90},// High threshold
	{0x3a1b ,0x4a},// WPT 2 
	{0x3a0f ,0x48},// WPT  
	{0x3a10 ,0x44},// BPT  
	{0x3a1e ,0x42},// BPT 2 
	{0x3a1f ,0x22},//; Low threshold 
	//Banding filter
	{0x3a08 ,0x00},// 50Hz banding step
	{0x3a09 ,0xe8},// 50Hz banding step	
	{0x3a0e ,0x03},// 50Hz banding Max
	{0x3a14 ,0x15},// 50Hz Max exposure
	{0x3a15 ,0xc6},// 50Hz Max exposure
	{0x3a0a ,0x00},// 60Hz banding step
	{0x3a0b ,0xc0},// 60Hz banding step
	{0x3a0d ,0x04},// 60Hz banding Max
	{0x3a02 ,0x18},// 60Hz Max exposure
	{0x3a03 ,0x20},// 60Hz Max exposure

	//50/60 detection
	{0x3c0a ,0x9c},// Number of samples
	{0x3c0b ,0x3f},// Number of samples

	// BLC control
	{0x4002 ,0x45},// BLC auto enable
	{0x4005 ,0x18},// BLC mode

	// VFIFO control
	{0x4601 ,0x16},// VFIFO control
	{0x460e ,0x82},	
	// VFIFO control

	// DVP control
	{0x4702 ,0x04},// Vsync control
	{0x4704 ,0x00},// Vsync mode 
	{0x4706 ,0x08},// Vsync control

	//MIPI control
	{0x4800 ,0x44},// MIPI control
	{0x4801 ,0x0f},//
	// MIPI control
	{0x4803 ,0x05},
	// MIPI control
	{0x4805 ,0x10},
	// MIPI control
	{0x4837 ,0x30},//20
	// MIPI control;; ISP control
	{0x5000 ,0xff},// [7] LC [6] Gamma [3] DNS [2] BPC [1] WPC [0] CIP
	{0x5001 ,0xff},// [7] SDE [6] UV adjust [4] scale [3] contrast [2] UV average [1] CMX [0] AWB
	{0x5003 ,0xff},// [7] PAD [5] Buffer [3] Vario [1] BLC [0] AWB gain

	// AWB
	{0x5180 ,0xf0}, //AWB setting
	{0x5181 ,0x00}, //;AWB setting
	{0x5182 ,0x41}, //AWB setting 
	{0x5183 ,0x42}, //AWB setting
	{0x5184 ,0x80}, //AWB setting
	{0x5185 ,0x68}, //AWB setting
	{0x5186 ,0x93}, //AWB setting 
	{0x5187 ,0xa8}, //AWB setting
	{0x5188 ,0x17}, //AWB setting
	{0x5189 ,0x45}, //AWB setting
	{0x518a ,0x27}, //AWB setting
	{0x518b ,0x41}, //AWB setting
	{0x518c ,0x2d}, //AWB setting
	{0x518d ,0xf0}, //AWB setting
	{0x518e ,0x10}, //;AWB setting
	{0x518f ,0xff}, //AWB setting
	{0x5190 ,0x00}, //AWB setting
	{0x5191 ,0xff}, //AWB setting 
	{0x5192 ,0x00}, //;AWB setting
	{0x5193 ,0xff}, //AWB setting 
	{0x5194 ,0x00}, //AWB setting 

	// DNS
	{0x529a ,0x02}, //DNS setting
	{0x529b ,0x08}, //DNS setting
	{0x529c ,0x0a}, //DNS setting
	{0x529d ,0x10}, //DNS setting
	{0x529e ,0x10}, //;DNS setting
	{0x529f ,0x28}, //	;;DNS setting
	{0x52a0 ,0x32}, //;DNS setting
	{0x52a2 ,0x00}, //DNS setting 
	{0x52a3 ,0x02}, //DNS setting 
	{0x52a4 ,0x00}, //DNS setting 
	{0x52a5 ,0x04}, //DNS setting  
	{0x52a6 ,0x00}, //DNS setting  
	{0x52a7 ,0x08}, //DNS setting  
	{0x52a8 ,0x00}, //DNS setting  
	{0x52a9 ,0x10}, //;;DNS setting
	{0x52aa ,0x00}, //DNS setting  
	{0x52ab ,0x38}, //;;DNS setting
	{0x52ac ,0x00}, //DNS setting  
	{0x52ad ,0x3c}, //;DNS setting 
	{0x52ae ,0x00}, //DNS setting   
	{0x52af ,0x4c}, //;DNS setting

	//CIP
	{0x530d ,0x06}, //CIP setting

	//CMX
	{0x5380 ,0x01},  //CMX setting  
	{0x5381 ,0x00},  //CMX setting  
	{0x5382 ,0x00},  //CMX setting   
	{0x5383 ,0x0d},  //CMX setting  
	{0x5384 ,0x00},  //CMX setting  
	{0x5385 ,0x2f},  //CMX setting   
	{0x5386 ,0x00},  //CMX setting  
	{0x5387 ,0x00},  //CMX setting   
	{0x5388 ,0x00},  //CMX setting   
	{0x5389 ,0xd3},  //CMX setting  
	{0x538a ,0x00},  //CMX setting   
	{0x538b ,0x0f},  //CMX setting   
	{0x538c ,0x00},  //CMX setting   
	{0x538d ,0x00},  //CMX setting  
	{0x538e ,0x00},  //CMX setting  
	{0x538f ,0x32},  //CMX setting   
	{0x5390 ,0x00},  //CMX setting  
	{0x5391 ,0x94},  //CMX setting    
	{0x5392 ,0x00},  //CMX setting  
	{0x5393 ,0xa4},  //CMX setting  
	{0x5394 ,0x18},  //CMX setting 
	//Contrast
	{0x5401 ,0x2c}, // Contrast setting
	{0x5403 ,0x28}, // Contrast setting
	{0x5404 ,0x06}, // Contrast setting	
	{0x5405 ,0xe0}, // Contrast setting

	//Y Gamma
	{0x5480 ,0x04}, //Y Gamma setting  
	{0x5481 ,0x12}, //Y Gamma setting 
	{0x5482 ,0x27}, //Y Gamma setting  
	{0x5483 ,0x49}, //Y Gamma setting  
	{0x5484 ,0x57}, //Y Gamma setting  
	{0x5485 ,0x66}, //Y Gamma setting  
	{0x5486 ,0x75}, //Y Gamma setting  
	{0x5487 ,0x81}, //Y Gamma setting 
	{0x5488 ,0x8c}, //Y Gamma setting 
	{0x5489 ,0x95}, //Y Gamma setting 
	{0x548a ,0xa5}, //Y Gamma setting 
	{0x548b ,0xb2}, //Y Gamma setting 
	{0x548c ,0xc8}, //Y Gamma setting 
	{0x548d ,0xd9}, //Y Gamma setting 
	{0x548e ,0xec}, //Y Gamma setting 

	//UV Gamma
	{0x5490 ,0x01}, //UV Gamma setting 
	{0x5491 ,0xc0}, //UV Gamma setting 
	{0x5492 ,0x03}, //UV Gamma setting 
	{0x5493 ,0x00}, //UV Gamma setting 
	{0x5494 ,0x03}, //UV Gamma setting 
	{0x5495 ,0xe0}, //UV Gamma setting 
	{0x5496 ,0x03}, //UV Gamma setting 
	{0x5497 ,0x10}, //UV Gamma setting 
	{0x5498 ,0x02}, //UV Gamma setting 
	{0x5499 ,0xac}, //UV Gamma setting 
	{0x549a ,0x02}, //UV Gamma setting 
	{0x549b ,0x75}, //UV Gamma setting 
	{0x549c ,0x02}, //;UV Gamma setting 
	{0x549d ,0x44}, //UV Gamma setting 
	{0x549e ,0x02}, //UV Gamma setting 
	{0x549f ,0x20}, //UV Gamma setting 
	{0x54a0 ,0x02}, //UV Gamma setting 
	{0x54a1 ,0x07}, //UV Gamma setting 
	{0x54a2 ,0x01}, //UV Gamma setting 
	{0x54a3 ,0xec}, //UV Gamma setting 
	{0x54a4 ,0x01}, //UV Gamma setting 
	{0x54a5 ,0xc0}, //UV Gamma setting 
	{0x54a6 ,0x01}, //UV Gamma setting 
	{0x54a7 ,0x9b}, //UV Gamma setting 
	{0x54a8 ,0x01}, //UV Gamma setting 
	{0x54a9 ,0x63}, //UV Gamma setting 
	{0x54aa ,0x01}, //UV Gamma setting 
	{0x54ab ,0x2b}, //UV Gamma setting 
	{0x54ac ,0x01}, //UV Gamma setting 
	{0x54ad ,0x22}, //UV Gamma setting 
	// UV adjust
	{0x5501 ,0x1c}, //UV adjust setting 
	{0x5502 ,0x00}, //;UV adjust setting 
	{0x5503 ,0x40}, //UV adjust setting 
	{0x5504 ,0x00}, //UV adjust setting 
	{0x5505 ,0x80}, //UV adjust setting 
	//Lens correction
	{0x5800 ,0x1c}, // Lens correction setting
	{0x5801 ,0x16}, // Lens correction setting
	{0x5802 ,0x15}, // Lens correction setting
	{0x5803 ,0x16}, // Lens correction setting
	{0x5804 ,0x18}, // Lens correction setting
	{0x5805 ,0x1a}, // Lens correction setting
	{0x5806 ,0x0c}, // Lens correction setting
	{0x5807 ,0x0a}, // Lens correction setting
	{0x5808 ,0x08}, // Lens correction setting
	{0x5809 ,0x08}, // Lens correction setting
	{0x580a ,0x0a}, // Lens correction setting
	{0x580b ,0x0b}, // Lens correction setting
	{0x580c ,0x05}, // Lens correction setting
	{0x580d ,0x02}, // Lens correction setting
	{0x580e ,0x00}, // Lens correction setting
	{0x580f ,0x00}, // Lens correction setting
	{0x5810 ,0x02}, // Lens correction setting
	{0x5811 ,0x05}, // Lens correction setting
	{0x5812 ,0x04}, // Lens correction setting
	{0x5813 ,0x01}, // Lens correction setting
	{0x5814 ,0x00}, // Lens correction setting
	{0x5815 ,0x00}, // Lens correction setting
	{0x5816 ,0x02}, // Lens correction setting
	{0x5817 ,0x03}, // Lens correction setting
	{0x5818 ,0x0a}, // Lens correction setting
	{0x5819 ,0x07}, // Lens correction setting
	{0x581a ,0x05}, // Lens correction setting
	{0x581b ,0x05}, // Lens correction setting
	{0x581c ,0x08}, // Lens correction setting
	{0x581d ,0x0b}, // Lens correction setting
	{0x581e ,0x15}, // Lens correction setting
	{0x581f ,0x14}, // Lens correction setting
	{0x5820 ,0x14}, // Lens correction setting
	{0x5821 ,0x13}, // Lens correction setting
	{0x5822 ,0x17}, // Lens correction setting
	{0x5823 ,0x16}, // Lens correction setting
	{0x5824 ,0x46}, // Lens correction setting
	{0x5825 ,0x4c}, // Lens correction setting
	{0x5826 ,0x6c}, // Lens correction setting
	{0x5827 ,0x4c}, // Lens correction setting
	{0x5828 ,0x80}, // Lens correction setting
	{0x5829 ,0x2e}, // Lens correction setting
	{0x582a ,0x48}, // Lens correction setting
	{0x582b ,0x46}, // Lens correction setting
	{0x582c ,0x2a}, // Lens correction setting
	{0x582d ,0x68}, // Lens correction setting
	{0x582e ,0x08}, // Lens correction setting
	{0x582f ,0x26}, // Lens correction setting
	{0x5830 ,0x44}, // Lens correction setting
	{0x5831 ,0x46}, // Lens correction setting
	{0x5832 ,0x62}, // Lens correction setting
	{0x5833 ,0x0c}, // Lens correction setting
	{0x5834 ,0x28}, // Lens correction setting
	{0x5835 ,0x46}, // Lens correction setting
	{0x5836 ,0x28}, // Lens correction setting
	{0x5837 ,0x88}, // Lens correction setting
	{0x5838 ,0x0e}, // Lens correction setting
	{0x5839 ,0x0e}, // Lens correction setting
	{0x583a ,0x2c}, // Lens correction setting
	{0x583b ,0x2e}, // Lens correction setting
	{0x583c ,0x46}, // Lens correction setting
	{0x583d ,0xca}, // Lens correction setting
	{0x583e ,0xf0}, // Lens correction setting
	{0x5842 ,0x02}, // Lens correction setting
	{0x5843 ,0x5e}, // Lens correction setting
	{0x5844 ,0x04}, // Lens correction setting
	{0x5845 ,0x32}, // Lens correction setting
	{0x5846 ,0x03}, // Lens correction setting
	{0x5847 ,0x29}, // Lens correction setting
	{0x5848 ,0x02}, // Lens correction setting
	{0x5849 ,0xcc}, // Lens correction setting
	//{0x501a ,0xe0}, //colorbar0x80
	{0x4300 ,0x32}, // YUV shunxu
	{0x0340 ,0x03},
	{0x0341 ,0xc4}, //vts to 0x3c4
	{0x3c01 ,0x80}, //band manual mode enable
	{0x3a0d ,0x05}, //60Hz banding Max
	{0x3a18 ,0x00}, //Gain Ceiling 8x
	{0x3a19 ,0x80},

	// Start streaming
	{0x0100 ,0x00}, // start streaming
};

static struct msm_camera_i2c_reg_conf ov9740_reset_settings[] = {
	{0x0103 ,0x01},
};

static struct v4l2_subdev_info ov9740_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static const struct i2c_device_id ov9740_i2c_id[] = {
	{OV9740_SENSOR_NAME, (kernel_ulong_t)&ov9740_s_ctrl},
	{ }
};

static int32_t msm_ov9740_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &ov9740_s_ctrl);
}

static struct i2c_driver ov9740_i2c_driver = {
	.id_table = ov9740_i2c_id,
	.probe  = msm_ov9740_i2c_probe,
	.driver = {
		.name = OV9740_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov9740_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id ov9740_dt_match[] = {
	{.compatible = "qcom,ov9740", .data = &ov9740_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, ov9740_dt_match);

static struct platform_driver ov9740_platform_driver = {
	.driver = {
		.name = "qcom,ov9740",
		.owner = THIS_MODULE,
		.of_match_table = ov9740_dt_match,
	},
};

static int32_t ov9740_platform_probe(struct platform_device *pdev)
{
	int32_t rc;
	const struct of_device_id *match;
	match = of_match_device(ov9740_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init ov9740_init_module(void)
{
	int32_t rc;
	pr_info("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&ov9740_platform_driver,
		ov9740_platform_probe);
	if (!rc)
		return rc;
	pr_err("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&ov9740_i2c_driver);
}

static void __exit ov9740_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	if (ov9740_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&ov9740_s_ctrl);
		platform_driver_unregister(&ov9740_platform_driver);
	} else
		i2c_del_driver(&ov9740_i2c_driver);
	return;
}

int32_t ov9740_sensor_config(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp)
{
	struct sensorb_cfg_data *cdata = (struct sensorb_cfg_data *)argp;
	long rc = 0;
	int32_t i = 0;
	mutex_lock(s_ctrl->msm_sensor_mutex);
	CDBG("%s:%d %s cfgtype = %d\n", __func__, __LINE__,
		s_ctrl->sensordata->sensor_name, cdata->cfgtype);
	switch (cdata->cfgtype) {
	case CFG_GET_SENSOR_INFO:
		memcpy(cdata->cfg.sensor_info.sensor_name,
			s_ctrl->sensordata->sensor_name,
			sizeof(cdata->cfg.sensor_info.sensor_name));
		cdata->cfg.sensor_info.session_id =
			s_ctrl->sensordata->sensor_info->session_id;
		for (i = 0; i < SUB_MODULE_MAX; i++)
			cdata->cfg.sensor_info.subdev_id[i] =
				s_ctrl->sensordata->sensor_info->subdev_id[i];
		CDBG("%s:%d sensor name %s\n", __func__, __LINE__,
			cdata->cfg.sensor_info.sensor_name);
		CDBG("%s:%d session id %d\n", __func__, __LINE__,
			cdata->cfg.sensor_info.session_id);
		for (i = 0; i < SUB_MODULE_MAX; i++)
			CDBG("%s:%d subdev_id[%d] %d\n", __func__, __LINE__, i,
				cdata->cfg.sensor_info.subdev_id[i]);

		break;
	case CFG_SET_INIT_SETTING:
		/* 1. Write Recommend settings */
		/* 2. Write change settings */
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, ov9740_reset_settings,
			ARRAY_SIZE(ov9740_reset_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
			msleep(10);
		break;
	case CFG_SET_RESOLUTION:
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, ov9740_prev_settings,
			ARRAY_SIZE(ov9740_prev_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
		break;
	case CFG_SET_STOP_STREAM:
		s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(s_ctrl->sensor_i2c_client,OV9740_MODE_SELECT, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		msleep(10);
		break;
	case CFG_SET_START_STREAM:
		s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(s_ctrl->sensor_i2c_client,OV9740_MODE_SELECT, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		msleep(50);
		break;
	case CFG_GET_SENSOR_INIT_PARAMS:
		cdata->cfg.sensor_init_params =
			*s_ctrl->sensordata->sensor_init_params;
		CDBG("%s:%d init params mode %d pos %d mount %d\n", __func__,
			__LINE__,
			cdata->cfg.sensor_init_params.modes_supported,
			cdata->cfg.sensor_init_params.position,
			cdata->cfg.sensor_init_params.sensor_mount_angle);
		break;
	case CFG_SET_SLAVE_INFO: {
		struct msm_camera_sensor_slave_info sensor_slave_info;
		struct msm_sensor_power_setting_array *power_setting_array;
		int slave_index = 0;
		if (copy_from_user(&sensor_slave_info,
		    (void *)cdata->cfg.setting,
		    sizeof(struct msm_camera_sensor_slave_info))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		/* Update sensor slave address */
		if (sensor_slave_info.slave_addr) {
			s_ctrl->sensor_i2c_client->cci_client->sid =
				sensor_slave_info.slave_addr >> 1;
		}

		/* Update sensor address type */
		s_ctrl->sensor_i2c_client->addr_type =
			sensor_slave_info.addr_type;

		/* Update power up / down sequence */
		s_ctrl->power_setting_array =
			sensor_slave_info.power_setting_array;
		power_setting_array = &s_ctrl->power_setting_array;
		power_setting_array->power_setting = kzalloc(
			power_setting_array->size *
			sizeof(struct msm_sensor_power_setting), GFP_KERNEL);
		if (!power_setting_array->power_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(power_setting_array->power_setting,
		    (void *)sensor_slave_info.power_setting_array.power_setting,
		    power_setting_array->size *
		    sizeof(struct msm_sensor_power_setting))) {
			kfree(power_setting_array->power_setting);
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		s_ctrl->free_power_setting = true;
		CDBG("%s sensor id %x\n", __func__,
			sensor_slave_info.slave_addr);
		CDBG("%s sensor addr type %d\n", __func__,
			sensor_slave_info.addr_type);
		CDBG("%s sensor reg %x\n", __func__,
			sensor_slave_info.sensor_id_info.sensor_id_reg_addr);
		CDBG("%s sensor id %x\n", __func__,
			sensor_slave_info.sensor_id_info.sensor_id);
		for (slave_index = 0; slave_index <
			power_setting_array->size; slave_index++) {
			CDBG("%s i %d power setting %d %d %ld %d\n", __func__,
				slave_index,
				power_setting_array->power_setting[slave_index].
				seq_type,
				power_setting_array->power_setting[slave_index].
				seq_val,
				power_setting_array->power_setting[slave_index].
				config_val,
				power_setting_array->power_setting[slave_index].
				delay);
		}
		kfree(power_setting_array->power_setting);
		break;
	}
	case CFG_WRITE_I2C_ARRAY: {
		struct msm_camera_i2c_reg_setting conf_array;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;

		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
			s_ctrl->sensor_i2c_client, &conf_array);
		kfree(reg_setting);
		break;
	}
	case CFG_WRITE_I2C_SEQ_ARRAY: {
		struct msm_camera_i2c_seq_reg_setting conf_array;
		struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;

		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_seq_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_seq_reg_array)),
			GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_seq_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_seq_table(s_ctrl->sensor_i2c_client,
			&conf_array);
		kfree(reg_setting);
		break;
	}

	case CFG_POWER_UP:
		if (s_ctrl->func_tbl->sensor_power_up)
			rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_POWER_DOWN:
		if (s_ctrl->func_tbl->sensor_power_down)
			rc = s_ctrl->func_tbl->sensor_power_down(
				s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_SET_STOP_STREAM_SETTING: {
		struct msm_camera_i2c_reg_setting *stop_setting =
			&s_ctrl->stop_setting;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;
		if (copy_from_user(stop_setting, (void *)cdata->cfg.setting,
		    sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = stop_setting->reg_setting;
		stop_setting->reg_setting = kzalloc(stop_setting->size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!stop_setting->reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(stop_setting->reg_setting,
		    (void *)reg_setting, stop_setting->size *
		    sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(stop_setting->reg_setting);
			stop_setting->reg_setting = NULL;
			stop_setting->size = 0;
			rc = -EFAULT;
			break;
		}
		break;
	}
	default:
		rc = -EFAULT;
		break;
	}

	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}

static struct msm_sensor_fn_t ov9740_sensor_func_tbl = {
	.sensor_config = ov9740_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = msm_sensor_match_id,
};

static struct msm_sensor_ctrl_t ov9740_s_ctrl = {
	.sensor_i2c_client = &ov9740_sensor_i2c_client,
	.power_setting_array.power_setting = ov9740_power_setting,
	.power_setting_array.size = ARRAY_SIZE(ov9740_power_setting),
	.msm_sensor_mutex = &ov9740_mut,
	.sensor_v4l2_subdev_info = ov9740_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov9740_subdev_info),
	.func_tbl = &ov9740_sensor_func_tbl,
};

module_init(ov9740_init_module);
module_exit(ov9740_exit_module);
MODULE_DESCRIPTION("ov9740");
MODULE_LICENSE("GPL v2");
