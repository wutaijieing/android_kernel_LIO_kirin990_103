obj-$(CONFIG_PERI_DVFS)		+= peri-dvfs-volt.o
ifeq ($(CONFIG_HW_PERI_DVS),y)
obj-$(CONFIG_PERI_DVFS)		+= hw-peri-dvs.o
else
obj-$(CONFIG_PERI_DVFS)		+= plat-peri-dvfs-volt.o
endif
