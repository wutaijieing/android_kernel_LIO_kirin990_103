GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/codec_bus/
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/codec_bus/
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/audiokit/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/config/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/adapter/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include/

snd-soc-da-combine-v3-objs :=da_combine_v3.o
snd-soc-kcontrol-objs :=kcontrol.o
snd-soc-resource-widget-objs :=resource_widget.o
snd-soc-path-widget-objs :=path_widget.o
snd-soc-switch-widget-objs :=switch_widget.o
snd-soc-route-objs :=route.o
snd-soc-headset-config-objs :=headset_config.o

obj-$(CONFIG_SND_SOC_DA_COMBINE_V3) += snd-soc-da-combine-v3.o
obj-$(CONFIG_SND_SOC_DA_COMBINE_V3) += snd-soc-kcontrol.o
obj-$(CONFIG_SND_SOC_DA_COMBINE_V3) += snd-soc-resource-widget.o
obj-$(CONFIG_SND_SOC_DA_COMBINE_V3) += snd-soc-path-widget.o
obj-$(CONFIG_SND_SOC_DA_COMBINE_V3) += snd-soc-switch-widget.o
obj-$(CONFIG_SND_SOC_DA_COMBINE_V3) += snd-soc-route.o
obj-$(CONFIG_SND_SOC_DA_COMBINE_V3) += snd-soc-headset-config.o
