/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2020. All rights reserved.
 *
 * regulator_debug.c
 *
 * regulator debug API
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "regulator_debug.h"
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/coupler.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <securec.h>

#include "regulator/internal.h"

#ifdef CONFIG_PMIC_PLATFORM_DEBUG
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#endif

#ifdef CONFIG_REGULATOR_TRACE
#include <platform_include/basicplatform/linux/rdr_dfx_ap_ringbuffer.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <platform_include/basicplatform/linux/util.h>
#include <linux/pm.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#endif

#include <platform_include/basicplatform/linux/pr_log.h>
#define PR_LOG_TAG PMIC_REGULATOR_DEBUG_TAG
#undef CREATE_TRACE_POINTS
#include <trace/hooks/hck_hook/hck_regulator.h>

#define ARRAY_LINE 128
#ifdef CONFIG_REGULATOR_TRACE

#define REGULATOR_NAME_LEN 16
#define REGULATOR_MAGIC_NUM 0x16022602U

#ifdef CONFIG_GCOV_KERNEL
#define STATIC
#else
#define STATIC static
#endif

struct pc_record_info {
	unsigned int   dump_magic;
	unsigned int   buffer_size;
	unsigned char *buffer_addr;
	unsigned char *percpu_addr[NR_CPUS];
	unsigned int   percpu_length[NR_CPUS];
};

struct regulator_record_info {
	u64 current_time;
	/* 0: enable 1: set vol  2: mode */
	u8 item;
	u8 mode;
	u16 enalbe_count;
	int max_uv;
	int min_uv;
	char comm[REGULATOR_NAME_LEN];
};

enum BUF_TYPE_EN {
	SINGLE_BUF = 0,
	MULTI_BUF,
};

static u64 g_regulator_rdr_core_id = RDR_REGULATOR;
static struct pc_record_info *g_regulator_track_addr;
static struct pc_record_info g_regulator_track_info;
static unsigned char g_regulator_hook_on;
static enum BUF_TYPE_EN g_regulator_sel_buf_type = MULTI_BUF;
#endif

#ifdef CONFIG_PMIC_PLATFORM_DEBUG
static LIST_HEAD(regulator_test_list);
struct regulator_test {
	struct regulator *regulator;
	struct list_head list;
};

static int regulator_state_show(struct device *dev, void *data)
{
	struct regulator_dev *rdev = container_of(dev, struct regulator_dev, dev);
	struct seq_file *s = data;
	const struct regulator_ops *ops = NULL;
	int enabled = 0;

	if (rdev->constraints->name == NULL)
		return 0;

	seq_printf(s, "%-15s", rdev->constraints->name);
	ops = rdev->desc->ops;
	if (ops == NULL)
		return 0;
	if (ops->is_enabled)
		enabled = ops->is_enabled(rdev);

	if (enabled)
		seq_printf(s, "%-20s", "ON");
	else
		seq_printf(s, "%-20s", "OFF");

	seq_printf(s, "%u\t\t", rdev->use_count);
	seq_printf(s, "%u\t\t", rdev->open_count);
	if (rdev->constraints->always_on)
		seq_printf(s, "%-17s\n\r", "ON");
	else
		seq_printf(s, "%-17s\n\r", "OFF");

	return 0;
}

void get_current_regulator_dev(struct seq_file *s)
{
	class_for_each_device(&regulator_class, NULL, s,
		regulator_state_show);
}

static int regulator_match(struct device *dev, const void *data)
{
	struct regulator_dev *r = container_of(dev, struct regulator_dev, dev);

	return strcmp(rdev_get_name(r), data) == 0;
}

static bool dbg_regulator_check(const char *ldo_name)
{
	struct device *dev = NULL;

	dev = class_find_device(&regulator_class, NULL, ldo_name, regulator_match);

	return dev ? true : false;
}

static struct regulator* r_test_get(const char *ldo_name)
{
	struct regulator_test *node = NULL;
	struct regulator_test *test_node = NULL;
	struct regulator_dev *rdev = NULL;
	struct regulator *r_test = NULL;

	if (!dbg_regulator_check(ldo_name)) {
		pr_err("%s not registerd in regulator framework\n", ldo_name);
		return ERR_PTR(-ENOMEM);
	}

