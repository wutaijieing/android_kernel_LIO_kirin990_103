#
#QIC Makefile
#

subdir-ccflags-$(CONFIG_DFX_BB)               += -Idrivers/platform_drivers/mntn/blackbox/
subdir-ccflags-$(CONFIG_DFX_SEC_QIC)              += -Idrivers/platform_drivers/mntn/ -Wall -Werror

obj-$(CONFIG_DFX_SEC_QIC) += dfx_sec_qic.o             \
                             dfx_sec_qic_err_probe.o   \
                             dfx_sec_qic_dump.o

