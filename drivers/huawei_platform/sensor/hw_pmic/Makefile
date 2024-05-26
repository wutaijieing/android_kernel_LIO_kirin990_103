#
# Makefile for hisilicon pmic sensor driver
#

obj-y += hw_pmic.o
obj-y += gp_buck.o
obj-$(CONFIG_KERNEL_CAMERA_PMIC_RT5112) += rt5112.o
obj-$(CONFIG_KERNEL_CAMERA_PMIC_FAN53880) += fan53880.o
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera/pmic
