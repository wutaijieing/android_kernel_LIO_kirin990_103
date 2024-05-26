/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: governor_pm_qos for ddr
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
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/devfreq.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <platform_include/display/linux/dpu_drmdriver.h>
#include <asm/page.h>
#include <governor.h>
#include "governor_pm_qos.h"
#include "securec.h"

#ifdef CONFIG_DDR_MEM_PERF_SCENE_AWARE
#include <ddr_define.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <platform_include/basicplatform/linux/ipc_msg.h>
#endif

#define DFPQ_MAX_BDUTILIZATION		100
#define DFPQ_MIN_BDUTILIZATION		1
#define DFPQ_BDUTILIZATION_CNT		1
#define FREQ_SCALE_MODULUS		1000000
#define BANDWIDTH_SCALE_MODULUS		100
#define DMSS_GLB_DATA_LEN		0x4
#define CHINTLV_MULTIPLE		128
#define get_chintlv(reg_val)		(1 << (((reg_val) >> 4) & 0x7))
#define FREQ_MIN_VAL			0
#define DEVFREQ_PM_QOS_IMMUTABLE	1
#define HZ_PER_KHZ			1000

#ifdef CONFIG_DDR_MEM_PERF_SCENE_AWARE
#define OK		0
#define ERR		-1
#define ACK_BUFFER_LEN		8
#define PERF_SCENE_CHAINING_MAGIC	0x1A1213B4
#endif

struct devfreq_pm_qos_notifier_block {
	struct list_head node;
	struct notifier_block nb;
	struct devfreq *df;
};

#ifdef CONFIG_DDR_MEM_PERF_SCENE_AWARE
static unsigned char g_perf_scen_id;
#endif

static ssize_t show_bd_utilization(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct devfreq_pm_qos_data *data = NULL;
	int ret;

	mutex_lock(&devfreq->lock);
	data = devfreq->data;
	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE,
			 "%u\n", data->bd_utilization);
	mutex_unlock(&devfreq->lock);

	return ret;
}

static ssize_t store_bd_utilization(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf,
				    size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct devfreq_pm_qos_data *data = NULL;
	unsigned int input;
	int ret;

	ret = sscanf_s(buf, "%u", &input);
	if (ret != DFPQ_BDUTILIZATION_CNT || input > DFPQ_MAX_BDUTILIZATION ||
	    input < DFPQ_MIN_BDUTILIZATION)
		return -EINVAL;

	mutex_lock(&devfreq->lock);
	data = devfreq->data;
	data->bd_utilization = input;

	ret = update_devfreq(devfreq);
	if (ret == 0)
		ret = count;

	mutex_unlock(&devfreq->lock);

	return ret;
}

static ssize_t show_ddr_bandwidth(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct devfreq_pm_qos_data *data = NULL;
	unsigned long freq, bw;
	int ret;

	mutex_lock(&devfreq->lock);
	data = devfreq->data;
	if (devfreq->profile->get_cur_freq == NULL) {
		freq = devfreq->previous_freq;
	} else {
		ret = devfreq->profile->get_cur_freq(devfreq->dev.parent, &freq);
		if (ret != 0) {
			pr_err("%s %d, failed to get cur freq\n",
			       __func__, __LINE__);
			mutex_unlock(&devfreq->lock);
			return ret;
		}
	}

	freq = freq / FREQ_SCALE_MODULUS;
	bw = freq * data->bd_utilization * data->bytes_per_sec_per_hz /
	     BANDWIDTH_SCALE_MODULUS;
	mutex_unlock(&devfreq->lock);

	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE, "%lu\n", bw);
	return ret;
}

#ifdef CONFIG_DDR_MEM_PERF_SCENE_AWARE
static int check_valid(unsigned char scen)
{
	int ret;

	switch (scen) {
	case PERF_SCENE_CASEI:
	case PERF_SCENE_CASEII:
	case PERF_SCENE_CASEIII:
	case PERF_SCENE_CASEIV:
	case PERF_SCENE_CASEV:
	case PERF_SCENE_CASEVI:
		ret = OK;
		break;
	default:
		ret = ERR;
		pr_err("[%s] perf scene invalid!\n", __func__);
		break;
	}

	return ret;
}

static ssize_t update_multi_perf_scen(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf,
				      size_t count)
{
	int ret;
	unsigned char input;
	struct devfreq *devfreq = to_devfreq(dev);

	ret = kstrtou8(buf, 0, &input);
	if (ret != 0 || check_valid(input) != OK)
		return -EINVAL;

	mutex_lock(&devfreq->lock);
	g_perf_scen_id = input;
	mutex_unlock(&devfreq->lock);

	return count;
}

static ssize_t show_perf_scen_id(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	int ret;
	struct devfreq *devfreq = to_devfreq(dev);

	mutex_lock(&devfreq->lock);
	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE,
			 "%u\n", g_perf_scen_id);
	mutex_unlock(&devfreq->lock);

	return ret;
}

