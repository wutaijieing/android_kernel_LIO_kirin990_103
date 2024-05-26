/*
 * freqdump_kernel.c
 *
 * freqdump
 *
 * Copyright (C) 2018-2020 Huawei Technologies Co., Ltd.
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

#include "freqdump_kernel.h"
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
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#ifdef CONFIG_LIBLINUX
#include <global_ddr_map_cdc_ace.h>
#else
#include <global_ddr_map.h>
#endif
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <securec.h>
#include <platform_include/see/bl31_smc.h>

#ifdef CONFIG_FREQDUMP_PLATFORM
#include "loadmonitor.h"
#endif

#if defined(CONFIG_DFX_DEBUG_FS)
#if defined(CONFIG_ENABLE_MIDEA_MONITOR)
#include <media_monitor.h>
static int g_delay_time_media;
#endif
#endif

#undef  pr_fmt
#define pr_fmt(fmt)	"[freqdump]:" fmt
#define CHIP_TYPE_BUF_SIZE	10
void __iomem *g_freqdump_virt_addr;
static phys_addr_t g_freqdump_phy_addr;
static char g_loadmonitor_buf[LOADMONITOR_BUFF_SIZE];
#ifdef CONFIG_DFX_DEBUG_FS
static int g_loadmonitor_status;
#endif
static int g_monitor_en_flags = -1;
static char g_chip_type_buf[CHIP_TYPE_BUF_SIZE];
static unsigned int g_chip_type = 2; /* 0:es 1:cs 2:none */
static struct dentry *g_freqdump_debugfs_root;
static struct freqdump_data *g_freqdump_read_data;
static struct semaphore g_loadmonitor_sem, g_freqdump_sem;
#define MAX_MONITOR_IP_PERI	32
#define MAX_MONITOR_IP_MARS	40

#define ES_MAX_INDEX	(MAX_MONITOR_IP_MARS * MONITOR_STAT_NUM)
#define ES_MAX_PERI_INDEX	(MAX_MONITOR_IP_PERI * MONITOR_STAT_NUM)

#ifdef CONFIG_FREQDUMP_PLATFORM

#define MONITOR_AO_FREQ_CS	157290000
#define MONITOR_PERI_FREQ_CS	166000000
#define FILE_RIGHT	0660

#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"

#ifdef CONFIG_DFX_DEBUG_FS
static unsigned int g_monitor_load_bos_cs[CS_MAX_PERI_INDEX];
#ifndef CONFIG_LIBLINUX
static u64 g_monitor_load_bos_cs_ao[CS_MAX_AO_INDEX];
#endif
#endif
#endif

void __iomem *g_loadmonitor_virt_addr;

static int chip_type_check_err(void)
{
	if (g_chip_type != ES && g_chip_type != CS) {
		pr_err("%s g_chip_type is error!\n", __func__);
		return -1;
	}
	return 0;
}

noinline int atfd_service_freqdump_smc(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
#ifndef CONFIG_LIBLINUX
		"smc	#0\n"
#else
		"svc	#0x4200\n"
#endif
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}

void sec_freqdump_read(void)
{
	down(&g_freqdump_sem);
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD,
					     (u64)0, g_freqdump_phy_addr, (u64)0);
	up(&g_freqdump_sem);
}

#ifdef CONFIG_FREQDUMP_PLATFORM
int ao_loadmonitor_read(u64 *addr)
{
#ifndef CONFIG_LIBLINUX
	struct loadmonitor_sigs sigs;
	int index = 0;  /* sensor id, in AO, from 32 to 63 */
	unsigned long i;
	int ret;

	if (addr == NULL) {
		pr_err("%s: input error!\n", __func__);
		return -1;
	}

	ret = _ao_loadmonitor_read(&sigs, sizeof(sigs));
	if (ret != 0 && ret != -ENODEV) { /* -ENODEV:closed by DTS config */
		pr_err("%s: get signal cnt error! ret:%d\n", __func__, ret);
		return -1;
	}

	for (i = 0; i < 32; ++i) {
		if (AO_MONITOR0_IP_MASK & BIT(i)) {
			*(addr + index) = sigs.sig[i].count[DATA_INDEX_IDLE];
			*(addr + MAX_MONITOR_IP_DEVICE_AO + index) =
				sigs.sig[i].count[DATA_INDEX_BUSY_NORMAL];
			*(addr + 2 * MAX_MONITOR_IP_DEVICE_AO + index) =
				sigs.sig[i].count[DATA_INDEX_BUSY_LOW];
			*(addr + 3 * MAX_MONITOR_IP_DEVICE_AO + index) =
				sigs.sig[i].samples;
			index++;
		}
	}
	return 0;
#else
	(void)addr;

	return 0;
#endif
}

