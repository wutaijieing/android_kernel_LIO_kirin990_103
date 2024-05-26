GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_separate/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/da_separate/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/adapter/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include/

snd-soc-da_separate_v3-objs :=da_separate_v3.o
obj-$(CONFIG_SND_SOC_CODEC_DA_SEPARATE_V3) += snd-soc-da_separate_v3.o
