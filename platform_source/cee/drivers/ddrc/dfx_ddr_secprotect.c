/*
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
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
#include "dfx_ddr_secprotect.h"
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include "securec.h"
#include <global_ddr_map.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <platform_include/basicplatform/linux/util.h>
#include <linux/platform_drivers/hisi_ddr.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#ifdef CONFIG_DDR_SUBREASON
#include "ddr_subreason/ddr_subreason.h"
#endif
#ifdef CONFIG_DRMDRIVER
#include <platform_include/display/linux/dpu_drmdriver.h>
#endif

#define DMR_SHARE_MEM_PHY_BASE (ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE + DRM_SHARE_MEM_OFFSET)
#define DMSS_OPTI_SHARE_MEM_SIZE 0x8
#define INTRSRC_MASK 0xFFFF

u64 *g_dmss_intr_fiq = NULL;
u64 *g_dmss_intr = NULL;
u64 *g_dmss_subreason = NULL;

struct dmss_subreason_mem {
	unsigned master_reason:8;
	unsigned sub_reason:8;
	unsigned init_flag:4;
};

#define DMSS_SUBREASON_OFFSET 28
#define DMSS_SUBREASON_ADDR (ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE + DMSS_SUBREASON_OFFSET)
#define DMSS_SUBREASON_SIZE 0x4
#define DMSS_SUBREASON_FAIL (-1)
#define DMSS_SUBREASON_SUCCESS 0
#define DMSS_MODID_FAIL 1
#define SUBREASON_MODID_FIND_FAIL 0

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#define ddr_ioremap ioremap
#else
#define ddr_ioremap ioremap_nocache
#endif

#ifdef CONFIG_DDR_SUBREASON
static void dmss_register_exception(void)
{
	u32 ret, i;
	u32 max_einfo;
	max_einfo = sizeof(g_ddr_einfo) / sizeof(struct rdr_exception_info_s);
	for (i = 0; i < max_einfo; i++) {
		ret = rdr_register_exception(&g_ddr_einfo[i]);
		if (ret == 0)
			pr_err("register dmss rxception fail ddr_einfo[%d]!\n", i);
	}
}

static u32 get_sub_reason_modid(u32 master_reason, u32 sub_reason)
{
	u32 i;
	u32 max_einfo;
	max_einfo = sizeof(g_ddr_einfo) / sizeof(struct rdr_exception_info_s);
	for (i = 0; i < max_einfo; i++)
		if (master_reason == g_ddr_einfo[i].e_exce_type &&
		    sub_reason == g_ddr_einfo[i].e_exce_subtype) {
			pr_info("[%s], modid is [0x%x]\n", __func__, g_ddr_einfo[i].e_modid);
			return g_ddr_einfo[i].e_modid;
		}

	pr_err("cannot find modid!\n");
	return DMSS_MODID_FAIL;
}

static s32 dmss_subreason_get(struct dmss_subreason_mem *para_mem)
{
	struct dmss_subreason_mem *subreason_info = NULL;

	if (g_dmss_subreason == NULL) {
		g_dmss_subreason = ddr_ioremap(DMSS_SUBREASON_ADDR, DMSS_SUBREASON_SIZE);
		if (g_dmss_subreason == NULL) {
			pr_err("%s fiq_handler ddr_ioremap fail\n", __func__);
			return DMSS_SUBREASON_FAIL;
		}
	}

	pr_err("%s share mem data is %pK\n", __func__, g_dmss_subreason);

	subreason_info = (struct dmss_subreason_mem *)g_dmss_subreason;

	para_mem->master_reason = subreason_info->master_reason;
	para_mem->sub_reason = subreason_info->sub_reason;

	pr_err("%s exe info master_reason[%u],sub_reason[%u]\n", __func__,
		para_mem->master_reason, para_mem->sub_reason);

	return DMSS_SUBREASON_SUCCESS;
}

static u32 dmss_subreason_modid_find(void)
{
	struct dmss_subreason_mem subreason_mem = { 0 };
	u32 ret;
	s32 eret;

	eret = dmss_subreason_get(&subreason_mem);
	if (eret == DMSS_SUBREASON_FAIL) {
		pr_err("%s subreason get fail\n", __func__);
		return 0;
	}

	ret = get_sub_reason_modid(subreason_mem.master_reason, subreason_mem.sub_reason);
	if (ret == DMSS_MODID_FAIL) {
		pr_err("%s subreason get modi fail\n", __func__);
		return 0;
	}

	return ret;
}
#else
static u32 dmss_subreason_modid_find(void)
{
	return SUBREASON_MODID_FIND_FAIL;
}
#endif

#ifdef CONFIG_MNTN_DMSS_OPTI

/*
 * for share memory,we get 32bit,use as follow:
 */
