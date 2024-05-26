ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DDEBUG_KERNEL_CAMERA
endif
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera/hisp
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera

obj-y += hisp110.o
