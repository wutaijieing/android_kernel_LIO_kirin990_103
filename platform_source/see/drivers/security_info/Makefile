
ccflags-$(CONFIG_SECURITY_INFO) += -I$(srctree)/drivers/platform_drivers/tzdriver

obj-$(CONFIG_SECURITY_INFO) += security_info_main.o
obj-$(CONFIG_SECURITY_INFO) += bypass_net_cert.o
obj-$(CONFIG_SECURITY_INFO) += uapp_ioctl.o

ccflags-y += -Wdate-time -Wextra -Wfloat-equal -Wformat=2 -Wshadow