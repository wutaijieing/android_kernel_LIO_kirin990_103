#obj-m += libdprx.o
obj-y += libdprx.o

libdprx-objs := dprx.o \
				rx_edid.o \
				rx_sdp.o \
				rx_core.o \
				rx_irq.o

EXTRA_CFLAGS += \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/common/ \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/debug_test/ \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/debug_test/debug/ \
	-I$(srctree)/arch/arm64/include/ \
	-I$(srctree)/include/

KERNEL_DIR = $(ANDROID_BUILD_TOP)/kernel/linux-5.10
KERNEL_OUT = $(ANDROID_PRODUCT_OUT)/obj/KERNEL_OBJ
PWD := $(shell pwd)
ARCH := arm64
CROSS_COMPILE := aarch64-linux-android-
CFLAGS_MODULE := -fno-pic

default:
	$(MAKE) -C $(KERNEL_OUT) ARCH=$(ARCH) CFLAGS=$(EXTRA_CFLAGS) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules
	$(MAKE) -C $(KERNEL_OUT) M=$(PWD) INSTALL_MOD_PATH=$(PWD)/signed INSTALL_MOD_STRIP=1 modules_install

clean:
	rm -rf *.o .*.cmd *.ko *.mod.c .tmp_versions *.symvers *.order *.symversions signed .*.d
