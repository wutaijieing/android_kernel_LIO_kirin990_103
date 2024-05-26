/*
 * io.c
 *
 * debug information for suspend
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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

#include "pm.h"
#include "pub_def.h"
#include "helper/log/lowpm_log.h"
#include "helper/dtsi/dtsi_ops.h"
#include "helper/debugfs/debugfs.h"
#include "helper/register/register_ops.h"

#include <lowpm_gpio_auto_gen.h>
#include <securec.h>

#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/init.h>


static const char *g_plat_name;
static int g_chipid[CHIPID_DIG_NUMS];

static void lpm_read_chipid(void)
{
	int ret, i;

	for (i = 0; i < CHIPID_DIG_NUMS; i++)
		g_chipid[i] = 0;
	ret = of_property_read_u32_array(of_find_node_by_path("/"),
					 "hisi,chipid", &g_chipid[0],
					 CHIPID_DIG_NUMS);
	if (ret != 0) {
		lp_err("not find chipid, but may not wrong");
		return;
	}

	lp_msg(NO_SEQFILE, "%s: chipid is:\t", __func__);
	for (i = 0; i < CHIPID_DIG_NUMS; i++)
		lp_msg_cont(NO_SEQFILE, "%x ", g_chipid[i]);

	lp_msg_cont(NO_SEQFILE, "\n");
}

static int chipid_match(const int *chipid)
{
	int i;

	for (i = 0; i < CHIPID_DIG_NUMS; i++) {
		if (chipid[i] != g_chipid[i])
			return -ENODEV;
	}

	return 0;
}

static struct iocfg_lp *get_iocfg_by_chipid(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(iocfg_tables); i++) {
		if (chipid_match(iocfg_tables[i].chipid) == 0) {
			lp_debug("find by chipid, board name is %s",
				  iocfg_tables[i].boardname);
			return iocfg_tables[i].iocfg;
		}
	}

	return NULL;
}

struct lp_plat_condition {
	const char *include;
	const char *exclude;
	const char *target;
};

static const struct lp_plat_condition g_plat_condition[] = {
	{"es", "", "chip_es"},
	{"cs2", "", "chip_cs2"},
	{"cs", "cs2", "chip_cs"},
	{"", "", "chip"},
};
static struct iocfg_lp *g_lp_iocfg_table;

static const char *plat_match(const char *plat_name, const char *board_name)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_plat_condition); i++) {
		if (strnstr(plat_name, g_plat_condition[i].include, strlen(plat_name)) &&
		    strnstr(board_name, g_plat_condition[i].target, strlen(board_name)) &&
		    !strnstr(board_name, g_plat_condition[i].exclude, strlen(board_name)))
			return g_plat_condition[i].target;
	}
	return NULL;
}

static struct iocfg_lp *get_iocfg_by_name(void)
{
	unsigned int i;
	const char *board_type = NULL;

	for (i = 0; i < ARRAY_SIZE(iocfg_tables); i++) {
		board_type = plat_match(g_plat_name, iocfg_tables[i].boardname);
		if (board_type != NULL) {
			lp_debug("board type is %s", board_type);
			return iocfg_tables[i].iocfg;
		}
	}

	return NULL;
}

static int lp_init_iocfg(void)
{
	g_lp_iocfg_table = get_iocfg_by_chipid();
	if (g_lp_iocfg_table != NULL)
		return 0;

	g_lp_iocfg_table = get_iocfg_by_name();
	if (g_lp_iocfg_table != NULL)
		return 0;

	lp_err("failed");
	return -ENODEV;
}

static int get_plat_name(void)
{
	int ret;

	ret = of_property_read_string(lp_dn_root(), "plat-name", &g_plat_name);
	if (ret != 0) {
		lp_err("no plat-name");
		return -ENODEV;
	}

	return 0;
}

static int sr_mntn_io_init_plat_info(void)
{
	int ret;

	lpm_read_chipid();

	ret = get_plat_name();
	if (ret != 0)
		return -EFAULT;

	ret = lp_init_iocfg();
	if (ret != 0)
		return -EFAULT;

	return 0;
}

static void dbg_disp_iocfg_tables(struct seq_file *s)
{
	unsigned int i, j;

	lp_msg(s, "iocfg table length is %lu: ", ARRAY_SIZE(iocfg_tables));
	for (i = 0; i < ARRAY_SIZE(iocfg_tables); i++) {
		lp_msg_cont(s, "chipid:< ");
		for (j = 0; j < CHIPID_DIG_NUMS; j++)
			lp_msg_cont(s, "%X ", iocfg_tables[i].chipid[j]);
		lp_msg_cont(s, ">; boardname:%s\n",
			       iocfg_tables[i].boardname);
	}
}

static void debug_print_platinfo(struct seq_file *s)
{
	dbg_disp_iocfg_tables(s);
	lp_msg(s, "%s", g_plat_name);
}

#define GPIO_GROUP_ID_OFFSET 3
#define RANGE_START 1
#define RANGE_STEP 2
#define REG_ADDR_SIZE 4

static u32 *g_sec_iomg_range;
static int g_sec_iomg_range_num;

static u32 *g_sec_iocg_range;
static int g_sec_iocg_range_num;

static bool find_addr_in_range_table(const u32 *table, int table_size, const u32 addr)
{
	int j;

	for (j = RANGE_START; j < table_size; j += RANGE_STEP)
		if (addr >= table[j - 1] && addr <= table[j])
			return true;
	return false;
}

static bool is_sec_iomg(struct iocfg_lp *iocfg)
{
	if (g_sec_iomg_range_num <= 0 || g_sec_iomg_range == NULL)
		return false;

	if (find_addr_in_range_table(g_sec_iomg_range,
				     g_sec_iomg_range_num,
				     iocfg->iomg_base + iocfg->iomg_offset))
		return true;

	return false;
}

static const char *gpio_func(u32 func, const struct iocfg_lp *iocfg)
{
	if (func == FUNC0)
		return iocfg->gpio_func0;
	if (func == FUNC1)
		return iocfg->gpio_func1;
	if (func == FUNC2)
		return iocfg->gpio_func2;
	if (func == FUNC3)
		return iocfg->gpio_func3;

	return "";
}

#define gpio_mismatch(current, target) \
	((target != -1) && current != target)

static void show_iomg_info(struct seq_file *s, struct iocfg_lp *iocfg,
			   const int iomg_func_width, const int iomg_error_width)
{
	u32 gpio_func_act;
	int gpio_func_cfg;
	void __iomem *addr = NULL;
	char buf_func[64] = { 0 };
	char buf_error[64] = { 0 };
	const int iomg_info_len = iomg_func_width + iomg_error_width;

	if (iocfg->iomg_offset == -1) {
		lp_msg_cont(s, "%*s", iomg_info_len, "null");
		return;
	}

	if (is_sec_iomg(iocfg)) {
		lp_msg_cont(s, "%*s", iomg_info_len, "sec iomg");
		return;
	}

	addr = ioremap(iocfg->iomg_base + iocfg->iomg_offset, REG_ADDR_SIZE);
	if (addr == NULL) {
		lp_msg_cont(s, "%*s", iomg_info_len, "ioremap fail");
		return;
	}
	gpio_func_act = readl(addr);
	iounmap(addr);

	if (sprintf_s(buf_func, sizeof(buf_func), "Func-%d(%s)", gpio_func_act, gpio_func(gpio_func_act, iocfg)) < 0)
		lp_err("failed for format iomg func");
	lp_msg_cont(s, "%*s", iomg_func_width, buf_func);

	gpio_func_cfg = iocfg->iomg_val;
	if (gpio_mismatch((int)gpio_func_act, gpio_func_cfg) &&
	    (sprintf_s(buf_error, sizeof(buf_error), "-E [Func-%d](%s)",
		      gpio_func_cfg, gpio_func(gpio_func_cfg, iocfg)) < 0))
		lp_err("failed for format iomg error info");

	lp_msg_cont(s, "%*s", iomg_func_width, buf_error);
}

static bool is_sec_iocg(struct iocfg_lp *iocfg)
{
	if (g_sec_iocg_range_num <= 0 || g_sec_iocg_range == NULL)
		return false;

	if (find_addr_in_range_table(g_sec_iocg_range,
				     g_sec_iocg_range_num,
				     iocfg->iocg_base + iocfg->iocg_offset))
		return true;

	return false;
}

#define IOCG_MASK		0x3
#define iocg_mismatch(current, target) \
	((target != N) && (current & IOCG_MASK) != target)

static void show_iocg_reg(struct seq_file *s, struct iocfg_lp *iocfg, const int iocg_width, const int iocg_error_width)
{
	u32 value;
	void __iomem *addr = NULL;
	char buf_error[16] = { 0 };
	const int iocg_info_len = iocg_width + iocg_error_width;

	if (is_sec_iocg(iocfg)) {
		lp_msg_cont(s, "%*s", iocg_info_len, "sec iocg");
		return;
	}

	addr = ioremap(iocfg->iocg_base + iocfg->iocg_offset, REG_ADDR_SIZE);
	if (addr == NULL) {
		lp_msg_cont(s, "%*s", iocg_info_len, "ioremap fail");
		return;
	}
	value = readl(addr);
	iounmap(addr);
	lp_msg_cont(s, "%*s", iocg_width, pulltype[value & IOCG_MASK]);

	if (iocg_mismatch(value, iocfg->iocg_val) &&
	    sprintf_s(buf_error, sizeof(buf_error), "-E [%s]", pulltype[iocfg->iocg_val]) < 0)
		lp_err("failed for format iocg err info");

	lp_msg_cont(s, "%*s", iocg_error_width, buf_error);
}

#define gpio_dir(x)		((x) + 0x400)
#define gpio_data(x, y)		((x) + (1 << (2 + (y))))
#define gpio_is_set(x, y)		(((x) >> (y)) & 0x1)

/* gpio is defined in gpio.dtsi, and now is max 38 groups, its index is from 0 to 37 */
static void __iomem **g_gpio_group_base;
static int g_lp_gpio_group_num;
#define is_sec_gpio(addr) ((addr) == NULL)

