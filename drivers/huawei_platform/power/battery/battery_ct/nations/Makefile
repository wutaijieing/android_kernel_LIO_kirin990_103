# SPDX-License-Identifier: GPL-2.0
ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DONEWIRE_STABILITY_DEBUG
obj-$(CONFIG_BATTCT_NS3300) += ns3300_eng.o
obj-$(CONFIG_BATTCT_NS3300) += algo/
obj-$(CONFIG_BATTCT_NS3300) += auth/
obj-$(CONFIG_BATTCT_NS3300) += platform/
obj-$(CONFIG_BATTCT_NS3300) += swi/
endif

ifeq ($(TARGET_VERSION_MODE),factory)
ccflags-y += -DBATTBD_FORCE_MATCH
endif

obj-$(CONFIG_BATTCT_NS3300) += ns3300.o
