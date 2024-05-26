# SPDX-License-Identifier: GPL-2.0
# hisilicon clock driver
ccflags-y +=  -I$(srctree)/drivers/hwspinlock

obj-$(CONFIG_VENDOR_HWSPINLOCK)		+= vendor_hwspinlock.o
obj-$(CONFIG_VENDOR_HWSPINLOCK_DEBUG)	+= vendor_hwspinlock_debugfs.o

ifeq ($(CONFIG_FMEA_FAULT_INJECTION), y)
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/ap/platform/common/
endif