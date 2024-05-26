obj-$(CONFIG_HW_WAUDIO_MODULE) += wifi_audio.o wifi_audio_utils.o wifi_audio_ptp.o
ifeq (v818, $(findstring v818,$(TARGET_DEVICE)))
$(warning waudio honghu v818 platform)
KBUILD_CFLAGS += -DHONGHU_WIFI_AUDIO_CAST_PCM_DATA -DHONGHU_V818_PLATFORM
else ifeq (v900, $(findstring v900,$(TARGET_DEVICE)))
$(warning waudio honghu v900 platform)
KBUILD_CFLAGS += -DHONGHU_WIFI_AUDIO_CAST_PCM_DATA -DHONGHU_V900_PLATFORM
else
$(warning waudio not honghu platform)
endif