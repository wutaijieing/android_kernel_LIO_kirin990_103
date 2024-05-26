EXTRA_CFLAGS += -DENV_ARMLINUX_KERNEL
EXTRA_CFLAGS += -DVCODEC_VDEC_SMMU_SUPPORT

EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/include
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vdec/drv_v1.0
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0/firmware/osal/linux_kernel
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0/firmware/osal
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0/firmware/common
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0/scene/adapter
EXTRA_CFLAGS += -fno-pic

ifeq ($(CONFIG_VCODEC_DEBUG), y)
EXTRA_CFLAGS += -DUSER_ENABLE_VDEC_PROC
endif

ifneq ($(findstring $(CONFIG_VCODEC_PLATFORM_VCODECV500)$(CONFIG_VCODEC_PLATFORM_VCODECV520), y),)
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0/firmware/hal/SCDV310
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0/firmware/product/VCodecV500
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0/firmware/hal/VDHV500R003
else
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0/firmware/hal/SCDV300
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0/firmware/product/VCodecV200
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vfmw/vfmw_v4.0/firmware/hal/VDHV500R002
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV200), y)
EXTRA_CFLAGS +=-DPLATFORM_VCODECV200
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV300), y)
EXTRA_CFLAGS += -DPLATFORM_VCODECV300
EXTRA_CFLAGS += -DVDEC_AVS_LOW_CFG
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV500), y)
EXTRA_CFLAGS += -DPLATFORM_VCODECV500
EXTRA_CFLAGS += -DLOWER_FREQUENCY_SUPPORT
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV520), y)
EXTRA_CFLAGS += -DPLATFORM_VCODECV520
EXTRA_CFLAGS += -DLOWER_FREQUENCY_SUPPORT
EXTRA_CFLAGS += -DVCODEC_SMMUV300
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV210), y)
EXTRA_CFLAGS += -DPLATFORM_VCODECV210
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV310), y)
EXTRA_CFLAGS += -DPLATFORM_VCODECV310
EXTRA_CFLAGS += -DLOWER_FREQUENCY_SUPPORT
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV320), y)
EXTRA_CFLAGS += -DPLATFORM_VCODECV320
EXTRA_CFLAGS += -DVCODEC_SMMUV300
endif

ifeq ($(es_low_freq),true)
EXTRA_CFLAGS += -DCONFIG_ES_VDEC_LOW_FREQ
endif

#build in
obj-$(CONFIG_VCODEC_VDEC) += omxvdec.o
obj-$(CONFIG_VCODEC_VDEC) += regulator.o
obj-$(CONFIG_VCODEC_VDEC) += memory.o
