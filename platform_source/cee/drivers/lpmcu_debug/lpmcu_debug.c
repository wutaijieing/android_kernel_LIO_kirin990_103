/*
 * lpmcu_debug.c
 *
 * debug for lpmcu
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2020. All rights reserved.
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
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/notifier.h>
#include <linux/pm_qos.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#ifdef CONFIG_LPMCU_BB
#include "m3_rdr_ddr_map.h"
#endif
#include <securec.h>

#define MODULE_NAME	"hisilicon,lowpowerm3"
#define MAX_INPUT_CNT	128
#define MAX_CMD	7
#define LPMCU_DEBUG_IPC_CMD_LEN	8
#define LPMCU_DEBUGFS_ARGS_MAX	10
#define VOLT_FREQ_CMD_ARGS	3
#define CLUSTER_FREQ_MAX	0xFFFF
#define CLUSTER_FREQ_MASK	0xFFFF0000
#define PMU_DEBUG_CMD_MAX	4
#define PMU_READ_CMD	0x000F0317
#define PMU_WRITE_CMD	0x000F0318
#define MAX_USE_MSG_LEN	3
#define PERI_VOLT_LEVEL_MAX	4
#define MSG_SIZE	8
#define INQUIRY_RST_OK	0
#define INQUIRY_RST_FAIL	0xFF

#define INQUIRY_CMD_SIZE	32
#define INQUIRY_MSG_CMD	0x00050217

#define INQUIRY_DATA_NUM	6
#define INQUIRY_ID_MAX	1024
#define MAX_OPS	6
#define AMPLIFIER_RATE	10
#define PARAM_LEN_MAX	5
#define	PROFILE_ID_MAX	0X7F
#define VOLT_FREQ_MAX	0xFFFF

enum cluster_volt_bias_type {
	DEFAULT_NO_BIAS = 0,
	POSITIVE_BIAS,
	NEGATIVE_BIAS,
	MAX_VOLT_BIAS_TYPE,
};

struct lpmcu_mbox_work {
	struct hisi_mbox_task *tx_task;
	struct work_struct work;
	struct hisi_mbox *p;
};

union remote_pmu_cmd {
	u32 data;
	struct {
		u16 addr;
		u8 slave;
		u8 value;
	} para;
	struct {
		u16 addr;
		u8 error;
		u8 value;
	} ack_cmd;
};

struct inquiry_info {
	u16 id;
	u8 result;
	u8 data_size;
	u32 data[INQUIRY_DATA_NUM];
};

struct inquiry_msg {
	u32 msg_cmd;
	u16 id;
	u8 result;
	u8 data_size;
	u32 data[INQUIRY_DATA_NUM];
};

struct lpmcu_peri_debug_data {
	u32 vol_kptime;
	u32 vol_cnt;
};

static union remote_pmu_cmd g_pmu_write_ack;
static union remote_pmu_cmd g_pmu_read_ack;
static struct dentry *g_lpmcu_debug_dir;
static struct notifier_block *g_notifier_block;
static unsigned int g_display_mailbox_rec;
const static char *g_cmd[MAX_CMD] = {
	"on",
	"off",
	"get",
	"set",
	"notify",
	"test",
	"nobusiness",
};

const static char *g_obj[] = {
	"ap",
	"little",
	"big",
	"gpu",
	"ddr",
	"asp",
	"hifi",
	"iom3",
	"lpmcu",
	"modem",
	"sys",
	"hkadc",
	"regulator",
	"clock",
	"temp",
	"pmu",
	"psci",
	"telemntn",
	"mca",
	"general_se",
	"test",
	"teeos",
	"mid",
	"L3",
};

#define MAX_OBJ	ARRAY_SIZE(g_obj)

const static char *g_type[] = {
	"power",
	"clk",
	"core",
	"cluster",
	"sleep",
	"sr",
	"mode",
	"uplimit",
	"dnlimit",
	"freq",
	"T",
	"volt",
	"reset",
	"pwc",
	"test",
	"on",
	"bist_cmd",
	"bist_algo",
	"off",
	"current",
	"ddr_time",
	"ddr_flux",
	"read",
	"write",
	"save_track",
	"ddr_volt_change",
	"battery",
	"topfreq",
};

#define MAX_TYPE	ARRAY_SIZE(g_type)

const static char *g_cluster_obj[] = {
	"little",
	"big",
	"gpu",
	"gpumem",
	"middle",
	"l3",
	"cpumem"
};

#define CLUSTER_OBJ_NUM	ARRAY_SIZE(g_cluster_obj)

struct inquiry_info g_inquiry_info = {
	INQUIRY_ID_MAX,
	INQUIRY_RST_FAIL,
	0,
	{0}
};

const static char *g_fop_str[MAX_OPS] = {
	"lpmcu_test",
	"cluster_volt",
	"cluster_freq",
	"pmu_debug",
	"peri_debug",
	"lpmcu_info",
};

static int mbox_mbox_notifier(struct notifier_block *str_nb,
				   unsigned long len, void *msg)
{
	unsigned int *_msg = (unsigned int *)msg;
	unsigned long i;

	(void)str_nb;
	if (g_display_mailbox_rec != 0) {
		pr_info("%s: receive mail\n", MODULE_NAME);
		for (i = 0; i < len; i++)
			pr_info("msg[%d] = 0x%x\n", (int)i, _msg[i]);
	}

	return 0;
}

static int lpmcu_debugfs_show(struct seq_file *s, void *data)
{
	(void)data;
	(void)s;
	pr_debug("%s: %s\n", MODULE_NAME, __func__);
	return 0;
}

static int lpmcu_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, lpmcu_debugfs_show, inode->i_private);
}

static int parse_para_to_buf(char *para_cmd, unsigned int cmd_len,
			     char *argv[], int max_args)
{
	int para_id = 0;

	(void)cmd_len;
	while (*para_cmd != '\0') {
		if (para_id >= max_args)
			break;
		while (*para_cmd == ' ')
			para_cmd++;

		if (*para_cmd == '\0')
			break;

		argv[para_id] = para_cmd;
		para_id++;

		while ((*para_cmd != ' ') && (*para_cmd != '\0'))
			para_cmd++;

		if (*para_cmd == '\0')
			break;

		*para_cmd = '\0';
		para_cmd++;
	}
	return para_id;
}

static ssize_t lpmcu_debugfs_write(struct file *filp,
				       const char __user *buf,
				       size_t count,
				       loff_t *ppos)
{
	char debug_cmd[MAX_INPUT_CNT] = {0};
	char *argv[LPMCU_DEBUGFS_ARGS_MAX] = {0};
	u32 result = 0;
	u32 argc;
	u32 msg[LPMCU_DEBUG_IPC_CMD_LEN] = {0};
	u32 i, j;
	s32 ret;
	rproc_id_t rproc_id = IPC_ACPU_LPM3_MBX_4;

	(void)filp;
	(void)ppos;
	if (buf == NULL) {
		pr_err("error, buf is null\n");
		goto msg_err;
	}
	ret = (s32)copy_from_user(debug_cmd, buf,
				  min_t(size_t, sizeof(debug_cmd) - 1, count));
	if (ret != 0) {
		pr_err("%s: copy buffer failed.\n", MODULE_NAME);
		goto msg_err;
	}
	pr_info("%s: [cmd: %s[count: %d]]\n", MODULE_NAME, debug_cmd,
		(int)count);
	/* debug max args is 10 */
	argc = (u32)parse_para_to_buf(debug_cmd, MAX_INPUT_CNT, argv, 10);
	/* max param is 3 */
	if (argc < 3) {
		pr_err("error, arg too few\n");
		goto msg_err;
	}
	for (i = 0; i < MAX_OBJ; i++) {
		if (strncmp(g_obj[i], argv[0], strlen(g_obj[i])) == 0) {
			/* cmd obj set */
			msg[0] |= (i << 16);
			break;
		}
	}
	if (i == MAX_OBJ) {
		pr_err("error, no such obj\n");
		goto msg_err;
	}
	for (i = 0; i < MAX_TYPE; i++) {
		if (strncmp(g_type[i], argv[1], strlen(g_type[i])) == 0) {
			msg[0] |= (i + 1);
			break;
		}
	}
	if (i == MAX_TYPE) {
		pr_err("error, no such cmd type\n");
		goto msg_err;
	}
	for (i = 0; i < MAX_CMD; i++) {
		/* ipc cmd set */
		if (strncmp(g_cmd[i], argv[2], strlen(g_cmd[i])) == 0) {
			msg[0] |= i << LPMCU_DEBUG_IPC_CMD_LEN;
			break;
		}
	}
	if (i == MAX_CMD) {
		pr_err("error, no such cmd\n");
		goto msg_err;
	}
	j = 1;
	/* ipc msg set */
	for (i = 3; i < argc; i++) {
		ret = kstrtou32(argv[i], 0, &result);
		if (ret != 0)
			goto msg_err;
		msg[j++] = result;
	}
	for (i = 0; i < LPMCU_DEBUG_IPC_CMD_LEN; i++)
		pr_info("0x%x\n", msg[i]);
	pr_info("%s: %s\n", MODULE_NAME, debug_cmd);
	ret = RPROC_ASYNC_SEND(rproc_id, (mbox_msg_t *)msg,
			       LPMCU_DEBUG_IPC_CMD_LEN);
	if (ret != 0)
		pr_err(" %s , line %d, send error\n", __func__, __LINE__);
