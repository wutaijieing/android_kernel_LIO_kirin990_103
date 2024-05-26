ccflags-y  += -I$(srctree)/include/linux/platform_drivers/contexthub

obj-$(CONFIG_INPUTHUB_AS) += common/common.o

obj-$(CONFIG_INPUTHUB_MOCK_SENSOR)  += mock_sensor_channel/mock_sensor_channel.o
obj-$(CONFIG_INPUTHUB_AS)   += inputhub_as/
obj-$(CONFIG_CONTEXTHUB)   += inputhub_wrapper/inputhub_wrapper.o
obj-$(CONFIG_INPUTHUB_STUB)   += inputhub_stub/inputhub_stub.o
obj-$(CONFIG_SENSORHUB_VERSION)   += version/sensorhub_version.o
obj-$(CONFIG_SENSORHUB_NODE)   += sensorhub_node/sensorhub_node.o
obj-$(CONFIG_SENSORHUB_CHANNEL_BUFFER) += sensorhub_channel_buffer/sensorhub_channel_buffer.o
