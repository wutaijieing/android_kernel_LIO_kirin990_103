obj-$(CONFIG_DKMD_DPU_DEVICE) += dpu_device.o
GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)

dpu_device-objs := \
	gfxdev_mgr.o \
	fb/dpu_fb.o \
	fb/dpu_fb_pan_display.o \
	fb/dpu_ion_mem.o \
	gfx/dpu_gfx.o

dpu_device-$(CONFIG_DRM) += \
	drm/dpu_drm.o

EXTRA_CFLAGS += \
	-I$(srctree)/include/platform_include/display/dkmd/ \
	-I$(srctree)/drivers/video/fbdev/dkmd/cmdlist \
	-I$(srctree)/drivers/video/fbdev/dkmd/connector \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm/chrdev \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/device \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/config \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/dvfs\
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/opr_mgr \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/device/fb \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/device/drm \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/device/gfx \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer \
	-I$(srctree)/arch/arm64/include/ \
	-I$(srctree)/include/

ifeq ($(CONFIG_DKMD_DPU_VERSION),$(filter 740 741,$(CONFIG_DKMD_DPU_VERSION)))
EXTRA_CFLAGS += \
	-I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_PRODUCT)/dpu_v$(CONFIG_DKMD_DPU_VERSION)
endif

ifeq ($(CONFIG_DKMD_DPU_DEVICE), m)
ifeq ($(chip_type),)
EXTRA_CFLAGS += \
	-I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_PRODUCT)
else
EXTRA_CFLAGS += \
	-I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_PRODUCT)_$(chip_type)
endif
endif

clean:
	rm -rf *.o .*.cmd *.ko *.mod *.mod.c .tmp_versions *.symvers *.order *.symversions signed .*.d
	find . -name "*.o" | xargs rm -f
	find . -name ".*.cmd" | xargs rm -f
	find . -name ".*.d" | xargs rm -f
	find . -name "*.gcno" | xargs rm -f
