
VFMW_DIR := platform_source/vcodec/vdec/vfmw/vfmw_v6.2

VFMW_INC_DIR := $(VFMW_DIR)

VDEC_DRV_PATH := $(srctree)/platform_source/vcodec/vdec/vdec/drv_v2.0

LINUX_VER := linux

EXTRA_CFLAGS += -DENV_ARMLINUX_KERNEL

EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/vdec/include
EXTRA_CFLAGS += -I$(VDEC_DRV_PATH)
EXTRA_CFLAGS += -I$(VDEC_DRV_PATH)/smmu
EXTRA_CFLAGS += -I$(srctree)/$(VFMW_INC_DIR)/include
EXTRA_CFLAGS += -I$(srctree)/$(VFMW_INC_DIR)/intf

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV600), y)
EXTRA_CFLAGS += -DPLATFORM_VCODECV600
include $(VDEC_DRV_PATH)/product/VCodecV600/product_make.cfg
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV700), y)
EXTRA_CFLAGS += -DPLATFORM_VCODECV700
include $(VDEC_DRV_PATH)/product/VCodecV700/product_make.cfg
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV720), y)
EXTRA_CFLAGS += -DPLATFORM_VCODECV720
include $(VDEC_DRV_PATH)/product/VCodecV720/product_make.cfg
endif

MOD_NAME := vcodec_dec_ctrl
obj-$(CONFIG_VCODEC_VDEC) += $(MOD_NAME).o

#build in
$(MOD_NAME)-objs += vcodec_vdec_regulator.o
$(MOD_NAME)-objs += vcodec_vdec_memory.o
$(MOD_NAME)-objs += vcodec_vdec_plat.o
$(MOD_NAME)-objs += vcodec_vdec_utils.o
$(MOD_NAME)-objs += vcodec_vdec_dbg.o

ifeq ($(CONFIG_VCODEC_DEBUG),y)
EXTRA_CFLAGS += -DENABLE_VDEC_PROC
endif

$(MOD_NAME)-objs += vcodec_vdec.o
$(MOD_NAME)-objs += smmu/smmu.o
$(MOD_NAME)-objs += product/$(PRODUCT_VER)/vcodec_vdec_plat_var.o

# when compile module, do not compile dpm and pcie
ifeq ($(CONFIG_VCODEC_VDEC),y)
ifeq ($(CONFIG_VCODEC_DPM_ENABLE),y)
EXTRA_CFLAGS += -DVDEC_DPM_ENABLE
obj-$(CONFIG_VCODEC_VDEC) += vcodec_dec_dpm.o
vcodec_dec_dpm-objs := vcodec_vdec_dpm.o
endif

ifeq ($(CONFIG_PCIE_KPORT_EP_FPGA_VERIFY),y)
EXTRA_CFLAGS += -DPCIE_FPGA_VERIFY
obj-$(CONFIG_VCODEC_VDEC) += vcodec_dec_pcie.o
vcodec_dec_pcie-objs := vcodec_vdec_pcie.o
endif
endif
