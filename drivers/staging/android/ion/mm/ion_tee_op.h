/* Copyright (c) Hisilicon Technologies Co., Ltd. 2001-2019. All rights reserved.
 * FileName: ion_tee_op.h
 * Description: This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;
 * either version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef _ION_TEE_OP_H
#define _ION_TEE_OP_H

#include "teek_client_api.h"
#include "teek_client_constants.h"
#include "teek_client_id.h"

enum SEC_Task {
	SEC_TASK_DRM = 0x0,
	SEC_TASK_SEC,
	SEC_TASK_TINY,
	SEC_TASK_MAX,
};

#define SECBOOT_CMD_ID_MEM_ALLOCATE 0x1

#ifdef CONFIG_MM_ION_SECSG_DEBUG
#define sec_debug(fmt, ...) \
	pr_info(fmt, ##__VA_ARGS__)
#else
#define sec_debug(fmt, ...)
#endif

struct mem_chunk_list {
	u32 protect_id;
	union {
		u32 nents;
		u32 buff_id;
	};
	u32 va;
	void *phys_addr;  /* Must be the start addr of struct tz_pageinfo */
	u32 size;
	u32 cache;
	int prot;
	int mode;
	u32 smmuid;
	u32 sid;
	u32 ssid;
};

struct tz_pageinfo {
	u64 addr;
	u32 nr_pages;
} __aligned(8);

struct ion_sec_tee {
	TEEC_Context *context;
	TEEC_Session *session;
};

enum ion_sessions_type {
	ION_SESSIONS_SECMEM,
	ION_SESSIONS_VLTMM,
	ION_SESSIONS_TEST, /* not supported at this time */
	ION_SESSIONS_MAX
};

#define TEE_SECMEM_NAME "sec_mem"
#define TEE_VLTMM_NAME  "vltmm"

int secmem_tee_init(TEEC_Context *context, TEEC_Session *session,
				const char *package_name);

int secmem_tee_exec_cmd(TEEC_Session *session,
		       struct mem_chunk_list *mcl, u32 cmd);

void secmem_tee_destroy(TEEC_Context *context, TEEC_Session *session);

void seccg_pre_alloc_wk_func(struct work_struct *work);

int sec_tee_alloc(struct ion_sec_tee *tee);

void sec_tee_free(struct ion_sec_tee *tee);

int sec_tee_init(struct ion_sec_tee *tee, enum ion_sessions_type type);

#endif
