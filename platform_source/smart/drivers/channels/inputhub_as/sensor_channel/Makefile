
ccflags-y  += -I$(srctree)/platform_source/smart/drivers/
ccflags-y  += -I$(srctree)/platform_source/smart/drivers/channels/

ccflags-y  += -I$(srctree)/include/linux/platform_drivers/contexthub/
ccflags-y  += -I$(srctree)/include/linux/platform_drivers/contexthub/inputhub_as/



obj-$(CONFIG_SENSOR_CHANNEL) += sensor_channel.o
obj-$(CONFIG_SENSOR_CHANNEL) += sensor_feima.o
obj-$(CONFIG_SENSOR_CHANNEL) += sensor_sysfs.o
obj-$(CONFIG_SENSOR_CHANNEL) += acc_feima_demo.o
obj-$(CONFIG_CTXHUB_AS_GYRO) += gyro_feima.o
obj-$(CONFIG_CTXHUB_AS_AR) += ar_feima.o
obj-$(CONFIG_CTXHUB_AS_AR) += ar.o
obj-$(CONFIG_CTXHUB_AS_PEDOMETER) += pedometer.o
obj-$(CONFIG_CTXHUB_AS_PEDOMETER) += pedo_feima.o
