# Add your debugging flag (or not) to CFLAGS

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV200), y)
PLATFORM := VCodecV200
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV210), y)
EXTRA_CFLAGS += -DVCODEC_V210
EXTRA_CFLAGS += -DVCODEC_PLATFORM_ECONOMIC
PLATFORM := VCodecV210
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV300), y)
PLATFORM := VCodecV300
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV310), y)
EXTRA_CFLAGS += -DVCODEC_V310
EXTRA_CFLAGS += -DVCODEC_PLATFORM_ECONOMIC
PLATFORM := VCodecV310
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV320), y)
PLATFORM := VCodecV320
SMMU_VER := smmu_v3
EXTRA_CFLAGS += -DVCODECV320
EXTRA_CFLAGS += -DVCODEC_PLATFORM_ECONOMIC
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV500), y)
PLATFORM := VCodecV500
SMMU_VER := smmu_v2
EXTRA_CFLAGS += -DVCODECV500
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV520), y)
PLATFORM := VCodecV500
SMMU_VER := smmu_v3
EXTRA_CFLAGS += -DVCODECV520
EXTRA_CFLAGS += -DVCODECV500
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV600), y)
ifeq ($(pcie_udp_verify),true)
PLATFORM := VCodecV700
SMMU_VER := smmu_v3
EXTRA_CFLAGS += -DVCODECV700
EXTRA_CFLAGS += -DVCODECV500
else
PLATFORM := VCodecV600
SMMU_VER := smmu_v3
EXTRA_CFLAGS += -DVCODECV600
EXTRA_CFLAGS += -DVCODECV500
endif
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV700), y)
PLATFORM := VCodecV700
SMMU_VER := smmu_v3
EXTRA_CFLAGS += -DVCODECV700
EXTRA_CFLAGS += -DVCODECV500
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_VCODECV720), y)
PLATFORM := VCodecV720
SMMU_VER := smmu_v3
EXTRA_CFLAGS += -DVCODECV700
EXTRA_CFLAGS += -DVCODECV500
endif

include $(srctree)/platform_source/vcodec/venc/drv_venc_make.cfg

EXTRA_CFLAGS += $(VENC_CFLAGS) -fno-pic
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/venc/include
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/venc
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/venc/product/$(PLATFORM)
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/venc/product/common

ifeq ($(chip_type), )
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_BOARD_PLATFORM)
EXTRA_CFLAGS += -DCONFIG_CS_VENC
else
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_BOARD_PLATFORM)_$(chip_type)
endif

ifeq ($(es_low_freq),true)
EXTRA_CFLAGS += -DCONFIG_ES_VENC_LOW_FREQ
endif

ifeq ($(CONFIG_VCODEC_DEBUG), y)
EXTRA_CFLAGS += -DVENC_DEBUG_ENABLE
endif

MOD_NAME := vcodec_venc
obj-$(CONFIG_VCODEC_VENC) += $(MOD_NAME).o
$(MOD_NAME)-objs :=  venc_regulator.o    \
                    drv_venc_intf.o      \
                    drv_venc_efl.o       \
                    drv_venc_osal.o      \
                    drv_venc.o           \
                    drv_mem.o         \
                    power_manager.o   \
                    product/$(PLATFORM)/hal_venc.o

ifeq ($(pcie_udp_verify),true)
EXTRA_CFLAGS += -DPCIE_UDP
pcie_link_enable := true
endif

ifeq ($(CONFIG_PCIE_KPORT_EP_FPGA_VERIFY),y)
pcie_link_enable := true
endif

ifeq ($(CONFIG_VCODEC_SECURE_VENC), y)
ifeq ($(CONFIG_TZDRIVER), y)
ifneq ($(VCODEC_PRODUCT_RANGE), base)
$(MOD_NAME)-objs += tvp_adapter.o
EXTRA_CFLAGS += -DSUPPORT_SECURE_VENC
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/tzdriver
endif
endif
endif

ifneq ($(filter VCodecV500 VCodecV600 VCodecV700 VCodecV720,$(PLATFORM)),)
$(MOD_NAME)-objs += product/common/venc/hal_venc_common.o \
                    product/common/hal_common.o
endif

ifneq ($(SMMU_VER), )
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/venc/product/$(SMMU_VER)
$(MOD_NAME)-objs += product/$(SMMU_VER)/smmu.o
endif

ifeq ($(SUPPORT_CMDLIST),YES)
EXTRA_CFLAGS += -I$(srctree)/platform_source/vcodec/venc/product/common/cmdlist
$(MOD_NAME)-objs += product/common/cmdlist/hal_cmdlist.o \
                    drv_cmdlist.o
endif

ifeq ($(VENC_MCORE_ENABLE),YES)
EXTRA_CFLAGS += -Idrivers/vcodec/venc/mcore
$(MOD_NAME)-objs += mcore/drv_venc_ipc.o
$(MOD_NAME)-objs += mcore/drv_venc_mcore.o
endif

# when compile module, do not compile dpm and pcie
ifeq ($(CONFIG_VCODEC_VENC),y)
ifeq ($(CONFIG_VCODEC_DPM_ENABLE),y)
EXTRA_CFLAGS += -DVENC_DPM_ENABLE
obj-$(CONFIG_VCODEC_VENC) +=  drv_venc_dpm.o
endif

ifeq ($(pcie_link_enable),true)
EXTRA_CFLAGS += -DPCIE_LINK
obj-$(CONFIG_VCODEC_VENC) += drv_venc_pcie.o
endif
endif
