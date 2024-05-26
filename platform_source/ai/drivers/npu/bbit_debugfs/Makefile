EXTRA_CFLAGS += $(NPU_GLOBAL_CFLAGS)
EXTRA_CFLAGS += -I$(NPU_DIR)/inc
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/comm
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/model
EXTRA_CFLAGS += -I$(NPU_DIR)/device
EXTRA_CFLAGS += -I$(NPU_DIR)/device/core
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/dts
EXTRA_CFLAGS += -I$(NPU_DIR)/platform
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/communication
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/memory
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)
EXTRA_CFLAGS += -I$(NPU_DIR)/bbit_debugfs
EXTRA_CFLAGS += -I$(NPU_DIR)/bbit_debugfs/core
EXTRA_CFLAGS += -I$(NPU_DIR)/bbit_debugfs/kprobe
EXTRA_CFLAGS += -I$(NPU_DIR)/bbit_debugfs/hiail

LOCAL_PATH := $(call my-dir)

obj-$(CONFIG_NPU_BBIT_DEBUGFS) += npu_bbit_debugfs_module.o
npu_bbit_debugfs_module-objs :=npu_bbit_debugfs.o core/npu_bbit_core.o core/npu_kprobe_core.o kprobe/npu_kprobe_debugfs.o kprobe/npu_kprobe.o

ifeq ($(NPU_ARCH_VERSION),npu_v100)
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/power_manager/v100
	npu_bbit_debugfs_module-objs += hiail/npu_bbit_hwts_stub.o
else
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/power_manager
	ifneq ($(CONFIG_NPU_BBIT_DEBUGFS),y)
		npu_bbit_debugfs_module-objs += hiail/npu_bbit_hwts_stub.o
	else
		ifeq ($(CONFIG_NPU_HWTS),hwts_v210)
			npu_bbit_debugfs_module-objs += hiail/npu_bbit_hwts_log_210.o
			npu_bbit_debugfs_module-objs += hiail/npu_bbit_hwts_config_210.o
		else
			npu_bbit_debugfs_module-objs += hiail/npu_bbit_hwts_log.o
			npu_bbit_debugfs_module-objs += hiail/npu_bbit_hwts_config.o
		endif

	endif
endif
