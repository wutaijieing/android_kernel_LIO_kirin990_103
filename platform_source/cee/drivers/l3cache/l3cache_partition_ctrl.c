/*
 * l3cache_partition_ctrl.c
 *
 * controls a group of ways to be marked as private to a scheme ID
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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
#include <linux/module.h>
#include <asm/sysreg.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/sched_perf_ctrl.h>

#define CLUSTERPARTCR_EL1        sys_reg(3, 0, 15, 4, 3)

#define MAX_PARTITION_NUMS 0x4
#define ALL_PARTITION_MASK ((0x1U << MAX_PARTITION_NUMS) - 1)
#define MAX_CLUSTER_NUMS 0x3
#define val_mask(val) ((1U << (val)) - 1)
#define cluster_shift(i) ((i) * MAX_PARTITION_NUMS)


void set_partition_control(unsigned int val)
{
	write_sysreg_s(val, CLUSTERPARTCR_EL1);
	isb();
}

void set_task_partition_control(unsigned int val, unsigned int cluster)
{
	unsigned int i;
	unsigned int cfg = 0;

	if (val >= MAX_PARTITION_NUMS || val == 0)
		return;

	for (i = 0; i < MAX_CLUSTER_NUMS; i++) {
		if (i == cluster)
			cfg |= (val_mask(val) << cluster_shift(i));
		else
			cfg |= ((ALL_PARTITION_MASK & ~val_mask(val)) << cluster_shift(i));
	}
	set_partition_control(cfg);
}

int perf_ctrl_set_task_l3c_part(void __user *uarg)
{
	struct task_config cfg;
	struct task_struct *p = NULL;

	if (uarg == NULL)
		return -EINVAL;

	if (copy_from_user(&cfg, uarg, sizeof(struct task_config)))
		return -EFAULT;

	if (cfg.value >= MAX_PARTITION_NUMS)
		return -EFAULT;

	rcu_read_lock();
	p = find_task_by_vpid(cfg.pid);
	if (p == NULL) {
		rcu_read_unlock();
		return -ESRCH;
	}

	p->l3c_part = cfg.value;
	rcu_read_unlock();

	if (cfg.value == 0)
		set_partition_control(0x0);

	return 0;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("l3cache partition control driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
