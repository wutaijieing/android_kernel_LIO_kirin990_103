ccflags-$(CONFIG_HHEE_DEBUG) += -I$(srctree)/platform_source/see/drivers/hhee/
ccflags-$(CONFIG_HHEE_DEBUG) += -I$(srctree)/security/selinux -I$(srctree)/security/selinux/ss -I$(srctree)/security/selinux/include
ccflags-$(CONFIG_HHEE_DEBUG) += -I$(objtree)/security/selinux -I$(objtree)/security/selinux/ss -I$(objtree)/security/selinux/include

obj-$(CONFIG_HHEE_DEBUG) += dev.o debugfs.o ioctl.o sandbox.o xomtest.o harness.o credtest.o
obj-$(CONFIG_DEBUG_HKIP_EARLY_RODATA_PROTECTION) += test_rodata_protection.o
