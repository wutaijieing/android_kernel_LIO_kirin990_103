ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif
-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/$(OBB_MODEM_CUST_CONFIG_DIR)/config/balong_product_config.mk 

EXTRA_CFLAGS += -Idrivers/hisi/tzdriver

obj-$(CONFIG_RFILE_SUPPORT)  += adp_rfile.o
ifeq ($(strip $(CFG_CONFIG_RFILE_USER)),YES)
obj-$(CONFIG_RFILE_SUPPORT)  += rfile_balong_user.o
else
obj-$(CONFIG_RFILE_SUPPORT)  += rfile_balong.o
endif
ifneq ($(CONFIG_RFILE_SUPPORT),y)
obj-y += adp_rfile_stub.o
endif
obj-$(CONFIG_ENABLE_TEST_CODE)  += rfile_api_test.o
