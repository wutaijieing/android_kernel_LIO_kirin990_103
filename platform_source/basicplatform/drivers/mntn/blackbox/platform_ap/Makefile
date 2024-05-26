ccflags-y += -Wall -Werror
EXTRA_CFLAGS += -Idrivers/platform_drivers/tzdriver
EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/mntn

obj-y += rdr_ap_adapter.o rdr_ap_ringbuffer.o rdr_ap_hook.o rdr_ap_mntn.o
obj-y += rdr_ap_subtype.o rdr_ap_cleartext.o rdr_ap_exception_logsave.o
obj-$(CONFIG_DFX_BB_DEBUG) += rdr_ap_test.o
