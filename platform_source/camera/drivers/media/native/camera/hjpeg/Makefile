
ccflags-y += -Wextra -Wdate-time -Wfloat-equal -Wshadow
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/jpeg

obj-y = hjpgenc.o

# CFLAGS_hwisp_stream.o = -I$(src)

ifeq ($(CONFIG_KERNEL_CAMERA_ISPV150), y)
obj-y += hjpeg150/
else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV160), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV120), y)
subdir-ccflags-y += -DHISP120_CAMERA
obj-y += hjpegenc/
else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV130), y)
subdir-ccflags-y += -DHISP120_CAMERA
subdir-ccflags-y += -DHISP130_USE_V3
obj-y += hjpegenc/
else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV200), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV250), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV210), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV310), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV300), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV350), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_CAMERA_ISPV220), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_CAMERA_ISPV230), y)
obj-y += hjpegenc/
endif