	list_for_each_entry(node, &regulator_test_list, list) {
		rdev = node->regulator->rdev;
		if (strncmp(rdev->constraints->name, ldo_name, strlen(ldo_name)) == 0) {
			r_test = node->regulator;
			return r_test;
		}
	}

	r_test = regulator_get(NULL, ldo_name);
	if (IS_ERR(r_test)) {
		pr_err("can not get regulator %s\n", ldo_name);
		return ERR_PTR(-ENOMEM);
	}
	test_node = kzalloc(sizeof(*test_node), GFP_KERNEL);
	if (test_node == NULL) {
		pr_err("test_node alloc failed\n");
		return ERR_PTR(-ENOMEM);
	}
	test_node->regulator = r_test;
	list_add(&test_node->list, &regulator_test_list);

	return r_test;
}

static void r_test_put(struct regulator *regulator)
{
	struct regulator_test *node = NULL;
	struct regulator_dev *rdev = NULL;
	struct regulator_dev *rdev_node = NULL;
	unsigned int len;

	if (regulator->enable_count == 0) {
		rdev = regulator->rdev;
		len = strlen(rdev->constraints->name);
		list_for_each_entry(node, &regulator_test_list, list) {
			rdev_node = node->regulator->rdev;
			if (strncmp(rdev_node->constraints->name, rdev->constraints->name, len) == 0) {
				list_del(&node->list);
				regulator_put(regulator);
				kfree(node);
				return;
			}
		}
	}
}

void set_regulator_state(const char *ldo_name, int value)
{
	struct regulator *r_test = NULL;
	const struct regulator_ops *ops = NULL;
	int ret;

	pr_info("ldo_name = %s,value = %d\n", ldo_name, value);
	r_test = r_test_get(ldo_name);
	if (IS_ERR(r_test)) {
		pr_err("can not get regulator %s\n", ldo_name);
		return;
	}

	ops = r_test->rdev->desc->ops;
	if ((value == 0) && ops->disable) {
		pr_info("close the %s\n", ldo_name);
		ret = regulator_disable(r_test);
		if (ret != 0)
			pr_err("disable regulator %s failed\n", ldo_name);
	} else if ((value == 1) && ops->enable) {
		pr_info("open the %s\n", ldo_name);
		ret = regulator_enable(r_test);
		if (ret != 0)
			pr_err("enable regulator %s failed\n", ldo_name);
	}
	r_test_put(r_test);

	return;
}

int set_regulator_voltage(const char *ldo_name, unsigned int vol_value)
{
	struct regulator *r_test = NULL;
	const struct regulator_ops *ops = NULL;
	int ret;

	pr_info("ldo_name=%s,vol_value=%u\n", ldo_name, vol_value);
	r_test = r_test_get(ldo_name);
	if (IS_ERR(r_test)) {
		pr_err("can not get regulator %s\n", ldo_name);
		return -1;
	}

	ops = r_test->rdev->desc->ops;
	if (ops->set_voltage) {
		ret = regulator_set_voltage(r_test, vol_value, vol_value);
		if (ret != 0) {
			pr_info("voltage set fail!\n");
			r_test_put(r_test);
		} else {
			pr_info("voltage set succ!\n");
			r_test_put(r_test);
			return 0;
		}
	}

	return -1;
}

void get_regulator_state(const char *ldo_name, int length)
{
	struct regulator *r_test = NULL;
	const struct regulator_ops *ops = NULL;
	int state_value;

	pr_info("ldo_name=%s\n", ldo_name);
	r_test = r_test_get(ldo_name);
	if (IS_ERR(r_test)) {
		pr_err("can not get regulator %s\n", ldo_name);
		return;
	}

	ops = r_test->rdev->desc->ops;
	if (ops->is_enabled) {
		state_value = regulator_is_enabled(r_test);
		pr_info("enabled_state is %d, ", state_value);
	}
	if (ops->get_voltage) {
		state_value = regulator_get_voltage(r_test);
		pr_info("voltage_value the %d\n", state_value);
	}
	r_test_put(r_test);
	return;
}

static u32 pmu_atoi(const char *s)
{
	const char *p = NULL;
	char c;
	u64 ret = 0;

	if (!s)
		return 0;
	p = s;
	while ((c = *p++) != '\0') {
		if (c >= '0' && c <= '9') {
			ret *= 10; /* ret = ret * 10 */
			ret += (u64)(unsigned int)((unsigned char)c - '0');
			if (ret > U32_MAX)
				return 0;
		} else {
			break;
		}
	}
	return (u32)ret;
}

