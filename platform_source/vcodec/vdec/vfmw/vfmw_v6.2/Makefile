# local variables
#===============================================================================
VFMW_DIR := platform_source/vcodec/vdec/vfmw/vfmw_v6.2

################################################################################################
#    PRODUCT
################################################################################################
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/vdec/drv_v2.0
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/include

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV600),y)
include $(VFMW_DIR)/product/VCodecV600/linux_cfg/vfmw_make.cfg
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV700),y)
include $(VFMW_DIR)/product/VCodecV700/linux_cfg/vfmw_make.cfg
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV720),y)
include $(VFMW_DIR)/product/VCodecV720/linux_cfg/vfmw_make.cfg
endif

ifeq ($(CONFIG_VCODEC_VDEC),m)
EXTRA_CFLAGS += -DENABLE_VDEC_MODULE
endif

EXTRA_CFLAGS += $(VFMW_CFLAGS)
EXTRA_CFLAGS += -fno-pic

MOD_NAME := vcodec_vfmw
obj-$(CONFIG_VCODEC_VDEC) += $(MOD_NAME).o

VFMW_OBJ := $(VFMW_SRC:%.c=%.o)
$(MOD_NAME)-objs := $(VFMW_OBJ)
