# IPP v320 release Makefile

obj-$(CONFIG_RELEASE_IPP_V320) += ipp_core.o ipp_com.o smc.o
subdir-ccflags-$(CONFIG_RELEASE_IPP_V320) += -Wall -Werror
subdir-ccflags-$(CONFIG_RELEASE_IPP_V320) += -I$(srctree)/platform_source/isp/drivers/ipp/v320/release \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v320/release/include \
                                    -I$(srctree)/drivers/platform_drivers/tzdriver
ifneq ($(TARGET_BUILD_VARIANT),user)
subdir-ccflags-$(CONFIG_RELEASE_IPP_V320) += -DCONFIG_IPP_DEBUG
endif
