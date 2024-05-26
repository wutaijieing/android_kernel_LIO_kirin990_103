ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera/hisp
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera

ifeq ($(CONFIG_CAMERA_ISPV230), y)
EXTRA_CFLAGS += -DHISP230_CAMERA
obj-y += hisp220.o
else ifeq ($(CONFIG_CAMERA_ISPV220), y)
EXTRA_CFLAGS += -DHISP220_CAMERA
obj-y += hisp220.o
endif
