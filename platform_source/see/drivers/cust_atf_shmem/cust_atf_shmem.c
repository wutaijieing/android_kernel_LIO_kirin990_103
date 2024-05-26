/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: shmem framework bl31 and kernel.
 * Create : 2022/03/18
 */

#define pr_fmt(fmt) "atf_shmem: " fmt

#include <linux/atomic.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/sizes.h>
#include <linux/printk.h>
#include <linux/arm-smccc.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <securec.h>
#include <atf_shmem.h>
#include <platform_include/see/bl31_smc.h>

#define FID_ATF_FRAMEWORK_SHMEMINIT     make_fid(FID_ATF_FRAMEWORK_GROUP, 0x00)

#define SHMEM_INVALID  0
#define SHMEM_VALID    0x5a
#define MAX_BUFF_NUM   64

struct _atf_shmem_struct {
	u64 phys_addr;
	u64 virt_addr;
	u64 size;
	u64 buff_num;
	u64 buff_status[MAX_BUFF_NUM];
	u64 per_size;
	u64 inited; /* critical zone */
	spinlock_t init_lock;
	spinlock_t buff_lock;
};

static struct _atf_shmem_struct shmem_struct = {
	.inited = SHMEM_INVALID,
	.buff_status = { 0 },
	.init_lock = __SPIN_LOCK_UNLOCKED(shmem_struct.init_lock),
	.buff_lock = __SPIN_LOCK_UNLOCKED(shmem_struct.buff_lock),
};

static void atf_shmem_init(void)
{
	unsigned long flags;
	struct arm_smccc_res res;

	spin_lock_irqsave(&shmem_struct.init_lock, flags); /* lock inited val */
	if (shmem_struct.inited == SHMEM_VALID) {
		spin_unlock_irqrestore(&shmem_struct.init_lock, flags);
		return;
	}

	/* get the actual addr from secure side */
	arm_smccc_smc(FID_ATF_FRAMEWORK_SHMEMINIT, 0, 0, 0, 0, 0, 0, 0, &res);

	shmem_struct.phys_addr = res.a0;
	shmem_struct.size = res.a1;
	shmem_struct.buff_num = res.a2;
	if (shmem_struct.buff_num == 0) {
		pr_err("there is no buffer for shmem\n");
		spin_unlock_irqrestore(&shmem_struct.init_lock, flags);
		return;
	}

	shmem_struct.per_size = shmem_struct.size / shmem_struct.buff_num;
	shmem_struct.virt_addr = (u64)(uintptr_t)ioremap_wc(shmem_struct.phys_addr, shmem_struct.size);

	shmem_struct.inited = SHMEM_VALID; /* this must be the last */
	spin_unlock_irqrestore(&shmem_struct.init_lock, flags);

	return;
}

static int get_one_buffer(void)
{
	unsigned long flags;
	int loop = 0;
	/* wait 2 seconds */
	unsigned long jiffies_expire = jiffies + 2 * HZ;

	if (shmem_struct.inited != SHMEM_VALID)
		atf_shmem_init();

	spin_lock_irqsave(&shmem_struct.buff_lock, flags); /* lock buff status */

	while(loop < (int)shmem_struct.buff_num) {
		if (!shmem_struct.buff_status[loop]) { /* find one */
			shmem_struct.buff_status[loop] = 1;
			break;
		}
		loop++;
		if (loop == (int)shmem_struct.buff_num)
			loop = 0;

		if (time_after(jiffies, jiffies_expire)) {
			spin_unlock_irqrestore(&shmem_struct.buff_lock, flags);
			return -EFAULT;
		}
	}

	spin_unlock_irqrestore(&shmem_struct.buff_lock, flags);
	return loop;
}

static void free_one_buffer(int index)
{
	if (index < 0 || index >= (int)shmem_struct.buff_num)
		return;

	/* no need lock here in free step */
	shmem_struct.buff_status[index] = 0;
}

u64 get_atf_shmem_size(void)
{
	if (shmem_struct.inited != SHMEM_VALID)
		atf_shmem_init();

	return shmem_struct.per_size;
}

int smccc_with_shmem(u64 smc_id, u64 shmem_type, u64 addr, u64 size,
		     u64 arg1, u64 arg2, u64 arg3, u64 arg4, struct arm_smccc_res *res)
{
	int index;
	int ret = 0;
	u64 buff_addr;

	if (shmem_struct.inited != SHMEM_VALID)
		atf_shmem_init();

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpointer-integer-compare"
	if (size > shmem_struct.per_size || res == NULL || addr == NULL) {
		pr_err("buffer over size or res or addr valid: %llx\n", size);
		return -EPERM;
	}
	#pragma GCC diagnostic pop

	if (unlikely(in_interrupt())) {
		pr_err("do not support share memory in irq\n");
		return -EPERM;
	}

	index = get_one_buffer();
	if (index < 0 || index >= (int)shmem_struct.buff_num) {
		pr_err("buffer index error: %d\n", index);
		/* error index no need free */
		return -EPERM;
	}

	buff_addr = shmem_struct.virt_addr + shmem_struct.per_size * (u64)index;
	if (buff_addr < shmem_struct.virt_addr || shmem_struct.per_size * (u64)index < (u64)index) {
		pr_err("buff_addr overflow\n");
		ret = -EPERM;
		goto free_buffer;
	}

	if (shmem_type == SHMEM_IN || shmem_type == SHMEM_INOUT) {
		ret = memcpy_s((void *)(uintptr_t)buff_addr, shmem_struct.per_size, (void *)(uintptr_t)addr, size);
		if (ret != EOK) {
			pr_err("memcpy_s failed in SHMEM_IN or SHMEM_INOUT\n");
			ret = -EPERM;
			goto free_buffer;
		}
	}

	/* the actual result is return from res */
	arm_smccc_smc(smc_id, shmem_type, index, size, arg1, arg2, arg3, arg4, res);

	if (shmem_type == SHMEM_OUT || shmem_type == SHMEM_INOUT) {
		/* the size is always okay here */
		ret = memcpy_s((void *)(uintptr_t)addr, size, (void *)(uintptr_t)buff_addr, size);
		if (ret != EOK) {
			pr_err("memcpy_s failed in SHMEM_OUT and SHMEM_INOUT\n");
			ret = -EPERM;
			goto free_buffer;
		}
	}

free_buffer:
	free_one_buffer(index);
	return ret;
}