struct dmss_int_mem {
	unsigned mst_type : 8;
	unsigned mid : 8;
	unsigned err_reason : 8;
	unsigned init_flag : 4;
	unsigned reserved : 4;
};

/* dmss err type */
enum dmss_error_reason {
	DMSS_ERR_REASON_ASI_DFX,
	DMSS_ERR_REASON_AMI_PROT,
	DMSS_ERR_REASON_AMI_ENHN,
	DMSS_ERR_REASON_REP_ERR,
	DMSS_ERR_REASON_ASI_SEC,
	DMSS_ERR_REASON_DMI_SEC,
	DMSS_ERR_REASON_DMI_ENHN,
	DMSS_ERR_REASON_DMI_ENHN_CFG,
	DMSS_ERR_REASON_AMI_SEC,
	DMSS_ERR_REASON_MPU,
	DMSS_ERR_REASON_DETECT,
	DMSS_ERR_REASON_TRACE,
	DMSS_ERR_REASON_RBUF_STAT,
	DMSS_ERR_REASON_TZMP,
	DMSS_ERR_REASON_CFG_LOCK,
	DMSS_ERR_REASON_BUTTOM,
};

struct dmss_mid_modid_trans_s {
	struct list_head s_list;
	u32 mid;
	u32 modid;
	enum dmss_error_reason reason;
	void *reserve_p;
};

struct dmss_err_para_s {
	u32 mid;
	u32 reserved;
	enum dmss_error_reason reason;
};

#define DMSS_OPTI_LIST_MAX_LENGTH 1024
#define DMSS_OPTI_FAIL (-1)
#define DMSS_OPTI_SUCCESS 0
#define SOC_MID_MAX 0x7C
#define MODID_EXIST 1
#define MODID_NEGATIVE 0xFFFFFFFF
#define DMSS_MASTER_TYPE_MODEM 0x01
#define DMSS_EXC_INFO_ADDR (ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE + 3 * DMSS_EXC_INFO_SIZE)
#define DMSS_EXC_INFO_SIZE 0x8
#define DMSS_OPTI_INITVAL 0x00
#define DMSS_OPTI_SHVAL 0xFF

struct semaphore dmss_exc_happen_sem;
unsigned char dmss_sem_init_flag;

static LIST_HEAD(__dmss_modid_list);
static DEFINE_SPINLOCK(__dmss_modid_list_lock);
static u32 g_mid_max;

static s32 dmss_opti_sec_check(s32 mod_cnt)
{
	if (mod_cnt > DMSS_OPTI_LIST_MAX_LENGTH)
		return DMSS_OPTI_FAIL;
	return DMSS_OPTI_SUCCESS;
}

static s32 dmss_opti_get_excreason(struct dmss_err_para_s *para_mem, s32 *master_type)
{
	struct dmss_int_mem *mem_info = NULL;

	if (g_dmss_intr_fiq == NULL) {
		g_dmss_intr_fiq = ddr_ioremap(DMSS_EXC_INFO_ADDR, DMSS_EXC_INFO_SIZE);
		if (g_dmss_intr_fiq == NULL) {
			pr_err("%s fiq_handler ddr_ioremap fail\n", __func__);
			return DMSS_OPTI_FAIL;
		}
	}

	pr_err("%s share mem data is %llx\n", __func__, *g_dmss_intr_fiq);

	mem_info = (struct dmss_int_mem *)g_dmss_intr_fiq;

	para_mem->mid    = mem_info->mid;
	para_mem->reason = mem_info->err_reason;
	*master_type     = mem_info->mst_type;

	pr_err("%s exe info mid[%u],reason[%d],msttype[%d]\n", __func__, para_mem->mid, para_mem->reason, *master_type);

	return DMSS_OPTI_SUCCESS;
}

