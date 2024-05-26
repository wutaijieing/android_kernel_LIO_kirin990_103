/*
 *
 * QIC. (QIC Mntn Module.)
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
#include "dfx_qic.h"
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
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
#include <linux/cpu.h>
#include <linux/arm-smccc.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <platform_include/see/bl31_smc.h>
#ifdef CONFIG_DFX_BB
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#endif
#ifdef CONFIG_DFX_QIC_MODID_REGISTER
#include <mntn_public_interface.h>
#endif
#include <securec.h>
#include "dfx_noc.h"
#include "dfx_noc_err_probe.h"
#include "dfx_noc_info.h"
#define PR_LOG_TAG QIC_TAG

#ifdef CONFIG_DFX_QIC
static struct qic_node *qic_node_array[MAX_QIC_NODE] = { NULL };
static unsigned int qic_node_array_idx;
static struct dfx_qic_regs *qic_dump_reg = NULL;
static struct dfx_qic_device *g_qic_dev = NULL;
static struct platform_device *g_this_pdev = NULL;
static struct dfx_qic_clk **qic_clk_info = NULL;
static unsigned int qic_modid[QIC_MODID_NUM_MAX] =  { QIC_MODID_NEGATIVE };
static unsigned int qic_modid_idx;
static unsigned int g_qic_ib_type = IB_DFX_ERR_INF_TYPE1;

static void free_qic_source(struct platform_device *pdev);

struct dfx_qic_clk **get_qic_clk_info(void)
{
	return qic_clk_info;
}

struct dfx_qic_device *get_qic_dev(void)
{
	return g_qic_dev;
}

char *qic_err_resp[] = {
	"OKAY: normal access success or Exclusive fail",
	"EXOKAY: Exclusive access success",
	"SLVERR: Slave receive Error response or Decoder Error",
	"TMOERR: accessed slave timeout",
	"SECERR: Firewall intercpet and return SLVERR",
	"HIDEERR: Firewall intercpet and not return SLVERR",
	"DISCERR: access powerdown area",
	"UNSERR: received unsupported Opcode",
};

struct qic_tb0_int_type qic_timeout_int_type[] = {
	{ 0x1, 0x1, "illegal wreq opcode or srcid" },
	{ 0x2, 0x2, "illegal wreq tsize " },
	{ 0x4, 0x4, "illegal rreq opcode or srcid" },
	{ 0x8, 0x8, "illegal rreq tsize" },
	{ 0x10, 0x10, "wreq timeout" },
	{ 0x20, 0x20, "rreq timeout" },
};

struct qic_tb1_dfx_stat qic_timeout_dfx_stat[] = {
	{ 0x1, 0x1, "timeout_detected" },
	{ 0x2, 0x2, "hop_miss_detected" },
	{ 0x4, 0x4, "hop_b2b_detected" },
	{ 0x8, 0x8, "unsupport_opc_detected" },
	{ 0x10, 0x10, "unsupport_tbid_detected" },
	{ 0x20, 0x10, "unsupport_ibid_detected" },
	{ 0x40, 0x40, "wrap_lt32b_detected" },
	{ 0x80, 0x80, "wrap_unaligned_detected" },
	{ 0x100, 0x100, "wrap_n2pow_detected" },
	{ 0x200, 0x200, "wrap_narrow_detected" },
	{ 0x400, 0x400, "slverror_detected" },
	{ 0x800, 0x800, "tsize_top_unmatch_detected" },
	{ 0x1000, 0x1000, "trans_over_burst16_detected" },
};

static struct of_device_id const dfx_qic_match[] = {
	{
		.compatible = "hisilicon,qic",
		.data = (void *)NULL
	},
	{}
};

#ifdef CONFIG_DFX_QIC_MODID_REGISTER
static LIST_HEAD(__qic_modid_list);
static DEFINE_SPINLOCK(__qic_modid_list_lock);

static s32 dfx_qic_cnt_check(s32 mod_cnt)
{
	if (mod_cnt > QIC_REGISTER_LIST_MAX_LENGTH)
		return -1;
	else
		return 0;
}
static unsigned int dfx_qic_check_para_registerd(unsigned int qic_coreid, unsigned int modid)
{
	struct qic_coreid_modid_trans_s *p_modid_e = NULL;
	struct list_head *cur = NULL;
	struct list_head *next = NULL;
	unsigned long lock_flag;

	spin_lock_irqsave(&__qic_modid_list_lock, lock_flag);

	list_for_each_safe(cur, next, &__qic_modid_list) {
		p_modid_e = list_entry(cur, struct qic_coreid_modid_trans_s, s_list);
		if (!p_modid_e) {
			pr_err("It might be better to look around here. %s:%d\n", __func__, __LINE__);
			continue;
		}

		if ((p_modid_e->qic_coreid == qic_coreid) ||
		    (p_modid_e->modid == modid)) {
			spin_unlock_irqrestore(&__qic_modid_list_lock, lock_flag);
			return QIC_MODID_EXIST;
		}
	}

	spin_unlock_irqrestore(&__qic_modid_list_lock, lock_flag);

	return 0;
}

static void __qic_modid_register(struct qic_coreid_modid_trans_s *node)
{
	unsigned long lock_flag;

	spin_lock_irqsave(&__qic_modid_list_lock, lock_flag);
	list_add_tail(&(node->s_list), &__qic_modid_list);
	spin_unlock_irqrestore(&__qic_modid_list_lock, lock_flag);
}

/*
 * qic modid registe API,for use to registe own process, and before this,
 * qic err use the AP_S_NOC process, after adapt,user can define his own process.
 */
void dfx_qic_modid_register(unsigned int qic_coreid, unsigned int modid)
{
	struct qic_coreid_modid_trans_s *node = NULL;
	s32 pret;
	u32 ret;
	static s32 mod_cnt = 0;

	pret = dfx_qic_cnt_check(mod_cnt);
	if (pret) {
		pr_err("%s node is full!\n", __func__);
		return;
	}

	if (qic_coreid >= QIC_CORE_ID_MAX) {
		pr_err("%s qic coreid is not exist\n", __func__);
		return;
	}

	/*
	 * before registering modid,we have to check that has modid been registered
	 * berore,double check.
	 */
	ret = dfx_qic_check_para_registerd(qic_coreid, modid);
	if (ret == QIC_MODID_EXIST) {
		pr_err("%s node is exist!\n", __func__);
		return;
	}

	node = kzalloc(sizeof(struct qic_coreid_modid_trans_s), GFP_ATOMIC);
	if (node == NULL)
		return;

	node->qic_coreid = qic_coreid;
	node->modid = modid;
	pr_err("=================================");
	pr_err("[%s]: modid register qic_coreid = 0x%x !\n", __func__, node->qic_coreid);
	pr_err("[%s]: modid register modid = 0x%x !\n", __func__, node->modid);
	pr_err("[%s]: modid register node is ok !\n", __func__);
	pr_err("=================================");
	mod_cnt++;
	/*
	 *  this func is the real func to registe the user's modid and
	 *  user's err judge
	 */
	__qic_modid_register(node);
}

