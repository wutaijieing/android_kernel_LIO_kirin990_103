#
# Makefile for hisilicon camera fpga driver
#

EXTRA_CFLAGS += -I$(srctree)/platform_source/camera/drivers/media/native/camera/
EXTRA_CFLAGS += -fno-pic

obj-$(CONFIG_KERNEL_CAMERA_DRIVER_IC) += hwdriver_ic.o
obj-$(CONFIG_KERNEL_CAMERA_DRIVER_IC_MP3336) += mp3336/
obj-$(CONFIG_KERNEL_CAMERA_DRIVER_IC_MP3331) += mp3331/
obj-$(CONFIG_KERNEL_CAMERA_DRIVER_IC) += lm3644/
