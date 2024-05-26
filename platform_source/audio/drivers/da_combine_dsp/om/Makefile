# da_combine dsp device driver
GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
subdir-ccflags-y += -Wall -Werror

ccflags-y += -I$(srctree)/drivers/devkit/audiokit/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/codec_bus/
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/codec_bus/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp/
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/da_combine_dsp/
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/audio/rdr/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/comm/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/download/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/include/audio/ipc
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include

ifeq ($(CONFIG_AUDIO_DEBUG),y)
ifeq ($(kcov),true)
EXTRA_CFLAGS += -DENABLE_AUDIO_KCOV
else
EXTRA_CFLAGS += -DENABLE_DA_COMBINE_HIFI_DEBUG
obj-$(CONFIG_CODEC_DSP_MISC) += om_debug.o
endif
endif

obj-$(CONFIG_CODEC_DSP_MISC) += om_hook.o
obj-$(CONFIG_CODEC_DSP_MISC) += om_beta.o
obj-$(CONFIG_CODEC_DSP_MISC) += om.o
