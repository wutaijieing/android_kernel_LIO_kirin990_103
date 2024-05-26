/*
 * usbaudio_mailbox.h
 *
 * usbaudio mailbox driver
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __USBUDIO_MAILBOX_H__
#define __USBUDIO_MAILBOX_H__

#include "../hifi/usbaudio_ioctl.h"

#define USBAUDIO_CHN_COMMON \
	unsigned short  msg_type; \
	unsigned short  reserved;

enum usbaudio_chn_msg_type {
	USBAUDIO_CHN_MSG_PROBE = 0xFF00,                    /* acpu->dsp */
	USBAUDIO_CHN_MSG_PROBE_RCV = 0xFF01,                /* dsp->acpu */
	USBAUDIO_CHN_MSG_DISCONNECT = 0xFF02,               /* acpu->dsp */
	USBAUDIO_CHN_MSG_DISCONNECT_RCV = 0xFF03,           /* dsp->acpu */
	USBAUDIO_CHN_MSG_PIPEOUTINTERFACE_ON_RCV = 0xFF04,  /* dsp->acpu */
	USBAUDIO_CHN_MSG_PIPEOUTINTERFACE_OFF_RCV = 0xFF05, /* dsp->acpu */
	USBAUDIO_CHN_MSG_PIPEININTERFACE_ON_RCV = 0xFF06,   /* dsp->acpu */
	USBAUDIO_CHN_MSG_PIPEININTERFACE_OFF_RCV = 0xFF07,  /* dsp->acpu */
	USBAUDIO_CHN_MSG_SETINTERFACE_COMPLETE = 0xFF08,    /* acpu->dsp */
	USBAUDIO_CHN_MSG_NV_CHECK = 0xFF09,                 /* acpu->dsp */
	USBAUDIO_CHN_MSG_NV_CHECK_RCV = 0xFF10,             /* dsp->acpu */
};

enum usbaudio_tbl_samplerate {
	USBAUDIO_TABLE_SAMPLERATE_44100,
	USBAUDIO_TABLE_SAMPLERATE_48000,
	USBAUDIO_TABLE_SAMPLERATE_88200,
	USBAUDIO_TABLE_SAMPLERATE_96000,
	USBAUDIO_TABLE_SAMPLERATE_176400,
	USBAUDIO_TABLE_SAMPLERATE_192000,
	USBAUDIO_TABLE_SAMPLERATE_384000,
	USBAUDIO_TABLE_MAX,
};

struct usbaudio_epdesc {
	unsigned char length;
	unsigned char descriptor_type;
	unsigned char endpoint_address;
	unsigned char attributes;
	unsigned short max_packet_size;
	unsigned char interval;
	/*
	 * NOTE:  these two are _only_ in audio endpoints
	 * use USB_DT_ENDPOINT*_SIZE in length, not sizeof
	 */
	unsigned char refresh;
	unsigned char synch_address;
};

struct usbaudio_ifdesc {
	unsigned char length;
	unsigned char descriptor_type;
	unsigned char interface_number;
	unsigned char alternate_setting;
	unsigned char num_endpoints;
	unsigned char interface_class;
	unsigned char interface_sub_class;
	unsigned char interface_protocol;
	unsigned char interface;
};

struct usbaudio_formats {
	unsigned long long formats;    /* ALSA format bits */
	unsigned int channels;         /* # channels */
	unsigned int fmt_type;         /* USB audio format type (1-3) */
	unsigned int frame_size;       /* samples per frame for non-audio */
	int iface;                     /* interface number */
	unsigned char altsetting;      /* corresponding alternate setting */
	unsigned char altset_idx;      /* array index of altenate setting */
	unsigned char attributes;      /* corresponding attributes of cs endpoint */
	unsigned char endpoint;        /* endpoint */
	unsigned char ep_attr;         /* endpoint attributes */
	unsigned char data_interval;   /* log_2 of data packet interval */
	unsigned char protocol;        /* UAC_VERSION_1/2 */
	unsigned int max_pack_size;    /* max. packet size */
	unsigned int rates;
	unsigned int rate_table[USBAUDIO_TABLE_MAX];
	unsigned char clock;           /* associated clock */
};

struct usbaudio_pcms {
	struct usbaudio_formats fmts[USBAUDIO_PCM_NUM];
	struct usbaudio_ifdesc ifdesc[USBAUDIO_PCM_NUM];
	struct usbaudio_epdesc epdesc[USBAUDIO_PCM_NUM];
	unsigned int customsized;
};

struct usbaudio_probe_rcv_data {
	unsigned int ret_val;
};

struct usbaudio_disconnect_rcv_data {
	unsigned int ret_val;
};

struct usbaudio_rcv_msg {
	USBAUDIO_CHN_COMMON
	union {
		unsigned int rate;
		struct usbaudio_probe_rcv_data probe_rcv_data;
		struct usbaudio_disconnect_rcv_data disconnect_rcv_data;
	};
	unsigned int period;
};

struct usbaudio_disconnect_msg {
	USBAUDIO_CHN_COMMON
	unsigned int dsp_reset_flag;
};

struct usbaudio_nv_check_msg {
	USBAUDIO_CHN_COMMON
};

/* usbaudio dsp receive test message */
struct usbaudio_probe_msg {
	USBAUDIO_CHN_COMMON
	struct usbaudio_pcms pcms;
};

/* usbaudio dsp receive test message */
struct usbaudio_setinterface_complete_msg {
	USBAUDIO_CHN_COMMON
	int dir;
	int val;
	unsigned int rate;
	int ret;
};

int usbaudio_mailbox_init(void);
void usbaudio_mailbox_deinit(void);
int usbaudio_probe_msg(struct usbaudio_pcms *pcms);
int usbaudio_disconnect_msg(unsigned int dsp_reset_flag);
int usbaudio_setinterface_complete_msg(int dir, int val, int ret, unsigned int rate);
int usbaudio_nv_check(void);

#endif /* __USBUDIO_MAILBOX_H__ */
