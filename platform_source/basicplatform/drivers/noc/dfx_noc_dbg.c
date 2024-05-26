/*
 *
 * NoC. (NoC Mntn Module.)
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/syscore_ops.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/string.h>
#include <platform_include/basicplatform/linux/util.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <securec.h>
#include "dfx_noc.h"
#include "dfx_noc_dbg.h"
#include "dfx_noc_packet.h"
#include "dfx_noc_transcation.h"
#include "dfx_noc_err_probe.h"
#define PR_LOG_TAG NOC_TAG
#define NOC_DBG_STRING_SIZE 256

#define M_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#ifdef CONFIG_DFX_DEBUG_FS
static struct dentry *noc_dbg_file_dentry;
#endif

static const struct noc_reg_dbg tbl_noc_dbg_reg[] = {
	/* p_reg_name    node_name      reg_len offset  rw_flag */
	{"DDRC_PACKET_FILTERLUT", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_FILTERLUT, NOC_B_RW},
	{"DDRC_PACKET_STATPERIOD", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_STATPERIOD, NOC_B_RW},
	{"DDRC_PACKET_F0_ROUTEIDBASE", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_F0_ROUTEIDBASE, NOC_B_RW},
	{"DDRC_PACKET_F0_ROUTEIDMASK", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_F0_ROUTEIDMASK, NOC_B_RW},
	{"DDRC_PACKET_F0_SECURITYBASE", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_F0_SECURITYBASE, NOC_B_RW},
	{"DDRC_PACKET_F0_SECURITYMASK", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_F0_SECURITYMASK, NOC_B_RW},
	{"DDRC_PACKET_F0_USERBASE", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_F0_USERBASE, NOC_B_RW},
	{"DDRC_PACKET_F0_USERMASK", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_F0_USERMASK, NOC_B_RW},
	{"DDRC_PACKET_COUNTERS_0_ALARMMODE", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_COUNTERS_0_ALARMMODE,
	 NOC_B_RW},
	{"DDRC_PACKET_COUNTERS_0_VAL", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_COUNTERS_0_VAL, NOC_B_R},
	{"DDRC_PACKET_STATALARMMAX", "sysbus_packet_bus", NOC_REG_LEN_32, PACKET_STATALARMMAX, NOC_B_RW},
	{"TRANS_F_USERBASE", "dss0_rd_transcation_bus", NOC_REG_LEN_32, TRANS_F_USERBASE, NOC_B_RW},
	{"TRANS_F_USERMASK", "dss0_rd_transcation_bus", NOC_REG_LEN_32, TRANS_F_USERMASK, NOC_B_RW},
	{"TRANS_P_PRESCALER", "dss0_rd_transcation_bus", NOC_REG_LEN_32, TRANS_P_PRESCALER, NOC_B_RW},
	{"TRANS_M_STATALARMMAX", "dss0_rd_transcation_bus", NOC_REG_LEN_32, TRANS_M_STATALARMMAX, NOC_B_RW},
	{"TRANS_P_MODE", "dss0_rd_transcation_bus", NOC_REG_LEN_32, TRANS_P_MODE, NOC_B_RW},
};

static const struct noc_dbg_cmd_lookup tbl_noc_dbg_cmd[NOC_DBG_CMD_SIZE] = {
/*   str                        cmd                     */
	{"get", E_NOC_DBG_CMD_RD_REG},
	{"set", E_NOC_DBG_CMD_WR_REG},
	{"packet_enable", E_NOC_DBG_CMD_PKT_ENABLE},
	{"packet_disable", E_NOC_DBG_CMD_PKT_DISABLE},
	{"transcation_enable", E_NOC_DBG_CMD_TRANS_ENABLE},
	{"transcation_disable", E_NOC_DBG_CMD_TRANS_DISABLE}
};

static enum e_noc_dbg_cmd noc_dbg_get_cmd_typ(const char *kn_buf, char (*reg_str)[NOC_CMD_CELL_SIZE], uint *reg_value)
{
	uint i;
	enum e_noc_dbg_cmd cmd = E_NOC_DBG_CMD_INVALID;

