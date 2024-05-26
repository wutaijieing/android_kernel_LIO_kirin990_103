obj-$(CONFIG_DDR_DDRC_FLUX) += ddr_ddrcflux.o
ifeq ($(CONFIG_DDR_CHAMAELEON),y)
	obj-$(CONFIG_DDR_DDRC_FLUX) += timer_v5.o
else
	obj-$(CONFIG_DDR_DDRC_FLUX) += timer.o
endif
obj-$(CONFIG_DDR_TRACKING) += ddr_tracking.o

obj-$(CONFIG_DDR_DDRC_SEC) += dfx_ddr_secprotect.o
obj-$(CONFIG_DDR_AUTO_FSGT) += ddr_autofsgt_proxy_kernel.o
obj-$(CONFIG_DDR_PERF_CTRL) += ddr_perf_ctrl.o
subdir-ccflags-$(CONFIG_DFX_BB) += 	-Idrivers/platform_drivers/mntn/blackbox

ccflags-y += -Wdate-time -Wextra -Wfloat-equal -Wformat=2 -Wshadow
