/*
 * freqdump_node_dump.c
 *
 * freqdump support dump data from kernel node
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <freqdump/freqdump_kernel.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/kernel.h>
#include <securec.h>
#include <platform_include/see/bl31_smc.h>

#ifdef CONFIG_FREQDUMP_NODE_DUMP
#define DUMP_NODE_AUTHORITY	0660

static struct dentry *g_node_dump_debugfs_root;

static int freq_node_dump_cluster0(struct seq_file *s, void *p)
{
	struct cpu_info cpu;

	(void)memset_s(&cpu, sizeof(cpu), 0, sizeof(cpu));
	down(&g_freqdump_sem);
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD, (u64)0, freqdump_phy_addr, (u64)NODE_DUMP_CLUSTER0);
	memcpy_fromio(&cpu, (void *)g_freqdump_virt_addr, sizeof(struct cpu_info));
	seq_printf(s, "freq:%u\n", cpu.freq_khz[CPU_LIT_CLUSTER]);
	seq_printf(s, "mem volt:%u\n", cpu.mem_voltage[CPU_LIT_CLUSTER]);
	seq_printf(s, "logic volt:%u\n", cpu.voltage[CPU_LIT_CLUSTER]);
	up(&g_freqdump_sem);
	return OK;
}

#ifdef CONFIG_SUPPORT_CLUSTER1
static int freq_node_dump_cluster1(struct seq_file *s, void *p)
{
	struct cpu_info cpu;

	(void)memset_s(&cpu, sizeof(cpu), 0, sizeof(cpu));
	down(&g_freqdump_sem);
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD, (u64)0, freqdump_phy_addr, (u64)NODE_DUMP_CLUSTER1);
	memcpy_fromio(&cpu, (void *)g_freqdump_virt_addr, sizeof(struct cpu_info));
#ifdef CONFIG_SUPPORT_CLUSTER2
	seq_printf(s, "freq:%u\n", cpu.freq_khz[CPU_MID_CLUSTER]);
	seq_printf(s, "mem volt:%u\n", cpu.mem_voltage[CPU_MID_CLUSTER]);
	seq_printf(s, "logic volt:%u\n", cpu.voltage[CPU_MID_CLUSTER]);
#else
	seq_printf(s, "freq:%u\n", cpu.freq_khz[CPU_BIG_CLUSTER]);
	seq_printf(s, "mem volt:%u\n", cpu.mem_voltage[CPU_BIG_CLUSTER]);
	seq_printf(s, "logic volt:%u\n", cpu.voltage[CPU_BIG_CLUSTER]);
#endif
	up(&g_freqdump_sem);
	return OK;
}
#endif

#ifdef CONFIG_SUPPORT_CLUSTER2
static int freq_node_dump_cluster2(struct seq_file *s, void *p)
{
	struct cpu_info cpu;

	(void)memset_s(&cpu, sizeof(cpu), 0, sizeof(cpu));
	down(&g_freqdump_sem);
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD, (u64)0, freqdump_phy_addr, (u64)NODE_DUMP_CLUSTER2);
	memcpy_fromio(&cpu, (void *)g_freqdump_virt_addr, sizeof(struct cpu_info));
	seq_printf(s, "freq:%u\n", cpu.freq_khz[CPU_BIG_CLUSTER]);
	seq_printf(s, "mem volt:%u\n", cpu.mem_voltage[CPU_BIG_CLUSTER]);
	seq_printf(s, "logic volt:%u\n", cpu.voltage[CPU_BIG_CLUSTER]);
	up(&g_freqdump_sem);
	return OK;
}
#endif

static int freq_node_dump_gpu(struct seq_file *s, void *p)
{
	struct gpu_info gpu;

	(void)memset_s(&gpu, sizeof(gpu), 0, sizeof(gpu));
	down(&g_freqdump_sem);
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD, (u64)0, freqdump_phy_addr, (u64)NODE_DUMP_GPU);
	memcpy_fromio(&gpu, (void *)g_freqdump_virt_addr, sizeof(struct gpu_info));
	seq_printf(s, "freq:%u\n", gpu.freq);
	seq_printf(s, "mem volt:%u\n", gpu.mem_voltage);
	seq_printf(s, "logic volt:%u\n", gpu.voltage);
	up(&g_freqdump_sem);
	return OK;
}

#ifndef CONFIG_NOT_SUPPORT_L3
static int freq_node_dump_l3(struct seq_file *s, void *p)
{
	struct l3_info l3;

	(void)memset_s(&l3, sizeof(l3), 0, sizeof(l3));
	down(&g_freqdump_sem);
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD, (u64)0, freqdump_phy_addr, (u64)NODE_DUMP_L3);
	memcpy_fromio(&l3, (void *)g_freqdump_virt_addr, sizeof(struct l3_info));
	seq_printf(s, "freq:%u\n", l3.freq_khz);
	seq_printf(s, "mem volt:%u\n", l3.mem_voltage);
	seq_printf(s, "logic volt:%u\n", l3.voltage);
	up(&g_freqdump_sem);
	return OK;
}
#endif

static int freq_node_dump_ddr(struct seq_file *s, void *p)
{
	struct ddr_info ddr;

	(void)memset_s(&ddr, sizeof(ddr), 0, sizeof(ddr));
	down(&g_freqdump_sem);
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD, (u64)0, freqdump_phy_addr, (u64)NODE_DUMP_DDR);
	memcpy_fromio(&ddr, (void *)g_freqdump_virt_addr, sizeof(struct ddr_info));
	seq_printf(s, "ddr freq:%u\n", ddr.ddr_freq_khz);
	seq_printf(s, "mem volt:%u\n", ddr.mem_voltage);
	seq_printf(s, "logic volt:%u\n", ddr.voltage);
	up(&g_freqdump_sem);
	return OK;
}

static int freq_node_dump_peri(struct seq_file *s, void *p)
{
	struct peri_info peri;

	(void)memset_s(&peri, sizeof(peri), 0, sizeof(peri));
	down(&g_freqdump_sem);
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD, (u64)0, freqdump_phy_addr, (u64)NODE_DUMP_PERI);
	memcpy_fromio(&peri, (void *)g_freqdump_virt_addr, sizeof(struct peri_info));
	seq_printf(s, "freq:%u\n", peri.freq);
	seq_printf(s, "mem volt:%u\n", peri.mem_voltage);
	seq_printf(s, "logic volt:%u\n", peri.voltage);
	up(&g_freqdump_sem);
	return OK;
}

#ifdef MAX_NPU_MODULE_SHOW
static int freq_node_dump_npu(struct seq_file *s, void *p)
{
	struct npu_info npu;
	unsigned int i;

	(void)memset_s(&npu, sizeof(npu), 0, sizeof(npu));
	down(&g_freqdump_sem);
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD, (u64)0, freqdump_phy_addr, (u64)NODE_DUMP_NPU);
	memcpy_fromio(&npu, (void *)g_freqdump_virt_addr, sizeof(struct npu_info));
	for (i = 0; i < MAX_NPU_MODULE_SHOW; i++) {
		seq_printf(s, "%s:\n", npu.name[i]);
		/* 0:freq 1:logic volt 2:mem volt */
		seq_printf(s, "\t freq:%u\n", npu.freq_volt[i][0] / 1000);
		seq_printf(s, "\t logic volt:%u\n", npu.freq_volt[0][1]);
