/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: interface header for kernel atf share memory
 * Create : 2022/03/06
 */

#ifndef __ATF_SHMEM__
#define __ATF_SHMEM__

#include <linux/printk.h>
#include <linux/errno.h>

#define SHMEM_IN          0xbeaf1001UL
#define SHMEM_OUT         0xbeaf1002UL
#define SHMEM_INOUT       0xbeaf1003UL

#ifdef CONFIG_CUST_ATF_SHMEM
/* get shmem size */
u64 get_atf_shmem_size(void);

/* shmem between bl31 and kernel
 *
 * type is shmem_xxx
 * addr is your memory to size
 * size is the memory size
 * arg1 ~4 is normal paramo
 * this interface is not suppose to use inside interrupt
 */
int smccc_with_shmem(u64 smc_id, u64 shmem_type, u64 addr, u64 size,
		     u64 arg1, u64 arg2, u64 arg3, u64 arg4, struct arm_smccc_res *res);
#else
static inline u64 get_atf_shmem_size(void)
{
	pr_err("not support CONFIG_CUST_ATF_SHMEM\n");
	return 0;
}

int inline smccc_with_shmem(u64 smc_id, u64 shmem_type, u64 addr, u64 size,
		     u64 arg1, u64 arg2, u64 arg3, u64 arg4, struct arm_smccc_res *res)
{
	pr_err("not support CONFIG_CUST_ATF_SHMEM\n");
	return -EPERM;
}
#endif

#endif /* __ATF_SHMEM__ */
