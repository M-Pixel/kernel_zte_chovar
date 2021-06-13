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
 * Created by ZTE_BOOT_JIA_20130729 jia.jia
 * Support for Dongwoon dw9714 actuator
 */

#include "msm_actuator.h"

static const struct of_device_id dw9714_dt_match[] = {
	{.compatible = "qcom,dw9714", .data = NULL},
	{}
};

MODULE_DEVICE_TABLE(of, dw9714_dt_match);

static struct platform_driver dw9714_platform_driver = {
	.driver = {
		.name = "qcom,dw9714",
		.owner = THIS_MODULE,
		.of_match_table = dw9714_dt_match,
	},
};

static int __init dw9714_init_module(void)
{
	int32_t rc = 0;
	rc = platform_driver_probe(&dw9714_platform_driver,
		msm_actuator_platform_probe);
	return rc;
}

static void __exit dw9714_exit_module(void)
{
	platform_driver_unregister(&dw9714_platform_driver);
	return;
}

module_init(dw9714_init_module);
module_exit(dw9714_exit_module);
MODULE_DESCRIPTION("Dongwoon dw9714 actuator driver");
MODULE_LICENSE("GPL v2");
