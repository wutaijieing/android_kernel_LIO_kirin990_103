#
# Makefile for hisilicon camera fpga driver
#
EXTRA_CFLAGS += -I$(srctree)/platform_source/camera/drivers/media/native/camera/fpga/ice40
EXTRA_CFLAGS += -I$(srctree)/platform_source/camera/drivers/media/native/camera/fpga
EXTRA_CFLAGS += -I$(srctree)/platform_source/camera/drivers/media/native/camera/
EXTRA_CFLAGS += -fno-pic

obj-$(CONFIG_KERNEL_CAMERA_FPGA_ICE40) += ice40_spi.o
obj-$(CONFIG_KERNEL_CAMERA_FPGA_ICE40) += ice40.o