msg_err:
	return count;
}

static u32 amplifier_parse(char *argv[VOLT_FREQ_CMD_ARGS])
{
	u32 i, len;
	u32 amplifier = 1;
	/* argv current index */
	u32 idx = 2;

	len = strlen(argv[idx]);
	for (i = 0; i < len; i++)
		if (argv[idx][i] == '.')
			break;

	/* 2,3,4:param indexes */
	if ((i == 3 || i == 4) && (len - i) == 2) {
		argv[idx][i] = argv[idx][i + 1];
		argv[idx][i + 1] = '\0';
	} else if (i == 1 && len <= PARAM_LEN_MAX) {
		if (argv[idx][0] == '0') {
			/* skip '0x' */
			argv[idx] += 2;
		} else {
			argv[idx][1] = argv[idx][0];
			argv[idx]++;
		}
		while (len < PARAM_LEN_MAX) {
			amplifier *= AMPLIFIER_RATE;
			len++;
		}
	}
	return amplifier;
}

static int volt_freq_cmd_parse(char *debug_cmd, u32 cmd_len,
			       u32 *msg, u32 msg_len)
{
	char *argv[VOLT_FREQ_CMD_ARGS] = {0};
	u32 argc, i, amplifier;
	u32 result = 0;
	u8 volt_bias_type = DEFAULT_NO_BIAS;
	int ret;
	s32 cmp_result;

	argc = (u32)parse_para_to_buf(debug_cmd, cmd_len, argv,
				      VOLT_FREQ_CMD_ARGS);
	if (argc != VOLT_FREQ_CMD_ARGS) {
		pr_err("error, arg number not right\n");
		return -EINVAL;
	}

	if (msg_len < MAX_USE_MSG_LEN) {
		pr_err("error, msg len not right\n");
		return -EINVAL;
	}

	for (i = 0; i < CLUSTER_OBJ_NUM; i++) {
		cmp_result = strncmp(g_cluster_obj[i], argv[0],
				     strlen(g_cluster_obj[i]));
		if (strlen(argv[0]) == strlen(g_cluster_obj[i]) &&
		    cmp_result == 0) {
			/* bit[15:8] used to store cluster ID */
			msg[1] |= i << LPMCU_DEBUG_IPC_CMD_LEN;
			break;
		}
	}

	if (i == CLUSTER_OBJ_NUM) {
		pr_err("error, no such cmd\n");
		return -EINVAL;
	}

	ret = kstrtou32(argv[1], 0, &result);
	if (ret != 0 || result > PROFILE_ID_MAX) {
		pr_err("para out of range\n");
		return -EINVAL;
	}
	/* bit[7:0] used to store profile ID */
	msg[1] |= result;

	/* symbol parse */
	if (argv[2][0] == '+') {
		volt_bias_type = POSITIVE_BIAS;
		argv[2]++;
	} else if (argv[2][0] == '-') {
		volt_bias_type = NEGATIVE_BIAS;
		argv[2]++;
	}
	/* bias type set */
	msg[2] |= volt_bias_type << 14;
	amplifier = amplifier_parse(argv);
	ret = kstrtou32(argv[2], 0, &result);
	result *= amplifier;
	if (ret != 0 || result > VOLT_FREQ_MAX) {
		pr_err("para out of range\n");
		return -EINVAL;
	}
	/* freq set */
	msg[2] |= result;

	return 0;
}

