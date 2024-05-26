ccflags-$(CONFIG_DPU_FB_V410) += -I$(srctree)/platform_source/display/drivers/jpu/jpgdecv400
ccflags-$(CONFIG_DPU_FB_V501) += -I$(srctree)/platform_source/display/drivers/jpu/jpgdecv501
ccflags-$(CONFIG_DPU_FB_V510) += -I$(srctree)/platform_source/display/drivers/jpu/jpgdecv511
ccflags-$(CONFIG_DPU_FB_V600) += -I$(srctree)/platform_source/display/drivers/jpu/jpgdecv600
ifeq ($(CONFIG_DKMD_DPU_VERSION), 700)
ccflags-y += -I$(srctree)/platform_source/display/drivers/jpu/jpgdecv700
endif


ifeq ($(es_low_freq),true)
	EXTRA_CFLAGS += -DCONFIG_ES_LOW_FREQ
endif
EXTRA_CFLAGS += -I$(srctree)/lib/libc_sec/securec_v2/include/

obj-$(CONFIG_JPU_SUPPORT) := devjpu.o
devjpu-objs := \
	jpu.o \
	jpu_iommu.o \
	jpu_utils.o \
	jpu_exec.o \
	jpgdec_irq.o

clean:
	rm *.o .*cmd
