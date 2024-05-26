#
# Makefile for CoreSight drivers.
#

ccflags-y  += -I$(srctree)/drivers/platform_drivers/tzdriver
ccflags-y  += -I$(srctree)/platform_source/basicplatform/drivers/printk

obj-$(CONFIG_CORESIGHT) += coresight.o
obj-$(CONFIG_OF) += of_coresight.o
obj-$(CONFIG_CORESIGHT_LINK_AND_SINK_TMC) += coresight_tmc.o
obj-$(CONFIG_CORESIGHT_SINK_TPIU) += coresight_tpiu.o
obj-$(CONFIG_CORESIGHT_SINK_ETBV10) += coresight-etb10.o
obj-$(CONFIG_CORESIGHT_LINKS_AND_SINKS) += coresight_funnel.o \
					   coresight_replicator.o
obj-$(CONFIG_DFX_CORESIGHT_TRACE) += coresight_cti.o
obj-$(CONFIG_CORESIGHT_SOURCE_ETM4X) += coresight_tsgen.o coresight_etm4x.o
obj-$(CONFIG_DFX_CORESIGHT_STM) += coresight_stm.o
obj-$(CONFIG_DFX_CORESIGHT_NOC) += coresight_noc.o
subdir-ccflags-$(CONFIG_DFX_BB)               += -I$(srctree)/platform_source/basicplatform/drivers/mntn/blackbox/ -Wall -Werror
subdir-ccflags-$(CONFIG_CORESIGHT_LINK_AND_SINK_TMC)               += -I$(srctree)/platform_source/basicplatform/drivers/memory_dump/ -Wall -Werror
subdir-ccflags-$(CONFIG_CORESIGHT_LINK_AND_SINK_TMC)               += -I$(srctree)/kernel/printk/ -Wall -Werror


