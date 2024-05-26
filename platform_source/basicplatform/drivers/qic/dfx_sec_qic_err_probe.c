/*
 * QIC err probe Module.
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

#include "dfx_sec_qic_err_probe.h"
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/smp.h>
#include <linux/slab.h>
#include <platform_include/see/bl31_smc.h>
#include <../common/sec_qic_share_info.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <platform_include/basicplatform/linux/util.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#define PR_LOG_TAG QIC_TAG

const char *g_dfx_sec_qic_err_resp[] = {
	"OKAY: normal access success or Exclusive fail",
	"EXOKAY: Exclusive access success",
	"SLVERR: Slave receive Error response or Decoder Error",
	"TMOERR: accessed slave timeout",
	"SECERR: Firewall intercpet and return SLVERR",
	"HIDEERR: Firewall intercpet and not return SLVERR",
	"DISCERR: access powerdown area",
	"UNSERR: received unsupported Opcode",
};

const char *g_dfx_sec_qic_safety_resp[] = {
	"Command in the Table has not been processed",
	"The QTP command channel is back pressed",
	"Current AXI command is not processed continuously at the entry",
	"AXI's R or B channels are back pressed",
	"Not support to parse",
	"Not support to parse",
	"Current Axi write data is not collected",
	"Not support to parse",
};

#ifdef CONFIG_DFX_SEC_QIC_MODID_REGISTER
static unsigned int g_qic_modid[QIC_MODID_NUM_MAX] =  { QIC_MODID_NEGATIVE };
static unsigned int g_qic_modid_idx;
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

void dfx_qic_mdm_modid_register(unsigned int modid)
{
	dfx_qic_modid_register(QIC_MDM, modid);
}
EXPORT_SYMBOL(dfx_qic_mdm_modid_register);
#endif

static void dfx_sec_qic_print_dfx_info(const struct sec_qic_dfx_info *dfx_info)
{
	pr_err("core_id is:0x%x\n", dfx_info->coreid);
	pr_err("irq_status is:0x%x\n", dfx_info->irq_status);
	pr_err("bus_key is:0x%x\n", dfx_info->bus_key);
	pr_err("ib_type is:0x%x\n", dfx_info->ib_type);
	pr_err("ib_dfx_low is:0x%llx\n", dfx_info->ib_dfx_low);
	pr_err("ib_dfx_high is:0x%llx\n", dfx_info->ib_dfx_high);
	if (dfx_info->ib_name[0])
		pr_err("ib_name is:%s\n", dfx_info->ib_name);
	else
		pr_err("no ib error on this bus\n");
	if (dfx_info->tb_name[0])
		pr_err("tb_name is:%s\n", dfx_info->tb_name);
	else
		pr_err("no tb error on this bus\n");
}

static struct qic_bus_info* dfx_sec_qic_match_bus_info(u32 bus_key, const struct dfx_sec_qic_device *qic_dev)
{
	u32 i;

	for (i = 0; i < qic_dev->bus_info_num; i++) {
		if (bus_key == qic_dev->bus_info[i].bus_key)
			return &(qic_dev->bus_info[i]);
	}

	return NULL;
}

static void dfx_sec_qic_parse_ib_info_with_type(const struct sec_qic_dfx_info *dfx_info, u32 type)
{
	u32 rw_opc;
	u32 qtp_rsp;
	const char *rsp_msg = NULL;

	/* bit[120:118]:if it is safety irq, use safety rsp */
	qtp_rsp = (u32)((dfx_info->ib_dfx_high >> DFX_SEC_QIC_QTP_RSP_OFFSET) & DFX_SEC_QIC_QTP_RSP_MASK);
	if (type == DFX_SEC_QIC_INT_NORMAL_TYPE) {
		rsp_msg = g_dfx_sec_qic_err_resp[qtp_rsp];
		rw_opc = ((dfx_info->irq_status >> DFX_SEC_QIC_RW_NORMAL_OFFSET) & DFX_SEC_QIC_RW_MASK);
	} else if (type == DFX_SEC_QIC_INT_SAFETY_TYPE) {
		rsp_msg = g_dfx_sec_qic_safety_resp[qtp_rsp];
		rw_opc = ((dfx_info->irq_status >> DFX_SEC_QIC_RW_SAFETY_OFFSET) & DFX_SEC_QIC_RW_MASK);
	} else {
		rsp_msg = "Not support to parse";
		rw_opc = DFX_SEC_QIC_RW_ERROR;
	}
	pr_err("qtp_rsp = 0x%x,%s\n", qtp_rsp, rsp_msg);

	if (rw_opc == DFX_SEC_QIC_WRITE_OPERATION)
		pr_err("opreation =%u: write operation\n", rw_opc);
	else if (rw_opc == DFX_SEC_QIC_READ_OPERATION)
		pr_err("opreation =%u: read operation\n", rw_opc);
	else
		pr_err("opreation =%u: read/write operation parse error\n", rw_opc);
}

