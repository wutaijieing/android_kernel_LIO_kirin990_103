ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera/hisp
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera

ifeq ($(CONFIG_KERNEL_CAMERA_ISPV120), y)
EXTRA_CFLAGS += -DHISP120_CAMERA
obj-y += hisp120.o
else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV130), y)
EXTRA_CFLAGS += -DHISP120_CAMERA
EXTRA_CFLAGS += -DHISP130_USE_V3
obj-y += hisp120.o
endif
