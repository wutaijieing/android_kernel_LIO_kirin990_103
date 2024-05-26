EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/slimbus/
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/audiokit/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_separate/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/adapter/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include/

#  da_separate_v2 codec
snd-soc-da-separate-v2-objs :=da_separate_v2.o
snd-soc-da-separate-v2-utility-objs :=da_separate_v2_utility.o
snd-soc-da-separate-v2-i2s-objs :=da_separate_v2_i2s.o

obj-$(CONFIG_SND_SOC_CODEC_DA_SEPARATE_V2) += snd-soc-da-separate-v2.o
obj-$(CONFIG_SND_SOC_CODEC_DA_SEPARATE_V2) += snd-soc-da-separate-v2-utility.o
obj-$(CONFIG_SND_SOC_CODEC_DA_SEPARATE_V2) += snd-soc-da-separate-v2-i2s.o
