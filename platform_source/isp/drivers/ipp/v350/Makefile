# Hisilicon IPP v350 Makefile
obj-$(CONFIG_HISPIPP_V350) += modules/ segment/ drivers/
obj-$(CONFIG_HISPIPP_V350) += memory.o

subdir-ccflags-$(CONFIG_HISPIPP_V350) += -Wall -Werror
subdir-ccflags-$(CONFIG_HISPIPP_V350) += -I$(srctree)/platform_source/isp/drivers/ipp/v350 \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v350/dataflow \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v350/segment \
				    -I$(srctree)/platform_source/isp/drivers/ipp/v350/modules \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v350/platform \
				    -I$(srctree)/platform_source/isp/drivers/ipp/v350/platform/reg_offset_field \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v350/include \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v350/include/config_table \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v350/include/dataflow \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v350/include/drivers \
				    -I$(srctree)/platform_source/isp/drivers/ipp/v350/include/modules \
                                    -I$(srctree)/platform_source/isp/drivers/ipp/v350/include/segment \
                                    -I$(srctree)/drivers/platform_drivers/tzdriver
ifneq ($(TARGET_BUILD_VARIANT),user)
subdir-ccflags-$(CONFIG_HISPIPP_V350) += -DCONFIG_IPP_DEBUG
endif