static ssize_t lpmcu_cluster_volt_write(struct file *filp,
					    const char __user *buf,
					    size_t count,
					    loff_t *ppos)
{
	char debug_cmd[MAX_INPUT_CNT] = {0};
	u32 msg[MSG_SIZE] = {0};
	s32 i, ret;
	rproc_id_t rproc_id = IPC_ACPU_LPM3_MBX_4;

	(void)filp;
	(void)ppos;
	if (buf == NULL) {
		pr_err("error, buf is null\n");
		goto msg_err;
	}
	ret = (s32)copy_from_user(debug_cmd, buf,
				  min_t(size_t, sizeof(debug_cmd) - 1, count));
	if (ret != 0) {
		pr_err("%s: copy buffer failed.\n", MODULE_NAME);
		goto msg_err;
	}

	/* cmd param set */
	msg[1] |= (11 << 24);
	/* cmd_type cmd cmd_obj cmd_src set */
	msg[0] = 0x0008030F;

	pr_info("%s: [cmd: %s[count: %d]]\n", MODULE_NAME, debug_cmd,
		(int)count);
	ret = volt_freq_cmd_parse(debug_cmd, MAX_INPUT_CNT, msg, MSG_SIZE);
	if (ret != 0)
		goto msg_err;

	for (i = 0; i < LPMCU_DEBUG_IPC_CMD_LEN; i++)
		pr_info("0x%x\n", msg[i]);

	pr_info("%s: %s\n", MODULE_NAME, debug_cmd);

	ret = RPROC_ASYNC_SEND(rproc_id, (mbox_msg_t *)msg,
			       LPMCU_DEBUG_IPC_CMD_LEN);
	if (ret != 0)
		pr_err(" %s , line %d, send error\n", __func__, __LINE__);

msg_err:
	return count;
}

