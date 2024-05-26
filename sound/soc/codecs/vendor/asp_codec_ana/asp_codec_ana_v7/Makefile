GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_separate/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/adapter/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/asp_codec_codecless/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/asp_codec_ana/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include/

snd-soc-asp-codec-ana-path-objs :=asp_codec_ana_path.o
obj-$(CONFIG_SND_SOC_ASP_CODEC_ANA_V7) += snd-soc-asp-codec-ana-path.o
