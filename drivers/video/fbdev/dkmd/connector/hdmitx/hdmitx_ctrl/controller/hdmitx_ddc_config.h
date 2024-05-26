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

#ifndef __HDMITX_DDC_CONFIG_H__
#define __HDMITX_DDC_CONFIG_H__

#include <linux/types.h>
#include <linux/mutex.h>

#define DDC_MAX_FIFO_SIZE 16
#define DDC_SEGMENT_ADDR  0x30
#define DDC_M_RD  0x0001 /* read data, from slave to master */

#define DDC_WRITE_MAX_BYTE_NUM 32

/* I2C Slave Address */
#define EDID_I2C_SALVE_ADDR 0xA0
#define HDCP_I2C_SALVE_ADDR 0x74
#define SCDC_I2C_SALVE_ADDR 0xA8

enum ddc_master_access {
	DDC_MASTER_DISABLE = 0,
	DDC_MASTER_ENABLE = 1
};

enum ddc_issue_cmd {
	CMD_READ_SINGLE_NO_ACK = 0x00,  /* 0'b0000: */
	CMD_READ_SINGLE_ACK = 0x01,     /* 0'b0001: */
	CMD_READ_MUTI_NO_ACK = 0x02,    /* 4'b0010: */
	CMD_READ_MUTI_ACK = 0x03,       /* 4'b0011: */
	CMD_READ_SEGMENT_NO_ACK = 0x04, /* 4'b0100: */
	CMD_READ_SEGMENT_ACK = 0x05,    /* 4'b0101: */
	CMD_WRITE_MUTI_NO_ACK = 0x06,   /* 4'b0110: */
	CMD_WRITE_MUTI_ACK = 0x07,      /* 4'b0111: */
	CMD_FIFO_CLR = 0x09,            /* 4'b1001: */
	CMD_SCL_DRV = 0x0a,             /* 4'b1010: */
	CMD_MASTER_ABORT = 0x0f         /* 4'b1111: */
};

enum ddc_issue_mode {
	MODE_READ_SINGLE_NO_ACK,  /* 0b0000 */
	MODE_READ_SINGLE_ACK,     /* 0b0001 */
	MODE_READ_MUTIL_NO_ACK,   /* 0b0010 */
	MODE_READ_MUTIL_ACK,      /* 0b0011 */
	MODE_READ_SEGMENT_NO_ACK, /* 0b0100 */
	MODE_READ_SEGMENT_ACK,    /* 0b0101 */
	MODE_WRITE_MUTIL_NO_ACK,  /* 0b0110 */
	MODE_WRITE_MUTIL_ACK,     /* 0b0111 */
	MODE_MAX
};

struct ddc_timeout {
	u32 access_timeout;  /* access_timeout */
	u32 hpd_timeout;     /* hpd_timeout */
	u32 in_prog_timeout; /* in_prog_timeout */
	u32 scl_timeout;     /* scl_timeout */
	u32 sda_timeout;     /* sda_timeout */
	u32 issue_timeout;   /* issue_timeout */
};

struct hdmitx_ddc {
	u32 id;
	void  *hdmi_regs;
	void  *hdmi_aon_regs;
	u8 slave_reg;     /* slave register offset */
	u8 slave_addr;    /* ddc slave address */
	bool is_segment;  /* is segment read */
	bool is_regaddr;  /* is register address */
	u8 xfer_mode;     /* work mode specified for huanglong ddc */
	struct mutex lock;  /* For ddc operation. */
	struct ddc_timeout timeout; /* for save ddc timeout */
};

struct ddc_msg {
	u16 addr;  /* slave address */
	u16 flags; /* operation mask */
	u16 len;   /* msg length */
	u8 *buf;   /* pointer to msg data */
};

s32 ddc_xfer(struct hdmitx_ddc *ddc, const struct ddc_msg *msgs, u8 num);

#endif