static u32 __qic_find_modid(unsigned int qic_coreid)
{
	struct qic_coreid_modid_trans_s *p_modid_e = NULL;
	struct list_head *cur = NULL;
	struct list_head *next = NULL;
	unsigned long lock_flag;
	unsigned int ret = QIC_MODID_NEGATIVE;

	spin_lock_irqsave(&__qic_modid_list_lock, lock_flag);

	list_for_each_safe(cur, next, &__qic_modid_list) {
		p_modid_e = list_entry(cur, struct qic_coreid_modid_trans_s, s_list);
		if (qic_coreid == p_modid_e->qic_coreid) {
			pr_err("=================================");
			pr_err("[%s]: modid register coreid is equal,value = 0x%x !\n", __func__, qic_coreid);
			pr_err("=================================");
			ret = p_modid_e->modid;
		}
	}
	spin_unlock_irqrestore(&__qic_modid_list_lock, lock_flag);

	return ret;
}

static u32 dfx_qic_modid_find(unsigned int qic_coreid)
{
	u32 ret = QIC_MODID_NEGATIVE;

	if (qic_coreid >= QIC_CORE_ID_MAX)
		return ret;

	ret = __qic_find_modid(qic_coreid);
	return ret;
}
#endif

static void  dfx_qic_tb1_irq(void __iomem *tb1_base)
{
	unsigned int val;
	unsigned int size;
	unsigned int i;
	struct qic_tb1_dfx_stat *qic_tb1_dfx_stat_array = NULL;
	char *dfx_stat = NULL;

	qic_tb1_dfx_stat_array = qic_timeout_dfx_stat;
	size = ARRAY_SIZE_NOC(qic_timeout_dfx_stat);

	val = readl_relaxed(tb1_base + qic_dump_reg->qic_tb1_offset->tb1_dfx_stat0);
	pr_err("[qic tb1_dfx_stat0] = 0x%x\n", val);
	writel_relaxed(val, (tb1_base + qic_dump_reg->qic_tb1_offset->tb1_dfx_stat0));

	for (i = 0; (qic_tb1_dfx_stat_array != NULL) && (i < size); qic_tb1_dfx_stat_array++, i++) {
		if ((val & qic_tb1_dfx_stat_array->mask) == qic_tb1_dfx_stat_array->val) {
			dfx_stat = qic_tb1_dfx_stat_array->dfx_stat;
			pr_err("[qic tb1 dfx_stat0] = 0x%x ( %s )\n", val, dfx_stat);
		}
	}

	val = readl_relaxed(tb1_base + qic_dump_reg->qic_tb1_offset->tb1_fc_in_stat0);
	pr_err("[qic dfx_tb_bp] = 0x%x\n", val);

	val = readl_relaxed(tb1_base + qic_dump_reg->qic_tb1_offset->tb1_qtp_dec_stat0);
	pr_err("[qic tb1_qtp_dec_stat0] = 0x%x\n", val);

	val = readl_relaxed(tb1_base + qic_dump_reg->qic_tb1_offset->tb1_qtp_enc_stat0);
	pr_err("[qic tb1_qtp_enc_stat0] = 0x%x\n", val);

	val = readl_relaxed(tb1_base + qic_dump_reg->qic_tb1_offset->tb1_fc_out_stat0);
	pr_err("[qic tb1_fc_out_stat0] = 0x%x\n", val);
}

static void dfx_qic_tb0_irq(void __iomem *tb0_base)
{
	unsigned int val;
	unsigned int size;
	unsigned int i;
	struct qic_tb0_int_type *qic_timeout_type_array;
	char *int_type = NULL;

	qic_timeout_type_array = qic_timeout_int_type;
	size = ARRAY_SIZE_NOC(qic_timeout_int_type);

	val = readl_relaxed(tb0_base + qic_dump_reg->qic_tb0_offset->tb_int_status);
	pr_err("[qic tb0_int_status] = 0x%x\n", val);
	if (val)
		writel_relaxed(val, tb0_base + qic_dump_reg->qic_tb0_offset->dfx_tb_int_clr);

	for (i = 0; (qic_timeout_type_array != NULL) && (i < size); qic_timeout_type_array++, i++) {
		if ((val & qic_timeout_type_array->mask) == qic_timeout_type_array->val) {
			int_type = qic_timeout_type_array->int_type;
			pr_err("[qic tb0 int_type] = 0x%x ( %s )\n", val, int_type);
		}
	}

	val = readl_relaxed(tb0_base + qic_dump_reg->qic_tb0_offset->dfx_tb_bp);
	pr_err("[qic dfx_tb_bp] = 0x%x\n", val);

	val = readl_relaxed(tb0_base + qic_dump_reg->qic_tb0_offset->dfx_tb_module);
	pr_err("[qic dfx_tb_module] = 0x%x\n", val);
}

static void qic_ib_dfx_err_info_type2_parse(unsigned long long qic_ib_inf0_inf1, unsigned int ib_dfx_int_status)
{
	unsigned int mid;
	unsigned int rw_opc;
	unsigned int qtp_sf;

	/* bit[6:1] master id */
	mid = (unsigned int)((qic_ib_inf0_inf1 >> IB_TYPE2_MID_SHIFT) & IB_TYPE2_MID_MASK);
	pr_err("dfx_qic master_id = %u\n", mid);

	qtp_sf = (unsigned int)((qic_ib_inf0_inf1 >> IB_TYPE2_SF_SHIFT) & IB_TYPE2_SF_MASK);
	pr_err("dfx_qic sf = %u: %s Security access\n", qtp_sf, qtp_sf == 1 ? "no" : "");

	rw_opc = ((ib_dfx_int_status >> IB_TYPE2_OPC_SHIFT) & IB_TYPE2_OPC_MASK);
	pr_err("dfx_qic opreation = %u: abnormal %s command\n", rw_opc, rw_opc == 1 ? "write" : "read");
}

