
EXTRA_CFLAGS += $(NPU_GLOBAL_CFLAGS)
EXTRA_CFLAGS += -Idrivers/platform_drivers/tzdriver

EXTRA_CFLAGS += -I$(NPU_DIR)/inc/comm
EXTRA_CFLAGS += -I$(NPU_DIR)/inc
EXTRA_CFLAGS += -I$(NPU_DIR)/device
EXTRA_CFLAGS += -I$(NPU_DIR)/device/core
EXTRA_CFLAGS += -I$(NPU_DIR)/device/format_convertor
EXTRA_CFLAGS += -I$(NPU_DIR)/device/heartbeat
EXTRA_CFLAGS += -I$(NPU_DIR)/device/stream_manager
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/model
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/memory
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/communication
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/id_allocator
ifneq ($(CONFIG_NPU_SWTS), y)
	EXTRA_CFLAGS += -I$(NPU_DIR)/dfx/log
endif
EXTRA_CFLAGS += -I$(NPU_DIR)/dfx
EXTRA_CFLAGS += -I$(NPU_DIR)/platform
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/ap/platform/common/

obj-$(CONFIG_NPU_DEVDRV_DEVMANAGER) += npu_manager_module.o

npu_manager_module-objs := npu_manager.o npu_manager_ioctl_services.o
npu_manager_module-objs += ../dfx/bbox/npu_dfx_black_box.o

ifeq ($(NPU_ARCH_VERSION),npu_v100)
    EXTRA_CFLAGS += -I$(NPU_DIR)/device/power_manager/v100
    EXTRA_CFLAGS += -I$(NPU_DIR)/dfx/bbox/v100
else
    EXTRA_CFLAGS += -I$(NPU_DIR)/device/power_manager
	EXTRA_CFLAGS += -I$(NPU_DIR)/dfx/bbox/adapter/$(NPU_CHIP_VERSION)/
endif
