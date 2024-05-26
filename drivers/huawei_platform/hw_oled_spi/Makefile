obj-$(CONFIG_HUAWEI_OLED_SPI) += oled_spi_interface.o
obj-$(CONFIG_HUAWEI_OLED_SPI) += oled_spi_display.o
obj-$(CONFIG_HUAWEI_OLED_SPI) += oled_register_fb.o

EXTRA_CFLAGS += -I$(srctree)/drivers/video/fbdev/dkmd/dpu/azalea \
		-I$(srctree)/drivers/video/dkmd/dpu/azalea \
		-I$(srctree)/drivers/staging/android \
		-I$(srctree)/drivers/atfdriver \
		-I$(srctree)/drivers/hisi/tzdriver \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0 \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/common/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/hisi/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/hisi/adapt \
		-I$(srctree)/include/chipset_common/log
