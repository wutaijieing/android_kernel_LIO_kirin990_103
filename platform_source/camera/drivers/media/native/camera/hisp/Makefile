
ccflags-y += -Wextra -Wdate-time -Wfloat-equal -Wshadow
ccflags-y += -I$(srctree)/platform_source/camera/drivers/media/native/camera
ccflags-y += -Idrivers/rpmsg

obj-y += hisp.o
CLT_CAMERA := n
ifeq ($(CLT_IV), true)
CLT_CAMERA := n
endif

ifeq ($(CLT_CAMERA), y)
obj-y += simulated_fw/

ifeq ($(CONFIG_KERNEL_CAMERA_ISPV150), y)
subdir-ccflags-y += -DHISP150_CAMERA
endif

ifeq ($(CONFIG_KERNEL_CAMERA_ISPV160), y)
subdir-ccflags-y += -DHISP160_CAMERA
endif

ifeq ($(CONFIG_KERNEL_CAMERA_ISPV120), y)
subdir-ccflags-y += -DHISP120_CAMERA
endif

ifeq ($(CONFIG_KERNEL_CAMERA_ISPV130), y)
subdir-ccflags-y += -DHISP120_CAMERA
subdir-ccflags-y += -DHISP130_USE_V3
endif

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV110), y)
	obj-y += hisp110/

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV150), y)
EXTRA_CFLAGS += -DHISP150_CAMERA
	obj-y += hisp150/

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV160), y)
EXTRA_CFLAGS += -DHISP160_CAMERA
	obj-y += hisp160/

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV120), y)
EXTRA_CFLAGS += -DHISP120_CAMERA
	obj-y += hisp120/

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV130), y)
EXTRA_CFLAGS += -DHISP120_CAMERA
EXTRA_CFLAGS += -DHISP130_USE_V3
	obj-y += hisp120/

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV200), y)
EXTRA_CFLAGS += -DHISP200_CAMERA
	obj-y += hisp200/

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV210), y)
EXTRA_CFLAGS += -DHISP210_CAMERA
	obj-y += hisp210/

else ifeq ($(CONFIG_CAMERA_ISPV220), y)
EXTRA_CFLAGS += -DHISP220_CAMERA
	obj-y += hisp220/

else ifeq ($(CONFIG_CAMERA_ISPV230), y)
EXTRA_CFLAGS += -DHISP230_CAMERA
	obj-y += hisp220/

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV250), y)
EXTRA_CFLAGS += -DHISP250_CAMERA
	obj-y += hisp250/

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV310), y)
EXTRA_CFLAGS += -DHISP310_CAMERA
	obj-y += hisp310/

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV300), y)
EXTRA_CFLAGS += -DHISP300_CAMERA
	obj-y += hisp300/

else ifeq ($(CONFIG_KERNEL_CAMERA_ISPV350), y)
EXTRA_CFLAGS += -DHISP350_CAMERA
	obj-y += hisp350/
endif



ifneq ($(CLT_IV), true)
ifneq ($(CONFIG_RPMSG),y)
	#obj-y += fakefw/
endif
endif