	for (i = 0; i < M_ARRAY_SIZE(tbl_noc_dbg_cmd); i++) {
		if (!memcmp(kn_buf, tbl_noc_dbg_cmd[i].str, strlen(tbl_noc_dbg_cmd[i].str))) {
			cmd = tbl_noc_dbg_cmd[i].cmd;
			break;
		}
	}
	if (i >= M_ARRAY_SIZE(tbl_noc_dbg_cmd)) {
		cmd = E_NOC_DBG_CMD_INVALID;
	} else {
		if (cmd == E_NOC_DBG_CMD_WR_REG) {
			/* cppcheck-suppress **/
			if (sscanf_s(kn_buf, "set %s %s 0x%x", reg_str[0], NOC_DBG_STRING_SIZE,
				reg_str[1], NOC_DBG_STRING_SIZE, reg_value) == DBG_RET_FAIL)
				cmd = E_NOC_DBG_CMD_INVALID;
		} else if (cmd == E_NOC_DBG_CMD_RD_REG) {
			if (sscanf_s(kn_buf, "get %s %s", reg_str[0], NOC_DBG_STRING_SIZE,
				reg_str[1], NOC_DBG_STRING_SIZE) == DBG_RET_FAIL)
				cmd = E_NOC_DBG_CMD_INVALID;
		}
	}
	return cmd;
}

static int noc_dbg_wr_reg(const struct noc_reg_dbg *pt_noc_dbg_reg, void __iomem *reg_addr, uint *reg_value)
{
	if (!(pt_noc_dbg_reg->rw_flag & NOC_B_W)) {
		pr_err("caution, reg[%s] can not be written!\n", pt_noc_dbg_reg->p_reg_name);
		return DBG_RET_FAIL;
	}

	switch (pt_noc_dbg_reg->reg_len) {
	case 32:
		writel_relaxed(*reg_value, reg_addr);
		break;
	case 16:
		writew_relaxed(*reg_value, reg_addr);
		break;
	case 8:
		writeb_relaxed((u8) *reg_value, reg_addr);
		break;
	default:
		pr_err("invalid reg len : %d\n", pt_noc_dbg_reg->reg_len);
		return DBG_RET_FAIL;
	}
	return DBG_RET_SUCCESS;
}

static int noc_dbg_rd_reg(const struct noc_reg_dbg *pt_noc_dbg_reg, const void __iomem *reg_addr, uint *reg_value)
{
	if (!(pt_noc_dbg_reg->rw_flag & NOC_B_R)) {
		pr_err("caution, reg[%s] can not be read!\n", pt_noc_dbg_reg->p_reg_name);
		return DBG_RET_FAIL;
	}
	switch (pt_noc_dbg_reg->reg_len) {
	case 32:
		*reg_value = readl_relaxed(reg_addr);
		break;
	case 16:
		*reg_value = readw_relaxed(reg_addr);
		break;
	case 8:
		*reg_value = readb_relaxed(reg_addr);
		break;
	default:
		pr_err("invalid reg len : %d\n", pt_noc_dbg_reg->reg_len);
		return DBG_RET_FAIL;
	}
	return 0;
}

/*
 * Function: noc_dbg_process_cmd
 * Description: process debug command
 * input: enum e_noc_dbg_cmd cmd_in: command type input
 * char (*reg_str)[NOC_CMD_CELL_SIZE]: command strings input
 * uint* reg_value: when this is 'set' function, register value input
 * output: none
 * return: 0->process successed -1->failed
 */