int peri_monitor_clk_init(const char *clk_name, unsigned int clk_freq)
{
	int ret;
	struct clk *monitor_clk = NULL;

	if (clk_name == NULL) {
		pr_err("%s input error!.\n", __func__);
		return -1;
	}

	monitor_clk = clk_get(NULL, clk_name);
	if (IS_ERR_OR_NULL(monitor_clk)) {
		pr_err("%s peri clk init error!.\n", __func__);
		return -1;
	}

	/* set frequency */
	ret = clk_set_rate(monitor_clk, clk_freq);
	if (ret < 0) {
		pr_err("%s peri clk set rate error!.\n", __func__);
		clk_put(monitor_clk);
		return -1;
	}
	/* enable clk_loadmonitor */
	ret = clk_prepare_enable(monitor_clk);
	if (ret != 0) {
		clk_put(monitor_clk);
		pr_err("%s peri clk enable error!.\n", __func__);
		return -1;
	}

	clk_put(monitor_clk);

	return 0;
}

int peri_monitor_clk_disable(const char *clk_name)
{
	struct clk *monitor_clk = NULL;

	if (clk_name == NULL) {
		pr_err("%s input error!.\n", __func__);
		return -1;
	}

	monitor_clk = clk_get(NULL, clk_name);
	if (IS_ERR_OR_NULL(monitor_clk)) {
		pr_err("%s peri clk disable error!.\n", __func__);
		return -1;
	}

	/* disable clk_loadmonitor */
	clk_disable_unprepare(monitor_clk);
	clk_put(monitor_clk);

	return 0;
}

static int all_peri_clk_init(void)
{
	int ret;

	ret = peri_monitor_clk_init("clk_loadmonitor", MONITOR_PERI_FREQ_CS);
	if (ret != 0) {
		pr_err("%s peri_monitor0_clk_init error.\n", __func__);
		return ret;
	}
	ret = peri_monitor_clk_init("clk_loadmonitor_l", MONITOR_PERI_FREQ_CS);
	if (ret != 0) {
		pr_err("%s peri_monitor1_clk_init error.\n", __func__);
		return ret;
	}
#ifndef CONFIG_PRODUCT_ARMPC
	ret = peri_monitor_clk_init("clk_loadmonitor_2", MONITOR_PERI_FREQ_CS);
	if (ret != 0) {
		pr_err("%s peri_monitor2_clk_init error.\n", __func__);
		return ret;
	}
#endif
	pr_err("%s success.\n", __func__);
	return 0;
}