static void qic_ib_dfx_err_info_type1_parse(unsigned long long qic_ib_inf0_inf1, unsigned long long qic_ib_inf1_inf2,
				struct qic_node *qic_node, unsigned int offset)
{
	unsigned int addr;
	unsigned int err_resp;
	unsigned int temp_mask;

	addr = (unsigned int)((qic_ib_inf0_inf1 & IB_ADDR_MASK) >> IB_DATA_CMD_START);
	pr_err("\t[qic_err_addr] = 0x%x\n", addr);

	temp_mask = IB_DATA_CMD_START + qic_node->cmd_id[offset] +
		qic_dump_reg->qtp_rop_width + qic_node->dst_id[offset];
	err_resp = (qic_ib_inf1_inf2 >> temp_mask) & QTP_RESP_MASK;
	pr_err("\t[qic_err_resp=%u] %s\n", err_resp, qic_err_resp[err_resp]);
}

static void qic_err_log_parse(struct qic_ib_reg qic_ib_reg_val, struct qic_node *qic_node, unsigned int offset)
{
	unsigned long long qic_ib_inf0_inf1;
	unsigned long long qic_ib_inf1_inf2;
	unsigned int rop_port_vcnum;
	unsigned int temp_mask;
	unsigned int err_resp;

	qic_ib_inf0_inf1 = ((unsigned long long)(qic_ib_reg_val.ib_dfx_err_inf1) << MAX_QIC_REG) +
		qic_ib_reg_val.ib_dfx_err_inf0;
	qic_ib_inf1_inf2 = ((unsigned long long)(qic_ib_reg_val.ib_dfx_err_inf2) << MAX_QIC_REG) +
		qic_ib_reg_val.ib_dfx_err_inf1;

	if (qic_node->ib_type[offset] == IB_TYPE_CFG) {
		switch (g_qic_ib_type) {
		case IB_DFX_ERR_INF_TYPE1:
			qic_ib_dfx_err_info_type1_parse(qic_ib_inf0_inf1, qic_ib_inf1_inf2, qic_node, offset);
			break;
		case IB_DFX_ERR_INF_TYPE2:
			qic_ib_dfx_err_info_type2_parse(qic_ib_inf0_inf1, qic_ib_reg_val.ib_dfx_int_status);
			break;
		default:
			pr_err("qic ib type is unknown[0x%x]\n", g_qic_ib_type);
			break;
		}
	}

	if (qic_node->ib_type[offset] == IB_TYPE_DATA) {
		rop_port_vcnum = qic_dump_reg->qtp_rop_width + qic_dump_reg->qtp_portnum_width +
			qic_dump_reg->qtp_vcnum_width;
		temp_mask = IB_DATA_CMD_START + qic_node->cmd_id[offset] + rop_port_vcnum + qic_node->dst_id[offset];
		err_resp = (qic_ib_inf0_inf1 >> temp_mask) & QTP_RESP_MASK;
		pr_err("\t[qic_err_resp=%d] %s\n", err_resp, qic_err_resp[err_resp]);
	}
}

static void dfx_qic_err_probe_irq(void __iomem *ib_base, struct qic_node *qic_node, unsigned int offset)
{
	struct qic_ib_reg qic_ib_reg_val;
	const char *node_name = "ib0_arcd";

	(void)memset_s(&qic_ib_reg_val, sizeof(struct qic_ib_reg), 0, sizeof(struct qic_ib_reg));
	/* Circumvent isp ib0 data interrupt */
	if (!strcmp(qic_node->name[offset], node_name))
		writel_relaxed(0x1, (u8 __iomem *) ib_base + IRQ_CTRL_OFFSET);
	qic_ib_reg_val.ib_dfx_int_status = readl_relaxed(ib_base + qic_dump_reg->qic_ib_offset->ib_dfx_int_status);
	pr_err("qic ib_dfx_int_status = 0x%x\n", qic_ib_reg_val.ib_dfx_int_status);
	/* clear interrupt */
	writel_relaxed(ERR_PROBE_CLEAR_BIT, (u8 __iomem *) ib_base + qic_dump_reg->qic_ib_offset->ib_dfx_int_clr);

	qic_ib_reg_val.ib_dfx_err_inf0 = readl_relaxed(ib_base + qic_dump_reg->qic_ib_offset->ib_dfx_err_inf0);
	pr_err("qic ib_dfx_err_inf0 = 0x%x\n", qic_ib_reg_val.ib_dfx_err_inf0);

	qic_ib_reg_val.ib_dfx_err_inf1 = readl_relaxed(ib_base + qic_dump_reg->qic_ib_offset->ib_dfx_err_inf1);
	pr_err("qic ib_dfx_err_inf1 = 0x%x\n", qic_ib_reg_val.ib_dfx_err_inf1);

	qic_ib_reg_val.ib_dfx_err_inf2 = readl_relaxed(ib_base + qic_dump_reg->qic_ib_offset->ib_dfx_err_inf2);
	pr_err("qic ib_dfx_err_inf2 = 0x%x\n", qic_ib_reg_val.ib_dfx_err_inf2);

	qic_ib_reg_val.ib_dfx_err_inf3 = readl_relaxed(ib_base + qic_dump_reg->qic_ib_offset->ib_dfx_err_inf3);
	pr_err("qic ib_dfx_err_inf3 = 0x%x\n", qic_ib_reg_val.ib_dfx_err_inf3);

	qic_err_log_parse(qic_ib_reg_val, qic_node, offset);
}

static unsigned int dfx_qic_irq_type(struct qic_node *node, int irq_bit)
{
	unsigned int i;
	unsigned int ret = QIC_MODID_NEGATIVE;

	for (i = 0; i < node->irq_num; i++) {
		if (node->irq_bit[i] == irq_bit) {
			pr_err("qic_node_name is %s\n", node->name[i]);
			switch (node->irq_type[i]) {
			case QIC_IB_ERR_IRQ:
				pr_err("qic_ib_err_irq happen\n");
				dfx_qic_err_probe_irq(node->qic_base[i + 1], node, i);
				goto find_modid;
			case QIC_TB0_IRQ:
				pr_err("qic_tb0_err_irq happen\n");
				dfx_qic_tb0_irq(node->qic_base[i + 1]);
				break;
			case QIC_TB1_IRQ:
				pr_err("qic_tb1_err_irq happen\n");
				dfx_qic_tb1_irq(node->qic_base[i + 1]);
				break;
			default:
				pr_err("qic irq type is unknown[%d]\n", node->irq_type[i]);
				break;
			}
		}
	}

	return ret;

find_modid:
#ifdef CONFIG_DFX_QIC_MODID_REGISTER
	ret = dfx_qic_modid_find(node->qic_coreid[i]);
#endif
	return ret;
}

