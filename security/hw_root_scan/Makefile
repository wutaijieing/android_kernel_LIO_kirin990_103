#
# Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
# Description: Makefile for the Huawei root scanner.
# Create: 2016-06-18
#

obj-$(CONFIG_HW_ROOT_SCAN) += hw_rscan_utils.o
obj-$(CONFIG_HW_ROOT_SCAN) += kcode.o
obj-$(CONFIG_HW_ROOT_SCAN) += rproc.o
obj-$(CONFIG_HW_ROOT_SCAN) += sescan.o
obj-$(CONFIG_HW_ROOT_SCAN) += setids.o
obj-$(CONFIG_HW_ROOT_SCAN) += hw_rscan_scanner.o
obj-$(CONFIG_HW_ROOT_SCAN) += hw_rscan_module.o
ifdef CONFIG_MTK_PLATFORM
ccflags-$(CONFIG_HW_ROOT_SCAN) += -Idrivers/ca_antiroot
else
ccflags-$(CONFIG_HW_ROOT_SCAN) += -I$(srctree)/drivers/hisi/ca_antiroot
endif

flags += -Wdate-time -Wfloat-equal
