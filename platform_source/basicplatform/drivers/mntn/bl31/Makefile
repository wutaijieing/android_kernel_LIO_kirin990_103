ccflags-y += -Wall -Werror
EXTRA_CFLAGS += -Idrivers/platform_drivers/tzdriver
ifeq ($(CONFIG_MNTN_SPI_ROUTE), y)
EXTRA_CFLAGS += -Idrivers/platform_drivers/mntn/blackbox
endif
obj-$(CONFIG_DFX_BL31_MNTN) += dfx_bl31_exception.o
obj-$(CONFIG_DFX_BB) += dfx_bl31_trace.o
obj-$(CONFIG_MNTN_SPI_ROUTE) += dfx_bl31_route_handler.o