static ssize_t lpmcu_cluster_freq_write(struct file *filp,
					    const char __user *buf,
					    size_t count,
					    loff_t *ppos)
{
	char debug_cmd[MAX_INPUT_CNT] = {0};
	u32 msg[MSG_SIZE] = {0};
	u32 i;
	s32 ret;
	rproc_id_t rproc_id = IPC_ACPU_LPM3_MBX_4;

	(void)filp;
	(void)ppos;
	if (buf == NULL) {
		pr_err("error, buf is null\n");
		goto msg_err;
	}
	ret = (s32)copy_from_user(debug_cmd, buf,
				  min_t(size_t, sizeof(debug_cmd) - 1, count));
	if (ret != 0) {
		pr_err("%s: copy buffer failed.\n", MODULE_NAME);
		goto msg_err;
	}

	/* cmd param set */
	msg[1] |= 12 << 24;
	/* cmd_type cmd cmd_obj cmd_src set */
	msg[0] = 0x0008030F;

	pr_info("%s: [cmd: %s[count: %d]]\n", MODULE_NAME, debug_cmd,
		(int)count);
	ret = volt_freq_cmd_parse(debug_cmd, MAX_INPUT_CNT, msg, MSG_SIZE);
	if (ret != 0)
		goto msg_err;

	/* freq value parse */
	i = msg[2] & 0xFFFF;
	/* if freq < 8000, freq *= 10 */
	if (i < 8000)
		msg[2] = (msg[2] & CLUSTER_FREQ_MASK) | (i * AMPLIFIER_RATE);

	for (i = 0; i < LPMCU_DEBUG_IPC_CMD_LEN; i++)
		pr_info("0x%x\n", msg[i]);
	pr_info("%s: %s\n", MODULE_NAME, debug_cmd);
	ret = RPROC_ASYNC_SEND(rproc_id, (mbox_msg_t *)msg,
			       LPMCU_DEBUG_IPC_CMD_LEN);
	if (ret != 0)
		pr_err(" %s , line %d, send error\n", __func__, __LINE__);
msg_err:
	return count;
}

