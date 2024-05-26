/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/iommu.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/mm_iommu.h>
#include <linux/vmalloc.h>
#include <linux/virtio.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_config.h>

#include "../include/dpu_communi_def.h"
#include "dpufe_dbg.h"
#include "dpufe_dev.h"
#include "dpufe.h"
#include "dpufe_ov_utils.h"
#include "dpufe_fb_buf_sync.h"
#include "dpufe_panel_def.h"
#include "dpufe_virtio_fb.h"
#include "dpufe_enum.h"

#define MAX_VFB_QUEUE_NUM VQ_VFB_MAX_NUM
#define SG_LEN 1
#define IRQ_MAX_NUM 3

#define SEND_BUF_TIME_OUT_US 16000
#define VIRT_COMMUNI_TIME_OUT_US 1000
#define SEND_BLANK_TIME_OUT_US 300000

int g_registered_fb_nums = 0;

/* virtqueue funcs:
   vq0 : display 0
   vq1 : display 1
   vq2 : blank 0,1
   vq3 : irq vsync 0
   vq4 : irq vactive start 1
   vq5 : irq vsync 1
*/
typedef enum vfb_vq_type {
	VQ_DISPLAY0 = 0,
	VQ_DISPLAY1,
	VQ_BLANK0,
	VQ_BLANK1,
	VQ_VSYNC0,
	VQ_VSYNC1,
	VQ_VSTART1,
	VQ_FB_NUMS,
	VQ_VFB_MAX_NUM
} vfb_vq_type_t;

struct virtio_fb_info {
	struct virtio_device *vdev;
	struct virtqueue **vqs;
	int num_vqs;
	struct vfb_irq_info *irq_buf[IRQ_MAX_NUM];
	int fb_nums;
};

struct vfb_blank_info {
	uint32_t fb_id;
	int blank_mode;
};

typedef struct vfb_irq_info {
	int32_t isr_type;
	uint64_t timestamp;
} vfb_irq_info_t;

static struct virtio_fb_info *g_vfb_info;

int get_registered_fb_nums(void)
{
	return g_registered_fb_nums;
}

static int get_fbidx_from_qidx(uint32_t qidx)
{
	int fbidx = -1;

	switch(qidx) {
	case VQ_DISPLAY0:
	case VQ_VSYNC0:
		fbidx = PRIMARY_PANEL_IDX;
		break;
	case VQ_DISPLAY1:
	case VQ_VSYNC1:
	case VQ_VSTART1:
		fbidx = EXTERNAL_PANEL_IDX;
		break;
	default:
		DPUFE_ERR("unknown qidx[%u]\n", qidx);
		break;
	}

	return fbidx;
}

static int add_inbuf(uint32_t qidx, char *buf, uint32_t size, uint32_t timeout)
{
	struct scatterlist sg;
	int ret;
	struct virtio_fb_info *vfi = g_vfb_info;
	static uint32_t add_count = 0;
	timeval_compatible tv_begin;

	if (vfi == NULL) {
		DPUFE_ERR("vfi is null\n");
		return -1;
	}

	dpufe_trace_ts_begin(&tv_begin);

	sg_init_one(&sg, buf, size);
	ret = virtqueue_add_inbuf(vfi->vqs[qidx], &sg, SG_LEN, buf, GFP_KERNEL);
	virtqueue_kick(vfi->vqs[qidx]);
	if (ret < 0) {
		DPUFE_ERR("virtqueue add buffer failed\n");
		return -1;
	}

	add_count++;
	if (g_dpufe_debug_vfb_transfer)
		DPUFE_INFO("add_count = %u\n", add_count);

	dpufe_trace_ts_end(&tv_begin, timeout, "vq%d add_inbuf timediff = ", qidx);
	return 0;
}