static int noc_dbg_cmd_wr_rd(enum e_noc_dbg_cmd cmd_in, char (*reg_str)[NOC_CMD_CELL_SIZE],
	uint *reg_value)
{
	uint i;
	uint match_index;
	uint cmdtbl_len;
	void __iomem *reg_addr = NULL;
	struct noc_node *node = NULL;
	struct noc_reg_dbg *pt_dbg_reg = NULL;
	int ret;

	/* important, set to invalid index first !! */
	cmdtbl_len = M_ARRAY_SIZE(tbl_noc_dbg_reg);
	match_index = cmdtbl_len;
	for (i = 0; i < cmdtbl_len; i++) {
		if (!strncmp(reg_str[0], tbl_noc_dbg_reg[i].p_reg_name, (size_t) NOC_DBG_REG_NAME)) {
			match_index = i;	 /* match , record index */
			pt_dbg_reg = (struct noc_reg_dbg *)&tbl_noc_dbg_reg[i];
			break;
		}
	}
	if (pt_dbg_reg == NULL) {
		pr_err("pt_dbg_reg not found\n");
		return FAIL;
	}
	/* try to get register base address */
	if (match_index >= cmdtbl_len) {
		pr_err("invalid cmd string : %s %s\n", reg_str[0], reg_str[1]);
		return FAIL;
	}
	if (get_probe_node((const char *)reg_str[1]) == NULL) {
		/* If the bus name is not entered, enter the default name */
		ret = strncpy_s(reg_str[1], (NOC_DBG_STRING_SIZE - 1),
						pt_dbg_reg->node_name, (size_t) NOC_DBG_NOD_NAME);
		if (ret != EOK)
			pr_err("[%s:%d]strncpy_s ret : %d", __func__, __LINE__, ret);
		pr_err("Add default bus (%s);", reg_str[1]);
	}
	/* use default name try again */
	node = get_probe_node((const char *)reg_str[1]);
	if (node == NULL)
		return FAIL;

	if (!is_noc_node_available(node)) {
		pr_err("[%s]: this node is power down, cannot access.\n", node->name);
		return FAIL;
	}

	/* get register virtual address */
	reg_addr = (void __iomem *)((unsigned char *)node->base + pt_dbg_reg->offset);

	if (cmd_in == E_NOC_DBG_CMD_WR_REG) {
		if (noc_dbg_wr_reg(pt_dbg_reg, reg_addr, reg_value) == DBG_RET_FAIL) {
			pr_err("noc_dbg_wr_reg fail\n");
			return FAIL;
		}
	} else if (cmd_in == E_NOC_DBG_CMD_RD_REG) {
		if (noc_dbg_rd_reg(pt_dbg_reg, reg_addr, reg_value) == DBG_RET_FAIL) {
			pr_err("noc_dbg_rd_reg fail\n");
			return FAIL;
		}
		pr_err("reg(%s %s)=[0x%08x];\n", reg_str[0], reg_str[1], *reg_value);
	}
	return 0;
}

static int noc_dbg_process_cmd(enum e_noc_dbg_cmd cmd_in, char (*reg_str)[NOC_CMD_CELL_SIZE], uint *reg_value)
{
	int ret;
	struct dfx_noc_device *noc_dev = NULL;

	switch (cmd_in) {
	case E_NOC_DBG_CMD_WR_REG:
	case E_NOC_DBG_CMD_RD_REG:
		/* search for reg information */
		/* get command tbl length */
		ret = noc_dbg_cmd_wr_rd(cmd_in, (char (*)[NOC_CMD_CELL_SIZE])reg_str, reg_value);
		if (ret < 0)
			goto err_out;
		break;
	case E_NOC_DBG_CMD_PKT_ENABLE:
		noc_dev = platform_get_drvdata(noc_get_pt_pdev());
		if (noc_dev != NULL) {
			noc_dev->noc_property->packet_enable = true;
			enable_noc_packet_probe(&(noc_get_pt_pdev()->dev));
		}
		break;
	case E_NOC_DBG_CMD_PKT_DISABLE:
		noc_dev = platform_get_drvdata(noc_get_pt_pdev());
		if ((noc_dev != NULL) && (noc_dev->noc_property->packet_enable)) {
			noc_dev->noc_property->packet_enable = false;
			disable_noc_packet_probe(&(noc_get_pt_pdev()->dev));
		}
		break;
	case E_NOC_DBG_CMD_TRANS_ENABLE:
		noc_dev = platform_get_drvdata(noc_get_pt_pdev());
		if (noc_dev != NULL) {
			noc_dev->noc_property->transcation_enable = true;
			enable_noc_transcation_probe(&(noc_get_pt_pdev()->dev));
		}
		break;
	case E_NOC_DBG_CMD_TRANS_DISABLE:
		noc_dev = platform_get_drvdata(noc_get_pt_pdev());
		if ((noc_dev != NULL) && (noc_dev->noc_property->transcation_enable)) {
			noc_dev->noc_property->transcation_enable = false;
			disable_noc_transcation_probe(&(noc_get_pt_pdev()->dev));
		}
		break;
	case E_NOC_DBG_CMD_INVALID:
		goto invalid_str;
	default:
		pr_err("invalid case\n");
		goto err_out;
	}

	return DBG_RET_SUCCESS;

invalid_str:
	pr_err("invalid cmd string : %s %s\n", reg_str[0], reg_str[1]);
err_out:
	return DBG_RET_FAIL;
}