#ifdef NPU_SUPPORT_MEM_VOLT
		seq_printf(s, "\t mem volt:%u\n", npu.freq_volt[0][2]);
#endif
	}
	up(&g_freqdump_sem);
	return OK;
}
#endif

static int cluster0_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, freq_node_dump_cluster0, NULL);
}

#ifdef CONFIG_SUPPORT_CLUSTER1
static int cluster1_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, freq_node_dump_cluster1, NULL);
}
#endif

#ifdef CONFIG_SUPPORT_CLUSTER2
static int cluster2_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, freq_node_dump_cluster2, NULL);
}
#endif

static int gpu_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, freq_node_dump_gpu, NULL);
}

#ifndef CONFIG_NOT_SUPPORT_L3
static int l3_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, freq_node_dump_l3, NULL);
}
#endif

static int ddr_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, freq_node_dump_ddr, NULL);
}

static int peri_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, freq_node_dump_peri, NULL);
}

#ifdef MAX_NPU_MODULE_SHOW
static int npu_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, freq_node_dump_npu, NULL);
}
#endif

static const struct file_operations g_cluster0_fops = {
	.owner = THIS_MODULE,
	.open = cluster0_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

#ifdef CONFIG_SUPPORT_CLUSTER1
static const struct file_operations g_cluster1_fops = {
	.owner = THIS_MODULE,
	.open = cluster1_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};
#endif

#ifdef CONFIG_SUPPORT_CLUSTER2
static const struct file_operations g_cluster2_fops = {
	.owner = THIS_MODULE,
	.open = cluster2_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};
#endif

static const struct file_operations g_gpu_fops = {
	.owner = THIS_MODULE,
	.open = gpu_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

#ifndef CONFIG_NOT_SUPPORT_L3
static const struct file_operations g_l3_fops = {
	.owner = THIS_MODULE,
	.open = l3_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};
#endif

static const struct file_operations g_ddr_fops = {
	.owner = THIS_MODULE,
	.open = ddr_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations g_peri_fops = {
	.owner = THIS_MODULE,
	.open = peri_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

#ifdef MAX_NPU_MODULE_SHOW
static const struct file_operations g_npu_fops = {
	.owner = THIS_MODULE,
	.open = npu_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};
#endif

static struct file_str_ops g_node_dump_ops[] = {
	{
		.str = "cluster0",
		.file_ops = &g_cluster0_fops,
	},
#ifdef CONFIG_SUPPORT_CLUSTER1
	{
		.str = "cluster1",
		.file_ops = &g_cluster1_fops,
	},
#endif
#ifdef CONFIG_SUPPORT_CLUSTER2
	{
		.str = "cluster2",
		.file_ops = &g_cluster2_fops,
	},
#endif
	{
		.str = "gpu",
		.file_ops = &g_gpu_fops,
	},
#ifndef CONFIG_NOT_SUPPORT_L3
	{
		.str = "l3",
		.file_ops = &g_l3_fops,
	},
#endif
	{
		.str = "ddr",
		.file_ops = &g_ddr_fops,
	},
	{
		.str = "peri",
		.file_ops = &g_peri_fops,
	},
#ifdef MAX_NPU_MODULE_SHOW
	{
		.str = "npu",
		.file_ops = &g_npu_fops,
	},
#endif
};

static int create_node_dump_node(void)
{
	struct dentry *file = NULL;
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(g_node_dump_ops); i++) {
		file = debugfs_create_file(g_node_dump_ops[i].str, DUMP_NODE_AUTHORITY, g_node_dump_debugfs_root,
					   NULL, g_node_dump_ops[i].file_ops);
		if (file == NULL) {
			pr_err("%s %d create file %s fail\n", __func__, __LINE__, g_node_dump_ops[i].str);
			return -ENOMEM;
		}
	}
	return OK;
}
#endif

int freqdump_node_dump_init(void)
{
#ifdef CONFIG_FREQDUMP_NODE_DUMP
	int ret;

	g_node_dump_debugfs_root = debugfs_create_dir("node_dump", g_freqdump_debugfs_root);
	if (g_node_dump_debugfs_root == NULL) {
		pr_err("%s %d create freqdump node_dump node fail\n", __func__, __LINE__);
		return -ENOENT;
	}

	ret = create_node_dump_node();
	if (ret != OK) {
		debugfs_remove_recursive(g_node_dump_debugfs_root);
		g_node_dump_debugfs_root = NULL;
		pr_err("%s %d freqdump node_dump sub node init fail\n", __func__, __LINE__);
		return -ENOENT;
	}
#endif
	return OK;
}