static int vfb_init_fb_vq(struct virtio_fb_info *vfi)
{
	int ret = 0;

	DPUFE_INFO("+\n");

	ret = add_inbuf(VQ_FB_NUMS, (char*)(&vfi->fb_nums), sizeof(int), VIRT_COMMUNI_TIME_OUT_US);
	if (ret != 0)
		DPUFE_ERR("add_inbuf failed index\n");

	DPUFE_INFO("-\n");
	return ret;
}

static int vfb_init_irq(struct virtio_fb_info *vfi)
{
	int ret = 0;
	uint32_t i;
	uint32_t irq_idx;

	for (i = VQ_VSYNC0; i <= VQ_VSTART1; i++) {
		irq_idx = i - VQ_VSYNC0;
		vfi->irq_buf[irq_idx] = kzalloc(sizeof(struct vfb_irq_info), GFP_KERNEL);
		if (vfi->irq_buf[irq_idx] == NULL) {
			DPUFE_ERR("irq buf[%d] alloc failed\n", irq_idx);
			ret = -1;
			goto free_irq_buf;
		}
		ret = add_inbuf(i, (char *)vfi->irq_buf[irq_idx], sizeof(vfb_irq_info_t), VIRT_COMMUNI_TIME_OUT_US);
		if (ret != 0) {
			DPUFE_ERR("add_inbuf failed index %d\n", i);
			goto free_irq_buf;
		}
	}
	return 0;

free_irq_buf:
	for (i = VQ_VSYNC0; i <= VQ_VSTART1; i++) {
		irq_idx = i - VQ_VSYNC0;
		if (vfi->irq_buf[irq_idx])
			kfree(vfi->irq_buf[irq_idx]);
	}
	return ret;
}

int vfb_send_blank_ctl(uint32_t fb_id, int blank_mode)
{
	int ret;
	struct vfb_blank_info blank_info;

	blank_info.fb_id = fb_id;
	blank_info.blank_mode = blank_mode;
	if (fb_id > EXTERNAL_PANEL_IDX) {
		DPUFE_ERR("no support fb id %u\n", fb_id);
		return -1;
	}

	ret = add_inbuf(VQ_BLANK0 + fb_id, (char *)&blank_info, sizeof(struct vfb_blank_info), SEND_BLANK_TIME_OUT_US);
	if (ret != 0) {
		DPUFE_ERR("add inbuf failed vq index %d\n", VQ_BLANK0 + fb_id);
		return -1;
	}
	return ret;
}

int vfb_send_buffer(struct dpu_core_disp_data *ov_info)
{
	int ret;

	DPUFE_DEBUG("fb[%d], frame_no[%u] +\n", ov_info->disp_id, ov_info->frame_no);
	if (ov_info->disp_id > EXTERNAL_PANEL_IDX) {
		DPUFE_ERR("invalid display index %d\n", ov_info->disp_id);
		return -1;
	}

	ret = add_inbuf(ov_info->disp_id, (char *)ov_info,
		sizeof(struct dpu_core_disp_data), SEND_BUF_TIME_OUT_US);
	if (ret != 0) {
		DPUFE_ERR("add inbuf failed index %d\n", ov_info->disp_id);
		return -1;
	}
	DPUFE_DEBUG("fb[%d], frame_no[%u] -\n", ov_info->disp_id, ov_info->frame_no);
	return ret;
}

