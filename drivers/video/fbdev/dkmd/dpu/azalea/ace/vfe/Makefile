obj-$(CONFIG_DPU_CDC_ACE) := dpufe.o

dpufe-objs := \
	dpufe_dbg.o \
	dpufe_dts.o \
	dpufe_sysfs.o \
	dpufe_ops.o \
	dpufe_vactive.o \
	dpufe_vsync.o \
	dpufe_dev.o \
	dpufe_fb_buf_sync.o \
	dpufe_ov_play.o \
	dpufe_ov_utils.o \
	dpufe_sync.o \
	dpufe_iommu.o \
	dpufe_virtio_fb.o \
	dpufe_async_play.o \
	dpufe_ion.o \
	dpufe_pan_display.o \
	dpufe_mmbuf_manager.o \
	dpufe_queue.o \
	dpufe_disp_recorder.o

EXTRA_CFLAGS += \
	-Idrivers/iommu/smmu/