static int is_qic_bus_clk_enable(struct dfx_qic_clk *qic_clk_s)
{
	void __iomem *reg_base = NULL;
	unsigned int reg_value;
	unsigned int i;
	unsigned int ret = 1;
	unsigned int clock, mask;

	reg_base = qic_clk_s->reg_base;

	for (i = 0; i < QIC_CLOCK_NUM; i++) {
		clock = qic_clk_s->bus_clk[i].clock;
		mask = qic_clk_s->bus_clk[i].mask;
		if (clock == DFX_QIC_CLOCK_REG_DEFAULT)
			continue;

		reg_value = readl_relaxed((u8 __iomem *) reg_base + clock);
		/* Clock is enabled */
		if ((reg_value & mask) == mask) {
			continue;
		} else {
			ret = 0;
			break;
		}
	}

	return ret;
}

static int is_qic_bus_available(struct dfx_qic_clk *qic_clk_s)
{
	void __iomem *reg_base = NULL;
	unsigned int request, status, pwr_mask;
	struct dfx_qic_device *qic_dev = NULL;

	qic_dev = platform_get_drvdata(g_this_pdev);
	if (!qic_dev)
		return 0;

	reg_base = qic_dev->pmctrl_base;
	request =  readl_relaxed((u8 __iomem *) reg_base + qic_clk_s->pwidle_offset.idle_req);
	status =  readl_relaxed((u8 __iomem *) reg_base + qic_clk_s->pwidle_offset.idle_status);
	pwr_mask = 1 << qic_clk_s->pwidle_bit;

	if (!((request | status) & pwr_mask) && is_qic_bus_clk_enable(qic_clk_s))
		return 1;

	return 0;
}

static void qic_bus_try_to_giveup_idle(struct dfx_qic_clk *qic_clk_s)
{
	struct dfx_qic_device *qic_dev = NULL;
	unsigned int pwr_mask, status;
	void __iomem *pm_idlereq_reg = NULL;
	void __iomem *pm_idle_reg = NULL;

	if (g_this_pdev == NULL) {
		pr_err("[%s] g_this_pdev is NULL!!!\n", __func__);
		return;
	}

	qic_dev = platform_get_drvdata(g_this_pdev);
	if (qic_dev == NULL) {
		pr_err("[%s] Can not get device node pointer!!\n", __func__);
		return;
	}

	pwr_mask = 1 << qic_clk_s->pwidle_bit;
	/* try to exit idle state */
	pm_idlereq_reg = qic_dev->pmctrl_base + qic_clk_s->pwidle_offset.idle_req;
	pm_idle_reg = qic_dev->pmctrl_base + qic_clk_s->pwidle_offset.idle_status;

	status = readl_relaxed(pm_idle_reg);
	pr_err("[%s] before +, QIC_POWER_IDLE = 0x%x;\n", __func__, status);

	status = readl_relaxed(pm_idlereq_reg);
	status &= ~pwr_mask;
	status |= (pwr_mask << QIC_HIGH_ENABLE_MASK);
	writel_relaxed(status, pm_idlereq_reg);

	status = readl_relaxed(pm_idle_reg);
	pr_err("[%s] after -, QIC_POWER_IDLE = 0x%x;\n", __func__, status);
}

int dfx_qic_check_crg_status(struct dfx_qic_clk *qic_clk_s)
{
	void __iomem *reg_base = NULL;
	unsigned int reg_value;
	unsigned int i;
	unsigned int ret = 1;
	unsigned int clock, mask;

	if (!qic_clk_s)
		return 0;

	reg_base = g_qic_dev->pericrg_base;

	for (i = 0; i < QIC_CLOCK_NUM; i++) {
		clock = qic_clk_s->crg_clk[i].clock;
		mask = qic_clk_s->crg_clk[i].mask;
		if (clock == DFX_QIC_CLOCK_REG_DEFAULT)
			continue;

		reg_value = readl_relaxed((u8 __iomem *) reg_base + clock);
		/* Clock is enabled */
		if ((reg_value & mask) == mask) {
			continue;
		} else {
			ret = 0;
			break;
		}
	}

	return ret;
}

static int dfx_qic_check_bus_status(struct dfx_qic_clk *qic_clk_s)
{
	int giveup_idle_retry = 0;

QIC_ERROR_CHECK:
	if (is_qic_bus_available(qic_clk_s)) {
		return 1;
	} else {
		pr_err("qic_bus: %s, module powerdown\n", qic_clk_s->bus_name);
		if (giveup_idle_retry++ < MAX_QIC_GIVEUP_IDLE_NUM) {
			qic_bus_try_to_giveup_idle(qic_clk_s);
			pr_err("qic_bus_try_to_giveup_idle , %d time\n", giveup_idle_retry);
			goto QIC_ERROR_CHECK;
		}
		pr_err("Finally, qic_bus: %s powerdown state change fail!\n", qic_clk_s->bus_name);
	}

	return 0;
}

static void dfx_qic_irq_process(unsigned int bus_id, unsigned int *reset_flag, struct dfx_qic_clk *qic_clk_s)
{
	struct qic_node *node = NULL;
	unsigned int i, pending, modid;
	unsigned long irq_bit;
	unsigned long pending_s;

	for (i = 0; i < qic_node_array_idx; i++) {
		node = qic_node_array[i];
		if (node == NULL || node->bus_id != bus_id)
			continue;

	/* qic_base[0] means media1 or media2 crg base */
		pending = readl_relaxed(node->qic_base[0] + node->offset);
		pending_s = (unsigned long)pending & MASK_UNSIGNED_INT;
		if (pending_s) {
			pr_err("qic irq status = 0x%lx(%s, 0x%x)\n", pending_s, node->bus_name, node->offset);
			if (dfx_qic_check_bus_status(qic_clk_s)) {
				for_each_set_bit(irq_bit, &pending_s, MAX_QIC_REG) {
					if (((unsigned int)(1 << irq_bit) & node->irq_mask) != 0)
						*reset_flag = 1;
					modid = dfx_qic_irq_type(node, irq_bit);
					if ((modid != QIC_MODID_NEGATIVE) && (qic_modid_idx < QIC_MODID_NUM_MAX))
						qic_modid[qic_modid_idx++] = modid;
				}
			} else {
				*reset_flag = 1;
			}
		}
	}
}

