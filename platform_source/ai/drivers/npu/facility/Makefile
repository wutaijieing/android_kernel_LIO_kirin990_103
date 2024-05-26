EXTRA_CFLAGS += $(NPU_GLOBAL_CFLAGS)
EXTRA_CFLAGS += -I$(srctree)/drivers/iommu/smmu
EXTRA_CFLAGS += -I$(srctree)/drivers/iommu/arm/smmu
EXTRA_CFLAGS += -I$(NPU_DIR)/services/atf
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/comm
EXTRA_CFLAGS += -I$(NPU_DIR)/inc
EXTRA_CFLAGS += -I$(NPU_DIR)/device/core
EXTRA_CFLAGS += -I$(NPU_DIR)/device/format_convertor
EXTRA_CFLAGS += -I$(NPU_DIR)/device/heartbeat
EXTRA_CFLAGS += -I$(NPU_DIR)/device/stream_manager
EXTRA_CFLAGS += -I$(NPU_DIR)/dfx
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/model
EXTRA_CFLAGS += -I$(NPU_DIR)/manager
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/memory
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/id_allocator
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/communication
EXTRA_CFLAGS += -I$(NPU_DIR)/platform
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/dts
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/ap/platform/common/

obj-$(CONFIG_NPU_DEVDRV_DEVFACILITY) += npu_facility.o

npu_facility-objs := atf/npu_atf_subsys.o
npu_facility-objs += memory/npu_iova.o memory/npu_pool.o memory/npu_shm.o memory/npu_ion.o memory/npu_svm.o
npu_facility-objs += id_allocator/npu_calc_cq.o id_allocator/npu_calc_sq.o id_allocator/npu_event.o \
					 id_allocator/npu_hwts_cq.o id_allocator/npu_hwts_sq.o id_allocator/npu_model.o \
					 id_allocator/npu_task.o id_allocator/npu_id_allocator.o id_allocator/npu_hwts_event.o
npu_facility-objs += communication/npu_intr_hub_intc.o
ifneq ($(CONFIG_NPU_SWTS),y)
	npu_facility-objs += id_allocator/npu_mailbox.o id_allocator/npu_dfx_cq.o id_allocator/npu_dfx_sq.o
	npu_facility-objs += communication/npu_doorbell.o communication/npu_mailbox_msg.o communication/npu_message.o
endif

ifeq ($(NPU_ARCH_VERSION),npu_v100)
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/power_manager/v100
	npu_facility-objs += communication/v100/npu_task_message.o
else
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/power_manager
	npu_facility-objs += communication/npu_task_message.o
endif