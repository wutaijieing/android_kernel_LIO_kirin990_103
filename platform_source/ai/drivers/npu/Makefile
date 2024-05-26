NPU_GLOBAL_CFLAGS := -Wextra -Wdate-time -Wfloat-equal -Wshadow -Wformat=2

NPU_CHIP_VERSION := $(TARGET_BOARD_PLATFORM)
NPU_DIR :=$(srctree)/platform_source/ai/drivers/npu

export NPU_CHIP_VERSION NPU_GLOBAL_CFLAGS NPU_DIR
ifneq ($(TARGET_BOARD_PLATFORM),)
include $(srctree)/platform_source/ai/drivers/npu/$(TARGET_BOARD_PLATFORM).mk
endif

ifeq ($(NPU_ARCH_VERSION),npu_v100)
	NPU_GLOBAL_CFLAGS += -DNPU_ARCH_V100
else ifeq ($(NPU_ARCH_VERSION),npu_v200)
	NPU_GLOBAL_CFLAGS += -DNPU_ARCH_V200
	NPU_GLOBAL_CFLAGS += -DCONFIG_NPU_HWTS
endif

ifneq ($(NPU_CUT_MAKEFILE), yes)
obj-$(CONFIG_NPU_DEVDRV) += platform/
obj-$(CONFIG_NPU_DEVDRV) += manager/
obj-$(CONFIG_NPU_DEVDRV) += device/
ifneq ($(CONFIG_NPU_SWTS), y)
	obj-$(CONFIG_NPU_DEVDRV) += dfx/log/
endif
obj-$(CONFIG_NPU_DEVDRV) += dfx/profiling/
obj-$(CONFIG_NPU_DEVDRV) += facility/
obj-$(CONFIG_NPU_DEBUG) += dfx/dbg/
obj-$(CONFIG_NPU_BBIT_DEBUGFS) += bbit_debugfs/
endif