static unsigned long dfx_qic_smc_call(unsigned long function_id, unsigned long power_type)
{
	struct arm_smccc_res res;

	arm_smccc_smc((unsigned long)MNTN_QIC_FID_VALUE, function_id, power_type, 0, 0, 0, 0, 0, &res);
	return res.a0;
}
int dfx_qic_vote_power_on(struct dfx_qic_clk *qic_clk_s)
{
	if (!qic_clk_s)
		return 0;

	/* bus always on , no need vote on and check crg status */
	if (qic_clk_s->fid_val == BUS_ALWAYS_ON) {
		pr_info("%s BUS_ALWAYS_ON\n", __func__);
		return 1;
	}

	if (!dfx_qic_smc_call((unsigned long)qic_clk_s->fid_val, VOTE_POWER_ON)) {
		/* vote success, double check clock reg status */
		if (dfx_qic_check_crg_status(qic_clk_s)) {
			return 1;
		} else {
			pr_err("[%s] double check clk reg fail\n", qic_clk_s->bus_name);
			/* check clock reg status fail, vote power down */
			if (dfx_qic_smc_call(qic_clk_s->fid_val, VOTE_POWER_DOWN))
				pr_err("[%s]power down vote fail\n", qic_clk_s->bus_name);
		}
	} else {
		pr_err("[%s] power on vote fail\n", qic_clk_s->bus_name);
	}

	return 0;
}
void dfx_qic_vote_power_down(struct dfx_qic_clk *qic_clk_s)
{
	if (!qic_clk_s)
		return;
	/* bus always on , no need vote down and check crg status */
	if (qic_clk_s->fid_val == BUS_ALWAYS_ON) {
		pr_info("%s BUS_ALWAYS_ON\n", __func__);
		return;
	}

	if (dfx_qic_smc_call(qic_clk_s->fid_val, VOTE_POWER_DOWN))
		pr_err("[%s]power down vote fail\n", qic_clk_s->bus_name);
}

static irqreturn_t dfx_qic_irq_handler(int irq, void *data)
{
	unsigned int reset_flag = 0;
	unsigned int bus_id, bus_num;
	unsigned int modid_match_flag = 0;
	struct dfx_qic_clk *qic_clk_s = NULL;
#ifdef CONFIG_DFX_QIC_MODID_REGISTER
	unsigned int i;
#endif
	struct dfx_qic_device *qic_dev = (struct dfx_qic_device *)data;

	pr_err("%s enter\n", __func__);
	qic_modid_idx = 0;
	bus_num = qic_dev->bus_num;

	for (bus_id = 0; bus_id < bus_num; bus_id++) {
		qic_clk_s = qic_clk_info[bus_id];

		if (dfx_qic_check_crg_status(qic_clk_s)) {
			/* vote power on */
			if (dfx_qic_vote_power_on(qic_clk_s)) {
				dfx_qic_irq_process(bus_id, &reset_flag, qic_clk_s);
				/* vote power down */
				dfx_qic_vote_power_down(qic_clk_s);
			} else {
				continue;
			}
		} else {
			pr_err("[%s] is power down\n", qic_clk_s->bus_name);
		}
	}

#ifdef CONFIG_DFX_BB
	if (check_mntn_switch(MNTN_NOC_ERROR_REBOOT)) {
		if (reset_flag) {
#ifdef CONFIG_DFX_QIC_MODID_REGISTER
			for (i = 0; i < qic_modid_idx; i++) {
				if (qic_modid[i] != QIC_MODID_NEGATIVE) {
					modid_match_flag = 1;
					pr_err("[%s] qic modid is matched, modid = %x!\n", __func__, qic_modid[i]);
					rdr_system_error(qic_modid[i], 0, 0);
				}
			}
#endif
			if (!modid_match_flag) {
				pr_err("%s qic reset begin\n", __func__);
				rdr_syserr_process_for_ap(MODID_AP_S_NOC, 0, 0);
			}
		}
	}
#endif

	return IRQ_HANDLED;
}

