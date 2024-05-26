GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/audiokit
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/da_combine/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include

ifeq ($(CONFIG_PLATFORM_SLIMBUS),y)
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/codec_bus
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/codec_bus
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/codec_bus/slimbus
endif

snd-soc-format-objs := format.o
snd-soc-platform-io-objs := platform_io.o
snd-soc-pcm-codec-objs := pcm_codec.o
snd-soc-cdc-custom-objs := cdc_custom.o
snd-soc-armpc-custom-objs := armpc_custom.o

ifeq ($(CONFIG_DFX_DEBUG_FS),y)
EXTRA_CFLAGS += -DAUDIO_PCM_CODEC_DEBUG
snd-soc-om-objs := om.o
obj-$(CONFIG_SND_SOC_PCM_CODECS) += snd-soc-om.o
endif

obj-$(CONFIG_SND_SOC_PCM_CODECS) += snd-soc-format.o
obj-$(CONFIG_SND_SOC_PCM_CODECS) += snd-soc-platform-io.o
obj-$(CONFIG_SND_SOC_PCM_CODECS) += snd-soc-pcm-codec.o
obj-$(CONFIG_SND_SOC_CDC_CUSTOM) += snd-soc-cdc-custom.o
obj-$(CONFIG_SND_SOC_ARMPC_CUSTOM) += snd-soc-armpc-custom.o