static int lpmcu_pmu_debug_show(struct seq_file *s, void *data)
{
	(void)data;
	seq_puts(s, "read info:\n");
	seq_printf(s, "\t addr:0x%x\n", g_pmu_read_ack.ack_cmd.addr);
	seq_printf(s, "\t value:0x%x\n", g_pmu_read_ack.ack_cmd.value);
	seq_printf(s, "\t state:%s\n",
		   (g_pmu_read_ack.ack_cmd.error == 0) ? "success" : "fail");

	seq_puts(s, "write info:\n");
	seq_printf(s, "\t addr:0x%x\n", g_pmu_write_ack.ack_cmd.addr);
	seq_printf(s, "\t current value:0x%x\n", g_pmu_write_ack.ack_cmd.value);
	seq_printf(s, "\t state:%s\n",
		   (g_pmu_write_ack.ack_cmd.error == 0) ? "success" : "fail");

	return 0;
}

static int lpmcu_pmu_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, lpmcu_pmu_debug_show, inode->i_private);
}

static int check_pmu_para(u32 slave, u32 addr, u32 value)
{
	/* slave, addr, value range check */
	if (slave > 0xFF || addr > 0xFFFF || value > 0xFF)
		return -EINVAL;

	return 0;
}

/*
 * Function name: pmu_debug
 * Description  :
 *                cmd0: write/read
 *                cmd1: slave id
 *                cmd2: addr
 *                cmd3: value(if cmd0 is write)
 *                read pmu:0x00080F17
 *                write pmu:0x00080F18
 * Parameters   : struct file  *filp :
 *                const char  __user :
 *                size_t  count :
 *                loff_t  *ppos :
 * Returns      : ssize_t
 */
static ssize_t lpmcu_pmu_debug(struct file *filp, const char __user *buf,
			      size_t count,
			      loff_t *ppos)
{
	char debug_cmd[MAX_INPUT_CNT] = {0};
	char *argv[PMU_DEBUG_CMD_MAX] = {0};
	u32 argc;
	rproc_msg_t tx_buf[LPMCU_DEBUG_IPC_CMD_LEN] = {PMU_READ_CMD};
	/* ack buffer init */
	rproc_msg_t ack_buf[2] = {0};
	int ret;
	u32 slave = 0;
	u32 addr = 0;
	u32 value = 0;
	union remote_pmu_cmd pmu_cmd;

	(void)filp;
	(void)ppos;
	if (buf == NULL) {
		pr_err("error, buf is null\n");
		goto err_handle;
	}
	ret = (s32)copy_from_user(debug_cmd, buf,
				  min_t(size_t, sizeof(debug_cmd) - 1, count));
	if (ret != 0) {
		pr_err("%s: copy buffer failed.\n", __func__);
		goto err_handle;
	}
	argc = (u32)parse_para_to_buf(debug_cmd, MAX_INPUT_CNT, argv,
				      PMU_DEBUG_CMD_MAX);
	if (argc == 0) {
		pr_err("%s: arg too few\n", __func__);
		goto err_handle;
	}
	ret = -EINVAL;
	/* cmd param: argv[0-3] : read/write, slave, addr, value */
	if (strncmp("write", argv[0], strlen("write")) == 0) {
		/* write mode parm check */
		if (argc != 4) {
			pr_err("%s: write must have 4 parameters\n", __func__);
			goto err_handle;
		}
		/* slave, addr, value parse */
		ret = kstrtou32(argv[1], 0, &slave);
		ret += kstrtou32(argv[2], 0, &addr);
		ret += kstrtou32(argv[3], 0, &value);
		tx_buf[0] = PMU_WRITE_CMD;
	} else if (strncmp("read", argv[0], strlen("read")) == 0) {
		/* read mode parm check */
		if (argc != 3) {
			pr_err("%s: read must have 3 parameters\n", __func__);
			goto err_handle;
		}
		ret = kstrtou32(argv[1], 0, &slave);
		ret += kstrtou32(argv[2], 0, &addr);
		tx_buf[0] = PMU_READ_CMD;
	}
	if (ret != 0 || check_pmu_para(slave, addr, value) != 0) {
		pr_err("%s: parameter overflow\n", __func__);
		goto err_handle;
	}
	pmu_cmd.para.addr = addr;
	pmu_cmd.para.slave = slave;
	pmu_cmd.para.value = value;
	tx_buf[1] = pmu_cmd.data;
	ret = RPROC_SYNC_SEND(IPC_ACPU_LPM3_MBX_2,
			      tx_buf, sizeof(tx_buf) / sizeof(rproc_msg_t),
			      ack_buf, sizeof(ack_buf) / sizeof(rproc_msg_t));

	if (ret != 0 || ack_buf[0] != tx_buf[0]) {
		pr_err("%s: ack error\n", __func__);
		goto err_handle;
	}
	pmu_cmd.data = ack_buf[1];
	if (ack_buf[0] == PMU_WRITE_CMD)
		g_pmu_write_ack.data = pmu_cmd.data;
	else
		g_pmu_read_ack.data = pmu_cmd.data;
	if (pmu_cmd.ack_cmd.error != 0) {
		pr_err("%s: [%s] execute fail\n", __func__, debug_cmd);
		goto err_handle;
	}
	pr_err("%s: 0x%x->0x%x\n", __func__, pmu_cmd.ack_cmd.addr,
	       pmu_cmd.ack_cmd.value);
err_handle:
	return count;
}

