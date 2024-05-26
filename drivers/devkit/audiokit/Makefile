# Audiokit driver

obj-$(CONFIG_HUAWEI_DSM_AUDIO) += dsm_audio/
obj-$(CONFIG_HUAWEI_DEVICEBOX_AUDIO) += boxid/
obj-$(CONFIG_HUAWEI_SMARTPAKIT_AUDIO) += smartpakit/
obj-$(CONFIG_HUAWEI_ANA_HS_KIT_AUDIO) += ana_hs_kit/
obj-$(CONFIG_HUAWEI_ADSP_MISC) += adsp_misc/
obj-$(CONFIG_AUDIO_TRACE) += trace/
obj-$(CONFIG_AUDIO_GPIO_CTL) += audio_gpio_ctl/
ifndef CONFIG_HUAWEI_ARMPC_PLATFORM
ifneq ($(TARGET_BUILD_VARIANT), user)
	obj-y += headset_debug/
endif
endif
