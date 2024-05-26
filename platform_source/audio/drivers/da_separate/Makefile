GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
subdir-ccflags-y += -Wall -Werror

EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/adapter/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/da_separate/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/hs_auto_calib/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/da_separate/da_separate_v3/
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/audiokit/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/asp_codec_ana/asp_codec_ana_v7/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include/

ifeq ($(CONFIG_HUAWEI_AUDIO),y)
ifneq ($(strip $(CONFIG_SWITCH)),)
ccflags-y += -DCONFIG_HUAWEI_HEADSET_DEBUG_SWITCH
endif

ifeq ($(CONFIG_AUDIO_DEBUG),y)
EXTRA_CFLAGS += -DCONFIG_HUAWEI_HEADSET_DEBUG
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/audiokit/headset_debug/
endif
endif

snd-soc-da_separate_mbhc-objs := da_separate_mbhc_custom.o
snd-soc-da_separate_mbhc-objs += da_separate_mbhc.o

obj-$(CONFIG_SND_SOC_DA_SEPARATE_MBHC) += snd-soc-da_separate_mbhc.o