static const char *dfx_sec_qic_get_mid_msg(u32 mid, const struct qic_master_info *master_info, u32 master_num)
{
	u32 i;

	for (i = 0; i < master_num; i++) {
		if (mid == master_info[i].masterid_value)
			return master_info[i].masterid_name;
	}

	return NULL;
}

static void dfx_sec_qic_parse_master_id(u32 mid, const struct sec_qic_dfx_info *dfx_info,
					const struct dfx_sec_qic_device *qic_dev)
{
	const char *mid_msg = NULL;
	const char *acpu_msg = NULL;
	u32 acpu_coreid;

	mid_msg = dfx_sec_qic_get_mid_msg(mid, qic_dev->mid_info, qic_dev->mid_info_num);
	if (mid_msg)
		pr_err("MID(Master id) = %u, means:%s\n", mid, mid_msg);
	else
		pr_err("MID(Master id) = %u, master name mismatch\n", mid);

	if (dfx_info->coreid == DFX_SEC_QIC_FCM_PERI_PORT) {
		acpu_coreid = (u32)(dfx_info->ib_dfx_low >> DFX_SEC_QIC_ACPU_COREID_OFFSET) &
				   DFX_SEC_QIC_ACPU_COREID_MASK;
		acpu_msg = dfx_sec_qic_get_mid_msg(acpu_coreid, qic_dev->acpu_core_info, qic_dev->acpu_core_info_num);
		if (acpu_msg)
			pr_err("FCM_PERI_PORT err, acpu coreid is:0x%x, means:%s\n", acpu_coreid, acpu_msg);
		else
			pr_err("FCM_PERI_PORT err, acpu coreid is:0x%x, core name mismatch\n", acpu_coreid);
	}
}

static void dfx_sec_qic_parse_dfx_info(const struct sec_qic_dfx_info *dfx_info,
					const struct dfx_sec_qic_device *qic_dev)
{
	u64 qic_address;
	u32 qtp_sf;
	u32 mid;
	u32 type;
#ifdef CONFIG_DFX_SEC_QIC_MODID_REGISTER
	u32 modid = QIC_MODID_NEGATIVE;
#endif

	struct qic_bus_info* bus_info= dfx_sec_qic_match_bus_info(dfx_info->bus_key, qic_dev);
	if (!bus_info) {
		pr_err("get bus info fail\n");
		return;
	}

	type = dfx_info->irq_status & DFX_SEC_QIC_INT_STATUS_MASK;
	if (!type) {
		pr_err("ib err probe not happen, no need to parse\n");
		return;
	}
	dfx_sec_qic_parse_ib_info_with_type(dfx_info, type);

	/* bit[84:47]:qtp addr */
	if (dfx_info->ib_type) {
		qic_address = dfx_sec_qic_parse_addr(dfx_info->ib_dfx_high, dfx_info->ib_dfx_low);
		pr_err("qic_addr = 0x%llx\n", qic_address);
	} else {
		pr_err("qic_addr not support, please check dmss/qice info\n");
	}
	/* bit[46:41]:only use last bit */
	qtp_sf = ((u32)(dfx_info->ib_dfx_low >> DFX_SEC_QIC_QTP_SF_OFFSET) & DFX_SEC_QIC_QTP_SF_MASK);
	if (qtp_sf == 1)
		pr_err("sf = %u: no Security access\n", qtp_sf);
	else
		pr_err("sf = %u: Security access\n", qtp_sf);

	mid = (u32)(dfx_info->ib_dfx_low >> (bus_info->mid_offset)) & DFX_SEC_QIC_MID_MASK;
	dfx_sec_qic_parse_master_id(mid, dfx_info, qic_dev);

#ifdef CONFIG_DFX_SEC_QIC_MODID_REGISTER
	modid = dfx_qic_modid_find(dfx_info->coreid);
	if ((modid != QIC_MODID_NEGATIVE) && (g_qic_modid_idx < QIC_MODID_NUM_MAX))
		g_qic_modid[g_qic_modid_idx++] = modid;
#endif
}