void sec_loadmonitor_switch_enable(unsigned int value_peri,
					unsigned int value_ao,
					unsigned int en_flags)
{
	int ret;

	if (chip_type_check_err() != 0) {
		pr_err("%s g_chip_type is error!\n", __func__);
		return;
	}

	if (g_chip_type == ES) {
		(void)atfd_service_freqdump_smc(
			(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE,
			(u64)value_peri, (u64)value_ao, (u64)en_flags);
		pr_err("%s es freqdump enable success, en_flags:%u!\n",
		       __func__, en_flags);
	}

	if (g_chip_type == CS) {
		if (en_flags == PERI_MONITOR_EN) {
			ret = all_peri_clk_init();
			if (ret != 0)
				return;
			pr_err("%s cs monitor clk init success en_flags:%d.\n",
			       __func__, en_flags);
			(void)atfd_service_freqdump_smc(
				(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE,
				(u64)value_peri, (u64)value_ao, (u64)en_flags);
			pr_err("%s end loadmonitor enable.\n", __func__);
		} else if (en_flags == AO_MONITOR_EN) {
#ifndef CONFIG_LIBLINUX
			ao_loadmonitor_enable(value_ao, MONITOR_AO_FREQ_CS);
#endif
		} else {
			ret = all_peri_clk_init();
			if (ret != 0)
				return;
			pr_err("%s cs monitor clk init success en_flags:%d.\n",
			       __func__, en_flags);
			(void)atfd_service_freqdump_smc(
				(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE,
				(u64)value_peri, (u64)value_peri, (u64)en_flags);
#ifndef CONFIG_LIBLINUX
			ao_loadmonitor_enable(value_ao, MONITOR_AO_FREQ_CS);
#endif
		}
	}
	pr_err("%s success. en_flags:%u\n", __func__, en_flags);
}

int all_peri_clk_disable(void)
{
	int ret;

	ret = peri_monitor_clk_disable("clk_loadmonitor");
	if (ret != 0) {
		pr_err("%s peri_monitor0_clk_disable error.\n", __func__);
		return ret;
	}
	ret = peri_monitor_clk_disable("clk_loadmonitor_l");
	if (ret != 0) {
		pr_err("%s peri_monitor1_clk_disable error.\n", __func__);
		return ret;
	}
	ret = peri_monitor_clk_disable("clk_loadmonitor_2");
	if (ret != 0) {
		pr_err("%s peri_monitor2_clk_disable error.\n", __func__);
		return ret;
	}
	pr_err("%s success.\n", __func__);
	return 0;
}

void sec_loadmonitor_switch_disable(void)
{
	int ret;
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_LOADMONITOR_SVC_DISABLE,
					     (u64)0, (u64)0, (u64)0);

	if (g_chip_type == CS) {
		if (g_monitor_en_flags == PERI_MONITOR_EN ||
		    g_monitor_en_flags == ALL_MONITOR_EN) {
			ret = all_peri_clk_disable();
			if (ret != 0)
				return;
		}
#ifndef CONFIG_LIBLINUX
		ao_loadmonitor_disable();
#endif
	}
	pr_err("%s success.\n", __func__);
}

void sec_loadmonitor_data_read(unsigned int enable_flags)
{
	(void)atfd_service_freqdump_smc(
		(u64)FREQDUMP_LOADMONITOR_SVC_REG_RD,
		(u64)enable_flags, g_freqdump_phy_addr, (u64)0);
}

void sec_chip_type_read(void)
{
	int res;

	(void)atfd_service_freqdump_smc(
		(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE_READ,
		(u64)0, g_freqdump_phy_addr, (u64)0);
	g_chip_type = CS;
	res = snprintf_s(g_chip_type_buf, sizeof(g_chip_type_buf),
			 sizeof(g_chip_type_buf) - 1, "%u", g_chip_type);
	if (res == -1)
		pr_err("%s err\n", __func__);
}

#ifdef CONFIG_DFX_DEBUG_FS
void hisi_sec_adc_set_param(void)
{
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_ADC_SVC_SET_PARAM,
					     (u64)0, (u64)0, (u64)0);
}

void hisi_sec_adc_disable(void)
{
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_ADC_SVC_DISABLE,
					     (u64)0, (u64)0, (u64)0);
}
#endif
#endif

#ifdef CONFIG_DFX_DEBUG_FS
static int freqdump_node_dump(struct seq_file *s, void *p)
{
	(void)p;
#ifndef CONFIG_PRODUCT_CDC_ACE
	sec_freqdump_read();
#endif
	(void)memset_s(g_freqdump_read_data,  sizeof(struct freqdump_data),
		       0, sizeof(struct freqdump_data));
#ifndef CONFIG_PRODUCT_CDC_ACE
	memcpy_fromio(g_freqdump_read_data, (void *)g_freqdump_virt_addr,
		      sizeof(struct freqdump_data));
#endif

#ifdef CONFIG_NPU_PM_DEBUG
	/* Add the NPU freq info at the end of data struct */
	if (get_npu_freq_info(g_freqdump_read_data->npu,
			      sizeof(g_freqdump_read_data->npu),
			      g_freqdump_read_data->npu_name,
			      sizeof(g_freqdump_read_data->npu_name)) != 0)
		pr_err("%s Failed to get NPU freqdump info!\n", __func__);
#endif
	seq_write(s, (const struct freqdump_data *)g_freqdump_read_data,
		  sizeof(struct freqdump_data));

	return OK;
}

static int read_format_cs_loadmonitor_data(void)
{
	int pos = 0;
	int pos_tmp, i;
	char *monitor_addr = (char *)g_freqdump_virt_addr + SHARED_MEMORY_SIZE -
			     CS_MAX_PERI_INDEX * 4;

	if (g_monitor_en_flags == PERI_MONITOR_EN ||
	    g_monitor_en_flags == ALL_MONITOR_EN) {
#ifdef CONFIG_ENABLE_MIDEA_MONITOR
		media_monitor_read(g_delay_time_media);
#else
		sec_loadmonitor_data_read((unsigned int)g_monitor_en_flags);
#endif
		memcpy_fromio(g_monitor_load_bos_cs, (void *)(monitor_addr),
			      CS_MAX_PERI_INDEX * 4);
	}

#ifndef CONFIG_LIBLINUX
	if (g_monitor_en_flags == AO_MONITOR_EN ||
	    g_monitor_en_flags == ALL_MONITOR_EN) {
		if (ao_loadmonitor_read(g_monitor_load_bos_cs_ao) != 0) {
			pr_err("%s : ao_loadmonitor_read error!\n", __func__);
			return -EINVAL;
		}
	}
#endif
	for (i = 0; i < CS_MAX_PERI_INDEX; i++) {
		pos_tmp = snprintf_s(g_loadmonitor_buf + pos,
				     sizeof(g_loadmonitor_buf) - pos,
				     sizeof(g_loadmonitor_buf) - pos - 1, "%u|",
				     g_monitor_load_bos_cs[i]);
		if (pos_tmp == -1) {
			pr_err("%s peri err.buffer size:%lu,pos:%d,value:%u\n",
			       __func__, sizeof(g_loadmonitor_buf), pos,
			       g_monitor_load_bos_cs[i]);
			return -EINVAL;
		}
		pos += pos_tmp;
	}
#ifndef CONFIG_LIBLINUX
	for (i = 0; i < CS_MAX_AO_INDEX; i++) {
		pos_tmp = snprintf_s(g_loadmonitor_buf + pos,
				     sizeof(g_loadmonitor_buf) - pos,
				     sizeof(g_loadmonitor_buf) - pos - 1, "%llu|",
				     g_monitor_load_bos_cs_ao[i]);
		if (pos_tmp == -1) {
			pr_err("%s ao err.buffer size:%lu,pos:%d,value:%llu\n",
			       __func__, sizeof(g_loadmonitor_buf), pos,
			       g_monitor_load_bos_cs_ao[i]);
			return -EINVAL;
		}
		pos += pos_tmp;
	}
#endif
	return OK;
}

static int loadmonitor_node_dump(struct seq_file *s, void *p)
{
	(void)p;

	down(&g_loadmonitor_sem);

	if (g_loadmonitor_status == 0) {
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	if (g_monitor_en_flags != PERI_MONITOR_EN &&
	    g_monitor_en_flags != AO_MONITOR_EN &&
	    g_monitor_en_flags != ALL_MONITOR_EN) {
		pr_err("%s loadmonitor node dump error!\n", __func__);
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	if (chip_type_check_err() != 0) {
		pr_err("%s g_chip_type is error!\n", __func__);
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	if (g_chip_type == CS) {
		if (read_format_cs_loadmonitor_data() == -EINVAL) {
			up(&g_loadmonitor_sem);
			return -EINVAL;
		}
	}
	seq_printf(s, "%s\n", g_loadmonitor_buf);

	up(&g_loadmonitor_sem);
	return OK;
}

/* set adc params, only NO 1 is effective */
static ssize_t adc_set_param_write(struct file *filp, const char __user *ubuf,
				   size_t cnt, loff_t *ppos)
{
	char buf[20] = {0};

	(void)filp;
	(void)ppos;

	down(&g_loadmonitor_sem);

	if (ubuf == NULL) {
		pr_err("ubuf is NULL\n");
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}
	if (copy_from_user(buf, ubuf,
			   min_t(size_t, sizeof(buf) - 1, cnt)) != 0) {
		pr_err("%s can not copy from user\n", __func__);
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	if (buf[0] == '1')
		hisi_sec_adc_set_param();
	else
		pr_err("%s only 1 can set\n", __func__);
	up(&g_loadmonitor_sem);
	return cnt;
}

/* set adc disable, only NO 1 is effective */
static ssize_t adc_disable(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char buf[20] = {0};

	(void)ppos;
	(void)filp;

	down(&g_loadmonitor_sem);

	if (ubuf == NULL) {
		pr_err("ubuf is NULL\n");
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}
	if (copy_from_user(buf, ubuf,
			   min_t(size_t, sizeof(buf) - 1, cnt)) != 0) {
		pr_err("%s can not copy from user\n", __func__);
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	if (buf[0] == '1')
		hisi_sec_adc_disable();
	else
		pr_err("%s only 1 can set\n", __func__);
	up(&g_loadmonitor_sem);
	return cnt;
}

static ssize_t loadmonitor_enable_write(struct file *filp,
					const char __user *ubuf,
					size_t cnt, loff_t *ppos)
{
	char buf[45] = {0};
	int ret;
	unsigned int delay_time_peri, delay_time_ao;
	int enable_flags;

	(void)ppos;
	(void)filp;

	pr_err("%s down!\n", __func__);
	down(&g_loadmonitor_sem);
	if (ubuf == NULL) {
		pr_err("ubuf is NULL\n");
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}
	if (copy_from_user(buf, ubuf,
			   min_t(size_t, sizeof(buf) - 1, cnt)) != 0) {
		pr_err("loadmonitor_switch_write can not copy from user\n");
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}
	ret = sscanf_s(buf, "%u,%u,%d", &delay_time_peri, &delay_time_ao,
		       &enable_flags);
	if (ret < 0 || (delay_time_peri == 0 && delay_time_ao == 0)) {
		pr_err("%s interpreting data error or the delay time should not be 0!\n",
		       __func__);
		up(&g_loadmonitor_sem);
		return (ssize_t)cnt;
	}

	pr_err("%s g_delay_time_media:%d\n",
	       __func__, MONITOR_PERI_FREQ_CS * g_media_info[INFO_MONITOR_FREQ]);
#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	g_delay_time_media = (int)(delay_time_peri / MONITOR_PERI_FREQ_CS * g_media_info[INFO_MONITOR_FREQ]);
#endif

	if (enable_flags != PERI_MONITOR_EN &&
	    enable_flags != AO_MONITOR_EN &&
	    enable_flags != ALL_MONITOR_EN) {
		pr_err("%s loadmonitor can not be enable!\n", __func__);
		up(&g_loadmonitor_sem);
		return (ssize_t)cnt;
	}
	g_monitor_en_flags = enable_flags;
	pr_err("%s enable:delay_time_peri:%d, delay_time_ao:%d, enable_flags:%d!\n",
	       __func__, delay_time_peri, delay_time_ao, enable_flags);
	sec_loadmonitor_switch_enable(delay_time_peri, delay_time_ao,
					   (unsigned int)enable_flags);
	g_loadmonitor_status = 1;
	up(&g_loadmonitor_sem);
	return (ssize_t)cnt;
}

static int chip_type_node_dump(struct seq_file *s, void *p)
{
	(void)p;

	sec_chip_type_read();
	seq_printf(s, "%s\n", g_chip_type_buf);

	return OK;
}

static int chip_type_node_open(struct inode *inode, struct file *file)
{
	(void)inode;

	return single_open(file, chip_type_node_dump, NULL);
}

static ssize_t loadmonitor_disable_write(struct file *filp, const char __user *ubuf,
					 size_t cnt, loff_t *ppos)
{
	char buf[20] = {0};

	(void)ppos;
	(void)filp;

	down(&g_loadmonitor_sem);

	if (g_loadmonitor_status == 0) {
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}
	if (ubuf == NULL) {
		pr_err("ubuf is NULL\n");
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}
	if (copy_from_user(buf, ubuf,
			   min_t(size_t, sizeof(buf) - 1, cnt)) != 0) {
		pr_err("loadmonitor_switch_write can not copy from user\n");
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	if (buf[0] == '1')
		sec_loadmonitor_switch_disable();
	g_loadmonitor_status = 0;
	g_monitor_en_flags = -1;
	up(&g_loadmonitor_sem);
	return cnt;
}

static int loadmonitor_debugfs_node_open(struct inode *inode, struct file *file)
{
	(void)inode;

	return single_open(file, loadmonitor_node_dump, NULL);
}

static int freqdump_debugfs_node_open(struct inode *inode, struct file *file)
{
	(void)inode;

	return single_open(file, freqdump_node_dump, NULL);
}

static const struct file_operations freqdump_fops = {
	.owner = THIS_MODULE,
	.open = freqdump_debugfs_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations loadmonitor_dump_fops = {
	.owner = THIS_MODULE,
	.open = loadmonitor_debugfs_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

#ifdef CONFIG_FREQDUMP_PLATFORM
static const struct file_operations adc_set_param_fops = {
	.owner = THIS_MODULE,
	.write = adc_set_param_write,
};

static const struct file_operations adc_disable_fops = {
	.owner = THIS_MODULE,
	.write = adc_disable,
};

static const struct file_operations loadmonitor_enable_fops = {
	.owner = THIS_MODULE,
	.write = loadmonitor_enable_write,
	.open = chip_type_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};
#else
static const struct file_operations loadmonitor_enable_fops = {
	.owner = THIS_MODULE,
	.write = loadmonitor_enable_write,
};
#endif
static const struct file_operations loadmonitor_disable_fops = {
	.owner = THIS_MODULE,
	.write = loadmonitor_disable_write,
};
#endif

#ifdef CONFIG_DFX_DEBUG_FS
static int freqdump_debugfile_init(void)
{
	struct dentry *freqdump_flie = NULL;
	struct dentry *loadmonitor_file = NULL;
	struct dentry *loadmonitor_enable_file = NULL;
	struct dentry *loadmonitor_disable_file = NULL;
#ifdef CONFIG_FREQDUMP_PLATFORM
	struct dentry *adc_set_param_file = NULL;
	struct dentry *adc_disable_file = NULL;
#endif

	freqdump_flie = debugfs_create_file("freqdump_data", FILE_RIGHT,
					    g_freqdump_debugfs_root, NULL,
					    &freqdump_fops);
	if (freqdump_flie == NULL)
		return -1;
	loadmonitor_file =
		debugfs_create_file("loadmonitor_data", FILE_RIGHT,
				    g_freqdump_debugfs_root, NULL,
				    &loadmonitor_dump_fops);
	if (loadmonitor_file == NULL)
		return -1;

#ifdef CONFIG_FREQDUMP_PLATFORM
	adc_set_param_file =
		debugfs_create_file("adc_param_set", FILE_RIGHT,
				    g_freqdump_debugfs_root, NULL,
				    &adc_set_param_fops);
	if (adc_set_param_file == NULL)
		return -1;

	adc_disable_file = debugfs_create_file("adc_disable", FILE_RIGHT,
					       g_freqdump_debugfs_root, NULL,
					       &adc_disable_fops);
	if (adc_disable_file == NULL)
		return -1;
#endif

	loadmonitor_enable_file =
		debugfs_create_file("loadmonitor_enable", FILE_RIGHT,
				    g_freqdump_debugfs_root, NULL,
				    &loadmonitor_enable_fops);
	if (loadmonitor_enable_file == NULL)
		return -1;
	loadmonitor_disable_file =
		debugfs_create_file("loadmonitor_disable", FILE_RIGHT,
				    g_freqdump_debugfs_root, NULL,
				    &loadmonitor_disable_fops);
	if (loadmonitor_disable_file == NULL)
		return -1;
	return 0;
}
#endif

static int __init freqdump_debugfs_init(void)
{
	struct device_node *np = NULL;
	u32 data[2] = {0};
	s32 ret;

	sema_init(&g_loadmonitor_sem, 1);
	sema_init(&g_freqdump_sem, 1);

	(void)memset_s(g_loadmonitor_buf, sizeof(g_loadmonitor_buf),
		       0, sizeof(g_loadmonitor_buf));
#ifdef CONFIG_DFX_DEBUG_FS
	g_loadmonitor_status = 0;
#endif
	np = of_find_compatible_node(NULL, NULL, "platform,freqdump");
	if (np == NULL) {
		pr_err("%s: dts %s node not found\n",
		       __func__, "platform,freqdump");
		return -ENODEV;
	}
	ret = of_property_read_u32_array(np, "reg", &data[0], (size_t)2);
	if (ret != 0) {
		pr_err("%s get hisilicon,freqdump attribute failed.\n",
		       __func__);
		return -ENODEV;
	}

	g_freqdump_phy_addr = (ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE +
			      data[0]);
	g_freqdump_virt_addr = ioremap(g_freqdump_phy_addr, (size_t)data[1]);
	memset_io(g_freqdump_virt_addr, 0, (size_t)data[1]);
	if (g_freqdump_virt_addr == NULL) {
		pr_err("freqdump ioremap failed!\n");
		return -ENOMEM;
	}


#ifndef CONFIG_FREQDUMP_PLATFORM
	g_loadmonitor_virt_addr = g_freqdump_virt_addr + sizeof(struct freqdump_data);
#else
	g_loadmonitor_virt_addr = g_freqdump_virt_addr + SHARED_MEMORY_SIZE -
				CS_MAX_PERI_INDEX * 4;
#endif

#ifdef CONFIG_DFX_DEBUG_FS
	g_freqdump_debugfs_root = debugfs_create_dir("freqdump", NULL);
	if (g_freqdump_debugfs_root == NULL)
		goto dir_fail;
	if (freqdump_debugfile_init() != 0)
		goto file_fail;
#endif

	g_freqdump_read_data = kzalloc(sizeof(*g_freqdump_read_data), GFP_KERNEL);
	if (g_freqdump_read_data == NULL) {
		debugfs_remove_recursive(g_freqdump_debugfs_root);
		iounmap(g_freqdump_virt_addr);
		g_freqdump_virt_addr = NULL;
		return -ENOMEM;
	}
	pr_err("freqdump debugfs init success\n");
	return OK;

#ifdef CONFIG_DFX_DEBUG_FS
file_fail:
	debugfs_remove_recursive(g_freqdump_debugfs_root);

dir_fail:
	iounmap(g_freqdump_virt_addr);
	g_freqdump_virt_addr = NULL;
	pr_err("freqdump debugfs init failed!\n");
	return -ENOENT;
#endif
}

static void __exit freqdump_debugfs_exit(void)
{
	debugfs_remove_recursive(g_freqdump_debugfs_root);
	kfree(g_freqdump_read_data);
	g_freqdump_read_data = NULL;
	iounmap(g_freqdump_virt_addr);
	g_freqdump_virt_addr = NULL;
	pr_err("freqdump removed!\n");
}
module_init(freqdump_debugfs_init);
module_exit(freqdump_debugfs_exit);
