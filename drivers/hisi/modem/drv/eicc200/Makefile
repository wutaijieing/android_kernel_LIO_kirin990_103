ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
GCOV_PROFILE_eicc_test.o := n
endif

-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/$(OBB_MODEM_CUST_CONFIG_DIR)/config/balong_product_config.mk

ifeq ($(strip $(CFG_CONFIG_EICC_V200)),YES)
obj-y += eicc_core.o
obj-y += eicc_device.o
obj-y += eicc_driver.o
obj-y += eicc_dts.o
obj-y += eicc_platform.o
obj-y += eicc_debug.o
obj-y += eicc_dump.o
obj-y += eicc_pmrst.o
obj-y += eicc_test.o
endif
