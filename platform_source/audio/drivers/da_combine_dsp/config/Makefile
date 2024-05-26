# da_combine dsp device driver
GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
subdir-ccflags-y += -Wall -Werror

ccflags-y += -I$(srctree)/drivers/devkit/audiokit/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/codec_bus/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp/
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/da_combine_dsp/
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/audio/rdr/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/soundtrigger/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/om/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/comm/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/download/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include/

ifeq ($(CONFIG_AUDIO_DEBUG),y)
ifeq ($(kcov),true)
EXTRA_CFLAGS += -DENABLE_AUDIO_KCOV
else
EXTRA_CFLAGS += -DENABLE_DA_COMBINE_HIFI_DEBUG
endif
endif

obj-$(CONFIG_CODEC_DSP_MISC) += da_combine_v3_dsp_config.o

ifeq ($(CONFIG_SND_SOC_DA_COMBINE_V5),y)
obj-$(CONFIG_SND_SOC_DA_COMBINE_V5) += da_combine_v5_dsp_config.o
endif
