ifeq ($(CONFIG_HISP_DEBUG), y)
subdir-ccflags-y += -DDEBUG_HISP
# pcie_fpga_verify may use CONFIG_PCIE_KPORT_EP_FPGA_VERIFY macro
obj-$(CONFIG_ISP) += hisp_pcie.o
endif

ifeq ($(es_low_freq),true)
subdir-ccflags-y += -DCONFIG_ES_ISP_LOW_FREQ
endif

ifneq ($(chip_type),es)
subdir-ccflags-y += -DCONFIG_CS_ISP
endif

obj-$(CONFIG_HISP_RDR) += hisp_rdr.o hisp_log.o

obj-$(CONFIG_ISP) += hisp_smc.o hisp_rpm.o hisp_cma.o hisp_ca.o \
                     hisp_nsec.o hisp_sec.o hisp_dump.o \
                     hisp_device.o hisp_pwr.o hisp_sensor.o \
                     hisp_mempool.o hisp_remoteproc.o hisp_notify.o

ifeq ($(CONFIG_ISP), y)
subdir-ccflags-y += -Wdate-time -Wextra -Wfloat-equal -Wformat=2 -Wshadow
subdir-ccflags-y += -I$(srctree)/drivers/platform_drivers/tzdriver \
                    -I$(srctree)/platform_source/see/drivers/load_image \
                    -I$(srctree)/platform_source/basicplatform/drivers/mntn/blackbox/ \
                    -I$(srctree)/platform_source/camera/drivers/media/native/camera \
                    -I$(srctree)/platform_source/camera/drivers/media/native/camera/sensor \
                    -I$(srctree)/platform_source/camera/drivers/media/native/camera/platform

endif

obj-$(CONFIG_HISP_DPM) += hisp_dpm.o
