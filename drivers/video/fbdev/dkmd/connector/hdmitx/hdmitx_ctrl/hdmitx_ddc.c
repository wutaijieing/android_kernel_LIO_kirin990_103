/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <securec.h>

#include "dkmd_log.h"

#include "hdmitx_common.h"
#include "hdmitx_ctrl.h"
#include "hdmitx_ddc_config.h"

/* DDC default timeout */
#define DDC_DEFAULT_TIMEOUT_ACCESS  100
#define DDC_DEFAULT_TIMEOUT_HPD	 100
#define DDC_DEFAULT_TIMEOUT_IN_PROG 20
#define DDC_DEFAULT_TIMEOUT_SCL	 1
#define DDC_DEFAULT_TIMEOUT_SDA	 30
#define DDC_DEFAULT_TIMEOUT_ISSUE   20

#define PWD_DDC_CTRL_BASE   0x38
#define AON_DDC_CTRL_BASE   0x40

static void ddc_timeout_init_default(struct hdmitx_ddc *ddc)
{
	ddc->timeout.access_timeout = DDC_DEFAULT_TIMEOUT_ACCESS;
	ddc->timeout.hpd_timeout = DDC_DEFAULT_TIMEOUT_HPD;
	ddc->timeout.in_prog_timeout = DDC_DEFAULT_TIMEOUT_IN_PROG;
	ddc->timeout.scl_timeout = DDC_DEFAULT_TIMEOUT_SCL;
	ddc->timeout.sda_timeout = DDC_DEFAULT_TIMEOUT_SDA;
	ddc->timeout.issue_timeout = DDC_DEFAULT_TIMEOUT_ISSUE;
}

struct hdmitx_ddc *hdmitx_ddc_init(struct hdmitx_ctrl *hdmitx)
{
	struct hdmitx_ddc *ddc = NULL;

	ddc = (struct hdmitx_ddc *)kmalloc(sizeof(struct hdmitx_ddc), GFP_KERNEL);
	if (ddc == NULL) {
		dpu_pr_err("kmalloc fail.\n");
		return NULL;
	}

	if (memset_s(ddc, sizeof(*ddc), 0, sizeof(*ddc)) != EOK) {
		dpu_pr_err("memset_s fail.\n");
		goto ddc_init_failed;
	}

	ddc->id = 0;
	ddc->hdmi_regs = hdmitx->base + PWD_DDC_CTRL_BASE;
	ddc->hdmi_aon_regs = hdmitx->base + AON_DDC_CTRL_BASE;

	// to fix : hal_ddc_enter;
	mutex_init(&ddc->lock);
	ddc_timeout_init_default(ddc);

	hdmitx->ddc = ddc;
	return ddc;

ddc_init_failed:
	kfree(ddc);
	return NULL;
}

static s32 ddc_msg_transfer(struct hdmitx_ddc *ddc, const struct ddc_msg *msgs, u8 num, u8 retry)
{
	s32 i;
	s32 ret;

	for (i = 0; i < num; i++) {
		if (msgs[i].len == 0) {
			dpu_pr_err("unsupported transfer %d/%d, no data\n", i + 1, num);
			return -1;
		}
	}

	do
		ret = ddc_xfer(ddc, msgs, num);
	while ((retry-- > 0) && (ret < 0));

	return ret;
}

s32 hdmitx_ddc_edid_read(struct hdmitx_ddc *ddc, u8 *buffer,
	u16 block, u16 size)
{
	u8 start;
	u8 segment = block >> 1;
	u8 xfers = segment ? 3 : 2; /* 3,2: msg num */
	s32 ret;
	const s32 retry = 5; /* 5: msg transfer retry times */
	u64 temp = block * HDMITX_DEFAULT_EDID_BUFLEN;
	struct ddc_msg msgs[] = {
		{
			.addr = DDC_SEGMENT_ADDR,
			.flags = 0,
			.len = 1,
			.buf = &segment,
		}, {
			.addr = EDID_I2C_SALVE_ADDR,
			.flags = 0,
			.len = 1,
			.buf = &start,
		}, {
			.addr = EDID_I2C_SALVE_ADDR,
			.flags = DDC_M_RD,
			.len = size,
			.buf = buffer,
		}
	};

	start = (u8)temp;
	if (ddc == NULL || buffer == NULL) {
		dpu_pr_err("null ptr.\n");
		return -1;
	}

	/* Avoid sending the segment addr to not upset non-compliant DDC monitors. */
	ret = ddc_msg_transfer(ddc, &msgs[3 - xfers], xfers, retry); /* 3: msg num */
	if (ret != 0)
		dpu_pr_debug("ddc_msg_transfer fail");

	return ret;
}


/*
 * read a block of data from SCDC, starting at a given offset.
 * @ddc: ddc operation struct
 * @offset: start offset of block to read
 * @buffer: return location for the block to read
 * @size: size of the block to read
 */
s32 hdmitx_ddc_scdc_read(struct hdmitx_ddc *ddc, u8 offset,
	u8 *buffer, u16 size)
{
	s32 ret;
	const s32 retry = 5; /* 5: msg transfer retry times */

	struct ddc_msg msgs[2] = { /* 2: msg num */
		{
			.addr = SCDC_I2C_SALVE_ADDR,
			.flags = 0,
			.len = 1,
			.buf = &offset,
		}, {
			.addr = SCDC_I2C_SALVE_ADDR,
			.flags = DDC_M_RD,
			.len = size,
			.buf = buffer,
		}
	};

	if (ddc == NULL || buffer == NULL) {
		dpu_pr_err("null ptr.\n");
		return -1;
	}

	ret = ddc_msg_transfer(ddc, msgs, hdmitx_array_size(msgs), retry);

	return ret;
}

/*
 * write a block of data to SCDC, starting at a given offset.
 * @ddc: ddc operation struct
 * @offset: start offset of block to write
 * @buffer: block of data to write
 * @size: size of the block to write
 */
s32 hdmitx_ddc_scdc_write(struct hdmitx_ddc *ddc, u8 offset,
	const void *buffer, u16 size)
{
	struct ddc_msg msg = {
		.addr = SCDC_I2C_SALVE_ADDR,
		.flags = 0,
		.len = 1 + size,
		.buf = NULL,
	};
	u8 data[DDC_WRITE_MAX_BYTE_NUM] = {0};
	const s32 retry = 5; /* 5: msg transfer retry times */
	s32 ret;

	if (ddc == NULL || buffer == NULL) {
		dpu_pr_err("null ptr.\n");
		return -1;
	}

	dpu_pr_info("start scdc write\n");
	if (size > (DDC_WRITE_MAX_BYTE_NUM - 1)) {
		dpu_pr_info("%s: write size too big\n", __func__);
		return -1;
	}

	data[0] = offset;
	if (memcpy_s(&data[1], sizeof(data) - 1, buffer, size) != EOK) {
		dpu_pr_err("memcpy_s fail.\n");
		return -1;
	}
	msg.buf = data;
	msg.len = size + 1;

	ret = ddc_msg_transfer(ddc, &msg, 1, retry);

	return ret;
}