static u32 dmss_opti_check_modid(u32 modid, struct dmss_err_para_s *para_check)
{
	struct dmss_mid_modid_trans_s *p_modid_e = NULL;
	struct list_head *cur = NULL;
	struct list_head *next = NULL;
	unsigned long lock_flag;

	spin_lock_irqsave(&__dmss_modid_list_lock, lock_flag);

	list_for_each_safe(cur, next, &__dmss_modid_list) {
		p_modid_e = list_entry(cur, struct dmss_mid_modid_trans_s, s_list);
		if (p_modid_e == NULL) {
			pr_err("It might be better to look around here. %s:%d\n",
			__func__, __LINE__);
			continue;
		}

		if ((modid == p_modid_e->modid) ||
		   ((para_check->mid == p_modid_e->mid) && (para_check->reason == p_modid_e->reason))) {
			spin_unlock_irqrestore(&__dmss_modid_list_lock, lock_flag);
			return MODID_EXIST;
		}
	}
	spin_unlock_irqrestore(&__dmss_modid_list_lock, lock_flag);

	return DMSS_OPTI_SUCCESS;
}

static s32 dmss_opti_para_check(struct dmss_err_para_s *para_judge)
{
	if (para_judge == NULL)
		return DMSS_OPTI_FAIL;

	if (para_judge->mid > g_mid_max) {
		pr_err("%s masterid %u over !,g_mid_max[%x]\n", __func__, para_judge->mid, g_mid_max);
		return DMSS_OPTI_FAIL;
	}

	if (para_judge->reason >= DMSS_ERR_REASON_BUTTOM) {
		pr_err("%s err reason %d over !\n", __func__, para_judge->reason);
		return DMSS_OPTI_FAIL;
	}

	return DMSS_OPTI_SUCCESS;
}

static u32 __dmss_modid_find(struct dmss_err_para_s *para_comp, s32 master_type)
{
	struct dmss_mid_modid_trans_s *p_modid_e = NULL;
	struct list_head *cur = NULL;
	struct list_head *next = NULL;
	unsigned long lock_flag;

	spin_lock_irqsave(&__dmss_modid_list_lock, lock_flag);

	list_for_each_safe(cur, next, &__dmss_modid_list) {
		p_modid_e = list_entry(cur, struct dmss_mid_modid_trans_s, s_list);
		if (p_modid_e == NULL) {
			pr_err("It might be better to look around here. %s:%d\n",
			__func__, __LINE__);
			continue;
		}
		if ((para_comp->mid == p_modid_e->mid) &&
		   (para_comp->reason == p_modid_e->reason)) {
			spin_unlock_irqrestore(&__dmss_modid_list_lock, lock_flag);
			pr_err("%s we got match modid[%x]\n", __func__, p_modid_e->modid);
			return p_modid_e->modid;
		}
	}

	pr_err("%s we didnot got match modid\n", __func__);
	spin_unlock_irqrestore(&__dmss_modid_list_lock, lock_flag);

	if (master_type == DMSS_MASTER_TYPE_MODEM)
		return RDR_MODEM_DMSS_MOD_ID;

	return MODID_NEGATIVE;
}

static u32 dmss_opti_modid_find(struct dmss_err_para_s *para_judge, s32 master_type)
{
	s32 pret;
	u32 ret;

	pret = dmss_opti_para_check(para_judge);
	if (pret == DMSS_OPTI_FAIL)
		return MODID_NEGATIVE;

	ret = __dmss_modid_find(para_judge, master_type);
	if (ret == MODID_NEGATIVE)
		return MODID_NEGATIVE;
	else
		return ret;
}

