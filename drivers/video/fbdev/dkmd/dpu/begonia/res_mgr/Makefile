obj-$(CONFIG_DKMD_DPU_RES_MGR) += opr_mgr/operators/
obj-$(CONFIG_DKMD_DPU_RES_MGR) += dpu_res_mgr.o
GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)

dpu_res_mgr-objs := \
	config/dpu_config_utils.o \
	config/v$(CONFIG_DKMD_DPU_VERSION)/dpu_config.o \
	config/v$(CONFIG_DKMD_DPU_VERSION)/dpu_dvfs.o \
	config/v$(CONFIG_DKMD_DPU_VERSION)/dpu_opr_config.o \
	dvfs/dvfs.o \
	gr_dev/gr_dev.o \
	opr_mgr/opr_mgr.o \
	scene_id_mgr/scene_id_mgr.o

ifeq ($(CONFIG_DKMD_DPU_VERSION),741)
dpu_res_mgr-objs += \
	res_mgr_v$(CONFIG_DKMD_DPU_VERSION).o
else
dpu_res_mgr-objs += \
	res_mgr.o
endif

EXTRA_CFLAGS += \
	-I$(srctree)/include/platform_include/display/dkmd/ \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/config \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/dvfs \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/mem_mgr \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/opr_mgr \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/gr_dev \
	-I$(srctree)/drivers/video/fbdev/dkmd/dpu/begonia/res_mgr/scene_id_mgr \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm \
	-I$(srctree)/drivers/video/fbdev/dkmd/dksm/chrdev \
	-I$(srctree)/drivers/video/fbdev/dkmd/cmdlist \
	-I$(srctree)/arch/arm64/include/ \
	-I$(srctree)/include/

ifeq ($(CONFIG_DKMD_DPU_VERSION),$(filter 740 741,$(CONFIG_DKMD_DPU_VERSION)))
EXTRA_CFLAGS += \
	-I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_PRODUCT)/dpu_v$(CONFIG_DKMD_DPU_VERSION)
endif

ifeq ($(CONFIG_DKMD_DPU_ENABLE),m)
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
	$(MAKE) -C opr_mgr/operators clean
