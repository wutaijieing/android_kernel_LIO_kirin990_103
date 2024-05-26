# Hisilicon IPP segment Makefile

obj-$(CONFIG_HISPIPP_V350) += ipp_com.o
obj-$(CONFIG_HISPIPP_V350) += ipp_core.o
obj-$(CONFIG_HISPIPP_V350) += ipp_smc.o

ifeq ($(pcie_fpga_verify),true)
subdir-ccflags-$(CONFIG_HISPIPP_V350) += -DIPP_KERNEL_USE_PCIE_VERIFY
obj-$(CONFIG_HISPIPP_V350) += ipp_pcie.o
endif
