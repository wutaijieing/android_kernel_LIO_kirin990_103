

obj-y        += clk-unit-test.o
obj-y        += clk-test.o
obj-y        += clk-debug.o

ifeq ($(CONFIG_FMEA_FAULT_INJECTION), y)
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/ap/platform/common/
endif
