ifeq ($(CONFIG_HI11XX_DRIVER_PATH),)
HI110X_DRIVER_BUILTIN_PATH ?= drivers/connectivity/hi11xx
else
HI110X_DRIVER_BUILTIN_PATH ?= $(subst ",,$(CONFIG_HI11XX_DRIVER_PATH))
endif

DRIVER_FLODER = platform
export _PRE_PRODUCT_VERSION ?= 1105
export HI110X_BOARD_VERSION ?= default
export PLAT_DEFCONFIG_FILE  ?= plat_$(_PRE_PRODUCT_VERSION)_$(HI110X_BOARD_VERSION)_defconfig

HI110X_COMM_DEFCONFIG := hi$(_PRE_PRODUCT_VERSION)_comm_defconfig

CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT ?= no

BUILDIN_FLAG ?= $(KERNELRELEASE)

ifneq ($(BUILDIN_FLAG),)
ifeq ($(HI1102_MAKE_FLAG),)
KERNEL_DIR := $(srctree)
HI110X_DRIVER_SRC_PATH=$(KERNEL_DIR)/$(HI110X_DRIVER_BUILTIN_PATH)/hi$(_PRE_PRODUCT_VERSION)
endif

DRIVER_PATH=$(HI110X_DRIVER_SRC_PATH)/$(DRIVER_FLODER)

include $(DRIVER_PATH)/$(HI110X_COMM_DEFCONFIG)
include $(DRIVER_PATH)/$(PLAT_DEFCONFIG_FILE)

$(warning defconfig: $(DRIVER_PATH)/$(PLAT_DEFCONFIG_FILE))

oal-objs := oal_bus_if.o oal_main.o oal_mem.o oal_net.o oal_hardware.o oal_schedule.o oal_aes.o oal_kernel_file.o
oal-objs += oal_cfg80211.o oal_sdio_host.o
oal-objs += oal_hcc_host.o oal_hcc_bus.o oal_hcc_bus_cfg.o oal_hcc_test.o oal_hcc_test_gt.o
oal-objs += oal_softwdt.o oal_dft.o oal_workqueue.o oal_fsm.o ftrace.o

usb-objs := oal_usb_host.o oal_usb_linux.o oal_usb_driver.o oal_usb_netbuf.o oal_usb_msg.o oal_usb_patch.o
usb-objs := $(addprefix usb/,$(usb-objs))
ifeq ($(PRODUCT_HOST_CHIP), HI5651L)
oal-objs +=  $(usb-objs)
endif
pcie-objs := pcie_dbg.o pcie_firmware.o pcie_host.o pcie_linux.o pcie_map.o
pcie-objs += pcie_msg.o pcie_netbuf.o pcie_res.o pcie_dump.o pcie_drv.o pcie_iatu.o
pcie-objs := $(addprefix pcie_116x/,$(pcie-objs))

pcie-ete-objs := ete_linux.o ete_host.o ete_comm.o ete_debug.o
pcie-ete-objs := $(addprefix pcie_116x/ete/,$(pcie-ete-objs))

pcie-chip-comm-objs := pcie_chip.o pcie_chiptest.o pcie_pm.o
pcie-chip-comm-objs := $(addprefix pcie_116x/chip/comm/,$(pcie-chip-comm-objs))

pcie-chip-hi1161-objs := pcie_chip_hi1161.o pcie_chiptest_hi1161.o pcie_iatu_hi1161.o pcie_pm_hi1161.o
pcie-chip-hi1161-objs := $(addprefix pcie_116x/chip/hi1161/,$(pcie-chip-hi1161-objs))

pcie-objs += $(pcie-ete-objs)
pcie-objs += $(pcie-chip-comm-objs)
pcie-objs += $(pcie-chip-hi1161-objs)
oal-objs +=  $(pcie-objs)

oal-objs := $(addprefix oal/,$(oal-objs))

frw-objs := frw_event_deploy.o frw_event_main.o frw_event_sched.o frw_ipc_msgqueue.o frw_main.o frw_task.o frw_timer.o
frw-objs := $(addprefix frw/,$(frw-objs))

oam-objs := oam_main.o oam_alarm.o oam_event.o oam_log.o oam_statistics.o oam_config.o oam_rdr.o
oam-objs := $(addprefix oam/,$(oam-objs))

sdt-objs := sdt_drv.o
sdt-objs := $(addprefix sdt/,$(sdt-objs))

customize-objs := hisi_ini.o plat_parse_changid.o hisi_conn_nve.o
customize-objs := $(addprefix ../common/customize/,$(customize-objs))

external-objs := lpcpu_feature.o
external-objs := $(addprefix oal/external/,$(external-objs))

chr_devs-objs := chr_devs.o
chr_devs-objs := $(addprefix ../common/chr_log/,$(chr_devs-objs))

main-objs := plat_main.o
main-objs := $(addprefix main/,$(main-objs))

pm-objs :=  plat_firmware.o plat_firmware_util.o plat_pm.o plat_pm_gt.o plat_pm_wlan.o plat_cali.o platform_common_clk.o


gt-objs := gt_dev.o
gt-objs := $(addprefix ../gt/,$(gt-objs))
ifeq ($(BFGX_UART_DOWNLOAD_SUPPORT),yes)
KBUILD_CFLAGS += -DBFGX_UART_DOWNLOAD_SUPPORT
$(warning *******BFGX_UART_DOWNLOAD_SUPPORT work*********)
pm-objs += wireless_patch.o
endif

pm-objs := $(addprefix pm/,$(pm-objs))

factory-objs :=factory.o
factory-objs :=$(addprefix factory/,$(factory-objs))

driver-objs :=  plat_sdio.o plat_uart.o
driver-objs := $(addprefix driver/,$(driver-objs))

bfgx-objs := bfgx_exception_rst.o bfgx_data_parse.o bfgx_platform_msg_handle.o bfgx_dev.o bfgx_gnss.o bfgx_gle.o bfgx_user_ctrl.o bfgx_cust.o
bfgx-objs := $(addprefix ../bfgx/,$(bfgx-objs))
board-objs := board_dts.o board_tv.o board_hi1161.o ssi_common.o ssi_hi1103.o ssi_shenkuo.o ssi_bisheng.o ssi_spmi.o ssi_dbg_cmd.o ssi_hi1161.o board.o board_bisheng.o
board-objs += board_hi1103.o
board-objs := $(addprefix board/,$(board-objs))

oneimage-objs := oneimage.o
oneimage-objs := $(addprefix ../common/oneimage/,$(oneimage-objs))

plat_$(_PRE_PRODUCT_VERSION)-objs += $(oal-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(frw-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(oam-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(sdt-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(pm-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(main-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(bfgx-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(gt-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(factory-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(chr_devs-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(external-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(board-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(oneimage-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(customize-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(driver-objs)

#plat ko
ifeq ($(CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT), yes)
obj-m += plat_$(_PRE_PRODUCT_VERSION).o
else
obj-y += plat_$(_PRE_PRODUCT_VERSION).o
endif

HI110X_INCLUDES := -I$(DRIVER_PATH)/inc
HI110X_INCLUDES += -I$(DRIVER_PATH)/inc/frw
HI110X_INCLUDES += -I$(DRIVER_PATH)/inc/oal
HI110X_INCLUDES += -I$(DRIVER_PATH)/inc/oal/linux
HI110X_INCLUDES += -I$(DRIVER_PATH)/inc/oam
HI110X_INCLUDES += -I$(DRIVER_PATH)/inc/pm
HI110X_INCLUDES += -I$(DRIVER_PATH)/frw
HI110X_INCLUDES += -I$(DRIVER_PATH)/oal
HI110X_INCLUDES += -I$(DRIVER_PATH)/oal/pcie_116x
HI110X_INCLUDES += -I$(DRIVER_PATH)/oal/external
HI110X_INCLUDES += -I$(DRIVER_PATH)/oal/usb
HI110X_INCLUDES += -I$(DRIVER_PATH)/oam
HI110X_INCLUDES += -I$(DRIVER_PATH)/pm
HI110X_INCLUDES += -I$(DRIVER_PATH)/../bfgx
HI110X_INCLUDES += -I$(DRIVER_PATH)/../gt
HI110X_INCLUDES += -I$(DRIVER_PATH)/sdt
HI110X_INCLUDES += -I$(DRIVER_PATH)/board
HI110X_INCLUDES += -I$(DRIVER_PATH)/driver
HI110X_INCLUDES += -I$(DRIVER_PATH)/../common/oneimage
HI110X_INCLUDES += -I$(DRIVER_PATH)/../common/customize
HI110X_INCLUDES += -I$(DRIVER_PATH)/../common/inc
HI110X_INCLUDES += -I$(DRIVER_PATH)/../common/chr_log
HI110X_INCLUDES += -I$(DRIVER_PATH)/factory
#This is not good
ifneq ($(KERNEL_DIR),)
HI110X_INCLUDES += -I$(KERNEL_DIR)
endif

ifneq (, $(findstring _PRE_WIFI_SUPPORT,$(COPTS)))
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/wifi/inc/hd_common
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/wifi/inc/
endif

ifneq (, $(findstring _PRE_GT_SUPPORT,$(COPTS)))
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/../drivers_gt/inc
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/../drivers_gt/inc/hd_common
endif

KBUILD_CFLAGS += -DPLATFORM_TYPE_FOR_HI110X=0
KBUILD_CFLAGS += -DPLATFORM_HI110X_k3v2oem1=1
KBUILD_CFLAGS += -DPLATFORM_HI110X_UEDGE=2

ifneq ($(TARGET_BUILD_VARIANT),user)
KBUILD_CFLAGS += -DPLATFORM_DEBUG_ENABLE
KBUILD_CFLAGS += -DPLATFORM_SSI_FULL_LOG
endif

KBUILD_CFLAGS += -DHI1XX_OS_BUILD_VARIANT_ROOT=1
KBUILD_CFLAGS += -DHI1XX_OS_BUILD_VARIANT_USER=2
ifneq ($(TARGET_BUILD_VARIANT),user)
KBUILD_CFLAGS += -DOS_HI1XX_BUILD_VERSION=HI1XX_OS_BUILD_VARIANT_ROOT
else
KBUILD_CFLAGS += -DOS_HI1XX_BUILD_VERSION=HI1XX_OS_BUILD_VARIANT_USER
endif

ifneq ($(HAVE_HISI_FEATURE_BT),false)
KBUILD_CFLAGS += -DHAVE_HISI_BT
endif

ifneq ($(CONFIG_PANGU_WIFI_DEVICE_HI110X), y)
ifneq (Hi3751, $(findstring Hi3751,$(TARGET_DEVICE)))
ifneq (v818, $(findstring v818,$(TARGET_DEVICE)))
ifneq ($(HAVE_HISI_FEATURE_FM),false)
KBUILD_CFLAGS += -DHAVE_HISI_FM
endif

ifneq ($(HAVE_HISI_FEATURE_GNSS),false)
KBUILD_CFLAGS += -DHAVE_HISI_GNSS
endif

ifneq ($(HAVE_HISI_FEATURE_IR),false)
KBUILD_CFLAGS += -DHAVE_HISI_IR
endif

#ifneq ($(HAVE_HISI_FEATURE_NFC),false)
#KBUILD_CFLAGS += -DHAVE_HISI_NFC
#endif
endif
endif
endif

HI110X_VER_COMMIT_ID = $(shell cd $(HI110X_DRIVER_SRC_PATH);if test -d .git;then git rev-parse --verify --short HEAD 2>/dev/null;fi)
ifneq ($(HI110X_VER_COMMIT_ID),)
HI110X_VER_COMMIT_TIME = $(shell cd $(HI110X_DRIVER_SRC_PATH);git log -1 --pretty=format:%ci)

HI110X_VERSION="\"commitId:$(HI110X_VER_COMMIT_ID), commitTime:$(HI110X_VER_COMMIT_TIME)\""

#ifeq ($(HI110X_KERNEL_MODULES_BUILD_VERSION), y)
KBUILD_CFLAGS += -DHI110X_DRV_VERSION=$(HI110X_VERSION)
#endif
endif
$(warning HI110X_VER_COMMIT_ID: $(HI110X_VER_COMMIT_ID))
$(warning HI110X_VER_COMMIT_TIME: $(HI110X_VER_COMMIT_TIME))
$(warning HI110X_VERSION: $(HI110X_VERSION))
#$(warning INCLUDE: $(HI110X_INCLUDES))

EXTRA_CFLAGS = $(HI110X_INCLUDES)
EXTRA_CFLAGS += $(COMM_COPTS)
EXTRA_CFLAGS += $(COPTS) $(KBUILD_CFLAGS)
ifeq ($(CONN_NON_STRICT_CHECK),y)
$(warning hisi connectivity driver compile strict check disable)
else
EXTRA_CFLAGS += -Werror -Wuninitialized -Wempty-body -Wclobbered -Wmissing-parameter-type -Wold-style-declaration -Woverride-init -Wtype-limits
endif

#ignore some option is not support by clang
EXTRA_CFLAGS += -Wno-unknown-warning-option
ifeq ($(CONFIG_FTRACE_ENABLE),yes)
EXTRA_CFLAGS += -finstrument-functions -DFTRACE_ENABLE
endif

EXTRA_CFLAGS +=  -fno-pic
MODFLAGS = -fno-pic
else

#make modules
export HI1102_MAKE_FLAG=MODULES
export HI110X_DRIVER_SRC_PATH ?= $(shell pwd)/..


PATH := $(CROSS_DIR):$(PATH)
CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT ?= yes

default:
	$(MAKE) -C $(KERNEL_DIR) $(MODULE_PARAM)  M=$(HI110X_DRIVER_SRC_PATH)/$(DRIVER_FLODER) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(HI110X_DRIVER_SRC_PATH)/$(DRIVER_FLODER) clean
	-find ../bfgx/ -type f -name "*.o" -exec rm -f {} \;
	-find ../bfgx/ -type f -name "*.o.cmd" -exec rm -f {} \;
	-find ../bfgx/ -type f -name "*.o.d" -exec rm -f {} \;
	-find ../bfgx/ -type f -name "*.o.symversions" -exec rm -f {} \;
	-find ../common/ -type f -name "*.o" -exec rm -f {} \;
	-find ../common/ -type f -name "*.o.cmd" -exec rm -f {} \;
	-find ../common/ -type f -name "*.o.d" -exec rm -f {} \;
	-find ../common/ -type f -name "*.o.symversions" -exec rm -f {} \;

endif
