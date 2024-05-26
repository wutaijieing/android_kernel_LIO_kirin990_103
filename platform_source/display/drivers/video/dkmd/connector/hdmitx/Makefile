obj-$(CONFIG_DKMD_DPU_CONNECTOR) += hdmitx_ctrl/

obj-$(CONFIG_DKMD_DPU_CONNECTOR) += dpu_hdmi.o

GCOV_PROFILE := $(CONFIG_GCOV_KERNEL)
dpu_hdmi-objs := \
	hdmitx_aon.o \
	hdmitx_drv.o \

EXTRA_CFLAGS += \
	-I$(srctree)/include/ \
	-I$(srctree)/arch/arm64/include/ \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/dksm \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/dksm/peri \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/dksm/isr \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/connector \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/connector/mipi \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/connector/dsc \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/connector/spr \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/connector/offline \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/connector/dp_ctrl \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/connector/hdmitx/hdmitx_ctrl \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/connector/hdmitx/hdmitx_ctrl/controller/include \
	-I$(srctree)/platform_source/display/drivers/video/dkmd/connector/hdmitx/hdmitx_ctrl/controller

ifeq ($(CONFIG_DKMD_DPU_VERSION),$(filter 740 741,$(CONFIG_DKMD_DPU_VERSION)))
EXTRA_CFLAGS += \
	-I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_PRODUCT)/dpu_v$(CONFIG_DKMD_DPU_VERSION)
endif

# If you want to compile into modular drive, will need to decide
# according to the following configuration to compile
ifeq ($(CONFIG_DKMD_DPU_CONNECTOR),m)
EXTRA_CFLAGS += \
	-I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_PRODUCT)_es

KERNEL_DIR = $(ANDROID_BUILD_TOP)/kernel/linux-5.10
KERNEL_OUT = $(ANDROID_PRODUCT_OUT)/obj/KERNEL_OBJ
PWD := $(shell pwd)
ARCH := arm64
CROSS_COMPILE := aarch64-linux-android-
CFLAGS_MODULE := -fno-pic

default:
	$(MAKE) -C $(KERNEL_OUT) ARCH=$(ARCH) CFLAGS=$(EXTRA_CFLAGS) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules
	$(MAKE) -C $(KERNEL_OUT) M=$(PWD) INSTALL_MOD_PATH=$(PWD)/signed INSTALL_MOD_STRIP=1 modules_install
endif

clean:
	rm -rf *.o .*.cmd *.ko *.mod.c .tmp_versions *.symvers *.order *.symversions signed .*.d
	find . -name "*.o" | xargs rm -f
	find . -name ".*.cmd" | xargs rm -f
	find . -name ".*.d" | xargs rm -f
	find . -name "*.gcno" | xargs rm -f
