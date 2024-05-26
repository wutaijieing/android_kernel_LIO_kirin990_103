# SPDX-License-Identifier: GPL-2.0
#
# Makefile for ARPP drivers.
#

obj-$(CONFIG_ARPP) += arpp.o

ccflags-$(CONFIG_ARPP_V100) += -I$(srctree)/platform_source/camera/drivers/arpp/platform/v100

arpp-objs := \
	arpp_smmu.o \
	arpp_isr.o \
	arpp_hwacc.o \
	arpp_core.o

obj-$(CONFIG_ARPP_V100) += /platform/v100/

clean:
	rm *.o .*cmd
