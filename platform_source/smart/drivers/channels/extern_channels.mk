ifeq ($(wildcard $(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)),)
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/default/
else
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)/
endif

ccflags-y  += -I$(srctree)/include/linux/platform_drivers/contexthub

obj-$(CONFIG_CONTEXTHUB) += inputhub_api/inputhub_api.o

ifeq ($(TARGET_BOARD_PLATFORM), kirin970)
obj-$(CONFIG_CONTEXTHUB) += common/$(TARGET_BOARD_PLATFORM)/common.o
else
obj-$(CONFIG_CONTEXTHUB) += common/common.o
endif

ifneq ($(CONFIG_INPUTHUB_30),y)
obj-$(CONFIG_CONTEXTHUB_SHMEM) += shmem/shmem.o
endif
obj-$(CONFIG_CONTEXTHUB_SHELL) += shell_dbg/shell_dbg.o
obj-$(CONFIG_CONTEXTHUB_LOADMONITOR) += loadmonitor/loadmonitor.o
obj-$(CONFIG_CONTEXTHUB_PLAYBACK) += dbg/playback.o
obj-$(CONFIG_CONTEXTHUB_CHRE) += chre/chre.o
obj-$(CONFIG_CONTEXTHUB)   += flp/
obj-$(CONFIG_CONTEXTHUB_IGS)   += igs/
obj-$(CONFIG_CONTEXTHUB_UDI) += dump_inject/dump_inject.o
obj-$(CONFIG_CONTEXTHUB_BLE) += ble/ble_channel.o
