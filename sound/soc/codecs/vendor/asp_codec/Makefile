GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/asp_codec/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/asp_codec/multi_kcontrol/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/asp_codec/single_kcontrol/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include/

snd-soc-aspcodec-objs :=asp_codec.o
snd-soc-aspcodec-utils-objs :=asp_codec_utils.o
snd-soc-aspcodec-kcontrol-objs := asp_codec_kcontrol.o
snd-soc-aspcodec-multi-objs :=/multi_kcontrol/asp_codec_resource_widget.o /multi_kcontrol/asp_codec_path_widget.o /multi_kcontrol/asp_codec_switch_widget.o /multi_kcontrol/asp_codec_route.o
snd-soc-aspcodec-single-objs :=/single_kcontrol/asp_codec_single_switch_widget.o /single_kcontrol/asp_codec_single_kcontrol.o  /single_kcontrol/asp_codec_single_route.o

obj-$(CONFIG_SND_SOC_ASP_CODEC) += snd-soc-aspcodec.o
obj-$(CONFIG_SND_SOC_ASP_CODEC) += snd-soc-aspcodec-utils.o
obj-$(CONFIG_SND_SOC_ASP_CODEC) += snd-soc-aspcodec-kcontrol.o
obj-$(CONFIG_SND_SOC_ASP_CODEC) += snd-soc-aspcodec-multi.o
obj-$(CONFIG_SND_SOC_ASP_CODEC) += snd-soc-aspcodec-single.o
