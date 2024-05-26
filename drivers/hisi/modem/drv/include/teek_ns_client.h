/*
 * teek_ns_client.h
 *
 * define structures and IOCTLs.
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef TEEK_NS_CLIENT_H
#define TEEK_NS_CLIENT_H

#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <securec.h>
#include "tc_ns_client.h"
#include "tc_ns_log.h"

#define TC_NS_CLIENT_IOC_MAGIC  't'
#define TC_NS_CLIENT_DEV        "tc_ns_client"
#define TC_NS_CLIENT_DEV_NAME   "/dev/tc_ns_client"

#define EXCEPTION_MEM_SIZE (8*1024) /* mem for exception handling */
#define TSP_REQUEST        0xB2000008
#define TSP_RESPONSE       0xB2000009
#define TSP_REE_SIQ        0xB200000A
#define TSP_CRASH          0xB200000B
#define TSP_PREEMPTED      0xB2000005
#define TC_CALL_GLOBAL     0x01
#define TC_CALL_SYNC       0x02
#define TC_CALL_LOGIN            0x04
#define TEE_REQ_FROM_USER_MODE   0U
#define TEE_REQ_FROM_KERNEL_MODE 1U
#define TEE_PARAM_NUM            4
#define VMALLOC_TYPE             0
#define RESERVED_TYPE            1

/* Max sizes for login info buffer comming from teecd */
#define MAX_PACKAGE_NAME_LEN 255
/* The apk certificate format is as follows:
  * modulus_size(4 bytes) + modulus buffer(512 bytes)
  * + exponent size(4 bytes) + exponent buffer(1 bytes)
  */
#define MAX_PUBKEY_LEN 1024

struct tc_ns_dev_list {
	struct mutex dev_lock; /* for dev_file_list */
	struct list_head dev_file_list;
};

struct tc_uuid {
	uint32_t time_low;
	uint16_t time_mid;
	uint16_t timehi_and_version;
	uint8_t clockseq_and_node[8]; /* clock len is 8 */
};

#define INVALID_MAP_ADDR ((void*)-1)
struct tc_ns_shared_mem {
	void *kernel_addr;
	void *user_addr;
	void *user_addr_ca; /* for ca alloc share mem */
	unsigned int len;
	int mem_type;
	struct list_head head;
	atomic_t usage;
	atomic_t offset;
};

struct tc_ns_service {
	unsigned char uuid[UUID_LEN];
	struct mutex session_lock; /* for session_list */
	struct list_head session_list;
	struct list_head head;
	struct mutex operation_lock; /* for session's open/close */
	atomic_t usage;
};

#define SERVICES_MAX_COUNT 32 /* service limit can opened on 1 fd */
struct tc_ns_dev_file {
	unsigned int dev_file_id;
	struct mutex service_lock; /* for service_ref[], services[] */
	uint8_t service_ref[SERVICES_MAX_COUNT]; /* a judge if set services[i]=NULL */
	struct tc_ns_service *services[SERVICES_MAX_COUNT];
	struct mutex shared_mem_lock; /* for shared_mem_list */
	struct list_head shared_mem_list;
	struct list_head head;
	/* Device is linked to call from kernel */
	uint8_t kernel_api;
	/* client login info provided by teecd, can be either package name and public
	 * key or uid(for non android services/daemons)
	 * login information can only be set once, dont' allow subsequent calls
	 */
	bool login_setup;
	struct mutex login_setup_lock; /* for login_setup */
	uint32_t pkg_name_len;
	uint8_t pkg_name[MAX_PACKAGE_NAME_LEN];
	uint32_t pub_key_len;
	uint8_t pub_key[MAX_PUBKEY_LEN];
	int load_app_flag;
	struct completion close_comp; /* for kthread close unclosed session */
};

union tc_ns_parameter {
	struct {
		unsigned int buffer;
		unsigned int size;
	} memref;
	struct {
		unsigned int a;
		unsigned int b;
	} value;
	struct {
		unsigned int buffer;
		unsigned int size;
	} sharedmem;
};

struct tc_ns_login {
	unsigned int method;
	unsigned int mdata;
};

struct tc_ns_operation {
	unsigned int paramtypes;
	union tc_ns_parameter params[TEE_PARAM_NUM];
	unsigned int    buffer_h_addr[TEE_PARAM_NUM];
	struct tc_ns_shared_mem *sharemem[TEE_PARAM_NUM];
	void *mb_buffer[TEE_PARAM_NUM];
};

struct tc_ns_temp_buf {
	void *temp_buffer;
	unsigned int size;
};

enum smc_cmd_type {
	CMD_TYPE_GLOBAL,
	CMD_TYPE_TA,
	CMD_TYPE_TA_AGENT,
	CMD_TYPE_TA2TA_AGENT, /* compatible with TA2TA2TA->AGENT etc. */
	CMD_TYPE_BUILDIN_AGENT,
};

struct tc_ns_smc_cmd {
	uint8_t      uuid[sizeof(struct tc_uuid)];
	unsigned int cmd_type;
	unsigned int cmd_id;
	unsigned int dev_file_id;
	unsigned int context_id;
	unsigned int agent_id;
	unsigned int operation_phys;
	unsigned int operation_h_phys;
	unsigned int login_method;
	unsigned int login_data_phy;
	unsigned int login_data_h_addr;
	unsigned int login_data_len;
	unsigned int err_origin;
	int          ret_val;
	unsigned int event_nr;
	unsigned int uid;
	unsigned int ca_pid; /* pid */
	unsigned int pid;    /* tgid */
	unsigned int eventindex;     /* tee audit event index for upload */
	bool started;
} __attribute__((__packed__));

/*
 * @brief
 */
struct tc_wait_data {
	wait_queue_head_t send_cmd_wq;
	int send_wait_flag;
};

#define NUM_OF_SO 1
#ifdef CONFIG_CMS_CAHASH_AUTH
#define KIND_OF_SO 1
#else
#define KIND_OF_SO 2
#endif
struct tc_ns_session {
	unsigned int session_id;
	struct list_head head;
	struct tc_wait_data wait_data;
	struct mutex ta_session_lock; /* for open/close/invoke on 1 session */
	struct tc_ns_dev_file *owner;
	uint8_t auth_hash_buf[MAX_SHA_256_SZ * NUM_OF_SO + MAX_SHA_256_SZ];
	atomic_t usage;
};

struct mb_cmd_pack {
	struct tc_ns_operation operation;
	unsigned char login_data[MAX_SHA_256_SZ * NUM_OF_SO + MAX_SHA_256_SZ];
};

#endif
