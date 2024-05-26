# Hisilicon IPP v300 Makefile

obj-$(CONFIG_HISPIPP_V300) += ipp_core.o ipp_com.o memory.o ipp_smc.o
obj-$(CONFIG_HISPIPP_V300) += v300/ device/
obj-$(CONFIG_HISPIPP_V300) += adapter_common.o adapter_vbk.o adapter_reorder.o\
                              adapter_compare.o adapter_orb.o ipp_adapter.o\
                              adapter_matcher.o adapter_gf.o \
                              adapter_orb_enh.o adapter_mc.o

subdir-ccflags-$(CONFIG_HISPIPP_V300) += -Wall -Werror
subdir-ccflags-$(CONFIG_HISPIPP_V300) += -I$(srctree)/platform_source/isp/drivers/ipp/v300 \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v300/v300 \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v300/device \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v300/include \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v300/v300/include \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v300/device/include \
                                    -I$(srctree)/drivers/platform_drivers/tzdriver
ifneq ($(TARGET_BUILD_VARIANT),user)
subdir-ccflags-$(CONFIG_HISPIPP_V300) += -DCONFIG_IPP_DEBUG
endif
