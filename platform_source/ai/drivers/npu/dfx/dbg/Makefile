EXTRA_CFLAGS += $(NPU_GLOBAL_CFLAGS)
EXTRA_CFLAGS += -I$(NPU_DIR)/services/atf
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/comm
EXTRA_CFLAGS += -I$(NPU_DIR)/inc
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/model
EXTRA_CFLAGS += -I$(NPU_DIR)/device/core
EXTRA_CFLAGS += -I$(NPU_DIR)/device/config
EXTRA_CFLAGS += -I$(NPU_DIR)/device/format_convertor
EXTRA_CFLAGS += -I$(NPU_DIR)/device/heartbeat
EXTRA_CFLAGS += -I$(NPU_DIR)/device/stream_manager
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/atf
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/memory
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/id_allocator
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/communication
EXTRA_CFLAGS += -I$(NPU_DIR)/manager
EXTRA_CFLAGS += -I$(NPU_DIR)/dfx
EXTRA_CFLAGS += -I$(NPU_DIR)/platform
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/dts
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)

obj-$(CONFIG_NPU_DBG_DEVDRV)   += npu_dbg.o

npu_dbg-objs := npu_debug_resource_count.o
