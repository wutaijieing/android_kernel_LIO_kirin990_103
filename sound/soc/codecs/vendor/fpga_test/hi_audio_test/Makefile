EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/fpga_test/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/da_separate/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_separate/
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/da_combine/
EXTRA_CFLAGS += -I$(srctree)/include/linux/platform_drivers/
EXTRA_CFLAGS += -I$(srctree)/include/audio/ipc/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include/
EXTRA_CFLAGS += -I$(srctree)/bsp/libc_sec/include/


obj-$(CONFIG_SND_SOC_CODEC_STUB) += snd-soc-hi-audio-test-driver.o
snd-soc-hi-audio-test-driver-objs := hi_audio_test.o \
	hi_audio_test_dma.o \
	hi_audio_test_file.o
#obj-$(CONFIG_SND_SOC_CODEC_STUB) += hi_soundwire_test.o

#open test case
#obj-m += da_combine_v5_codec_test.o
#obj-m += hi_asp_codec_test.o
#obj-m += da_separate_codec_test.o
#obj-m += hi_dsd_test.o
#obj-m += hi_sio_test.o
#obj-m += hi_soundwire_test.o
#obj-m += hi_asp_wear_codec_test.o