static int send_perf_scen_info(void)
{
	int ret;
	union ipc_data msg;
	rproc_id_t mbx = IPC_ACPU_LPM3_MBX_3;
	rproc_msg_t ack[ACK_BUFFER_LEN] = {0};

	msg.cmd_mix.cmd = CMD_SETTING;
	msg.cmd_mix.cmd_src = OBJ_AP;
	msg.cmd_mix.cmd_obj = OBJ_DDR;
	msg.cmd_mix.cmd_type = TYPE_PERF_SCENE;

	ret = memset_s(msg.cmd_mix.cmd_para, sizeof(msg.cmd_mix.cmd_para),
		       0, sizeof(msg.cmd_mix.cmd_para));
	if (ret != EOK) {
		pr_err("[%s]memset_s fail[%d]\n", __func__, ret);
		return ERR;
	}

	msg.cmd_mix.cmd_para[0] = g_perf_scen_id;
	ret = RPROC_SYNC_SEND(mbx, (rproc_msg_t *)&msg,
			      MAX_MAIL_SIZE, ack, ACK_BUFFER_LEN);
	if (ret != 0) {
		pr_err("[%s]send info failed err %d!\n", __func__, ret);
		return ERR;
	}

	return OK;
}

static ssize_t perf_scen_chainig(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf,
				 size_t count)
{
	int ret;
	unsigned int input;
	struct devfreq *devfreq = to_devfreq(dev);

	ret = kstrtouint(buf, 0, &input);
	if (ret != 0)
		return -EINVAL;

	if (input != PERF_SCENE_CHAINING_MAGIC) {
		pr_err("[%s]can not update chaining!\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&devfreq->lock);
	ret = send_perf_scen_info();
	if (ret != OK)
		ret = -EINVAL;
	else
		ret = count;
	mutex_unlock(&devfreq->lock);

	return ret;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static void get_freq_range(struct devfreq *devfreq,
			   unsigned long *min_freq,
			   unsigned long *max_freq)
{
	unsigned long *freq_table = devfreq->profile->freq_table;
	s32 qos_min_freq, qos_max_freq;

	lockdep_assert_held(&devfreq->lock);

	if (freq_table[0] < freq_table[devfreq->profile->max_state - 1]) {
		*min_freq = freq_table[0];
		*max_freq = freq_table[devfreq->profile->max_state - 1];
	} else {
		*min_freq = freq_table[devfreq->profile->max_state - 1];
		*max_freq = freq_table[0];
	}

	qos_min_freq = dev_pm_qos_read_value(devfreq->dev.parent,
					     DEV_PM_QOS_MIN_FREQUENCY);
	qos_max_freq = dev_pm_qos_read_value(devfreq->dev.parent,
					     DEV_PM_QOS_MAX_FREQUENCY);
	*min_freq = max(*min_freq, (unsigned long)HZ_PER_KHZ * qos_min_freq);
	if (qos_max_freq != PM_QOS_MAX_FREQUENCY_DEFAULT_VALUE)
		*max_freq = min(*max_freq,
				(unsigned long)HZ_PER_KHZ * qos_max_freq);

	*min_freq = max(*min_freq, devfreq->scaling_min_freq);
	*max_freq = min(*max_freq, devfreq->scaling_max_freq);

	if (*min_freq > *max_freq)
		*min_freq = *max_freq;
}
#endif

static DEVICE_ATTR(bd_utilization, 0644,
		   show_bd_utilization, store_bd_utilization);
static DEVICE_ATTR(ddr_bandwidth, 0444, show_ddr_bandwidth, NULL);

#ifdef CONFIG_DDR_MEM_PERF_SCENE_AWARE
static DEVICE_ATTR(perf_scen_select, 0600,
		   show_perf_scen_id, update_multi_perf_scen);
static DEVICE_ATTR(perf_scen_notify, 0200, NULL, perf_scen_chainig);
#endif

static const struct attribute *g_governor_pm_qos_attrs[] = {
	&dev_attr_bd_utilization.attr,
	&dev_attr_ddr_bandwidth.attr,
#ifdef CONFIG_DDR_MEM_PERF_SCENE_AWARE
	&dev_attr_perf_scen_select.attr,
	&dev_attr_perf_scen_notify.attr,
#endif
	NULL,
};

static LIST_HEAD(g_devfreq_pm_qos_list);
static DEFINE_MUTEX(g_devfreq_pm_qos_mutex);

static int devfreq_pm_qos_func(struct devfreq *df, unsigned long *freq)
{
	struct devfreq_pm_qos_data *data = df->data;
	unsigned long megabytes_per_sec, mhz;
	unsigned long ul_max = ULONG_MAX / FREQ_SCALE_MODULUS /
			       BANDWIDTH_SCALE_MODULUS *
			       data->bytes_per_sec_per_hz *
			       data->bd_utilization;

	megabytes_per_sec = pm_qos_request(data->pm_qos_class);
	if (megabytes_per_sec > FREQ_MIN_VAL && megabytes_per_sec < ul_max) {
		megabytes_per_sec = megabytes_per_sec *
				    BANDWIDTH_SCALE_MODULUS;
		megabytes_per_sec = DIV_ROUND_UP(megabytes_per_sec,
						 data->bd_utilization);
		mhz = DIV_ROUND_UP(megabytes_per_sec,
				   data->bytes_per_sec_per_hz);
		*freq = mhz * FREQ_SCALE_MODULUS;
	} else if (megabytes_per_sec >= ul_max) {
		*freq = ULONG_MAX;
	} else {
		*freq = FREQ_MIN_VAL;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	if (df->min_freq && *freq < df->min_freq)
		*freq = df->min_freq;

	if (df->max_freq && *freq > df->max_freq)
		*freq = df->max_freq;
#else
	unsigned long min_freq, max_freq;

	get_freq_range(df, &min_freq, &max_freq);

	if (*freq < min_freq)
		*freq = min_freq;

	if (*freq > max_freq)
		*freq = max_freq;
#endif

	return 0;
}

static int devfreq_pm_qos_notifier(struct notifier_block *nb,
				   unsigned long val,
				   void *v)
{
	struct devfreq_pm_qos_notifier_block *pq_nb = NULL;

	pq_nb = container_of(nb, struct devfreq_pm_qos_notifier_block, nb);
	mutex_lock(&pq_nb->df->lock);
	(void)update_devfreq(pq_nb->df);
	mutex_unlock(&pq_nb->df->lock);

	return NOTIFY_OK;
}

static int devfreq_pm_qos_gov_init(struct devfreq *df)
{
	int ret;
	struct devfreq_pm_qos_notifier_block *pq_nb = NULL;
	struct devfreq_pm_qos_data *data = df->data;

	if (data == NULL) {
		pr_err("%s %d, invalid pointer\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (data->bytes_per_sec_per_hz == 0 || data->bd_utilization == 0) {
		pr_err("%s %d, invalid parameter\n", __func__, __LINE__);
		return -EINVAL;
	}

	pq_nb = kzalloc(sizeof(*pq_nb), GFP_KERNEL);
	if (pq_nb == NULL) {
		pr_err("%s %d, kzalloc fail\n", __func__, __LINE__);
		return -ENOMEM;
	}

	pq_nb->df = df;
	pq_nb->nb.notifier_call = devfreq_pm_qos_notifier;
	INIT_LIST_HEAD(&pq_nb->node);

	ret = pm_qos_add_notifier(data->pm_qos_class, &pq_nb->nb);
	if (ret < 0) {
		pr_err("%s %d, Failed to add pm qos notifier\n",
		       __func__, __LINE__);
		kfree(pq_nb);
		return ret;
	}

	ret = sysfs_create_files(&df->dev.kobj, g_governor_pm_qos_attrs);
	if (ret != 0) {
		pr_err("%s: failed to create sysfs file\n", __func__);
		(void)pm_qos_remove_notifier(data->pm_qos_class, &pq_nb->nb);
		kfree(pq_nb);
		return ret;
	}

	mutex_lock(&g_devfreq_pm_qos_mutex);
	list_add_tail(&pq_nb->node, &g_devfreq_pm_qos_list);
	mutex_unlock(&g_devfreq_pm_qos_mutex);

	return 0;
}

static void devfreq_pm_qos_gov_exit(struct devfreq *df)
{
	struct devfreq_pm_qos_notifier_block *pq_nb = NULL;
	struct devfreq_pm_qos_notifier_block *tmp = NULL;
	struct devfreq_pm_qos_data *data = NULL;

	sysfs_remove_files(&df->dev.kobj, g_governor_pm_qos_attrs);

	mutex_lock(&g_devfreq_pm_qos_mutex);

	list_for_each_entry_safe(pq_nb, tmp, &g_devfreq_pm_qos_list, node) {
		if (pq_nb->df == df) {
			data = pq_nb->df->data;
			(void)pm_qos_remove_notifier(data->pm_qos_class, &pq_nb->nb);
			list_del(&pq_nb->node);
			kfree(pq_nb);
			break;
		}
	}

	mutex_unlock(&g_devfreq_pm_qos_mutex);
}

static int devfreq_pm_qos_handler(struct devfreq *devfreq,
				  unsigned int event,
				  void *data)
{
	int ret = 0;

	switch (event) {
	case DEVFREQ_GOV_START:
		ret = devfreq_pm_qos_gov_init(devfreq);
		break;
	case DEVFREQ_GOV_STOP:
		devfreq_pm_qos_gov_exit(devfreq);
		break;
	default:
		break;
	}

	return ret;
}

struct devfreq_governor g_devfreq_pm_qos = {
	.name = "pm_qos",
	.immutable = DEVFREQ_PM_QOS_IMMUTABLE,
	.get_target_freq = devfreq_pm_qos_func,
	.event_handler = devfreq_pm_qos_handler,
};

static int __init devfreq_pm_qos_init(void)
{
	return devfreq_add_governor(&g_devfreq_pm_qos);
}

subsys_initcall(devfreq_pm_qos_init);

static void __exit devfreq_pm_qos_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&g_devfreq_pm_qos);
	if (ret != 0)
		pr_err("%s: failed remove governor %d\n", __func__, ret);
}

module_exit(devfreq_pm_qos_exit);
MODULE_LICENSE("GPL v2");
