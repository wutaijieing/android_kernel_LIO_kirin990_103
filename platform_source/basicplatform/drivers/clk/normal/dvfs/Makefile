
obj-$(CONFIG_PERI_DVFS)		+= peri_dvfs_volt.o
ifeq ($(CONFIG_HW_PERI_DVS),y)
obj-$(CONFIG_PERI_DVFS)		+= hisi_hw_peri_dvs.o
else
obj-$(CONFIG_PERI_DVFS)		+= hisi_peri_dvfs_volt.o
endif