static int lpmcu_peri_debug_show(struct seq_file *s, void *data)
{
#if defined(M3_RDR_PERIAVS_PARA_ADDR) && defined(CONFIG_LPMCU_BB)
	struct lpmcu_peri_debug_data *peri_data = NULL;
	unsigned int i;

	(void)data;
	if (M3_RDR_SYS_CONTEXT_BASE_ADDR == 0)
		return 0;
	peri_data = (struct lpmcu_peri_debug_data *)M3_RDR_PERIAVS_PARA_ADDR;
	for (i = 0; i < PERI_VOLT_LEVEL_MAX; i++)
		seq_printf(s, "VOL_LEVEL%u keepslice: 0x%x cnt:%u\n", i,
			   readl((void __iomem *)(&peri_data[i].vol_kptime)),
			   readl((void __iomem *)(&peri_data[i].vol_cnt)));
#endif
	return 0;
}

static int lpmcu_peri_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, lpmcu_peri_debug_show, inode->i_private);
}

static int lpmcu_inquiry_show(struct seq_file *s, void *data)
{
	unsigned int i;

	seq_printf(s, "lpmcu info: %s\n",
		   (g_inquiry_info.result == INQUIRY_RST_OK) ? "ok" : "fail");
	seq_printf(s, "\t id - %d\n", g_inquiry_info.id);
	seq_puts(s, "\t data -");
	for (i = 0; i < g_inquiry_info.data_size; i++)
		seq_printf(s, " 0x%x", g_inquiry_info.data[i]);
	seq_puts(s, "\n");

	return 0;
}

static int lpmcu_inquiry_open(struct inode *inode, struct file *file)
{
	return single_open(file, lpmcu_inquiry_show, inode->i_private);
}

static ssize_t lpmcu_inquiry_write(struct file *filp,
					const char __user *buf,
					size_t count,
					loff_t *ppos)
{
	char inquiry_cmd[INQUIRY_CMD_SIZE] = {0};
	struct inquiry_msg tx_msg = {INQUIRY_MSG_CMD};
	struct inquiry_msg ack_msg = {INQUIRY_MSG_CMD};
	unsigned int inquiry_id = 0;
	unsigned int i;
	int ret;

	(void)filp;
	(void)ppos;
	if (buf == NULL) {
		pr_err("error, buf is null\n");
		goto err_handle;
	}
	ret = (s32)copy_from_user(inquiry_cmd, buf,
				  min_t(size_t, sizeof(inquiry_cmd) - 1,
					count));
	if (ret != 0) {
		pr_err("%s: copy buffer failed.\n", __func__);
		goto err_handle;
	}
	ret = kstrtou32(inquiry_cmd, 0, &inquiry_id);
	if (ret != 0) {
		pr_err("%s: cmd error\n", __func__);
		goto err_handle;
	}
	if (inquiry_id >= INQUIRY_ID_MAX) {
		pr_err("%s: inquiry_id%u > max%d overflow\n", __func__,
		       inquiry_id, INQUIRY_ID_MAX);
		goto err_handle;
	}

	tx_msg.id = inquiry_id;
	ret = RPROC_SYNC_SEND(IPC_ACPU_LPM3_MBX_2,
			      (rproc_msg_t *)&tx_msg,
			      sizeof(tx_msg) / sizeof(rproc_msg_t),
			      (rproc_msg_t *)&ack_msg,
			      sizeof(ack_msg) / sizeof(rproc_msg_t));

	if (ret != 0 || ack_msg.msg_cmd != tx_msg.msg_cmd ||
	    ack_msg.id != tx_msg.id || ack_msg.result != INQUIRY_RST_OK) {
		pr_err("%s: ack error\n", __func__);
		goto err_handle;
	}
	g_inquiry_info.id = ack_msg.id;
	g_inquiry_info.result = ack_msg.result;
	g_inquiry_info.data_size = (ack_msg.data_size >= INQUIRY_DATA_NUM) ?
				   INQUIRY_DATA_NUM : ack_msg.data_size;
	for (i = 0; i < g_inquiry_info.data_size; i++)
		g_inquiry_info.data[i] = ack_msg.data[i];

err_handle:
	return count;
}