static void irq_callback(char *callback_buf, uint32_t qidx)
{
	struct vfb_irq_info* irq_info = (struct vfb_irq_info* )callback_buf;
	int fb_idx;
	struct dpufe_data_type * dfd = NULL;
	timeval_compatible tv_begin;
	int ret;

	DPUFE_DEBUG("vq:%u isr_type: %u, timestamp: %lu\n",
		qidx, irq_info->isr_type, irq_info->timestamp);

	fb_idx = get_fbidx_from_qidx(qidx);
	if (fb_idx == -1) {
		DPUFE_ERR("invalid fb_idx\n");
		return;
	}
	dfd = dpufe_get_dpufd((uint32_t)fb_idx);
	dpufe_check_and_no_retval(!dfd, "dfd is null\n");

	ret = add_inbuf(qidx, callback_buf, sizeof(vfb_irq_info_t), VIRT_COMMUNI_TIME_OUT_US);
	dpufe_check_and_no_retval(ret != 0, "vq%d add inbuf failed\n", qidx);

	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("qidx: %d.\n", qidx);

	dpufe_trace_ts_begin(&tv_begin);
	switch (qidx) {
	case VQ_VSYNC0:
		dpufe_queue_push_tail(&g_dpu_dbg_queue[DBG_DPU0_VSYNC], ktime_get());
		dpufe_buf_sync_signal(dfd);
		// only primary panel chn need upload vsync timestamp to hwc
		dpufe_vsync_isr_handler(&dfd->vsync_ctrl, irq_info->timestamp, dfd->index);
		break;
	case VQ_VSYNC1:
		dpufe_queue_push_tail(&g_dpu_dbg_queue[DBG_DPU1_VSYNC], ktime_get());
		dpufe_buf_sync_signal(dfd);
		break;
	case VQ_VSTART1:
		dpufe_vstart_isr_handler(&dfd->vstart_ctl);
		break;
	default:
		break;
	}
	dpufe_trace_ts_end(&tv_begin, VIRT_COMMUNI_TIME_OUT_US, "vsync irq_callback timediff =");
}

static char *get_priv_buf(struct virtqueue *vq, unsigned int *len)
{
	char *cb_buf = NULL;
	timeval_compatible tv_begin;

	dpufe_trace_ts_begin(&tv_begin);
	cb_buf = (char *)virtqueue_get_buf(vq, len);
	dpufe_trace_ts_end(&tv_begin, VIRT_COMMUNI_TIME_OUT_US, "get_buf irq_callback timediff =");

	return cb_buf;
}

static void vfb_display_feedback_handler(struct virtqueue *vq)
{
	struct dpufe_data_type * dfd = NULL;
	int fb_idx;

	DPUFE_DEBUG("vq%u +\n", vq->index);

	fb_idx = get_fbidx_from_qidx(vq->index);
	dpufe_check_and_no_retval(fb_idx == -1, "invalid fb_idx:%d\n", fb_idx);

	dfd = dpufe_get_dpufd((uint32_t)fb_idx);
	dpufe_check_and_no_retval(!dfd, "dfd is null\n");

	dpufe_update_frame_refresh_state(dfd);

	DPUFE_DEBUG("vq%u -\n", vq->index);
}

static void virtio_fb_callback(struct virtqueue *vq)
{
	uint32_t len;
	char *callback_buf = NULL;
	static uint32_t cb_count = 0;

	DPUFE_DEBUG("vq%u +\n", vq->index);
	cb_count++;

	if (g_dpufe_debug_vfb_transfer)
		DPUFE_INFO("cb_count = %u\n", cb_count);

	callback_buf = get_priv_buf(vq, &len);
	if (!callback_buf) {
		DPUFE_ERR("callback buf is NULL\n");
		return;
	}

	switch (vq->index) {
	case VQ_DISPLAY0:
		vfb_display_feedback_handler(vq);
		break;
	case VQ_DISPLAY1:
	case VQ_BLANK0:
	case VQ_BLANK1:
		break;
	case VQ_VSYNC0:
	case VQ_VSTART1:
	case VQ_VSYNC1:
		irq_callback(callback_buf, vq->index);
		break;
	case VQ_FB_NUMS:
		g_registered_fb_nums = *(int*)callback_buf;
		DPUFE_INFO("fb_nums is %d\n", g_registered_fb_nums);
		break;
	default:
		DPUFE_ERR("unknown index[%u]\n", vq->index);
		break;
	}
	DPUFE_DEBUG("vq%u -\n", vq->index);
}

