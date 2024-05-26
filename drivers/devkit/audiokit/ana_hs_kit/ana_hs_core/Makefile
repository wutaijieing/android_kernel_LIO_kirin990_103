# analog headset driver
ifdef CONFIG_ANA_HS_CORE
	 ccflags-y += -Idrivers/devkit/audiokit/
endif

obj-$(CONFIG_ANA_HS_CORE) += ana_hs_core.o

ifeq ($(CONFIG_ANA_HS_EXTERN_OPS_KIT), y)
obj-$(CONFIG_ANA_HS_CORE) += ana_hs_i2c.o
else ifeq ($(CONFIG_ANA_HS_EXTERN_OPS), y)
obj-$(CONFIG_ANA_HS_CORE) += ana_hs_i2c.o
endif
