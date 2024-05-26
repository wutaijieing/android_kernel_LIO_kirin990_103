#
# Makefile for the touchscreen drivers.
#

obj-$(CONFIG_ST_TS_KIT_3_0) += fts.o
obj-$(CONFIG_ST_TS_KIT_3_0) += fts_proc.o
obj-$(CONFIG_ST_TS_KIT_3_0) += fts_lib/
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/tzdriver
EXTRA_CFLAGS += -I$(srctree)/drivers/tzdriver
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/common/include
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/tpkit/3_0