/*
 * Function: noc_dbg_wr
 * Description:
 * process string get from user input, such as "get xxx xxx" or "set xxx xxx xxx"
 * input: struct file *file: file pointer that this process operate to
 * const char __user *userbuf: user buffer pointer, get input charactors
 * size_t bytes: input string lengh
 * loff_t *off: input offset
 * output: none
 * return: string length to be processed
 */
static ssize_t noc_dbg_wr(struct file *file, const char __user *userbuf, size_t bytes, loff_t *off)
{
	char *kn_buf = NULL;
	enum e_noc_dbg_cmd cmd_in;
	char reg_str[DBG_REG_ARRAY_SIZE][NOC_DBG_STRING_SIZE];
	uint reg_value = DBG_INIT_VALUE;
	ssize_t byte_writen = DBG_INIT_VALUE;

	if (file != NULL)
		pr_info("%s 0x%pK\n", __func__, file);

	if (bytes >= NOC_DBG_STRING_SIZE) {
		pr_err("Invalide buffer length\n");
		return DBG_RET_FAIL;
	}

	if (userbuf == NULL) {
		pr_err("Invalide userbuf\n");
		return DBG_RET_FAIL;
	}
	if (off == NULL) {
		pr_err("Invalide off\n");
		return DBG_RET_FAIL;
	}

	(void)memset_s(reg_str, sizeof(reg_str), DBG_INIT_VALUE, sizeof(reg_str));

	kn_buf = kzalloc((size_t) NOC_DBG_STRING_SIZE, GFP_KERNEL);
	if (kn_buf == NULL)
		return -EFAULT;

	byte_writen = simple_write_to_buffer(kn_buf, (size_t) NOC_DBG_STRING_SIZE,
										off, userbuf, bytes);
	if (byte_writen <= 0) {
		pr_err("Invalide buffer data\n");
		kfree(kn_buf);
		return byte_writen;
	}

	/* add '\0' to make sure, string has an end-char */
	if (byte_writen < NOC_DBG_STRING_SIZE - 1)
		kn_buf[byte_writen] = '\0';
	else
		kn_buf[NOC_DBG_STRING_SIZE - 1] = '\0';

	/* parse string */
	cmd_in = noc_dbg_get_cmd_typ(kn_buf, (char (*)[NOC_CMD_CELL_SIZE])reg_str, &reg_value);
	if (cmd_in != E_NOC_DBG_CMD_INVALID)
		noc_dbg_process_cmd(cmd_in, (char (*)[NOC_CMD_CELL_SIZE])reg_str, &reg_value);
	else
		pr_err("invalid cmd : %s\n", reg_str[0]);

	kfree(kn_buf);

	return byte_writen;
}

/*
 * Function: noc_dbg_fops
 * Description:
 * rovide method to process node "dbg", mainly depends on write function
 */
static const struct file_operations noc_dbg_fops = {
	.write = noc_dbg_wr,
};

#ifdef CONFIG_DFX_DEBUG_FS
/*
 * Function: noc_dbg_creat_node
 * Description: noc debug function nodes creat
 * input: none
 * output: none
 * return: 0->success -1->failed
 */
int noc_dbg_creat_node(void)
{
	struct dentry *_noc_root = NULL;
	int ret = DBG_RET_FAIL;

	/* creat this dir first, because multi-platform will use this dir */
	_noc_root = debugfs_create_dir("noc", NULL);
	if (_noc_root != NULL) {
		noc_dbg_file_dentry = debugfs_create_file("dbg", DBG_FILE_RIGHT, _noc_root, NULL, &noc_dbg_fops);
		if ((noc_dbg_file_dentry == NULL)
		    || IS_ERR(noc_dbg_file_dentry)) {
			noc_dbg_file_dentry = NULL;
		} else {
			ret = DBG_RET_SUCCESS;
		}
	}

	return ret;
}
#endif
/* END OF FILE */
