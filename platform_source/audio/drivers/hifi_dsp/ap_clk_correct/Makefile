# hifi dsp device driver
EXTRA_CFLAGS += -I$(srctree)/platform_drivers/hifi_dsp
EXTRA_CFLAGS += -I$(srctree)/linux/mfd
EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/mmc/host

ifeq ($(CONFIG_HIFI_EXTRA_MODEM_MODE),y)
obj-$(CONFIG_HIFI_DSP_ONE_TRACK )+= ap_clk_correct.o
EXTRA_CFLAGS += -DEXTERNAL_MODEM
endif
