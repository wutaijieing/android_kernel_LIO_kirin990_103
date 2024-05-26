GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
subdir-ccflags-y += -Wall -Werror

EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/audiokit
ifeq ($(CONFIG_HIFI_MAILBOX), y)
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/mailbox
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/ipcm
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp
endif

EXTRA_CFLAGS +=	-I$(srctree)/platform_source/audio/drivers/codec_bus/
EXTRA_CFLAGS +=	-I$(srctree)/include/linux/platform_drivers/
EXTRA_CFLAGS +=	-I$(srctree)/platform_source/audio/drivers/hifi_dsp/
EXTRA_CFLAGS += -I$(srctree)/include/audio/ipc
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include

ifeq ($(CONFIG_SND_SOC_CDC_PCM), y)
snd-soc-pcm-dsp-objs := audio_pcm_hifi.o cdc_pcm_config.o
else
snd-soc-pcm-dsp-objs := audio_pcm_hifi.o phone_pcm_config.o
endif
snd-soc-asp-cfg-objs := asp_cfg.o asp_dma.o
obj-$(CONFIG_HIFI_DSP_ONE_TRACK) += snd-soc-pcm-dsp.o

# DP Support
snd-soc-dp-objs := audio_dp_machine.o audio_pcm_dp.o asp_hdmi_dma.o

snd-soc-da-separate-machine-objs += da_separate_machine.o
snd-soc-da-combine-machine-objs := da_combine_machine.o
snd-soc-analog-less-machine-objs := analog_less_machine.o
platform-base-addr-info-objs := common_image/platform_base_addr_info.o

obj-$(CONFIG_SND_DA_SEPARATE_MACHINE) +=snd-soc-da-separate-machine.o
obj-$(CONFIG_SND_DA_COMBINE_MACHINE) += snd-soc-da-combine-machine.o
obj-$(CONFIG_SND_ANALOG_LESS_MACHINE) += snd-soc-analog-less-machine.o
obj-y += platform-base-addr-info.o

obj-$(CONFIG_SND_SOC_DP_AUDIO) += snd-soc-dp.o
obj-$(CONFIG_SND_SOC_PCM_CODECS) += pcm_codec/
obj-y += snd-soc-asp-cfg.o
