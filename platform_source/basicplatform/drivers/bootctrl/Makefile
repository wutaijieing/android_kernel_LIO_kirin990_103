#
# Makefile for bootctrl
#
obj-$(CONFIG_AB_PARTITION_TABLE) += bootctrl.o bootctrl_utility.o
ifeq ($(partition_type), recovery_ab)
obj-y += bootctrl.o bootctrl_utility.o
endif
