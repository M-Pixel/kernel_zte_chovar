ccflags-y += -Idrivers/media/platform/msm/camera_v2
ccflags-y += -Idrivers/media/platform/msm/camera_v2/sensor/io

#
# Modified by ZTE_JIA_20130925 jia.jia
#
ccflags-y += -Idrivers/media/platform/msm/camera_v2/sensor/cci

obj-$(CONFIG_MSMB_CAMERA) += msm_led_flash.o

#
# Modified by ZTE_JIA_20130925 jia.jia
#
# --->
#obj-$(CONFIG_MSMB_CAMERA) += msm_led_trigger.o
#obj-$(CONFIG_MSMB_CAMERA) += msm_led_i2c_trigger.o
#obj-$(CONFIG_MSMB_CAMERA) += adp1660.o
#obj-$(CONFIG_MSMB_CAMERA) += msm_led_torch.o
obj-$(CONFIG_MSMB_CAMERA) += msm_led_cci_trigger_zte.o
obj-$(CONFIG_LM3642) += lm3642.o
# <---
