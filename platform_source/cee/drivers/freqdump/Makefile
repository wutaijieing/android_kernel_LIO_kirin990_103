ccflags-y += -Wextra -Wdate-time -Wfloat-equal -Wshadow -Wformat=2
ccflags-y  += -I$(srctree)/platform_source/smart/drivers/channels/loadmonitor \
	-I$(TOP_DIR)/vendor/hisi/ap/kernel/platform_source/smart/drivers/channels/loadmonitor \
	-I$(srctree)/platform_source/cee/drivers/freqdump \
	-I$(srctree)/include/linux/platform_drivers \
	-I$(srctree)/platform_source/cee/drivers/freqdump/node_dump \
	-I$(srctree)/drivers/platform_drivers/ap/platform/common

ifeq ($(chip_type),cs2) # for 990 cs2
ccflags-y += -I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_BOARD_PLATFORM)_cs2
else ifeq ($(chip_type),es)
ccflags-y += -I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_BOARD_PLATFORM)_es
else
ccflags-y += -I$(srctree)/drivers/platform_drivers/ap/platform/$(TARGET_BOARD_PLATFORM)
endif

obj-$(CONFIG_FREQDUMP)	+= freqdump_kernel.o
obj-$(CONFIG_DPM_PLATFORM_JUPITER) += dpm_kernel_jupiter.o dpm_common.o freqdump_kernel.o monitor_ap_m3_ipc.o
obj-$(CONFIG_DPM_PLATFORM_MARS) += dpm_kernel_mars.o dpm_common.o freqdump_kernel.o monitor_ap_m3_ipc.o
obj-$(CONFIG_DPM_PLATFORM_VENUS) += dpm_kernel_venus.o dpm_common.o freqdump_kernel.o monitor_ap_m3_ipc.o
ifneq ($(CONFIG_COMMON_LOADMONITOR_PLATFORM),y)
obj-$(CONFIG_ENABLE_MIDEA_MONITOR) += media_monitor.o
endif
obj-$(CONFIG_DPM_PLATFORM_URANUS) += dpm_kernel_uranus.o dpm_common.o monitor_ap_m3_ipc.o uranus/freqdump_kernel.o
obj-$(CONFIG_FREQDUMP_URANUS)	+= uranus/freqdump_kernel.o

ifeq ($(chip_type),cs2) # for 990 cs2
ccflags-$(CONFIG_DPM_PLATFORM_MERCURY)  += -DCONFIG_DPM_PLATFORM_MERCURY_CS2
obj-$(CONFIG_DPM_PLATFORM_MERCURY) += dpm_kernel_mercury_cs2.o dpm_common.o monitor_ap_m3_ipc.o mercury_cs2/freqdump_kernel.o dubai_peri_mercury_common.o
obj-$(CONFIG_FREQDUMP_MERCURY)	+= mercury_cs2/freqdump_kernel.o
else
obj-$(CONFIG_DPM_PLATFORM_MERCURY) += dpm_kernel_mercury.o dpm_common.o monitor_ap_m3_ipc.o mercury/freqdump_kernel.o dubai_peri_mercury_common.o
obj-$(CONFIG_FREQDUMP_MERCURY)	+= mercury/freqdump_kernel.o
endif

ifeq ($(CONFIG_DPM_PLATFORM_PLUTO),y)
ccflags-y += -I$(srctree)/platform_source/cee/drivers/freqdump/pluto
obj-y += pluto/dpm_kernel.o dpm_common.o monitor_ap_m3_ipc.o pluto/freqdump_kernel.o
endif
obj-$(CONFIG_FREQDUMP_PLUTO) += pluto/freqdump_kernel.o

ifeq ($(CONFIG_DPM_PLATFORM_LUNA),y)
ccflags-y += -I$(srctree)/platform_source/cee/drivers/freqdump/luna
obj-y += luna/dpm_kernel.o dpm_common.o monitor_ap_m3_ipc.o luna/freqdump_kernel.o
endif
obj-$(CONFIG_FREQDUMP_LUNA) += luna/freqdump_kernel.o

# just for neptune
ifeq ($(CONFIG_NEPTUNE_PLATFORM),y)
ccflags-y += -I$(srctree)/platform_source/cee/drivers/freqdump/neptune

obj-$(CONFIG_COMMON_LOADMONITOR_PLATFORM) += \
	neptune/loadmonitor_k.o \
	neptune/media_monitor_k.o \
	neptune/loadmonitor_common.o monitor_ap_m3_ipc.o \
	neptune/dubai_peri.o
ifeq ($(CONFIG_COMMON_LOADMONITOR_PLATFORM),y)
obj-$(CONFIG_ITS) += neptune/dubai_its.o
obj-$(CONFIG_DPM_HWMON) += neptune/dubai_dpm.o
endif

ifdef CONFIG_DEBUG_FS
obj-$(CONFIG_COMMON_FREQDUMP_PLATFORM) += neptune/freqdump_k.o
endif
endif

# platform after neptune
ifneq ($(CONFIG_NEPTUNE_PLATFORM),y)
ccflags-$(CONFIG_COMMON_LOADMONITOR_PLATFORM) += \
	-I$(srctree)/platform_source/cee/drivers/freqdump/freqdump \
	-I$(srctree)/platform_source/cee/drivers/freqdump/loadmonitor \
	-I$(srctree)/platform_source/cee/drivers/freqdump/its \
	-I$(srctree)/platform_source/cee/drivers/freqdump/dpm
obj-$(CONFIG_COMMON_LOADMONITOR_PLATFORM) += \
	loadmonitor/loadmonitor_kernel.o loadmonitor/dubai_peri_kernel.o \
	loadmonitor/loadmonitor_common_kernel.o monitor_ap_m3_ipc.o
ifeq ($(CONFIG_COMMON_LOADMONITOR_PLATFORM),y)
obj-$(CONFIG_ENABLE_MIDEA_MONITOR) += loadmonitor/loadmonitor_media_kernel.o
obj-$(CONFIG_ITS) += its/dubai_its_kernel.o
obj-$(CONFIG_DPM_HWMON) += dpm/dubai_dpm_kernel.o
endif

ifdef CONFIG_DEBUG_FS
obj-$(CONFIG_COMMON_FREQDUMP_PLATFORM) += freqdump/freqdump_kernel.o
obj-$(CONFIG_COMMON_FREQDUMP_PLATFORM) += node_dump/freqdump_node_dump.o
endif
endif
