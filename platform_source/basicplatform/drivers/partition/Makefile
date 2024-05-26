ifneq ($(CONFIG_PARTITION_ARMPC),y)
subdir-ccflags-y += -I$(srctree)/include
subdir-ccflags-y += -I$(srctree)/include/platform_include/basicplatform/linux/partition
subdir-ccflags-y += -I$(srctree)/include/platform_include/basicplatform/linux/partition/plat/$(TARGET_BOARD_PLATFORM)
obj-$(CONFIG_PARTITION_TABLE) += partition_ap_kernel.o
else
obj-$(CONFIG_PARTITION_TABLE) += partition_ap_kernel_armpc.o
endif
