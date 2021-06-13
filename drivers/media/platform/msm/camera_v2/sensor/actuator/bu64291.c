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
 * Support for Rohm bu64291 actuator
 */

#include "msm_actuator.h"

static const struct of_device_id bu64291_dt_match[] = {
	{.compatible = "qcom,bu64291", .data = NULL},
	{}
};

MODULE_DEVICE_TABLE(of, bu64291_dt_match);

static struct platform_driver bu64291_platform_driver = {
	.driver = {
		.name = "qcom,bu64291",
		.owner = THIS_MODULE,
		.of_match_table = bu64291_dt_match,
	},
};

static int __init bu64291_init_module(void)
{
	int32_t rc = 0;
	rc = platform_driver_probe(&bu64291_platform_driver,
		msm_actuator_platform_probe);
	return rc;
}

static void __exit bu64291_exit_module(void)
{
	platform_driver_unregister(&bu64291_platform_driver);
	return;
}

module_init(bu64291_init_module);
module_exit(bu64291_exit_module);
MODULE_DESCRIPTION("Rohm bu64291 actuator driver");
MODULE_LICENSE("GPL v2");
