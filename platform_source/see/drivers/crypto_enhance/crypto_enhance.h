/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: Call interface for TA
 * Create: 2018-12-01
 */

#ifndef CRYPTO_ENHANCE_H
#define CRYPTO_ENHANCE_H

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <securec.h>

#define HIEPS_OK                        0
#define HIEPS_ERROR                     (-1000)
#define HIEPS_KCREATE_ERROR             (-1001)
#define HIEPS_CMA_MALLOC_ERROR          (-1002)
#define HIEPS_GET_SEM_ERROR             (-1003)
#define HIEPS_GET_MUTEX_ERROR           (-1004)
#define HIEPS_REGULATOR_MALLOC_ERROR    (-1005)
#define HIEPS_MEDIA2_GET_ERROR          (-1006)
#define HIEPS_MEDIA2_ENABLE_ERROR       (-1007)
#define HIEPS_MEDIA2_DISENABLE_ERROR    (-1008)
#define HIEPS_HIFACE_GET_ERROR          (-1009)
#define HIEPS_HIFACE_ENABLE_ERROR       (-1010)
#define HIEPS_HIFACE_DISENABLE_ERROR    (-1011)
#define HIEPS_TIMEOUT_ERROR             (-1012)
#define HIEPS_CMD_ERROR                 (-1013)
#define HIEPS_RETURN_DRR_FLAG_IS_NULL   (-1014)
#define HIEPS_RETURN_DRR_FLAG_IS_GETTED (-1015)
#define HIEPS_RETURN_ADDR_TO_ATF        (-1016)
#define HIEPS_PARAM_ERROR               (-1017)
#define HIEPS_IOCTL_NODE_CREATE_ERROR   (-1018)
#define HIEPS_IOCTL_CMD_UNSUPPORT       (-1019)
#define MSPE_OK                         0
#define MSPE_ERROR                      (-1000)
#define MSPE_AT_OBJ_NOT_FOUND_ERROR     (-1020)
#define MSPE_AT_OPS_NOT_FOUND_ERROR     (-1021)
#define MSPE_AT_CMD_FUNC_NULL_ERROR     (-1022)
#define MSPE_AT_PARAM_INVALID_ERROR     (-1023)
#define MSPE_AT_MEMCPY_ERROR            (-1024)
#define MSPE_AT_CTRL_NODE_CREATE_ERROR  (-1025)

#define HIEPS_ALLOC_ERR                 (-2001)

/* ATF service id */
#define HIEPS_SMC_FID                   0x850000a0
#define SIZE_CMA                        (sizeof(uint64_t)*4)
#define HIEPS_DEVICE_NAME               "hieps"
#define HIEPS_REGULATOR                 "ip-hiface"

#define MAX_SEM_WAITTIME                1000 /* 1000ms */
#define HIEPS_ATF_KERNEL_TEST_SUCCESS   0xC96B3F74

#define VOTE_TIMEOUT                    6000 /* 6000 times */
#define VOTE_SET_DELAY                  300 /* delay 300us */
/* atf<-->kernel cma struct */
struct hieps_cma_info {
	uint64_t x1;
	uint64_t x2;
	uint64_t x3;
	uint64_t x4;
};

struct hieps_module_data {
	struct device *dev;                /* This device. */
	struct device *cma_device;         /* cma memory allocator device */
	void *hieps_buff_virt;             /* Virtual address */
	phys_addr_t hieps_buff_phy;        /* Physical address */
	struct semaphore atf_sem;          /* do sync between kernel and atf */
	struct mutex hisee_mutex;          /* mutex for global resources */
	struct regulator *hieps_regulator; /* To power on or off hieps. */
	struct regulator *smmu_tcu_supply; /* To power on or off smmu. */
	struct clk *hieps_clk;             /* To do dvfs buck0 voltage. */
	struct clk *hieps_clk_source;      /* Used to opend hieps clock(ppll2) */
};

typedef enum {
	HIEPS_POWER_ON_CMD        = 0x1001,
	HIEPS_POWER_OFF_CMD       = 0x1002,
	HIEPS_DVFS_CMD            = 0x1003,
	HIEPS_KDR_SET_CMD         = 0x1004, /* Donot use kernel. */
	HIEPS_GET_LCS_CMD         = 0x1005, /* Donot use kernel. */
	HIEPS_SYNC_CMA_ALLOC_CMD  = 0x1006,
	HIEPS_GET_SMX_CMD         = 0x1007,
	HIEPS_SET_SMX_CMD         = 0x1008,
	/* test cmd, need add macro. */
#ifdef CONFIG_DFX_DEBUG_FS
	HIEPS_LOOP_TEST_CMD       = 0x2001, /* Test the whole process. */
	HIEPS_ATF_TEEOS_TEST_CMD  = 0x2002, /* Test cmd Donot use in kernel. */
	HIEPS_ATF_KERNEL_TEST_CMD = 0x2003, /* Test kernel <-> atf. */
#endif
	HIEPS_CMD_END,
} hieps_smc_cmd_type;

struct hieps_ioctl_func {
	char *func_name;
	int (*func_ptr)(const char *buf);
};

extern struct hieps_module_data g_hieps_data;

noinline int hieps_smc_send(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2);

#endif /* CRYPTO_ENHANCE_H */
