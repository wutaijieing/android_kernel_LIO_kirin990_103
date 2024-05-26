obj-$(CONFIG_DKMD_DPU_COMPOSER) += dpu_composer.o
GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)

dpu_composer-objs := \
	abnormal/dpu_comp_abnormal_handle.o \
	cmdlist/dpu_cmdlist.o \
	config/v$(CONFIG_DKMD_DPU_VERSION)/dpu_comp_config.o \
	dacc/dpu_dacc.o \
	isr/dpu_isr_alsc.o \
	isr/dpu_isr_hiace.o \
	init/dpu_comp_init.o \
	init/dpu_lut_init.o \
	offline/dpu_comp_offline.o \
	online/dpu_comp_vactive.o \
	online/dpu_comp_vsync.o \
	online/dpu_comp_sysfs.o \
	present/dpu_comp_present.o \
	scene/dpu_comp_scene.o

ifeq ($(CONFIG_DKMD_DPU_VERSION),$(filter $(CONFIG_DKMD_DPU_VERSION), 740 741))
dpu_composer-objs += \
	dpu_comp_mgr_v$(CONFIG_DKMD_DPU_VERSION).o \
	power/dpu_comp_power_v$(CONFIG_DKMD_DPU_VERSION).o \
	smmu/dpu_comp_smmu_v740.o \
	online/dpu_comp_online_v740.o \
	isr/dpu_isr_m1_qic.o
else
dpu_composer-objs += \
	dpu_comp_mgr.o \
	power/dpu_comp_power.o \
	online/dpu_comp_online.o \
	smmu/dpu_comp_smmu.o
endif

EXTRA_CFLAGS += \
	-I$(srctree)/include/ \
	-I$(srctree)/arch/arm64/include/ \
	-I$(srctree)/include/platform_include/display/dkmd/ \
	-I$(srctree)/drivers/video/fbdev/dkmd/cmdlist \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm/timeline \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm/buf_sync \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm/fence \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm/chrdev \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm/peri \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm/isr \
	-I$(srctree)/drivers/video/fbdev/dkmd/connector \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/dvfs \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/config \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/device \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/abnormal \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/cmdlist \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/config \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/dacc \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/isr \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/init \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/offline \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/online \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/power \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/present \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/scene \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/composer/smmu

ifeq ($(CONFIG_DKMD_DPU_VERSION),$(filter 740 741,$(CONFIG_DKMD_DPU_VERSION)))
EXTRA_CFLAGS += \
	-I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_PRODUCT)/dpu_v$(CONFIG_DKMD_DPU_VERSION)
endif

ifeq ($(CONFIG_DKMD_DPU_COMPOSER),m)
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