static int dbg_regulator_show(struct seq_file *s, void *data)
{
	if (!s)
		return -1;
	seq_puts(s, "\n\r");
	seq_printf(s, "%-13s %-15s %-15s %-15s %-15s\n\r",
		"LDO_NAME", "ON/OFF", "Use_count", "Open_count", "Always_on");
	get_current_regulator_dev(s);
	return 0;
}

static int dbg_regulator_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_regulator_show, inode->i_private);
}

static const struct file_operations debug_regulator_state_fops = {
	.open		= dbg_regulator_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int dbg_control_regulator_show(struct seq_file *s, void *data)
{
	pr_err("                                                                             \n\r \
		---------------------------------------------------------------------------------\n\r \
		|usage:                                                                         |\n\r \
		|	S = state	R = read	V = voltage                                         |\n\r \
		|	set ldo state and voltage                                                   |\n\r \
		|	get ldo state and current voltage                                           |\n\r \
		|example:                                                                       |\n\r \
		|	echo S ldo16 0   > control_regulator	:disable ldo16                      |\n\r \
		|	echo S ldo16 1   > control_regulator	:enable ldo16                       |\n\r \
		|	echo R ldo16     > control_regulator	:get ldo16 state and voltage        |\n\r \
		|	echo V ldo16 xxx > control_regulator	:set ldo16 voltage                  |\n\r \
		---------------------------------------------------------------------------------\n\r");
	return 0;
}

static int dbg_control_regulator_arg_check(const char __user *buffer,
					size_t count)
{
	if (count >= 128 || count == 0) {
		pr_info("error! buffer size illegal!\n");
		return -EFAULT;
	}

	if (!buffer) {
		pr_info("error! buffer is NULL!\n");
		return -EFAULT;
	}
	return 0;
}

static void dbg_control_regulator_set_value_handler(const char *tmp, size_t count)
{
	char ptr[ARRAY_LINE] = {0};
	unsigned int i;
	unsigned int j;
	int next_flag = 1;
	const char *vol = NULL;
	char num = 0;

	if (tmp[0] == 'R' || tmp[0] == 'r') {
		for (i = 2, j = 0; i < (count - 1); i++, j++)
			ptr[j] = tmp[i];
		ptr[j] = '\0';
		get_regulator_state(ptr, ARRAY_LINE);
	} else if (tmp[0] == 'S' || tmp[0] == 's') {
		for (i = 2, j = 0; i < (count - 1); i++, j++) {
			if (tmp[i] == ' ') {
				next_flag = 0;
				ptr[j] = '\0';
				continue;
			}
			if (next_flag)
				ptr[j] = tmp[i];
			else
				num = tmp[i] - '0';
		}
		set_regulator_state(ptr, num);
	} else if (tmp[0] == 'V' || tmp[0] == 'v') {
		for (i = 2, j = 0; i < (count - 1); i++, j++) {
			if (tmp[i] == ' ') {
				next_flag = 0;
				ptr[j] = '\0';
				continue;
			}
			if (next_flag) {
				ptr[j] = tmp[i];
			} else {
				vol = &tmp[i];
				break;
			}
		}
		set_regulator_voltage(ptr, pmu_atoi(vol));
	}
}

static ssize_t dbg_control_regulator_set_value(struct file *filp,
	const char __user *buffer, size_t count, loff_t *ppos)
{
	char tmp[ARRAY_LINE] = {0};
	int ret;

	if (!buffer) {
		pr_info("error! buffer is NULL!\n");
		return -EFAULT;
	}

	ret = dbg_control_regulator_arg_check(buffer, count);
	if (ret) {
		pr_info("error!\n");
		return -EFAULT;
	}

	if (count >= ARRAY_LINE) {
		pr_info("error!\n");
		return -EFAULT;
	}
	if (copy_from_user(tmp, buffer, count)) {
		pr_info("error!\n");
		return -EFAULT;
	}
	dbg_control_regulator_set_value_handler(tmp, count);

	*ppos += count;

	return count;
}

static int dbg_control_regulator_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return single_open(file, dbg_control_regulator_show, &inode->i_private);
}