static int dfx_get_qic_node_data(struct device_node *np, struct qic_node *node)
{
	unsigned int irq_nums;

	irq_nums = node->irq_num;

	if (of_property_read_u32(np, "busid", (u32 *)&(node->bus_id)) != 0) {
		pr_err("[%s] Get bus_id from DTS error\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32(np, "irq_offset", (u32 *)&(node->offset)) != 0) {
		pr_err("[%s] Get irq_offset from DTS error\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32(np, "irq_mask", (u32 *)&(node->irq_mask)) != 0) {
		pr_err("[%s] Get irq_mask from DTS error\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32_array(np, "hwirq_bit", (u32 *)node->irq_bit, irq_nums) != 0) {
		pr_err("[%s] Get hwirq_bit list from DTS error\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32_array(np, "hwirq_type", (u32 *)node->irq_type, irq_nums) != 0) {
		pr_err("[%s] Get hwirq_type list from DTS error\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32_array(np, "ib_type", (u32 *)node->ib_type, irq_nums) != 0) {
		pr_err("[%s] Get ib_type list from DTS error\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32_array(np, "cmd_id", (u32 *)node->cmd_id, irq_nums) != 0) {
		pr_err("[%s] Get cmd_id list from DTS error\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32_array(np, "dst_id", (u32 *)node->dst_id, irq_nums) != 0) {
		pr_err("[%s] Get dst_id list from DTS error\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32_array(np, "qic_coreid", (u32 *)node->qic_coreid, irq_nums) != 0) {
		pr_err("[%s] Get qic_coreid list from DTS error\n", __func__);
		return FAIL;
	}

	return SUCCESS;
}

static int dfx_qic_node_malloc(struct platform_device *pdev, struct qic_node *node)
{
	unsigned int irq_nums;

	irq_nums = node->irq_num;

	node->irq_bit = devm_kzalloc(&pdev->dev, sizeof(unsigned int) * irq_nums, GFP_KERNEL);
	if (!node->irq_bit)
		return FAIL;

	node->irq_type = devm_kzalloc(&pdev->dev, sizeof(unsigned int) * irq_nums, GFP_KERNEL);
	if (!node->irq_type)
		return FAIL;

	node->ib_type = devm_kzalloc(&pdev->dev, sizeof(unsigned int) * irq_nums, GFP_KERNEL);
	if (!node->ib_type)
		return FAIL;

	node->cmd_id = devm_kzalloc(&pdev->dev, sizeof(unsigned int) * irq_nums, GFP_KERNEL);
	if (!node->cmd_id)
		return FAIL;

	node->dst_id = devm_kzalloc(&pdev->dev, sizeof(unsigned int) * irq_nums, GFP_KERNEL);
	if (!node->dst_id)
		return FAIL;
	node->qic_coreid = devm_kzalloc(&pdev->dev, sizeof(unsigned int) * irq_nums, GFP_KERNEL);
	if (!node->qic_coreid)
		return FAIL;

	return SUCCESS;
}

static int dfx_qic_get_node_name_and_base(struct platform_device *pdev,
	struct device_node *np, struct qic_node *node)
{
	int ret;
	unsigned int irq_nums;
	unsigned int i;
	char *name[MAX_QIC_REG] = {NULL};

	irq_nums = node->irq_num;
	ret = of_property_read_string_array(np, "irq_name", (const char **)&name[0], irq_nums);
	if (ret < 0) {
		pr_err("[%s], get irq names fail in dts\n", __func__);
		return FAIL;
	}

	for (i = 0; i < irq_nums; i++) {
		node->name[i] = devm_kzalloc(&pdev->dev, MAX_QIC_NODE_NAME_LEN, GFP_KERNEL);
		if (!node->name[i])
			return FAIL;

		ret = strncpy_s(node->name[i], MAX_QIC_NODE_NAME_LEN, name[i], strlen(name[i]));
		if (ret != EOK) {
			pr_err("[%s:%d]strncpy_s ret : %d, i= %d\n", __func__, __LINE__, ret, i);
			return FAIL;
		}
	}

	for (i = 0; i < irq_nums + 1; i++) {
		node->qic_base[i] = NULL;
		node->qic_base[i] = of_iomap(np, i);
		if (!node->qic_base[i]) {
			pr_err("[%s:%d]qic_base[%d] iomap fail\n", __func__, __LINE__, i);
			return FAIL;
		}
	}

	return SUCCESS;
}

static int dfx_qic_get_irqnum_and_name(struct platform_device *pdev,
	struct device_node *np, struct qic_node **node)
{
	unsigned int irq_nums;
	int ret;

	if (of_property_read_u32(np, "irq_nums", &irq_nums) != 0) {
		pr_err("[%s] Get irq_nums from DTS error.\n", __func__);
		return FAIL;
	}
	*node = devm_kzalloc(&pdev->dev, sizeof(struct qic_node), GFP_KERNEL);
	if (!(*node))
		return FAIL;

	(*node)->irq_num = irq_nums;

	(*node)->bus_name = devm_kzalloc(&pdev->dev, (MAX_QIC_NODE_NAME_LEN * irq_nums), GFP_KERNEL);
	if (!(*node)->bus_name)
		return FAIL;

	ret = strncpy_s((*node)->bus_name, MAX_QIC_NODE_NAME_LEN, np->name, (MAX_QIC_NODE_NAME_LEN - 1));
	if (ret != EOK) {
		pr_err("[%s:%d]strncpy_s ret : %d\n", __func__, __LINE__, ret);
		return FAIL;
	}

	return SUCCESS;
}

static int dfx_qic_node_register(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = NULL;
	struct qic_node *node = NULL;

	for_each_compatible_node(np, NULL, "hisilicon,qic-node") {
		if (qic_node_array_idx >= MAX_QIC_NODE) {
			pr_err("[%s]qic_node_array_idx error\n", __func__);
			return FAIL;
		}

		ret = dfx_qic_get_irqnum_and_name(pdev, np, &node);
		if (ret < 0) {
			pr_err("[%s]dfx_qic_get_irqnum_and_name fail\n", __func__);
			return FAIL;
		}

		ret = dfx_qic_node_malloc(pdev, node);
		if (ret < 0) {
			pr_err("[%s]dfx_qic_node_malloc fail\n", __func__);
			return FAIL;
		}

		ret = dfx_qic_get_node_name_and_base(pdev, np, node);
		if (ret < 0) {
			pr_err("[%s] dfx_qic_get_node_name_and_base fail\n", __func__);
			return FAIL;
		}

		ret = dfx_get_qic_node_data(np, node);
		if (ret < 0) {
			pr_err("[%s] dfx_get_qic_node_data fail\n", __func__);
			return FAIL;
		}

		qic_node_array[qic_node_array_idx] = node;
		qic_node_array_idx++;
	}

	return SUCCESS;
}

static int dfx_qic_get_width(struct platform_device *pdev, struct dfx_qic_regs *qic_dump_reg_temp,
	struct device_node *np)
{
	int nums;

	/* get qic tb1_reg from dts */
	if (of_property_read_u32(np, "tb1_nums", &nums) != 0) {
		pr_err("[%s] Get tb0_nums from DTS error.\n", __func__);
		return FAIL;
	}

	qic_dump_reg_temp->qic_tb1_offset = devm_kzalloc(&pdev->dev, sizeof(struct qic_tb1_reg), GFP_KERNEL);
	if (!qic_dump_reg_temp->qic_tb1_offset)
		return FAIL;

	if (of_property_read_u32_array(np,
				       "tb1_list",
				       (u32 *)qic_dump_reg_temp->qic_tb1_offset, nums) != 0) {
		pr_err("[%s] Get qic_tb1_offset from DTS error.\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32(np, "qtp_rop_width", (u32 *)&(qic_dump_reg_temp->qtp_rop_width)) != 0) {
		pr_err("[%s] Get qtp_rop_width from DTS error\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32(np, "qtp_portnum_width", (u32 *)&(qic_dump_reg_temp->qtp_portnum_width)) != 0) {
		pr_err("[%s] Get qtp_portnum_width from DTS error\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32(np, "qtp_vcnum_width", (u32 *)&(qic_dump_reg_temp->qtp_vcnum_width)) != 0) {
		pr_err("[%s] Get qtp_vcnum_width from DTS error\n", __func__);
		return FAIL;
	}

	return SUCCESS;
}

static int dfx_qic_get_ib_and_tb(struct platform_device *pdev,
	struct dfx_qic_regs *qic_dump_reg_temp, struct device_node *np)
{
	int nums;

	/* get qic ib_reg from dts */
	if (of_property_read_u32(np, "ib_nums", &nums) != 0) {
		pr_err("[%s] Get ib_nums from DTS error.\n", __func__);
		return FAIL;
	}

	qic_dump_reg_temp->qic_ib_offset = devm_kzalloc(&pdev->dev, sizeof(struct qic_ib_reg), GFP_KERNEL);
	if (!qic_dump_reg_temp->qic_ib_offset)
		return FAIL;

	if (of_property_read_u32_array(np,
				       "ib_list",
				       (u32 *)qic_dump_reg_temp->qic_ib_offset, nums) != 0) {
		pr_err("[%s] Get qic_ib_offset from DTS error.\n", __func__);
		return FAIL;
	}

	/* get qic tb0_reg from dts */
	if (of_property_read_u32(np, "tb0_nums", &nums) != 0) {
		pr_err("[%s] Get tb0_nums from DTS error.\n", __func__);
		return FAIL;
	}

	qic_dump_reg_temp->qic_tb0_offset = devm_kzalloc(&pdev->dev, sizeof(struct qic_tb0_reg), GFP_KERNEL);
	if (!qic_dump_reg_temp->qic_tb0_offset)
		return FAIL;

	if (of_property_read_u32_array(np,
				       "tb0_list",
				       (u32 *)qic_dump_reg_temp->qic_tb0_offset, nums) != 0) {
		pr_err("[%s] Get qic_tb0_offset from DTS error.\n", __func__);
		return FAIL;
	}

	return SUCCESS;
}

static int dfx_qic_get_reg_base(struct platform_device *pdev, struct device_node *np)
{
	void __iomem *reg_base[QIC_MAX_BASE] = {NULL};
	struct dfx_qic_device *qic_dev = NULL;
	int i;

	qic_dev = platform_get_drvdata(pdev);
	if (qic_dev == NULL) {
		pr_err("[%s]: qic_dev is NULL!!!\n", __func__);
		return FAIL;
	}

	for (i = 0; i < QIC_MAX_BASE; i++) {
		reg_base[i] = of_iomap(np, i);
		if (!reg_base[i])
			goto err_unmap;
	}

	qic_dev->pericrg_base = reg_base[PERI_CRG_BASE];
	qic_dev->pmctrl_base = reg_base[PMCTRL_BASE];

	return SUCCESS;
err_unmap:
	for (i = 0; i < QIC_MAX_BASE; i++) {
		if (reg_base[i]) {
			iounmap(reg_base[i]);
			reg_base[i] = NULL;
		}
	}

	return FAIL;
}

static int dfx_qic_read_dts(struct platform_device *pdev)
{
	struct device_node *np = NULL;
	int ret;
	struct dfx_qic_regs *qic_dump_reg_temp = NULL;

	qic_dump_reg_temp = devm_kzalloc(&pdev->dev, sizeof(struct dfx_qic_regs), GFP_KERNEL);
	if (!qic_dump_reg_temp)
		return FAIL;
	qic_dump_reg = qic_dump_reg_temp;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,qic-dump-reg-list");
	if (!np)
		return FAIL;

	ret = dfx_qic_get_reg_base(pdev, np);
	if (ret < 0) {
		pr_err("[%s] Get reg base from DTS error\n", __func__);
		return FAIL;
	}

	ret = dfx_qic_get_ib_and_tb(pdev, qic_dump_reg_temp, np);
	if (ret < 0) {
		pr_err("[%s] Get ib and tb from DTS error\n", __func__);
		return FAIL;
	}

	ret = dfx_qic_get_width(pdev, qic_dump_reg_temp, np);
	if (ret < 0) {
		pr_err("[%s] Get tb1 and width from DTS error\n", __func__);
		return FAIL;
	}

	return SUCCESS;
}

static int dfx_qic_get_clk_info(struct platform_device *pdev, struct device_node *np,
	struct dfx_qic_clk *qic_clk_s)
{
	int i, ret;
	char clock_name[MAX_QIC_CLK_NAME_LEN] = {0};

	/* 2 means req_reg and status_reg */
	if (of_property_read_u32_array(np, "pwidle_offset", (u32 *)&(qic_clk_s->pwidle_offset), 2) != 0) {
		pr_err("[%s] Get pwidle_bit from DTS error\n", __func__);
		return FAIL;
	}

	for (i = 0; i < QIC_CLOCK_NUM; i++) {
		ret = snprintf_s(clock_name, sizeof(clock_name), sizeof(clock_name) - 1, "crg_clk%d", i);
		if (ret < 0) {
			pr_err("%s():%d:snprintf_s fail!\n", __func__, __LINE__);
			return FAIL;
		}
		/* 2 is offset and mask */
		ret = of_property_read_u32_array(np, clock_name, (u32 *)&qic_clk_s->crg_clk[i], 2);
		if (ret) {
			continue;
		}
	}
	for (i = 0; i < QIC_CLOCK_NUM; i++) {
		ret = snprintf_s(clock_name, sizeof(clock_name), sizeof(clock_name) - 1, "bus_clk%d", i);
		if (ret < 0) {
			pr_err("%s():%d:snprintf_s fail!\n", __func__, __LINE__);
			return FAIL;
		}
		/* 2 is offset and mask */
		ret = of_property_read_u32_array(np, clock_name, (u32 *)&qic_clk_s->bus_clk[i], 2);
		if (ret) {
			continue;
		}
	}
	ret = of_property_read_string(np, "bus-name", (const char **)&qic_clk_s->bus_name);
	if (ret < 0) {
		pr_err("[%s]: cannot find bus-name\n", __func__);
		return FAIL;
	}

	if (of_property_read_u32(np, "reg_nums", &qic_clk_s->dump_reg_nums) != 0) {
		pr_err("[%s] Get busid from DTS error.\n", __func__);
		return FAIL;
	}

	qic_clk_s->reg_offset = devm_kzalloc(&pdev->dev, sizeof(unsigned int) * qic_clk_s->dump_reg_nums, GFP_KERNEL);
	if (!qic_clk_s->reg_offset) {
		pr_err("[%s] qic_clk_s->reg_offset is null\n", __func__);
		return FAIL;
	}

	ret = of_property_read_u32_array(np, "offset", (u32 *)qic_clk_s->reg_offset, qic_clk_s->dump_reg_nums);
	if (ret) {
		pr_err("[%s] get qic_clk_s->reg_offset from dts error\n", __func__);
		return FAIL;
	}

	return SUCCESS;
}

static int dfx_get_qic_clock(struct platform_device *pdev)
{
	struct device_node *np = NULL;
	struct dfx_qic_clk *qic_clk_s = NULL;
	int count = 0;
	unsigned int bus_id;

	for_each_compatible_node(np, NULL, "hisilicon,qic-clock-reg") {
		if (count > MAX_QIC_BUS_NUM)
			break;
		if (of_property_read_u32(np, "busid", &bus_id) != 0) {
			pr_err("[%s] Get busid from DTS error.\n", __func__);
			return -ENOMEM;
		}

		qic_clk_info[bus_id] = devm_kzalloc(&pdev->dev, sizeof(struct dfx_qic_clk), GFP_KERNEL);
		qic_clk_s = qic_clk_info[bus_id];
		if (!qic_clk_s)
			return FAIL;
		/* Init struct data as 0xFFFFFFFF */
		(void)memset_s(&qic_clk_s->crg_clk[0], sizeof(struct qic_clk) * QIC_CLOCK_NUM, 0xff,
			sizeof(struct qic_clk) * QIC_CLOCK_NUM);
		(void)memset_s(&qic_clk_s->bus_clk[0], sizeof(struct qic_clk) * QIC_CLOCK_NUM, 0xff,
			sizeof(struct qic_clk) * QIC_CLOCK_NUM);

		if (of_property_read_u32(np, "pwidle_bit", (u32 *)&(qic_clk_s->pwidle_bit)) != 0) {
			pr_err("[%s] Get pwidle_bit from DTS error\n", __func__);
			return FAIL;
		}
		if (of_property_read_u32(np, "fid_val", (u32 *)&(qic_clk_s->fid_val)) != 0) {
			pr_err("[%s] Get fid_val from DTS error\n", __func__);
			return FAIL;
		}

		qic_clk_s->reg_base = of_iomap(np, 0);
		if (!qic_clk_s->reg_base) {
			pr_err("[%s] Get qic_clk_s->reg_base from DTS error\n", __func__);
			return FAIL;
		}
		if (dfx_qic_get_clk_info(pdev, np, qic_clk_s)) {
			pr_err("[%s] Get dfx_qic_get_clk_info error\n", __func__);
			return FAIL;
		}

		count++;
	}

	return SUCCESS;
}

static int dfx_qic_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = NULL;
	struct dfx_qic_device *qic_dev = NULL;
	const struct of_device_id *match = NULL;
	struct device_node *np = NULL;
	unsigned int bus_num;

	if (!pdev)
		return -ENOMEM;

	g_this_pdev = pdev;	 /* Get platform device pointer */
	dev = &pdev->dev;
	/* to check which type of regulator this is */
	match = of_match_device(dfx_qic_match, dev);
	if (match == NULL) {
		pr_err("[%s]: mismatch of dfx noc driver\n\r", __func__);
		return -ENOMEM;
	}

	qic_dev = devm_kzalloc(&pdev->dev, sizeof(struct dfx_qic_device), GFP_KERNEL);
	if (!qic_dev)
		return -ENOMEM;
	g_qic_dev = qic_dev;

	platform_set_drvdata(pdev, qic_dev);
	np = dev->of_node;

	if (of_property_read_u32(np, "qic_bus_num", &bus_num) != 0) {
		pr_err("[%s] Get qic_bus_num from DTS error.\n", __func__);
		return -ENOMEM;
	}
	qic_dev->bus_num = bus_num;

	if (of_property_read_u32(np, "qic_ib_type", &g_qic_ib_type) != 0)
		pr_info("use defalut qic_ib_type\n");

	pr_err("[%s] g_qic_ib_type 0x%x\n", __func__, g_qic_ib_type);

	qic_clk_info = devm_kzalloc(&pdev->dev, sizeof(struct dfx_qic_clk *) * bus_num, GFP_KERNEL);
	if (!qic_clk_info)
		return -ENOMEM;

	ret = dfx_get_qic_clock(pdev);
	if (ret < 0)
		goto err;

	ret = dfx_qic_node_register(pdev);
	if (ret < 0)
		goto err;

	ret = dfx_qic_read_dts(pdev);
	if (ret < 0)
		goto err;

	ret = platform_get_irq(pdev, 0);
	if (ret < 0)
		dev_err(&pdev->dev, "cannot find qic irq\n");
	qic_dev->qic_irq = ret;

	ret = devm_request_irq(&pdev->dev, qic_dev->qic_irq, dfx_qic_irq_handler,
		IRQF_TRIGGER_HIGH, "dfx_qic", qic_dev);
	if (ret < 0)
		dev_err(&pdev->dev, "request qic_irq fail!\n");

	if (dfx_qic_dump_init())
		pr_err("[%s] dfx_qic_dump_init fail!\n", __func__);

	pr_err("[%s] qic init success!\n", __func__);

	return SUCCESS;
err:
	pr_err("[%s] qic init fail!\n", __func__);
	free_qic_source(pdev);
	return ret;
}

static void free_qic_source(struct platform_device *pdev)
{
	unsigned int i;
	int j;
	struct qic_node *node = NULL;
	struct dfx_qic_device *qic_dev = g_qic_dev;

	if (qic_dev) {
		if (qic_dev->pericrg_base)
			iounmap(qic_dev->pericrg_base);
		if (qic_dev->pmctrl_base)
			iounmap(qic_dev->pmctrl_base);
	}

	for (i = 0; i <= qic_node_array_idx; i++) {
		node = qic_node_array[i];
		if (node) {
			for (j = 0; j < MAX_QIC_REG; j++) {
				if (node->qic_base[j])
					iounmap(node->qic_base[j]);
			}
		}
	}
}

static int dfx_qic_remove(struct platform_device *pdev)
{
	struct dfx_qic_device *qic_dev = NULL;

	if (!pdev)
		return FAIL;

	qic_dev = platform_get_drvdata(pdev);
	if (qic_dev != NULL)
		free_irq(qic_dev->qic_irq, qic_dev);

	free_qic_source(pdev);

	return 0;
}

MODULE_DEVICE_TABLE(of, dfx_qic_match);

static struct platform_driver dfx_qic_driver = {
	.probe = dfx_qic_probe,
	.remove = dfx_qic_remove,
	.driver = {
		   .name = QIC_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(dfx_qic_match),
		   },
};

static int __init dfx_qic_init(void)
{
	return platform_driver_register(&dfx_qic_driver);
}

static void __exit dfx_qic_exit(void)
{
	platform_driver_unregister(&dfx_qic_driver);
}

late_initcall(dfx_qic_init);
module_exit(dfx_qic_exit);
#endif
