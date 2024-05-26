/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: Contexthub chre driver. not enable in USER build.
 * Create: 2017-10-19
 */

#ifndef __CONTEXTHUB_DYNLOAD_H__
#define __CONTEXTHUB_DYNLOAD_H__
#include <linux/types.h>
#include <platform_include/smart/linux/base/ap/protocol.h>

#define CHRE_MAX_PACKET_LEN  128

#define MODULE_NAME "chre"
#define MAX_DEVTOAPP_MSG_SIZE 0x400

#define CHRE_NANOAPP_HEADER_SIZE 56

#define CHRE_MSG_RESP_TIMEOUT 300 /*ms*/

#define CHRE_WAIT_MSG_STATUS 0xFF

#define CHRE_RESP_ERR -1
#define CHRE_RESP_OK  0

enum nanoapp_status {
	IDLE,
	LOAD_PREPARE = 0x1,
	LOAD_DONE = 0x2,
	UNLOAD_PREPARE = 0x3,
	UNLOAD_DONE = 0x4,
};

struct chre_local_message {
	struct list_head list;
	char *data;
	unsigned int data_len;
};

struct chre_nanoapp_node {
	struct list_head list;
	u64 app_id;
	enum nanoapp_status status;
	char bin_path[CHRE_MAX_PACKET_LEN];
};

struct chre_dev {
	struct semaphore msg_dev2app_read_wait;
	struct semaphore msg_mcu_resp;
	spinlock_t msg_app2dev_lock;
	spinlock_t msg_dev2app_lock;
	struct list_head msg_app2dev_list;
	struct list_head msg_dev2app_list;
	struct work_struct msg_app2dev_hanlder_work;
	struct list_head nanoapp_list;
	struct mutex msg_resp_stat_lock;
	int msg_result;
};

typedef enum {
	CONTEXT_HUB_APPS_ENABLE  = 1,
	CONTEXT_HUB_APPS_DISABLE = 2,
	CONTEXT_HUB_LOAD_APP     = 3,
	CONTEXT_HUB_UNLOAD_APP   = 4,
	CONTEXT_HUB_QUERY_APPS   = 5,
	CONTEXT_HUB_QUERY_MEMORY = 6,
	CONTEXT_HUB_OS_REBOOT    = 7,
	CONTEXT_HUB_PRIVATE_MSG  = 0x400,
} chre_hal_command;

struct mem_range_t {
	u32 total_bytes;
	u32 free_bytes;
	u32 type;
	u32 mem_flags;
};

struct chre_app_info {
	u64 app_id;
	u32 version;
	u32 num_mem_ranges;
	struct mem_range_t mem_usage[2];
};

struct chreNanoAppBinaryHeader {
	uint32_t headerVersion;        // 0x1 for this version
	uint32_t magic;                // "NANO" (see NANOAPP_MAGIC in context_hub.h)
	uint64_t appId;                // App Id, contains vendor id
	uint32_t appVersion;           // Version of the app
	uint32_t flags;                // Signed, encrypted
	uint64_t hwHubType;            // Which hub type is this compiled for
	uint8_t targetChreApiMajorVersion; // Which CHRE API version this is compiled for
	uint8_t targetChreApiMinorVersion;
	uint8_t reserved[6];
} __attribute__((packed));

struct chre_app2dev_message {
	u32 event_id;
	u64 app_id;
	u8  len;
	u32 message_type;
	u8  data[CHRE_MAX_PACKET_LEN];
	u8 reserved[3];
} __attribute__((packed));

struct chre_dev2app_message {
	u32 message_type;
	u64 app_id;
	u8  len;
	u8  data[CHRE_MAX_PACKET_LEN];
} __attribute__((packed));

struct chre_dev2app_query_msg {
	u32 event_id;
	u32 message_type;
	u64 app_id;
	u8  len;
	struct chre_app_info apps[2];
} __attribute__((packed));

#endif
