ccflags-y  += -I$(srctree)/platform_source/smart/drivers/
ccflags-y  += -I$(srctree)/platform_source/smart/drivers/channels/
ccflags-y  += -I$(srctree)/platform_source/smart/drivers/channels/inputhub_as/

ccflags-y  += -I$(srctree)/include/linux/platform_drivers/contexthub/
ccflags-y  += -I$(srctree)/include/linux/platform_drivers/contexthub/inputhub_as/

obj-$(CONFIG_INPUTHUB_AS) += device_common.o
obj-$(CONFIG_CONTEXTHUB_DEVICE_DEMO) += acc_device_demo.o
obj-$(CONFIG_CONTEXTHUB_DEVICE_GYRO) += gyro_device.o
obj-$(CONFIG_CONTEXTHUB_DEVICE_DEMO) += airpress_device_demo.o
obj-$(CONFIG_CONTEXTHUB_DEVICE_CONNECTIVITY) += connectivity_device_demo.o
obj-$(CONFIG_FINGERPRINT_CHANNEL) += fingerprint_device_demo.o
