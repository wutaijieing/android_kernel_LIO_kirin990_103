# Hisilicon IPP v250cs Makefile

obj-$(CONFIG_HISPIPP_V250) += ipp_core.o ipp_com.o smc.o

ifeq ($(CONFIG_HISPIPP_CDC), )
obj-$(CONFIG_HISPIPP_V250) += v250/ device/
obj-$(CONFIG_HISPIPP_V250) += adapter_common.o \
                            adapter_mcf.o \
                            adapter_slam.o \
                            adapter_vbk.o \
                            adapter_reorder.o \
                            adapter_compare.o \
                            adapter_matcher.o \
                            memory.o
endif
subdir-ccflags-$(CONFIG_HISPIPP_V250) += -Wdate-time -Wextra -Wfloat-equal -Wformat=2 -Wshadow
subdir-ccflags-$(CONFIG_HISPIPP_V250) += -I$(srctree)/platform_source/isp/drivers/ipp/v250/v250 \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v250/device \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v250/include \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v250/v250/include \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v250/device/include \
                                    -I$(srctree)/drivers/platform_drivers/tzdriver

ifneq ($(TARGET_BUILD_VARIANT),user)
subdir-ccflags-$(CONFIG_HISPIPP_V250) += -DCONFIG_IPP_DEBUG
endif

ifeq ($(CONFIG_HISPIPP_CDC), y)
subdir-ccflags-y += -DHIPP_CDC_BYPASS
endif
