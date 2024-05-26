# IPP v320 Makefile

obj-$(CONFIG_HISPIPP_V320) += ipp_core.o ipp_com.o memory.o smc.o
obj-$(CONFIG_HISPIPP_V320) += v320/ device/
obj-$(CONFIG_HISPIPP_V320) += adapter_common.o adapter_orb.o adapter_gf.o \
							  ipp_adapter.o
subdir-ccflags-$(CONFIG_HISPIPP_V320) += -Wall -Werror
subdir-ccflags-$(CONFIG_HISPIPP_V320) += -I$(srctree)/platform_source/isp/drivers/ipp/v320/base\
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v320/base/v320 \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v320/base/device \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v320/base/include \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v320/base/v320/include \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v320/base/device/include \
                                    -I$(srctree)/drivers/platform_drivers/tzdriver
ifneq ($(TARGET_BUILD_VARIANT),user)
subdir-ccflags-$(CONFIG_HISPIPP_V320) += -DCONFIG_IPP_DEBUG
endif
