/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: Contexthub data playback driver. Test only, not enable in USER build.
 * Create: 2017-04-22
 */
#ifndef __CONTEXTHUB_PLAYBACK_H__
#define __CONTEXTHUB_PLAYBACK_H__
#include "inputhub_api/inputhub_api.h"

#define PLAYBACK_MAX_PATH_LEN   512
#define MAX_FILE_SIZE           (1<<24) /* 16M */
#define MAX_SUPPORT_TAG_COUNT   8 // can't over 8, will exceed ipc body len

#define IOCTL_PLAYBACK_INIT             _IOW(0xB3, 0x01, unsigned int)
#define IOCTL_PLAYBACK_START            _IO(0xB3, 0x02)
#define IOCTL_PLAYBACK_STOP             _IO(0xB3, 0x03)
#define IOCTL_PLAYBACK_REPLAY_COMPLETE  _IOR(0xB3, 0x04, unsigned int)

enum {
	FUNCTION_OPEN   = (1<<0),
	FUNCTION_INIT   = (1<<1),
	FUNCTION_START  = (1<<2),
};

enum {
	BUFFER1_READY = 1,
	BUFFER2_READY,
	BUFFER_ALL_READY,
};

enum {
	COMPLETE_CLOSE = 0,
	COMPLETE_REPLAY_DONE,
};

struct sm_info_t {
	unsigned short  tag_id;
	unsigned int    buf1_addr;
	unsigned int    buf2_addr;
	unsigned int    buf_size;
} __packed;

struct sm_open_cmd_t {
	struct pkt_header    pkt;
	unsigned char   mode;   /* 0:play 1:record */
	struct sm_info_t       sm_info[];
} __packed;

struct data_status_t {
	unsigned short  tag_id;
	unsigned short  buf_status; // 1: buffer1 ready 2: buffer2 ready 3: buffer1 and 2 ready
	unsigned int    data_size[2];
} __packed;

struct sm_data_ready_cmd_t {
	struct pkt_header    pkt;
	unsigned int    sub_cmd;
	struct data_status_t   data_status[];
} __packed;

struct buf_status_t {
	unsigned short  tag_id;
	unsigned short  buf_status; // 1: buffer1 ready 2: buffer2 ready 3: buffer1 and 2 ready
} __packed;

struct sm_buf_ready_cmd_t {
	struct pkt_header    pkt;
	unsigned int    sub_cmd;
	struct buf_status_t    buf_status[];
} __packed;

struct sensor_data_format_t {
	unsigned short  tag_id;
	unsigned short  data_len;
	unsigned char   sensor_data[];
} __packed;

struct app_init_cmd_t {
	unsigned int    mode; // 0 means data playback, 1 means data recording
	void            *path;
};

struct playback_info_t {
	unsigned short  tag_id;
	unsigned int    buf1_addr;
	unsigned int    buf2_addr;
	unsigned int    buf_size;
	unsigned int    file_offset;
} __packed;

struct playback_dev_t {
	unsigned int    status;
	unsigned int    current_mode;
	char            current_path[PLAYBACK_MAX_PATH_LEN];
	char            filename[PLAYBACK_MAX_PATH_LEN];
	unsigned int    current_count;
	unsigned int    phy_addr;
	unsigned int    size;
	void __iomem    *vaddr;
	struct mutex    mutex_lock;
	struct completion done;
	unsigned int    complete_status;
	struct work_struct work;
	unsigned int    data;
	struct playback_info_t *info;
};

#endif
