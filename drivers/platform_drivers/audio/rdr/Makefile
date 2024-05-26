obj-y += rdr_audio_adapter.o
obj-y += rdr_audio_codec.o
obj-$(CONFIG_HIFI_DSP_ONE_TRACK) += rdr_audio_soc.o
obj-y += rdr_audio_dump_socdsp.o

ifeq ($(CONFIG_HIFI_EXTRA_MODEM_MODE),y)
EXTRA_CFLAGS += -DEXTERNAL_MODEM
obj-y += rdr_audio_notify_modem.o
EXTRA_CFLAGS += -DHIFI_RESET_CDEV

endif

ifeq ($(CONFIG_HIFI_RESET_CDEV),y)
obj-y += rdr_audio_notify_modem.o
EXTRA_CFLAGS += -DHIFI_RESET_CDEV
endif

EXTRA_CFLAGS += -I$(srctree)/sound/soc/platforms/common_image/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp/om/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/om/
EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/mntn/blackbox/
EXTRA_CFLAGS += -I$(srctree)/sound/usb/hifi/
EXTRA_CFLAGS += -I$(srctree)/include/audio/ipc
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include

ccflags-y += -I$(srctree)/drivers/platform_drivers/tzdriver

EXTRA_CFLAGS += -DSECOS_RELOAD_HIFI
ifeq ($(CONFIG_HIFI_DSP_ONE_TRACK),y)
EXTRA_CFLAGS += -DRDR_SOCDSP
endif
EXTRA_CFLAGS += -DRDR_CODECDSP

ccflags-y += -I$(srctree)/drivers/devkit/audiokit/
