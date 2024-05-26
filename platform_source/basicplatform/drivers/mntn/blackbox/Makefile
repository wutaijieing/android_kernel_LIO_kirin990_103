ccflags-y += -Wall -Werror
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/tzdriver
EXTRA_CFLAGS += -Wno-error=date-time
EXTRA_CFLAGS += -Wno-implicit-fallthrough
ifeq ($(CONFIG_MNTN_HM_KLOG), y)
EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/mntn/hm_klog
endif
ifeq ($(CONFIG_MNTN_BBOX_LOG_MAP), y)
EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/mntn
endif


obj-$(CONFIG_DFX_BB) += rdr_core.o
obj-$(CONFIG_DFX_BB) += rdr_common.o
obj-$(CONFIG_DFX_BB) += rdr_exception_core.o
obj-$(CONFIG_DFX_BB) += rdr_module_core.o
obj-$(CONFIG_DFX_BB) += rdr_dump_core.o
obj-$(CONFIG_DFX_BB) += rdr_logmonitor_core.o
obj-$(CONFIG_DFX_BB) += rdr_field_core.o
obj-$(CONFIG_DFX_BB) += rdr_bootcheck.o
obj-$(CONFIG_DFX_BB_SHUTDOWN_REASON) += rdr_poweroff_log.o

obj-$(CONFIG_DFX_BB_DEBUG) += rdr_debug.o
obj-$(CONFIG_DFX_BB_DEBUG) += rdr_demo.o
obj-$(CONFIG_DFX_BB) += platform_ap/
obj-$(CONFIG_DFX_BB) += diaginfo/
obj-$(CONFIG_LPMCU_BB) += platform_lpm3/

obj-$(CONFIG_DFX_CORE) += rdr_dfx_core.o
obj-$(CONFIG_DFX_BB) += rdr_cleartext.o
obj-$(CONFIG_DFX_BB) += rdr_exception_trace.o
obj-$(CONFIG_DFX_BB) += rdr_recovery_exception.o
