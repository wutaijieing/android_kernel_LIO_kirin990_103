obj-$(CONFIG_SND_SOC_DA_SEPARATE_MBHC) += da_separate/
ifneq ($(filter y,$(CONFIG_DA_COMBINE_V3_CODEC) $(CONFIG_SND_SOC_DA_COMBINE_V5)),)
    obj-y += da_combine/
endif
obj-$(CONFIG_HIFI_DSP_ONE_TRACK) += hifi_dsp/
obj-$(CONFIG_SOUND_TRIGGER) += soundtrigger/
obj-$(CONFIG_HIFI_MAILBOX) += hifi_mailbox/
obj-$(CONFIG_CODEC_DSP_MISC) += da_combine_dsp/
obj-$(CONFIG_CODEC_BUS) += codec_bus/
obj-$(CONFIG_SOC_MAD) += soc_mad/
