ccflags-$(CONFIG_SECURITY_INFO) += -I$(srctree)/drivers/platform_drivers/tzdriver
ccflags-$(CONFIG_VENDOR_EFUSE)  += -I$(srctree)/drivers/platform_drivers/efuse

obj-$(CONFIG_VENDOR_EFUSE)	+= efuse_driver.o
obj-$(CONFIG_VENDOR_EFUSE)	+= efuse.o
obj-$(CONFIG_VENDOR_EFUSE)	+= efuse_internal.o
obj-$(CONFIG_SECURITY_INFO)	+= socid.o
obj-$(CONFIG_GENERAL_SEE)	+= flash_general_see_otp.o
obj-$(CONFIG_DFX_DEBUG_FS)	+= efuse_driver_debugfs.o

ccflags-y += -Wdate-time -Wextra -Wfloat-equal -Wformat=2 -Wshadow