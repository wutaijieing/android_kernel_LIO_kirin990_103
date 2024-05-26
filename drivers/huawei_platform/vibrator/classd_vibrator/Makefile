
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/mailbox
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/ipcm
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include
EXTRA_CFLAGS += -I$(srctree)/include/audio/ipc

obj-$(CONFIG_CLASSD_VIBRATOR) += classd_vibrator.o
obj-$(CONFIG_CLASSD_VIBRATOR) += dsp_vibrator.o

ifeq ($(TARGET_VERSION_MODE),factory)
ccflags-y += -DCLASSD_VIBRATOR_DEBUG
endif
