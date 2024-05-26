ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DDEBUG_KERNEL_CAMERA
endif
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera/platform

ifeq (,$(filter $(TARGET_BOARD_PLATFORM), napa))
obj-y += camerafs.o
endif
obj-y += hwsensor.o flash/ vcm/ ois/
