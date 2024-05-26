#
# Makefile for camera laser driver
#

ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera
EXTRA_CFLAGS += -fno-pic

obj-y += laser_common.o
obj-$(CONFIG_KERNEL_CAMERA_VL53L0_970) += hw_laser_subdev.o
obj-$(CONFIG_KERNEL_CAMERA_VL53L0) += vl53l0.o
obj-$(CONFIG_KERNEL_CAMERA_VL53L0_970) += vl53Lx/
# obj-$(CONFIG_KERNEL_CAMERA_VL53L0_970) += vi53xx/
