# soundtrigger driver
GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
subdir-ccflags-y += -Wall -Werror

EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/codec_bus
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/codec_bus
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/da_combine
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/comm
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/mailbox
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms
EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/include/audio/ipc
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include

obj-$(CONFIG_SOUND_TRIGGER) += soundtrigger_ring_buffer.o
obj-$(CONFIG_SOUND_TRIGGER) += soundtrigger_pcm_drv.o
obj-$(CONFIG_SOUND_TRIGGER) += soundtrigger_pcm_drv_comm.o
ifneq ($(filter y,$(CONFIG_SND_SOC_DA_COMBINE_V3) $(CONFIG_SND_SOC_DA_COMBINE_V5)),)
obj-y += soundtrigger_pcm_drv_da_combine.o
endif
obj-$(CONFIG_SND_SOC_THIRD_CODEC) += soundtrigger_pcm_drv_third_codec.o
obj-$(CONFIG_SND_SOC_THIRD_CODEC) += soundtrigger_third_codec_data_proc.o
obj-$(CONFIG_SOUND_TRIGGER) += soundtrigger_event.o
obj-$(CONFIG_SOUND_TRIGGER) += soundtrigger_socdsp_mailbox.o
obj-$(CONFIG_SOUND_TRIGGER) += soundtrigger_socdsp_pcm.o
