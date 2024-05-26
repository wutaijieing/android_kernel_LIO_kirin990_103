/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: for hwts register config
 */
#ifndef __NPU_BBIT_HWTS_CONFIG_H
#define __NPU_BBIT_HWTS_CONFIG_H
#include "npu_common.h"
#include "npu_proc_ctx.h"
#include "npu_bbit_debugfs.h"

#define ISPNN_HWTS_SQ_COUNT 2
#define AICORE_COUNT 2
#define HWTS_LOG_SIZE 64
#ifndef read_uint32
#define read_uint32(uw_value, addr) \
	((uw_value) = *((volatile u32 *)((uintptr_t)(addr))))
#endif
#ifndef read_uint64
#define read_uint64(ull_value, addr) \
	((ull_value) = *((volatile u64 *)((uintptr_t)(addr))))
#endif
#ifndef write_uint32
#define write_uint32(uw_value, addr) \
	(*((volatile u32 *)((uintptr_t)(addr))) = (uw_value))
#endif
#ifndef write_uint64
#define write_uint64(ull_value, addr) \
	(*((volatile u64 *)((uintptr_t)(addr))) = (ull_value))
#endif

typedef int (*hwts_func)(struct user_msg msg);

typedef struct hwts_para {
	u64 sq_base_addr;
	u8 sq_idx;
	u16 sq_tail;
	u32 time_out;
} hwts_para_t;

typedef struct syscache_para {
	int fd;
	u32 pid;
	u64 offset;
	u64 size;
} syscache_para_t;

typedef struct hwts_task_log {
	volatile u8 type : 3;
	volatile u8 res0 : 1;
	volatile u8 cnt : 4;
	volatile u8 res0_coreid;
	volatile u16 random;
	volatile u16 res0_blkid;
	volatile u16 taskid;
	volatile u32 syscnt_lo;
	volatile u32 syscnt_hi;
	volatile u32 streamid;
	volatile u32 res1;
	volatile u32 cyc_lo;
	volatile u32 cyc_hi;
	volatile u32 pmu0;
	volatile u32 pmu1;
	volatile u32 pmu2;
	volatile u32 pmu3;
	volatile u32 pmu4;
	volatile u32 pmu5;
	volatile u32 pmu6;
	volatile u32 pmu7;
} hwts_task_log_t;

typedef struct prof_log_buff {
	u64 size;
	vir_addr_t virt_addr;
	unsigned long iova_addr;
} prof_log_buff_t;

typedef struct pmu_config {
	u8 events[8];
	u8 aic_on[2];
} pmu_config_t;

enum HWTS_INTERFACE {
	CONFIG_HWTS_SQ,
	UPDATE_HWTS_SQ,
	UPDATE_SQ_TAIL,
	CHECK_HWTS_SQ,
	ATTACH_ION_SC,
	DETACH_ION_SC,
	ENABLE_PROF,
	DISABLE_PROF,
	GET_PROF_LOG,
	PRINT_PROF_LOG,
	HWTS_MAX
};

int npu_bbit_enable_prof(struct user_msg msg);
int npu_bbit_disable_prof(struct user_msg msg);
int npu_bbit_get_prof_log(struct user_msg msg);
int npu_bbit_print_prof_log(struct user_msg msg);

ssize_t npu_bbit_hwts_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *ppos);
ssize_t npu_bbit_hwts_read(struct file *filp, char __user *buf,
	size_t count, loff_t *ppos);

#endif
