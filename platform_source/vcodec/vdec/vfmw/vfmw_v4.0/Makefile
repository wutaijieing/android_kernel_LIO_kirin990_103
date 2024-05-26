VFMW_DIR := platform_source/vcodec/vdec/vfmw

ifneq ($(findstring $(CONFIG_VCODEC_PLATFORM_VCODECV500)$(CONFIG_VCODEC_PLATFORM_VCODECV520), y),)
include $(VFMW_DIR)/vfmw_v4.0/firmware/product/VCodecV500/NOR_CFG/vfmw_make.cfg
EXTRA_CFLAGS += -DSECURE_VS_NOR_SECURE
else
include $(VFMW_DIR)/vfmw_v4.0/firmware/product/VCodecV200/NOR_CFG/vfmw_make.cfg
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV300), y)
EXTRA_CFLAGS += -DSECURE_VS_NOR_SECURE
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV500), y)
EXTRA_CFLAGS += -DVCODEC_SMMUV170
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV310), y)
EXTRA_CFLAGS += -DVCODEC_SMMUV170
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV520), y)
EXTRA_CFLAGS += -DVCODEC_SMMUV300
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV320), y)
EXTRA_CFLAGS += -DVCODEC_SMMUV300
endif

EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/include
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vdec/common
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vdec/drv_v1.0
EXTRA_CFLAGS += $(VFMW_CFLAGS) -fno-pic
EXTRA_CFLAGS += -fstack-protector-all

#build in
obj-$(CONFIG_VCODEC_VDEC) += vcodec_vfmw.o
vcodec_vfmw-objs := $(VFMW_CFILES)

#module
#obj-m += vcodec_vfmw.o
#vcodec_vfmw-y := $(VFMW_CFILES)