static void __dmss_opti_modid_registe(struct dmss_mid_modid_trans_s *node)
{
	struct dmss_mid_modid_trans_s *p_info = NULL;
	struct list_head *cur = NULL;
	struct list_head *next = NULL;
	unsigned long lock_flag;

	if (node == NULL) {
		pr_err("invalid node:%pK\n", node);
		return;
	}

	spin_lock_irqsave(&__dmss_modid_list_lock, lock_flag);

	if (list_empty(&__dmss_modid_list)) {
		pr_err("list_add_tail masterid is [0x%x]\n", node->mid);
		list_add_tail(&(node->s_list), &__dmss_modid_list);
		goto out;
	}

	list_for_each_safe(cur, next, &__dmss_modid_list) {
		p_info = list_entry(cur, struct dmss_mid_modid_trans_s, s_list);
		if (node->mid < p_info->mid) {
			pr_err("list_add2 masterid is [0x%x]\n", node->mid);
			list_add(&(node->s_list), cur);
			goto out;
		}
	}
	pr_err("list_add_tail2 masterid is [0x%x]\n", node->mid);
	list_add_tail(&(node->s_list), &__dmss_modid_list);

out:
	spin_unlock_irqrestore(&__dmss_modid_list_lock, lock_flag);
}

/*
 * noc modid registe API,for use to registe own process, and before this,
 * noc err use the AP_S_NOC process, after adapt,user can define his own process.
 */
static void dmss_opti_modid_registe(struct dmss_err_para_s *para_judge, u32 modid)
{
	struct dmss_mid_modid_trans_s *node = NULL;
	static s32 mod_cnt;
	s32 pret;
	u32 ret;

	/* for sec,we check the max len of list */
	pret = dmss_opti_sec_check(mod_cnt);
	if (pret == DMSS_OPTI_FAIL) {
		pr_err("%s node is full!\n", __func__);
		return;
	}

	/* for sec,we check the input para */
	pret = dmss_opti_para_check(para_judge);
	if (pret == DMSS_OPTI_FAIL)
		return;

	/*
	 * before registe modid,we have to check is modid has been register
	 * berore,double check.
	 */
	ret = dmss_opti_check_modid(modid, para_judge);
	if (ret == MODID_EXIST) {
		pr_err("%s node is exist!\n", __func__);
		return;
	}

	node = kmalloc(sizeof(*node), GFP_ATOMIC);
	if (node == NULL) {
		pr_err("%s mem alloc fail!\n", __func__);
		return;
	}

	(void)memset_s(node, sizeof(*node), DMSS_OPTI_INITVAL, sizeof(*node));

	node->mid    = para_judge->mid;
	node->reason = para_judge->reason;
	node->modid  = modid;

	mod_cnt++;

	/*
	 * this func is the real func to registe the user's modid and
	 * user's err judge
	 */
	__dmss_opti_modid_registe(node);
}

void dmss_ipi_handler(void)
{
	pr_err("%s dmss_ipi_inform start\n", __func__);
	if (dmss_sem_init_flag == 0) {
		pr_err("%s dmss sem is not init\n", __func__);
		return;
	}
	up(&dmss_exc_happen_sem);
}

static int dmss_exc_process(void *arg)
{
	struct dmss_err_para_s para_mem;
	u32 modid_ret;
	u32 ret;
	s32 eret;
	s32 master_type = DMSS_OPTI_FAIL;
	s32 subreason_flag = DMSS_SUBREASON_SUCCESS;

	pr_err("%s start\n", __func__);

	down(&dmss_exc_happen_sem);

	pr_err("%s dmss exc happen,goto rdr_system_error\n", __func__);

	(void)memset_s(&para_mem, sizeof(para_mem), DMSS_OPTI_SHVAL, sizeof(para_mem));

	eret = dmss_opti_get_excreason(&para_mem, &master_type);
	if (eret == DMSS_OPTI_FAIL)
		pr_err("%s dmss_exc get excinfo fail\n", __func__);

	modid_ret = dmss_subreason_modid_find();
	if (modid_ret == 0) {
		subreason_flag = DMSS_SUBREASON_FAIL;
		pr_err("%s get suberason modid fail\n", __func__);
	}

	/* not use stop cpu,for rdr process will stop */
	ret = dmss_opti_modid_find(&para_mem, master_type);
	if (ret == MODID_NEGATIVE) {
		pr_err("%s we didnot got match modid\n", __func__);

		if (master_type == DMSS_MASTER_TYPE_MODEM) {
			rdr_system_error(RDR_MODEM_DMSS_MOD_ID, 0, 0);
		} else {
			if (subreason_flag == DMSS_SUBREASON_FAIL)
				rdr_syserr_process_for_ap(MODID_AP_S_DDRC_SEC, 0ULL, 0ULL);
			else
				rdr_syserr_process_for_ap(modid_ret, 0ULL, 0ULL);
		}
	} else {
		pr_err("%s we got match modid\n", __func__);

		rdr_system_error(ret, 0, 0);
	}

	return DMSS_OPTI_SUCCESS;
}

