# hifi dsp device driver
ifeq ($(CONFIG_HIFI_IPC_3650),y)
EXTRA_CFLAGS += -DPLATFORM_HI3XXX
endif

ifeq ($(CONFIG_HIFI_IPC_6250),y)
EXTRA_CFLAGS += -DPLATFORM_HI3XXX
#todo fixme: (PLATFORM_FPGA should be deleted on asic)
EXTRA_CFLAGS += -DPLATFORM_FPGA
endif

EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/mailbox
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/ipcm
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image

ifeq ($(CONFIG_AUDIO_DEBUG),y)
EXTRA_CFLAGS += -DENABLE_HIFI_DEBUG
endif

EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp
EXTRA_CFLAGS += -I$(srctree)/include/audio/ipc
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include

obj-$(CONFIG_VOICE_PROXY) += voice_proxy.o
obj-$(CONFIG_VOICE_PROXY_VOLTE) += voice_proxy_volte.o
obj-$(CONFIG_VOICE_PROXY_VOWIFI) += voice_proxy_vowifi.o
obj-$(CONFIG_VOICE_PROXY_FUSION) += voice_proxy_fusion.o
