# SPDX-License-Identifier: GPL-2.0-only
obj-$(CONFIG_CDNS_I3C_MASTER)		+= i3c-master-cdns.o
obj-$(CONFIG_DW_I3C_MASTER)		+= dw-i3c-master.o

ifeq ($(CONFIG_I3C_V100_DESIGNWARE), y)
EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/i3c/
endif