GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
subdir-ccflags-y += -Wall -Werror

EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/mailbox
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/ipcm
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp/
EXTRA_CFLAGS += -I$(srctree)/sound/usb/hifi/
EXTRA_CFLAGS += -I$(srctree)/include/audio/ipc
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include

ifeq ($(CONFIG_AUDIO_DEBUG),y)
ifeq ($(kcov),true)
EXTRA_CFLAGS += -DENABLE_AUDIO_KCOV
else
EXTRA_CFLAGS += -DENABLE_HIFI_DEBUG
endif
endif

ifeq ($(CONFIG_AUDIO_DEBUG),y)
ifeq ($(CONFIG_SND_SOC_CODEC_STUB),y)
EXTRA_CFLAGS += -DDYNAMIC_LOAD_HIFIIMG
endif
endif

obj-y += dsp_om.o
obj-y += om_debug.o
obj-$(CONFIG_HUAWEI_AUDIO) += om_bigdata.o

ccflags-$(CONFIG_HIFI_DSP_ONE_TRACK) += -I$(srctree)/drivers/devkit/audiokit/