static const char *gpio_direction(u32 direction)
{
	if (direction == IN)
		return "In";
	if (direction == OUT)
		return "Out";
	if (direction == N)
		return "N";
	return "Err";
}

static const char *gpio_level(u32 level)
{
	if (level == LOW)
		return "Low";
	if (level == HIGH)
		return "High";
	if (level == N)
		return "N";
	return "Err";
}

static void show_gpio_info(struct seq_file *s, struct iocfg_lp *iocfg,
			   const int func0_width, const int func0_error_width)
{
	u32 direction, level;
	void __iomem *base = NULL;
	char func0_current[32] = { 0 };
	char func0_target[32] = { 0 };
	const int gpio_info_len = func0_width + func0_width + func0_error_width;

	base = g_gpio_group_base[iocfg->gpio_group_id];
	if (is_sec_gpio(base)) {
		lp_msg_cont(s, "%*s", gpio_info_len, "sec gpio");
		return;
	}

	/* FUNC0: multiplexed as the GPIO function */
	if (iocfg->iomg_val != FUNC0)
		return;

	direction = gpio_is_set(readl(gpio_dir(base)), iocfg->ugpio_bit);
	level = gpio_is_set(readl(gpio_data(base, iocfg->ugpio_bit)), iocfg->ugpio_bit);
	if (sprintf_s(func0_current, sizeof(func0_current), "%s/%s", gpio_direction(direction), gpio_level(level)) < 0)
		lp_err("failed for format gpio func0 current");
	lp_msg_cont(s, "%*s", func0_width, func0_current);

	if (sprintf_s(func0_target, sizeof(func0_target), "[%s/%s]",
			gpio_direction(iocfg->gpio_dir),
			gpio_level(iocfg->gpio_val)) < 0)
		lp_err("failed for format gpio func0 target");

	lp_msg_cont(s, "%*s", func0_width, func0_target);

	if (direction == OUT && level != iocfg->gpio_val)
		lp_msg_cont(s, "%*s", func0_error_width, "-E");
}

