obj-y += mdc.o

mdc-y := dpu_mdc_dev.o dpu_mdc_drv.o dpu_mdc_overlay.o

mdc-objs := $(mdc-y)

EXTRA_CFLAGS += \
    -I$(srctree)/drivers/video/fbdev/dkmd/dpu/azalea \
    -I$(srctree)/drivers/video/fbdev/dkmd/dpu/azalea/overlay

clean:
	rm *.o .*cmd