static void dfx_sec_qic_parse_process(const struct dfx_sec_qic_device *qic_dev)
{
	u64 *dfx_info_num = qic_dev->qic_share_base;
	struct sec_qic_dfx_info *dfx_info = NULL;
	u32 i;
	struct qic_bus_info *bus_info = NULL;

	if (!dfx_info_num || *dfx_info_num > MAX_QIC_DFX_INFO) {
		pr_err("[%s] share data error!\n", __func__);
		return;
	}

	dfx_info = (struct sec_qic_dfx_info *)(dfx_info_num + 1);

	pr_err("start to show qic error info!\n");
	for (i = 0; i < *dfx_info_num; i++) {
		bus_info = dfx_sec_qic_match_bus_info((dfx_info + i)->bus_key, qic_dev);
		if (!bus_info) {
			pr_err("get bus info fail\n");
			return;
		}
		pr_err("******qic_err occurred on %s******\n", bus_info->bus_name);
		dfx_sec_qic_print_dfx_info(dfx_info + i);
		dfx_sec_qic_parse_dfx_info(dfx_info + i, qic_dev);
	}
	pr_err("end to show qic error info!\n");
}

static bool dfx_sec_qic_need_stop_acpu(u32 bus_key, const struct dfx_sec_qic_device *qic_dev)
{
	u32 i;

	for (i = 0; i < qic_dev->need_stop_cpu_buses_num; i++) {
		if (bus_key == qic_dev->need_stop_cpu_buses[i])
			return true;
	}

	return false;
}

static void dfx_sec_qic_err_smp_stop_acpu(unsigned long bus_key, const struct dfx_sec_qic_device *qic_dev)
{
	if (check_mntn_switch(MNTN_NOC_ERROR_REBOOT) && dfx_sec_qic_need_stop_acpu((u32)bus_key, qic_dev)) {
		/* Change Consloe Log Level. */
		console_verbose();
		/* Stop Other CPUs. */
		smp_send_stop();
		/* Debug Print. */
		pr_err("%s -- Stop CPUs First.\n", __func__);
	}
}

void dfx_sec_qic_reset_handler(void)
{
	unsigned int modid_match_flag = 0;
#ifdef CONFIG_DFX_SEC_QIC_MODID_REGISTER
	unsigned int i;
#endif

	if (check_mntn_switch(MNTN_NOC_ERROR_REBOOT)) {
#ifdef CONFIG_DFX_SEC_QIC_MODID_REGISTER
		for (i = 0; i < g_qic_modid_idx; i++) {
			if (g_qic_modid[i] != QIC_MODID_NEGATIVE) {
				modid_match_flag = 1;
				pr_err("[%s] qic modid is matched, modid = 0x%x!\n", __func__, g_qic_modid[i]);
				rdr_system_error(g_qic_modid[i], 0, 0);
			}
		}
#endif
		if (modid_match_flag == 0) {
			pr_err("%s qic reset begin\n", __func__);
			rdr_syserr_process_for_ap(MODID_AP_S_NOC, 0, 0);
		}
	}
}

int dfx_sec_qic_err_probe_handler(const struct dfx_sec_qic_device *qic_dev)
{
	unsigned long position;
	unsigned long value;
	u32 i;
	u32 j;

	if (!qic_dev) {
		pr_err("[%s] qic_init fail!\n", __func__);
		return -1;
	}

#ifdef CONFIG_DFX_SEC_QIC_MODID_REGISTER
	g_qic_modid_idx = 0;
#endif

	for (i = 0; i < qic_dev->irq_reg_num; i++) {
		for (j = 0; j < qic_dev->irq_reg[i].len; j++) {
			value = readl_relaxed(qic_dev->irq_reg[i].reg_base +
				qic_dev->irq_reg[i].offsets[j]);
			pr_err("qic irq status:0x%lx\n", value);
			value = value & qic_dev->irq_reg[i].reg_mask[j];
			for_each_set_bit(position, &value, BITS_PER_LONG) {
				pr_err("irq bit:%lu\n", position);
				dfx_sec_qic_err_smp_stop_acpu(position, qic_dev);
				(void)dfx_sec_qic_smc_call(QIC_SECIRQ_FID_SUBTYPE, position + j * BITS_PER_TYPE(u32)
					+ i * qic_dev->irq_reg[i].len * BITS_PER_TYPE(u32));
			}
		}
	}
	dfx_sec_qic_parse_process(qic_dev);
	return 0;
}