static const int align_left = -1;
static const int gpio_id_width = 8 * align_left;
static const int gpio_logic_width = 40 * align_left;
static const int iomg_func_width = 32 * align_left;
static const int iomg_error_width = 32 * align_left;
static const int iocg_width = 16 * align_left;
static const int iocg_error_width = 16 * align_left;
static const int func0_width = 16 * align_left;
static const int func0_error_width = 16 * align_left;
static int io_status_show_inner(struct seq_file *s, void *data)
{
	unsigned int i, gpio_id;
	struct iocfg_lp *iocfg = NULL;

	no_used(data);

	if (lp_is_fpga()) {
		lp_msg(s, "gpio not support fpga");
		return -EPERM;
	}

	if (g_lp_iocfg_table == NULL) {
		lp_msg(s, "canot find iocfg table");
		return -ENODEV;
	}

	for (i = 0; g_lp_iocfg_table[i].iomg_offset != CFG_END; i++)
		;
	lp_msg(s, "io list length is %u :", i);
	lp_msg(s, "%*s%*s%*s%*s%*s%*s%*s%*s%*s",
		gpio_id_width, "gpio-id",
		gpio_logic_width, "gpio_logic",
		iomg_func_width, "iomg-current",
		iomg_error_width, "iomg-target",
		iocg_width, "iocg-current",
		iocg_error_width, "iocg-target",
		func0_width, "func0-current",
		func0_error_width, "func0-target",
		func0_error_width, "func0-error"
		);

	for (i = 0; g_lp_iocfg_table[i].iomg_offset != CFG_END; i++) {
		iocfg = &g_lp_iocfg_table[i];
		gpio_id = (iocfg->gpio_group_id << GPIO_GROUP_ID_OFFSET) +
			  iocfg->ugpio_bit;

		lp_msg_cont(s, "%*d%*s",
			       gpio_id_width, gpio_id, gpio_logic_width, iocfg->gpio_logic);

		show_iomg_info(s, iocfg, iomg_func_width, iomg_error_width);
		show_iocg_reg(s, iocfg, iocg_width, iocg_error_width);
		show_gpio_info(s, iocfg, func0_width, func0_error_width);

		lp_msg_cont(s, "\n");
	}

	return 0;
}

