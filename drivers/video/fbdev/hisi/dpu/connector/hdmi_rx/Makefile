obj-y += libhdmirx.o

libhdmirx-objs := hdmirx_cmd.o \
					hdmirx_common.o \
					hdmirx_init.o \
					audio/hdmirx_audio.o \
					control/hdmirx_ctrl.o \
					control/hdmirx_ctrl_irq.o \
					hdcp/hdmirx_hdcp.o \
					hdcp/hdmirx_hdcp_tee.o \
					link/hdmirx_link.o \
					link/hdmirx_link_cec.o \
					link/hdmirx_link_ddc.o \
					link/hdmirx_link_frl.o \
					link/hdmirx_link_hpd.o \
					link/hdmirx_link_tmds.o \
					link/hdmirx_link_edid.o \
					packet/hdmirx_packet.o \
					phy/hdmirx_phy.o \
					video/hdmirx_video.o \
					video/hdmirx_video_dsc.o \
					video/hdmirx_video_hdr.o

#ifeq ($(CONFIG_HISI_DISP_TEST_ENABLE),y)
libhdmirx-objs += \
	test/hdmirx_test.o
#endif

EXTRA_CFLAGS += -I$(srctree)/drivers/video/fbdev/hisi/dpu \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/device \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/device/fbdev \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/device/chrdev \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/composer \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/composer/core \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/composer/utils \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/composer/init \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/composer/operators \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/composer/operators/utils \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/panel \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/smmu \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/power_manager \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/power_manager/dacc \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/power_manager/low_power \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/resource_manager \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/common \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/common/utils \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/common/cmdlist \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/common/config \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/connector/mipi \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/connector/dp \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/connector/hdmi_rx \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/connector/utils \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/debug_test/test_drv \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/debug_test/debug \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/lights \
	-I$(srctree)/drivers/video/fbdev/hisi/dpu/gralloc \
	-I$(srctree)/drivers/vcodec/venc/include/ \
	-I$(srctree)/include

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