void dfx_mntn_dmss_opti_register_test(u32 modid, u32 mid, u32 reason)
{
	struct dmss_err_para_s test;

	test.mid = mid;
	test.reason = reason;

	dmss_opti_modid_registe(&test, modid);
}

static void fiq_print_src(u64 intrsrc)
{
	unsigned int intr = intrsrc & INTRSRC_MASK;

	/* check with FIQ number */
	if (intr == IRQ_WDT_INTR_FIQ) {
		pr_err("fiq triggered by: Watchdog\n");
	} else if (intr == IRQ_DMSS_INTR_FIQ) {
		smp_send_stop();
		pr_err("fiq triggered by: DMSS\n");
	} else {
		pr_err("fiq triggered by: Unknown, intr=0x%x\n", (unsigned int)intrsrc);
	}
}

void dmss_fiq_handler(void)
{
	u32 ret = MODID_NEGATIVE;

	if (g_dmss_intr == NULL) {
		pr_err("fiq_handler intr ptr is null\n");
		g_dmss_intr = ddr_ioremap(ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE, DMSS_OPTI_SHARE_MEM_SIZE);
		if (g_dmss_intr == NULL) {
			pr_err("fiq_handler ddr_ioremap fail\n");
			return;
		}
	}
	fiq_print_src(*g_dmss_intr);

	if ((g_dmss_intr != NULL) && ((*g_dmss_intr) == (u64)(DMSS_INTR_FIQ_FLAG | IRQ_DMSS_INTR_FIQ))) {
		pr_err("dmss fiq handler\n");
		pr_err("dmss intr happened. please see bl31 log\n");

		ret = dmss_subreason_modid_find();
		if (ret == 0) {
			pr_err("%s get suberason modid fail\n", __func__);
			rdr_syserr_process_for_ap(MODID_AP_S_DDRC_SEC, 0ULL, 0ULL);
		} else {
			rdr_syserr_process_for_ap(ret, 0ULL, 0ULL);
		}
	}
}

#else

struct semaphore modemddrc_happen_sem;

void dmss_ipi_handler(void)
{
	pr_crit("dmss_ipi_inform start\n");
	up(&modemddrc_happen_sem);
}

static int modemddrc_happen(void *arg)
{
	pr_err("modemddrc happen start\n");
	down(&modemddrc_happen_sem);
	pr_err("modemddrc happen goto rdr_system_error\n");
	rdr_system_error(RDR_MODEM_DMSS_MOD_ID, 0, 0);
	return 0;
}

static void fiq_print_src(u64 intrsrc)
{
	unsigned int intr = intrsrc & INTRSRC_MASK;

	/* check with FIQ number */
	if (intr == IRQ_WDT_INTR_FIQ) {
		pr_err("fiq triggered by: Watchdog\n");
	} else if (intr == IRQ_DMSS_INTR_FIQ) {
		smp_send_stop();
		pr_err("fiq triggered by: DMSS\n");
	} else {
		pr_err("fiq triggered by: Unknown, intr=0x%x\n", (unsigned int)intrsrc);
	}
}

void dmss_fiq_handler(void)
{
	u32 ret;

	if (g_dmss_intr_fiq == NULL) {
		pr_err("fiq_handler intr ptr is null\n");
		g_dmss_intr_fiq = ddr_ioremap(ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE,
			DMSS_OPTI_SHARE_MEM_SIZE);
		if (g_dmss_intr_fiq == NULL) {
			pr_err("fiq_handler ddr_ioremap fail\n");
			return;
		}
	}
	fiq_print_src(*g_dmss_intr_fiq);
	if ((g_dmss_intr_fiq != NULL) && ((u64)(DMSS_INTR_FIQ_FLAG | IRQ_DMSS_INTR_FIQ) == (*g_dmss_intr_fiq))) {
		pr_err("dmss fiq handler\n");
		pr_err("dmss intr happened. please see bl31 log\n");

		ret = dmss_subreason_modid_find();
		if (ret == 0) {
			pr_err("%s get suberason modid fail\n", __func__);
			rdr_syserr_process_for_ap(MODID_AP_S_DDRC_SEC, 0ULL, 0ULL);
		} else {
			rdr_syserr_process_for_ap(ret, 0ULL, 0ULL);
		}
	}
}