static int find_vqs(struct virtio_device *vdev, unsigned nvqs,
	struct virtqueue *vqs[], vq_callback_t *callbacks[], const char * const names[], struct irq_affinity *desc)
{
	int ret;
	timeval_compatible tv_begin;

	dpufe_trace_ts_begin(&tv_begin);
	ret = virtio_find_vqs(vdev, nvqs, vqs, callbacks, names, desc);
	dpufe_trace_ts_end(&tv_begin, VIRT_COMMUNI_TIME_OUT_US, "find_vqs timediff =");
	return ret;
}

static int init_vqs(struct virtio_device *vdev, struct virtio_fb_info *vfi, u32 queue_num)
{
	const char *io_names[MAX_VFB_QUEUE_NUM] = {
		"vq_display0",
		"vq_display1",
		"vq_blank0",
		"vq_blank1",
		"vq_vsync0",
		"vq_vsync1",
		"vq_vstart1",
		"vq_fb_nums",
	};

	vq_callback_t **io_callbacks = NULL;
	int ret;
	int i;

	vfi->vqs = kmalloc(queue_num * sizeof(struct virtqueue *), GFP_KERNEL);
	io_callbacks = kmalloc(queue_num * sizeof(vq_callback_t *), GFP_KERNEL);
	if ((!vfi->vqs) || (!io_callbacks)) {
		DPUFE_ERR("vqs alloc failed\n");
		ret = -ENOMEM;
		goto free;
	}

	for (i = 0 ; i < queue_num; i++)
		io_callbacks[i] = virtio_fb_callback;

	ret = find_vqs(vdev, queue_num, vfi->vqs, io_callbacks, (const char **)io_names, NULL);
	if (ret != 0) {
		DPUFE_ERR("vqs find failed,err:%d\n", ret);
		goto free;
	}
	return 0;

free:
	if (io_callbacks)
		kfree(io_callbacks);
	if (vfi->vqs)
		kfree(vfi->vqs);
	return ret;
}

static int virtio_fb_probe(struct virtio_device *vdev)
{
	struct virtio_fb_info *vfi = NULL;
	int ret;

	DPUFE_INFO("+\n");
	vfi = kmalloc(sizeof(struct virtio_fb_info), GFP_KERNEL);
	if (!vfi) {
		DPUFE_ERR("vfi alloc failed\n");
		return -1;
	}
	g_vfb_info = vfi;

	ret = init_vqs(vdev, vfi, MAX_VFB_QUEUE_NUM);
	if (ret < 0) {
		DPUFE_ERR("Error initializing vqs\n");
		kfree(vfi);
		return ret;
	}

	vfb_init_irq(vfi);
	vfb_init_fb_vq(vfi);

	DPUFE_INFO("-\n");
	return ret;
}

static void virtio_fb_remove(struct virtio_device *vdev)
{
	struct virtio_fb_info *vfi = g_vfb_info;
	uint32_t i;
	uint32_t irq_idx;

	for (i = VQ_VSYNC0; i <= VQ_VSTART1; i++) {
		irq_idx = i - VQ_VSYNC0;
		if (vfi->irq_buf[irq_idx])
			kfree(vfi->irq_buf[irq_idx]);
	}

	if (vfi->vqs)
		kfree(vfi->vqs);

	if (g_vfb_info) {
		kfree(g_vfb_info);
		g_vfb_info = NULL;
	}
}

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_FB, VIRTIO_DEV_ANY_ID },
};

static struct virtio_driver virtio_fb_driver = {
	.driver.name = KBUILD_MODNAME,
	.driver.owner = THIS_MODULE,
	.id_table = id_table,
	.probe = virtio_fb_probe,
	.remove = virtio_fb_remove,
};

int virtio_fb_init(void)
{
	DPUFE_INFO("vfb init\n");
	return register_virtio_driver(&virtio_fb_driver);
}

module_init(virtio_fb_init)

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Virtio framebuffer driver");