static const struct file_operations g_lpmcu_debugfs_fops[MAX_OPS] = {
	{
		.open = lpmcu_debugfs_open,
		.read = seq_read,
		.write = lpmcu_debugfs_write,
		.llseek = seq_lseek,
		.release = single_release,
	},
	{
		.open = lpmcu_debugfs_open,
		.read = seq_read,
		.write = lpmcu_cluster_volt_write,
		.llseek = seq_lseek,
		.release = single_release,
	},
	{
		.open = lpmcu_debugfs_open,
		.read = seq_read,
		.write = lpmcu_cluster_freq_write,
		.llseek = seq_lseek,
		.release = single_release,
	},
	{
		.open = lpmcu_pmu_debug_open,
		.read = seq_read,
		.write = lpmcu_pmu_debug,
		.llseek = seq_lseek,
		.release = single_release,
	},
	{
		.open = lpmcu_peri_debugfs_open,
		.read = seq_read,
		.write = lpmcu_debugfs_write,
		.llseek = seq_lseek,
		.release = single_release,
	},
	{
		.open = lpmcu_inquiry_open,
		.read = seq_read,
		.write = lpmcu_inquiry_write,
		.llseek = seq_lseek,
		.release = single_release,
	},
};

static int __init lpmcu_init(void)
{
	int i;
	int ret = 0;
	rproc_id_t rproc_id = IPC_LPM3_ACPU_MBX_1;
	struct dentry *pfile = NULL;

	g_display_mailbox_rec = 0;
	g_notifier_block = (struct notifier_block *)kzalloc(sizeof(struct notifier_block),
						GFP_KERNEL);
	if (g_notifier_block == NULL) {
		ret = -ENOMEM;
		goto err_alloc_nb;
	}

	g_notifier_block->next = NULL;
	g_notifier_block->notifier_call = mbox_mbox_notifier;
	/* register the rx notify callback */
	ret = RPROC_MONITOR_REGISTER(rproc_id, g_notifier_block);
	if (ret != 0) {
		pr_info("%s:RPROC_MONITOR_REGISTER failed", __func__);
		goto err_alloc_nb_nor;
	}

#ifdef CONFIG_DEBUG_FS
	g_lpmcu_debug_dir = debugfs_create_dir("lpmcu_debug", NULL);
	if (g_lpmcu_debug_dir == NULL) {
		ret = -ENODEV;
		goto err_alloc_nb_nor;
	}
	for (i = 0; i < MAX_OPS; i++) {
		/* 0660 : mode = S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP */
		pfile = debugfs_create_file(g_fop_str[i], 0660,
					    g_lpmcu_debug_dir, NULL,
					    &g_lpmcu_debugfs_fops[i]);
		if (pfile == NULL) {
			debugfs_remove_recursive(g_lpmcu_debug_dir);
			ret = -ENODEV;
			goto err_alloc_nb_nor;
		}
	}
#endif

	return 0;

err_alloc_nb_nor:
	kfree(g_notifier_block);
	g_notifier_block = NULL;
err_alloc_nb:
	return ret;
}
fs_initcall(lpmcu_init);

static void __exit lpmcu_exit(void)
{
	rproc_id_t rproc_id = IPC_LPM3_ACPU_MBX_1;

	/*
	 * the RPROC_LPM3 is a shared channel by many IP,
	 * but the exit function should never be used
	 */
	RPROC_PUT(rproc_id);
	debugfs_remove_recursive(g_lpmcu_debug_dir);
	kfree(g_notifier_block);
	g_notifier_block = NULL;
}
module_exit(lpmcu_exit);

MODULE_AUTHOR("wangtao");
MODULE_DESCRIPTION("LOWPOWER M3 DEBUG DRIVER");
MODULE_LICENSE("GPL v2");