static const struct file_operations set_control_regulator_fops = {
	.open		= dbg_control_regulator_open,
	.read		= seq_read,
	.write		= dbg_control_regulator_set_value,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init regulator_debugfs_init(void)
{
	struct dentry *d = NULL;

	d = debugfs_create_dir("regulator_debugfs", NULL);
	if (!d) {
		pr_err("failed to create regulator debugfs dir!\n");
		return -ENOMEM;
	}
	(void)debugfs_create_file("regulator_state", S_IRUSR, d, NULL,
		&debug_regulator_state_fops);

	(void)debugfs_create_file("control_regulator", S_IRUSR, d, NULL,
		&set_control_regulator_fops);

	return 0;
}
late_initcall(regulator_debugfs_init);
#endif

static int ip_regulator_state_show(struct device *dev, void *data)
{
	struct regulator_dev *rdev = container_of(dev, struct regulator_dev, dev);
	const struct regulator_ops *ops = NULL;
	int enabled = 0;

	if ((rdev == NULL)              ||
		(rdev->desc == NULL)        ||
		(rdev->desc->ops == NULL)   ||
		(rdev->constraints == NULL) ||
		(rdev->constraints->name == NULL))
		return 0;

	if ((strstr(rdev->constraints->name, "ldo") != NULL) ||
		(strstr(rdev->constraints->name, "buck") != NULL) ||
		(strstr(rdev->constraints->name, "schg") != NULL))
		return 0;

	ops = rdev->desc->ops;
	if (ops->is_enabled) {
		enabled = ops->is_enabled(rdev);
		if (enabled)
			pr_err("N:%s, S:%d, C:%u\n",
				rdev->constraints->name, enabled,
				rdev->use_count);
	}

	return 0;
}

void get_ip_regulator_state(void)
{
	pr_info("Get IP Regulator State!\n");
	class_for_each_device(&regulator_class, NULL, NULL,
		ip_regulator_state_show);
}

#ifdef CONFIG_REGULATOR_TRACE
static const char *track_regulator_rdev_get_name(struct regulator_dev *rdev)
{
	if (rdev->constraints && rdev->constraints->name)
		return rdev->constraints->name;
	else if (rdev->desc->name)
		return rdev->desc->name;
	else
		return "";
}

static void __track_regulator(struct regulator_dev *rdev,
	enum track_regulator_type track_item, u8 mode, int max_uv, int min_uv)
{
	struct regulator_record_info info = {0};
	u8 cpu;
	int ret;

	if (!g_regulator_hook_on)
		return;

	cpu = (u8)raw_smp_processor_id();
	if (g_regulator_sel_buf_type == SINGLE_BUF)
		cpu = 0;

	info.current_time = dfx_getcurtime();
	info.item = track_item;
	info.mode = mode;
	info.enalbe_count = rdev->use_count;
	info.max_uv = max_uv;
	info.min_uv = min_uv;
	ret = strncpy_s(info.comm, sizeof(info.comm), track_regulator_rdev_get_name(rdev),
		sizeof(info.comm) - 1);
	if (ret != EOK)
		pr_debug("copy regulator name failed!\n");

	info.comm[REGULATOR_NAME_LEN - 1] = '\0';
	pr_debug("######%s!\n", info.comm);
	dfx_ap_ringbuffer_write(
		(struct dfx_ap_ringbuffer_s *)g_regulator_track_info.percpu_addr[cpu],
		(u8 *)&info);
}

static void track_regulator_onoff(void *data, struct regulator_dev *rdev)
{
	if (IS_ERR_OR_NULL(rdev)) {
		pr_err("%s param is null!\n", __func__);
		return;
	}

	__track_regulator(rdev, TRACK_ON_OFF, 0, 0, 0);
}

static void track_regulator_set_vol(void *data, struct regulator_dev *rdev,
	int max_uv, int min_uv)
{
	if (IS_ERR_OR_NULL(rdev)) {
		pr_err("%s param is null!\n", __func__);
		return;
	}

	__track_regulator(rdev, TRACK_VOL, 0, max_uv, min_uv);
}

static void track_regulator_set_mode(void *data, struct regulator_dev *rdev,
	u8 mode)
{
	if (IS_ERR_OR_NULL(rdev)) {
		pr_err("%s param is null!\n", __func__);
		return;
	}

	__track_regulator(rdev, TRACK_VOL, mode, 0, 0);
}

static void track_regulator_reset(u32 modid, u32 etype, u64 coreid)
{
}

static void track_regulator_dump(u32 modid, u32 etype, u64 coreid,
	char *pathname, pfn_cb_dump_done pfn_cb)
{
	if (pfn_cb)
		pfn_cb(modid, coreid);

	pr_info("%s dump!\n", __func__);
}

STATIC int track_regulator_rdr_register(
	struct rdr_register_module_result *result)
{
	struct rdr_module_ops_pub s_module_ops = {
		.ops_dump = NULL,
		.ops_reset = NULL
	};

	int ret = -1;

	if (result == NULL) {
		pr_err("%s para null!\n", __func__);
		return ret;
	}

	pr_info("%s start!\n", __func__);

	s_module_ops.ops_dump = track_regulator_dump;
	s_module_ops.ops_reset = track_regulator_reset;
	ret = rdr_register_module_ops(g_regulator_rdr_core_id, &s_module_ops,
		result);

	pr_info("%s end!\n", __func__);

	return ret;
}

#define ALIGN8(size) ((size / BITS_PER_BYTE) * BITS_PER_BYTE)
static int regulator_percpu_buffer_init(u8 *addr, u32 size, u32 fieldcnt,
	u32 magic_number, u32 ratio[][BITS_PER_BYTE], u32 max_cpu_nums,
	enum BUF_TYPE_EN buf_type)
{
	int i, ret;
	errno_t err;
	u32 cpu_num = num_possible_cpus();
	if (cpu_num > max_cpu_nums) {
		pr_err("[%s] cpu number error!\n", __func__);
		return -1;
	}

	if (IS_ERR_OR_NULL(addr)) {
		pr_err("[%s], buffer_addr [0x%pK], buffer_size [0x%x]\n",
			__func__, addr, size);
		return -1;
	}
	if (buf_type == SINGLE_BUF)
		cpu_num = 1;
	pr_info("[%s], num_online_cpus [%d]!\n", __func__, num_online_cpus());

	/* set pc info for parse */
	g_regulator_track_addr = (struct pc_record_info *)addr;
	g_regulator_track_info.buffer_addr = addr;
	g_regulator_track_info.buffer_size = size - sizeof(struct pc_record_info);
	g_regulator_track_info.dump_magic = magic_number;

	/* set per cpu buffer */
	for (i = 0; i < cpu_num; i++) {
		pr_info("[%s], ratio[%u][%d] = [%u]\n", __func__, (cpu_num - 1),
			i, ratio[cpu_num - 1][i]);
		/* 16: elements for per line */
		g_regulator_track_info.percpu_length[i] =
			g_regulator_track_info.buffer_size / 16 * ratio[cpu_num - 1][i];
		g_regulator_track_info.percpu_length[i] =
			ALIGN8(g_regulator_track_info.percpu_length[i]);

		if (i == 0)
			g_regulator_track_info.percpu_addr[0] =
				g_regulator_track_info.buffer_addr +
				sizeof(struct pc_record_info);
		else
			g_regulator_track_info.percpu_addr[i] =
				g_regulator_track_info.percpu_addr[i - 1] +
				g_regulator_track_info.percpu_length[i - 1];

		pr_info("[%s], [%d]: percpu_addr [0x%pK], percpu_length [0x%x], fieldcnt [%u]\n",
			__func__, i, g_regulator_track_info.percpu_addr[i],
			g_regulator_track_info.percpu_length[i], fieldcnt);

		ret = dfx_ap_ringbuffer_init((struct dfx_ap_ringbuffer_s *)g_regulator_track_info.percpu_addr[i],
			g_regulator_track_info.percpu_length[i], fieldcnt,
			"regulator");
		if (ret) {
			pr_err("[%s], cpu [%d] ringbuffer init failed!\n",
				__func__, i);
			return ret;
		}
	}
	err = memcpy_s(g_regulator_track_addr, sizeof(struct pc_record_info),
		&g_regulator_track_info, sizeof(struct pc_record_info));
	if (err != EOK) {
		pr_err("[%s], memcpy_s fail\n", __func__);
		return -1;
	}
	return 0;
}

static int regulator_buffer_init(u8 *addr, u32 size, enum BUF_TYPE_EN buf_type)
{
	unsigned int record_ratio[BITS_PER_BYTE][BITS_PER_BYTE] = {
		{ 16, 0, 0, 0, 0, 0, 0, 0 },
		{  8, 8, 0, 0, 0, 0, 0, 0 },
		{  6, 5, 5, 0, 0, 0, 0, 0 },
		{  4, 4, 4, 4, 0, 0, 0, 0 },
		{  4, 4, 4, 3, 1, 0, 0, 0 },
		{  4, 4, 3, 3, 1, 1, 0, 0 },
		{  4, 3, 3, 3, 1, 1, 1, 0 },
		{  3, 3, 3, 3, 1, 1, 1, 1 }
	};

	return regulator_percpu_buffer_init(addr, size,
		sizeof(struct regulator_record_info), REGULATOR_MAGIC_NUM,
		record_ratio, BITS_PER_BYTE, buf_type);
}

static int track_regulator_vir_addr_reinit(void)
{
	errno_t err;

	if (!g_regulator_track_addr) {
		pr_err("[%s], g_regulator_track_addr is NULL\n", __func__);
		return -1;
	}
	err = memcpy_s(g_regulator_track_addr, sizeof(struct pc_record_info),
		&g_regulator_track_info, sizeof(struct pc_record_info));
	if (err != EOK) {
		pr_err("[%s], memcpy_s fail\n", __func__);
		return -1;
	}
	pr_info("[%s], reinit success\n", __func__);
	return 0;
}

static int track_regulator_pm_callback(
	struct notifier_block *nb, unsigned long action, void *ptr)
{
	if (action == PM_POST_HIBERNATION)
		return track_regulator_vir_addr_reinit();

	return NOTIFY_OK;
}

static struct notifier_block g_track_regulator_pm_callback_nb = {
	.notifier_call = track_regulator_pm_callback,
	.priority = 0
};

STATIC int __init track_regulator_record_init(void)
{
	int ret;
	struct rdr_register_module_result regulator_rdr_info;
	unsigned char *vir_addr = NULL;

	register_pm_notifier(&g_track_regulator_pm_callback_nb);
	/* alloc rdr memory and init */
	ret = track_regulator_rdr_register(&regulator_rdr_info);
	if (ret)
		return ret;

	if (regulator_rdr_info.log_len == 0) {
		pr_err("%s clk_rdr_len is 0x0!\n", __func__);
		return 0;
	}
	vir_addr = (unsigned char *)dfx_bbox_map(
		(phys_addr_t)regulator_rdr_info.log_addr,
		regulator_rdr_info.log_len);
	pr_info("%s log_addr is 0x%llx, log_len is 0x%x!\n", __func__,
		regulator_rdr_info.log_addr, regulator_rdr_info.log_len);
	if (IS_ERR_OR_NULL(vir_addr)) {
		pr_err("%s vir_addr err!\n", __func__);
		return -1;
	}

	ret = memset_s(vir_addr, regulator_rdr_info.log_len, 0, regulator_rdr_info.log_len);
	if (ret != EOK) {
		pr_err("%s memset vir addr failed!\n", __func__);
		return -1;
	}


	ret = regulator_buffer_init(vir_addr, regulator_rdr_info.log_len,
		g_regulator_sel_buf_type);
	if (ret) {
		pr_err("%s buffer init err!\n", __func__);
		return -1;
	}

	if (check_mntn_switch(MNTN_TRACE_CLK_REGULATOR))
		g_regulator_hook_on = 1;

	register_trace_hck_vh_regulator_on_off(track_regulator_onoff, NULL);
	register_trace_hck_vh_regulator_set_vol(track_regulator_set_vol, NULL);
	register_trace_hck_vh_regulator_set_mode(track_regulator_set_mode, NULL);

	pr_err("%s: hook_on = %u,rdr_phy_addr = 0x%llx, rdr_len = 0x%x, rdr_virt_add = 0x%pK\n",
		__func__, g_regulator_hook_on, regulator_rdr_info.log_addr,
		regulator_rdr_info.log_len, vir_addr);

	return 0;
}

STATIC void __exit track_regulator_record_exit(void)
{
	unregister_pm_notifier(&g_track_regulator_pm_callback_nb);
}

module_init(track_regulator_record_init);
module_exit(track_regulator_record_exit);
#endif

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("regulator debug func");
