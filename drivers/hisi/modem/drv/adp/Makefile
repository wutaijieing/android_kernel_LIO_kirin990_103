#
# created by y000184236 for k3v3 fpga
#
#gcov
ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif

-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/$(OBB_MODEM_CUST_CONFIG_DIR)/config/balong_product_config.mk
KBUILD_CPPFLAGS += -I$(srctree)/drivers/hisi/modem/drv/icc
KBUILD_CPPFLAGS += -I$(srctree)/drivers/hisi/modem/drv/rtc
KBUILD_CPPFLAGS += -I$(srctree)/drivers/hisi/modem/drv/nvim
KBUILD_CPPFLAGS += -I$(srctree)/drivers/hisi/modem/drv/mem
KBUILD_CPPFLAGS += -I$(srctree)/drivers/hisi/modem/drv/om \
                   -I$(srctree)/drivers/hisi/modem/drv/om/common \
                   -I$(srctree)/drivers/hisi/modem/drv/om/dump \
                   -I$(srctree)/drivers/hisi/modem/drv/om/log
KBUILD_CPPFLAGS += -I$(srctree)/drivers/hisi/modem/drv/udi
KBUILD_CPPFLAGS += -I$(srctree)/drivers/hisi/modem/drv/balong_timer
KBUILD_CPPFLAGS += -I$(srctree)/drivers/hisi/modem/drv/hds
KBUILD_CPPFLAGS += -I$(srctree)/drivers/usb/gadget
KBUILD_CPPFLAGS += -I$(srctree)/drivers/hisi/modem/drv/diag/scm \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/cpm \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/debug \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/ppm \
                   -I$(srctree)/drivers/hisi/modem/drv/socp/soft_decode

ifneq ($(strip $(CFG_ATE_VECTOR)),YES)
obj-y           += adp_ipc.o
obj-y           += adp_icc.o
obj-y           += adp_pm_om.o
obj-y           += adp_version.o
obj-y           += adp_socp.o
obj-y			+= adp_om.o
obj-$(CONFIG_USB_GADGET)     += adp_usb.o
obj-y           += adp_nvim.o
obj-y           += adp_reset.o
obj-y           += adp_timer.o
obj-y           += adp_wifi.o
obj-y           += adp_mem_balong.o
obj-y           += adp_cpufreq_balong.o
obj-y           += adp_applog.o

obj-y           += adp_hds.o

ifeq ($(strip $(CFG_CONFIG_BALONG_CORESIGHT)),YES)
obj-y			+= adp_coresight.o
endif
obj-y           += adp_charger.o
obj-y           += adp_mmc.o
obj-y           += adp_dload.o
obj-y           += adp_misc.o
obj-y           += adp_vic.o
else
obj-y           += adp_pm_om.o
obj-y           += adp_reset.o
obj-y           += adp_timer.o
obj-y           += adp_vic.o
endif
ifeq ($(strip $(CFG_FEATURE_HISOCKET)),FEATURE_ON)
obj-y           += hisocket.o
endif
obj-y           += adp_slt.o


obj-y           += adp_print.o
