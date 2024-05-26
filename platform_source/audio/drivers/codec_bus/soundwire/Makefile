# soundwire driver
GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
subdir-ccflags-y += -Wall -Werror

ccflags-y += -I$(srctree)/platform_source/audio/drivers/codec_bus/
ccflags-y += -I$(srctree)/include/linux/platform_drivers/codec_bus/
ccflags-y += -I$(srctree)/platform_source/audio/drivers/codec_bus/soundwire/
ccflags-y += -I$(srctree)/drivers/platform_drivers/audio/rdr/
ccflags-y += -I$(srctree)/include/linux/platform_drivers/
ccflags-y += -I$(srctree)/sound/soc/platforms/common_image/
ccflags-y += -I$(srctree)/platform_source/audio/include/

soundwire-platform-objs += soundwire_utils.o
soundwire-platform-objs += soundwire_irq.o
soundwire-platform-objs += soundwire.o
soundwire-platform-objs += soundwire_bandwidth_adp.o

obj-$(CONFIG_PLATFORM_SOUNDWIRE) += soundwire-platform.o
