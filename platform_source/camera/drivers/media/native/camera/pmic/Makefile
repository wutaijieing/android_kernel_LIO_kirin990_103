#
# Makefile for camera pmic driver
#
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera

obj-y += hw_pmic.o
obj-y += hw_pmic_i2c.o
obj-y += ncp6925.o
obj-$(CONFIG_KERNEL_CAMERA_PMIC_RT5112) += rt5112.o
obj-$(CONFIG_KERNEL_CAMERA_PMIC_FAN53880) += fan53880.o
