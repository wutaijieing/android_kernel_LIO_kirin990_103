#
# Makefile for hisilicon camera mcu driver
#
EXTRA_CFLAGS += -I$(srctree)/platform_source/camera/drivers/media/native/camera/fpga/stm32
EXTRA_CFLAGS += -I$(srctree)/platform_source/camera/drivers/media/native/camera/fpga
EXTRA_CFLAGS += -I$(srctree)/platform_source/camera/drivers/media/native/camera/
EXTRA_CFLAGS += -fno-pic

obj-$(CONFIG_KERNEL_CAMERA_FPGA_STM32) += stm32_spi.o
obj-$(CONFIG_KERNEL_CAMERA_FPGA_STM32) += stm32.o
