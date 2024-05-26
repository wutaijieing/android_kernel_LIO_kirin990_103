EXTRA_CFLAGS += $(NPU_GLOBAL_CFLAGS)
EXTRA_CFLAGS += -I$(NPU_DIR)/inc
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/comm
EXTRA_CFLAGS += -I$(NPU_DIR)/device
EXTRA_CFLAGS += -I$(NPU_DIR)/device/core
EXTRA_CFLAGS += -I$(NPU_DIR)/device/format_convertor
EXTRA_CFLAGS += -I$(NPU_DIR)/device/heartbeat
EXTRA_CFLAGS += -I$(NPU_DIR)/device/lowpower
EXTRA_CFLAGS += -I$(NPU_DIR)/device/stream_manager
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/model
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/memory
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/atf
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/communication
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/id_allocator
EXTRA_CFLAGS += -I$(NPU_DIR)/dfx
ifeq ($(CONFIG_NPU_SWTS),y)
	EXTRA_CFLAGS += -I$(NPU_DIR)/dfx/bbox
else
	EXTRA_CFLAGS += -I$(NPU_DIR)/dfx/log
endif
EXTRA_CFLAGS += -I$(NPU_DIR)/platform
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/dts
EXTRA_CFLAGS += -I$(NPU_DIR)/manager
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/ap/platform/common/
ifeq ($(CONFIG_NPU_SWTS),y)
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/schedule
	EXTRA_CFLAGS += -I$(NPU_DIR)/platform/npu_v200/baltimore/schedule
endif

obj-$(CONFIG_NPU_DEVDRV_DEVDRIVER) += npu_devdrv.o

npu_devdrv-objs := npu_common.o npu_devinit.o core/npu_ioctl_services.o core/npu_proc_ctx.o core/npu_recycle.o
npu_devdrv-objs += format_convertor/npu_comm_sqe_fmt.o format_convertor/npu_comm_task_verify.o
npu_devdrv-objs += stream_manager/npu_calc_channel.o stream_manager/npu_hwts.o stream_manager/npu_sink_stream.o stream_manager/npu_stream.o
npu_devdrv-objs += core/npu_easc.o

ifneq ($(CONFIG_NPU_SWTS),y)
	npu_devdrv-objs += core/npu_ts_report.o
	npu_devdrv-objs += heartbeat/npu_heart_beat.o
endif


ifeq ($(CONFIG_NPU_DPM_ENABLED), y)
	EXTRA_CFLAGS += -DCONFIG_NPU_DPM_ENABLED
	ifeq ($(CONFIG_DPM_HWMON_V1),y)
		npu_devdrv-objs += lowpower/npu_dpm_v1.o
	endif
	ifeq ($(CONFIG_DPM_HWMON_V2),y)
		npu_devdrv-objs += lowpower/npu_dpm_v2.o
	endif
	ifeq ($(CONFIG_DPM_HWMON_V3),y)
		npu_devdrv-objs += lowpower/npu_dpm_v3.o
	endif
endif

npu_devdrv-$(CONFIG_NPU_SYSCACHE) += core/npu_syscache.o
npu_devdrv-$(CONFIG_NPU_SWTS) += schedule/hwts_channel_manager.o schedule/hwts_config.o schedule/schedule_stream_manager.o \
				schedule/schedule_stream.o schedule/task_pool.o schedule/schedule_interface.o \
				schedule/event_manager.o schedule/event.o schedule/hwts_sq_cq_manager.o schedule/hwts_event_manager.o schedule/hwts_sq_cq.o \
				schedule/schedule_model_manager.o schedule/schedule_model.o schedule/schedule_stream_engine.o schedule/schedule_task_process.o \
				schedule/hwts_interrupt.o schedule/irq_bottom_half.o schedule/placeholder.o schedule/response.o schedule/schedule_model_engine.o \
				schedule/mutex_ops.o

ifeq ($(NPU_ARCH_VERSION),npu_v100)
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/power_manager/v100
	npu_devdrv-objs += power_manager/v100/npu_pm_framework.o
else
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/power_manager
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/config

	ifneq ($(CONFIG_NPU_SWTS),y)
		npu_devdrv-objs += core/npu_hw_exp_irq.o
	endif
	npu_devdrv-objs += power_manager/npu_pm_framework.o
	ifneq ($(CONFIG_NPU_PM_FUSION),y)
		npu_devdrv-objs += config/npu_pm_config_aicore.o config/npu_pm_config_npusubsys.o
		npu_devdrv-objs += config/npu_pm_config_nputop.o config/npu_pm_config_smmutbu.o config/npu_pm_config_tscpu.o
	endif
	
	npu_devdrv-$(CONFIG_NPU_PCR_ENABLED) += lowpower/npu_powercapping.o
	npu_devdrv-$(CONFIG_NPU_AUTOFS) += lowpower/npu_autofs.o

endif

