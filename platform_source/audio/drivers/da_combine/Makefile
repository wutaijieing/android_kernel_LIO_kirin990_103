GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
subdir-ccflags-y += -Wall -Werror

EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/da_combine_dsp/om/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/soundtrigger/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/fpga_test/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/hs_auto_calib/
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/audio/rdr/
EXTRA_CFLAGS += -I$(srctree)/sound/soc/codecs/vendor/adapter/
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/audiokit/
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/include/

ifeq ($(CONFIG_HUAWEI_AUDIO),y)
ifneq ($(strip $(CONFIG_SWITCH)),)
ccflags-y += -DCONFIG_HUAWEI_HEADSET_DEBUG_SWITCH
endif
endif

ifeq ($(CONFIG_AUDIO_DEBUG),y)
ifeq ($(CONFIG_HUAWEI_AUDIO),y)
EXTRA_CFLAGS += -DCONFIG_HUAWEI_HEADSET_DEBUG
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/audiokit/headset_debug/
endif
ifeq ($(kcov),true)
EXTRA_CFLAGS += -DKCOV_AUDIO
else
EXTRA_CFLAGS += -DENABLE_DA_COMBINE_CODEC_DEBUG
obj-y += da_combine_mad.o
endif
#ifeq ($(CONFIG_SND_SOC_CODEC_STUB),y)
#EXTRA_CFLAGS += -DDA_SEPARATE_FPGA_CODEC_STUB
#endif
endif

ccflags-y += -I$(srctree)/drivers/devkit/audiokit/

obj-y += hi_cdc_ctrl.o
obj-y += hi_cdc_ssi.o
obj-y += da_combine_resmgr.o
obj-y += da_combine_mbhc.o
obj-y += da_combine_mbhc_rear_jack.o
obj-y += da_combine_irq.o
obj-y += da_combine_compat.o
obj-y += da_combine_utils.o
obj-y += da_combine_vad.o
