GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
subdir-ccflags-y += -Wall -Werror

ifeq ($(CONFIG_DP_MULTI_CHANNELS),y)
EXTRA_CFLAGS += -DCONFIG_DP_MULTI_CHANNELS
endif

ifeq ($(CONFIG_HIFI_SUPPORT_VIBRATOR),y)
EXTRA_CFLAGS += -DCONFIG_HIFI_SUPPORT_VIBRATOR
endif


ifeq ($(CONFIG_HIFI_IPC_6250),y)
EXTRA_CFLAGS += -DEXPAND_HIFI_RUNNING_REGION
endif

EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/mailbox/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/ipcm/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/soundtrigger/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp/om/
EXTRA_CFLAGS += -I$(srctree)/sound/usb/hifi/
EXTRA_CFLAGS += -I$(srctree)/include/audio/ipc/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include/

ifeq ($(CONFIG_AUDIO_DEBUG),y)
ifeq ($(kcov),true)
EXTRA_CFLAGS += -DENABLE_AUDIO_KCOV
else
EXTRA_CFLAGS += -DENABLE_HIFI_DEBUG
endif
endif

obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= om/
obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= dsp_misc.o
obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= dsp_msg.o
obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= dsp_lpp.o
obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= algorithm_dft.o

ifeq ($(CONFIG_HIFI_EXTRA_MODEM_MODE),y)
obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= audio_pcie_proc.o
obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= modem_strategy_acore.o
EXTRA_CFLAGS += -DEXTERNAL_MODEM
endif
ifeq ($(CONFIG_AUDIO_DEBUG),y)
obj-$(CONFIG_SND_SOC_CODEC_STUB)	+= memcpy_opt.o
EXTRA_CFLAGS += -DDYNAMIC_LOAD_HIFIIMG
endif

obj-y	+= voice_proxy/

ifeq ($(CONFIG_HIFI_EXTRA_MODEM_MODE),y)
obj-y	+= ap_clk_correct/
endif

ccflags-$(CONFIG_HIFI_DSP_ONE_TRACK) += -I$(srctree)/drivers/devkit/audiokit/