static int io_status_show(void)
{
	return io_status_show_inner(NO_SEQFILE, NULL);
}

static const struct lowpm_debugdir g_lpwpm_debugfs_io = {
	.dir = "lowpm_func",
	.files = {
		{"io_status", 0400, io_status_show_inner, NULL},
		{},
	},
};

static struct sr_mntn g_sr_mntn_io = {
	.owner = "io_status",
	.enable = false,
	.suspend = io_status_show,
	.resume = NULL,
};

static int init_sec_iocg(const struct device_node *lp_dn)
{
	g_sec_iocg_range_num = lp_read_property_u32_array(lp_dn, "cg-sec-reg", &g_sec_iocg_range);
	if (g_sec_iocg_range_num <= 0)
		return -ENODEV;

	return 0;
}

static int init_sec_iomg(const struct device_node *lp_dn)
{
	g_sec_iomg_range_num = lp_read_property_u32_array(lp_dn, "mg-sec-reg", &g_sec_iomg_range);
	if (g_sec_iomg_range_num <= 0)
		return -ENODEV;

	return 0;
}

#define GPIO_DTSI_COMPATIBLE "arm,pl061"
static int init_gpio_group_table(void)
{
	int i, ret, sec_mode;
	struct device_node *dn = NULL;

	g_lp_gpio_group_num = 0;
	for_each_compatible_node(dn, NULL, GPIO_DTSI_COMPATIBLE)
		g_lp_gpio_group_num++;

	lp_info("find lp gpio group num: %u", g_lp_gpio_group_num);

	g_gpio_group_base = kcalloc(g_lp_gpio_group_num, sizeof(*g_gpio_group_base), GFP_KERNEL);
	if (g_gpio_group_base == NULL)
		goto err;

	dn = NULL;
	i = 0;
	for_each_compatible_node(dn, NULL, GPIO_DTSI_COMPATIBLE) {
		sec_mode = 0;
		ret = of_property_read_u32(dn, "secure-mode", &sec_mode);
		if (sec_mode != 0)
			g_gpio_group_base[i++] = NULL;
		else
			g_gpio_group_base[i++] = of_iomap(dn, 0);
	}

	return 0;

err:
	for (i = 0; (i < g_lp_gpio_group_num) && unlikely(g_gpio_group_base != NULL); i++)
		_lowpm_free(g_gpio_group_base[i], iounmap);
	lowpm_kfree(g_gpio_group_base);
	g_lp_gpio_group_num = 0;

	return -EFAULT;
}

static int init_io_table(const struct device_node *lp_dn)
{
	int ret;

	ret = init_gpio_group_table();
	if (ret != 0) {
		lp_err("map gpio base failed");
		goto err;
	}

	ret = init_sec_iomg(lp_dn);
	if (ret < 0) {
		lp_err("init sec iomg failed");
		goto err;
	}

	ret = init_sec_iocg(lp_dn);
	if (ret < 0) {
		lp_err("init sec iocg failed");
		goto err;
	}

	lp_info("init gpio data success");

	return 0;

err:
	lp_err("init fail");
	return ret;
}

static __init int init_sr_mntn_io(void)
{
	int ret;

	if (sr_unsupported())
		return 0;

	ret = sr_mntn_io_init_plat_info();
	if (ret != 0) {
		lp_err("init plat failed");
		return ret;
	}

	ret = init_io_table(lp_dn_root());
	if (ret != 0) {
		lp_err("init failed");
		return ret;
	}

	ret = lowpm_create_debugfs(&g_lpwpm_debugfs_io);
	if (ret != 0) {
		lp_err("create debug sr file failed");
		return ret;
	}

	ret = register_sr_mntn(&g_sr_mntn_io, SR_MNTN_PRIO_L);
	if (ret != 0) {
		lp_err("register mntn module failed");
		return ret;
	}

	debug_print_platinfo(NO_SEQFILE);

	lp_crit("success");

	return 0;
}

late_initcall(init_sr_mntn_io);