#endif

static int dfx_ddr_secprotect_probe(struct platform_device *pdev)
{
#ifdef CONFIG_MNTN_DMSS_OPTI
	s32 ret;
	struct device_node *np = NULL;
#endif

#ifdef CONFIG_DDR_SUBREASON
	dmss_register_exception();
#endif

#ifdef CONFIG_MNTN_DMSS_OPTI
	g_dmss_intr_fiq = ddr_ioremap(DMSS_EXC_INFO_ADDR, DMSS_EXC_INFO_SIZE);
#else
	g_dmss_intr_fiq = ddr_ioremap(ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE, DMSS_OPTI_SHARE_MEM_SIZE);
#endif
	if (g_dmss_intr_fiq == NULL) {
		pr_err(" ddr ddr_ioremap fail\n");
		return -ENOMEM;
	}

	g_dmss_subreason = ddr_ioremap(DMSS_SUBREASON_ADDR, DMSS_SUBREASON_SIZE);
	if (g_dmss_subreason == NULL) {
		pr_err(" ddr subreason ddr_ioremap fail\n");
		return -ENOMEM;
	}

#ifdef CONFIG_MNTN_DMSS_OPTI
	g_dmss_intr = ddr_ioremap(ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE, DMSS_OPTI_SHARE_MEM_SIZE);
	if (g_dmss_intr == NULL) {
		pr_err(" g_dmss_intr ddr_ioremap fail\n");
		return -ENOMEM;
	}
#endif

#ifdef CONFIG_MNTN_DMSS_OPTI
	sema_init(&dmss_exc_happen_sem, 0);
	dmss_sem_init_flag = DMSS_SEM_INIT_SUCCESS;
#else
	sema_init(&modemddrc_happen_sem, 0);
#endif

#ifdef CONFIG_MNTN_DMSS_OPTI
	np = of_find_compatible_node(NULL, NULL, "hisilicon,dmss_opti_mntn");
	if (np == NULL) {
		pr_err("[%s], find node fail!\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "mid_max", &g_mid_max);
	if (ret) {
		pr_err("[%s], cannot find mid_max in dts!\n", __func__);
		return -ENODEV;
	}
#endif

#ifdef CONFIG_MNTN_DMSS_OPTI
	if (!kthread_run(dmss_exc_process, NULL, "dmss_exc_process"))
		pr_err("create thread dmss_exc_process faild\n");
#else
	if (!kthread_run(modemddrc_happen, NULL, "modemddrc_emit"))
		pr_err("create thread modemddrc_happen faild\n");
#endif
	return 0;
}

static int dfx_ddr_secprotect_remove(struct platform_device *pdev)
{
	if (g_dmss_intr_fiq != NULL)
		iounmap(g_dmss_intr_fiq);

	g_dmss_intr_fiq = NULL;

	if (g_dmss_intr != NULL)
		iounmap(g_dmss_intr);

	g_dmss_intr = NULL;

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id hs_ddr_of_match[] = {
	{ .compatible = "hisilicon,ddr_secprotect", },
	{},
};
MODULE_DEVICE_TABLE(of, hs_ddr_of_match);
#endif

static struct platform_driver dfx_ddr_secprotect_driver = {
	.probe        = dfx_ddr_secprotect_probe,
	.remove       = dfx_ddr_secprotect_remove,
	.driver       = {
		.name           = "ddr_secprotect",
		.owner          = THIS_MODULE,
		.of_match_table = of_match_ptr(hs_ddr_of_match),
	},
};
module_platform_driver(dfx_ddr_secprotect_driver);

MODULE_LICENSE("GPL v2");
