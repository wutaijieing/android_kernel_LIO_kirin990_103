ccflags-y  += -I$(srctree)/platform_source/smart/drivers/
ccflags-y  += -I$(srctree)/platform_source/smart/drivers/channels/
ccflags-y  += -I$(srctree)/platform_source/smart/drivers/channels/inputhub_as

ccflags-y  += -I$(srctree)/include/linux/platform_drivers/contexthub/
ccflags-y  += -I$(srctree)/include/linux/platform_drivers/contexthub/inputhub_as/

obj-$(CONFIG_INPUTHUB_AS) += iomcu_route.o
obj-$(CONFIG_INPUTHUB_AS) += device_manager.o
obj-$(CONFIG_INPUTHUB_AS) += plat_func.o
obj-$(CONFIG_INPUTHUB_AS_DMD) += iomcu_dmd.o
obj-$(CONFIG_INPUTHUB_AS) += iomcu_logbuff.o
obj-$(CONFIG_INPUTHUB_DEBUG) += iomcu_debug.o
obj-$(CONFIG_INPUTHUB_DEBUG) += iomcu_sysfs.o
obj-$(CONFIG_INPUTHUB_AS) += dft_channel.o
obj-$(CONFIG_INPUTHUB_AS) += bus_interface.o
obj-$(CONFIG_INPUTHUB_AS) += device_interface.o
obj-$(CONFIG_INPUTHUB_AS) += device/
obj-$(CONFIG_SENSOR_CHANNEL) += sensor_channel/
obj-$(CONFIG_FINGERPRINT_CHANNEL) += fingerprint_channel/
