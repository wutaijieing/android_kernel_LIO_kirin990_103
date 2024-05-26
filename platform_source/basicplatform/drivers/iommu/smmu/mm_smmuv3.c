/*
 * Copyright(C) 2019-2020 Hisilicon Technologies Co., Ltd. All rights reserved.
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

#define pr_fmt(fmt) "smmuv3_svm:" fmt

#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <platform_include/basicplatform/linux/dfx_bbox_diaginfo.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/sched/mm.h>
#include <linux/sched/task.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/arm-smccc.h>
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 4, 0))
#include <asm/compiler.h>
#endif


#include "platform_include/see/bl31_smc.h"
#include "mm_svm.h"
#include "platform_smmu_v3.h"

enum mm_svm_lvl {
	SVM_ERROR = 0,
	SVM_WARN = 1,
	SVM_DEBUG = 2,
	SVM_INFO = 3,
	SVM_TRACE = 4,
};

static int svm_debug_level = SVM_INFO;
module_param_named(level, svm_debug_level, int, 0444);

#define mm_svm_print(level, x...)                                        \
	do { if (svm_debug_level >= (level)) pr_err(x); } while (0)

static struct mutex mm_svm_mutex;

LIST_HEAD(mm_svm_list);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#define MMF_SVM           32      /* start from high 32bit */
#endif

static unsigned long smc_ops_list[MM_TCU_OPS_MAX] = {
	MM_SMMU_FID_TCUCTL_UNSEC, /* MM_TCU_CTL_UNSEC */
	MM_SMMU_FID_NODE_STATUS, /* MM_TCU_NODE_STATUS */
	MM_SMMU_FID_TCU_CLK_ON, /* MM_TCU_CLK_ON */
	MM_SMMU_FID_TCU_CLK_OFF, /* MM_TCU_CLK_OFF */
	MM_SMMU_FID_TCU_TTWC_BYPASS_ON, /* TTWC_BYPASS_ON */
};

int mm_smmu_poweron(struct device *dev)
{
	int ret;

	if (!dev) {
		pr_err("%s, dev is null\n", __func__);
		return -ENODEV;
	}

	mutex_init(&mm_svm_mutex);
	ret = arm_smmu_poweron(dev);
	if (ret)
		dev_err(dev, "%s, smmu power on failed\n", __func__);
	return ret;
}

int mm_smmu_poweroff(struct device *dev)
{
	int ret;

	if (!dev) {
		pr_err("%s, dev is null\n", __func__);
		return -ENODEV;
	}

	ret = arm_smmu_poweroff(dev);
	if (ret)
		dev_err(dev, "%s, smmu power off failed\n", __func__);
	return ret;
}

int mm_svm_get_ssid(struct mm_svm *svm, u16 *ssid, u64 *ttbr, u64 *tcr)
{
	int ret;

	if (!svm || !ssid || !ttbr || !tcr)
		return -EINVAL;

	if (!svm->dom)
		return -EINVAL;
	ret = arm_smmu_svm_get_ssid(svm->dom, ssid, ttbr, tcr);
	if (ret)
		mm_svm_print(SVM_ERROR, "%s get ssid error!\n", __func__);

	return ret;
}

void mm_svm_dump_pagetable(pgd_t *base_pgd, unsigned long addr, size_t size)
{
	size_t step;
	pgd_t *pgd = NULL;
	pud_t *pud = NULL;
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;

	do {
		step = SZ_4K;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		pgd = pgd_offset_raw(base_pgd, addr);
#else
		pgd = pgd_offset_pgd(base_pgd, addr);
#endif
		pr_err("[0x%08lx], *pgd=0x%016llx", addr, pgd_val(*pgd));
		if (pgd_none(*pgd) || pgd_bad(*pgd)) {
			step = SZ_1G;
			continue;
		}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		pud = pud_offset(pgd, addr);
#else
		pud = pud_offset((p4d_t *)pgd, addr);
#endif
		pr_err(", *pud=0x%016llx", pud_val(*pud));
		if (pud_none(*pud) || pud_bad(*pud)) {
			step = SZ_1G;
			continue;
		}

		pmd = pmd_offset(pud, addr);
		pr_err(", *pmd=0x%016llx", pmd_val(*pmd));
		if (pmd_none(*pmd) || pmd_bad(*pmd)) {
			step = SZ_2M;
			continue;
		}

		pte = pte_offset_map(pmd, addr);
		pr_err(", *pte=0x%016llx", pte_val(*pte));
		pte_unmap(pte);
		pr_err("\n");
	} while (size >= step && (addr += step, size -= step));
}

void mm_svm_show_pte(struct mm_svm *svm, unsigned long addr, size_t size)
{
	struct mm_struct *mm = NULL;
	struct task_struct *task = NULL;
	struct pid *bindpid = NULL;

	if (!svm || !svm->mm || !size)
		return;

	bindpid = find_get_pid(svm->pid);
	if (!bindpid) {
		pr_err("task exit,%d", svm->pid);
		return;
	}

	task = get_pid_task(bindpid, PIDTYPE_PID);
	if (!task) {
		pr_err("task exit,%d", svm->pid);
		put_pid(bindpid);
		return;
	}

	if (task->mm != svm->mm) {
		pr_err("task exit,%d, %pK, %pK", svm->pid, task->mm, svm->mm);
		put_pid(bindpid);
		put_task_struct(task);
		return;
	}

	put_pid(bindpid);
	put_task_struct(task);

	spin_lock(&svm->mm->page_table_lock);

	mm = svm->mm;
	pr_alert("cached pgd = %pK\n", mm->pgd);

	mm_svm_dump_pagetable(mm->pgd, addr, size);

	spin_unlock(&svm->mm->page_table_lock);
}

int mm_svm_flag_set(struct task_struct *task, u32 flag)
{
	struct mm_struct *mm = NULL;

	if (!task) {
		mm_svm_print(SVM_ERROR, "%s param invalid!\n", __func__);
		return -EINVAL;
	}

	mm = get_task_mm(task);
	if (!mm) {
		mm_svm_print(SVM_ERROR, "%s get_task_mm failed!\n", __func__);
		return -EINVAL;
	}
	if (flag)
		set_bit(MMF_SVM, &mm->flags);
	else
		clear_bit(MMF_SVM, &mm->flags);

	mmput(mm);
	mm_svm_print(SVM_INFO, "into %s, pid:%d,name:%s,pgd:%pK,flag:%d\n",
		__func__, task->pid, task->comm, mm->pgd, flag);
	return 0;
}

static int mm_svm_set_task_info(struct mm_svm *svm,
				struct task_struct *task)
{
	struct mm_struct *mm = NULL;

	svm->name = task->comm;
	mm = get_task_mm(task);
	if (!mm) {
		mm_svm_print(SVM_ERROR, "%s get_task_mm err!\n", __func__);
		return -EINVAL;
	}
	svm->mm = mm;
	svm->pid = task->pid;
	/*
	 * Drop the reference to the mm_struct here. We rely on the
	 * mmu_notifier release call-back to inform us when the mm
	 * is going away.
	 */
	mmput(mm);

	return 0;
}

static int mm_svm_domain_set_pgd(struct device *dev,
				struct iommu_domain *dom,
				struct task_struct *task)
{
	int ret;
	struct iommu_fwspec *fwspec = NULL;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	fwspec = dev->iommu_fwspec;
#else
	fwspec = dev_iommu_fwspec_get(dev);
#endif
	if (!fwspec) {
		mm_svm_print(SVM_ERROR, "%s fwspec is null\n", __func__);
		return -ENODEV;
	}

	if (!fwspec->ops) {
		mm_svm_print(SVM_ERROR, "%s fwspec ops is null\n", __func__);
		return -ENODEV;
	}

	if (!fwspec->ops->domain_set_attr) {
		mm_svm_print(SVM_ERROR, "%s domain_set_attr is null\n",
			__func__);
		return -ENODEV;
	}

	ret = fwspec->ops->domain_set_attr(dom, DOMAIN_ATTR_PGD, task);
	if (ret)
		mm_svm_print(SVM_ERROR, "%s domain_set_attr pgd err %d\n",
			__func__, ret);

	return ret;
}

static struct arm_smmu_domain *to_smmu_domain(struct iommu_domain *dom)
{
	return container_of(dom, struct arm_smmu_domain, domain);
}

struct mm_svm *mm_svm_bind_task(struct device *dev,
				struct task_struct *task)
{
	struct mm_svm *svm = NULL;
	struct iommu_domain *dom = NULL;
	struct iommu_group *group = NULL;
	struct arm_smmu_domain *smmu_domain = NULL;

	mm_svm_print(SVM_INFO, "into %s\n", __func__);
	if (!dev || !task) {
		mm_svm_print(SVM_ERROR, "%s param invalid!\n", __func__);
		return NULL;
	}

	mutex_lock(&mm_svm_mutex);
	svm = kzalloc(sizeof(*svm), GFP_KERNEL);
	if (!svm) {
		mm_svm_print(SVM_ERROR, "%s kzalloc failed!\n", __func__);
		goto out;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	dom = iommu_domain_alloc(dev->bus);
#else
	dom = iommu_fwspec_domain_alloc(dev->iommu->fwspec, IOMMU_DOMAIN_UNMANAGED);
#endif
	if (!dom) {
		mm_svm_print(SVM_ERROR, "%s domain alloc err!\n", __func__);
		goto out_free;
	}
	svm->dom = dom;

	smmu_domain = to_smmu_domain(dom);
	if (unlikely(!smmu_domain)) {
		dev_err(dev, "smmu_domain is null %s\n", __func__);
		goto dom_free;
	}

	group = dev->iommu_group;
	if (mm_svm_domain_set_pgd(dev, dom, task)) {
		mm_svm_print(SVM_ERROR, "%s set task info err!\n", __func__);
		goto dom_free;
	}

	dev->iommu_group = NULL;
	if (iommu_attach_device(dom, dev)) {
		mm_svm_print(SVM_ERROR, "%s attach device err!\n", __func__);
		goto dom_free;
	}
	svm->dev = dev;

	if (mm_svm_set_task_info(svm, task)) {
		mm_svm_print(SVM_ERROR, "%s set task info err!\n", __func__);
		goto iommu_attach;
	}

	dev->iommu_group = group;

	if (!smmu_domain->smmu->is_arm_smmu)
		list_add_tail(&svm->list, &mm_svm_list);

	mm_svm_print(SVM_INFO, "out %s\n", __func__);
	mutex_unlock(&mm_svm_mutex);
	return svm;

iommu_attach:
	iommu_detach_device(svm->dom, svm->dev);

dom_free:
	dev->iommu_group = group;
	iommu_domain_free(dom);

out_free:
	kfree(svm);

out:
	mutex_unlock(&mm_svm_mutex);
	return NULL;
}

void mm_svm_unbind_task(struct mm_svm *svm)
{
	struct iommu_group *group = NULL;
	struct arm_smmu_domain *smmu_domain = NULL;

	mm_svm_print(SVM_INFO, "in %s\n", __func__);
	if (!svm) {
		mm_svm_print(SVM_ERROR, "%s:svm NULL\n", __func__);
		return;
	}

	mutex_lock(&mm_svm_mutex);

	smmu_domain = to_smmu_domain(svm->dom);

	if (!smmu_domain->smmu->is_arm_smmu)
		list_del(&svm->list);

	group = svm->dev->iommu_group;
	svm->dev->iommu_group = NULL;
	iommu_detach_device(svm->dom, svm->dev);
	svm->dev->iommu_group = group;

	iommu_domain_free(svm->dom);
	kfree(svm);

	mutex_unlock(&mm_svm_mutex);
	mm_svm_print(SVM_INFO, "out %s\n", __func__);
}

static void pte_flush_range(pmd_t *pmd, unsigned long addr, unsigned long end)
{
	pte_t *pte = NULL;
	pte_t *pte4k = NULL;

	pte = pte_offset_map(pmd, addr);
	pte4k = (pte_t *)round_down((u64)pte, PAGE_SIZE);
	__flush_dcache_area(pte4k, PAGE_SIZE);

	pte_unmap(pte);
}

static void pmd_flush_range(pud_t *pud, unsigned long addr, unsigned long end)
{
	pmd_t *pmd = NULL;
	pmd_t *pmd4k = NULL;
	unsigned long next;

	pmd = pmd_offset(pud, addr);
	pmd4k = (pmd_t *)round_down((u64)pmd, PAGE_SIZE);
	do {
		next = pmd_addr_end(addr, end);
		if (pmd_sect(*pmd))
			continue;

		pte_flush_range(pmd, addr, next);
	} while (pmd++, addr = next, addr != end);

	__flush_dcache_area(pmd4k, PAGE_SIZE);
}

static void pud_flush_range(pgd_t *pgd, unsigned long addr, unsigned long end)
{
	pud_t *pud = NULL;
#if CONFIG_PGTABLE_LEVELS > 3
	pud_t *pud4k = NULL;
#endif
	unsigned long next;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	pud = pud_offset(pgd, addr);
#else
	pud = pud_offset((p4d_t *)pgd, addr);
#endif
#if CONFIG_PGTABLE_LEVELS > 3
	pud4k = (pud_t *)round_down((u64)pud, PAGE_SIZE);
#endif

	do {
		next = pud_addr_end(addr, end);
		if (pud_sect(*pud))
			continue;

		pmd_flush_range(pud, addr, next);
	} while (pud++, addr = next, addr != end);

#if CONFIG_PGTABLE_LEVELS > 3
	__flush_dcache_area(pud4k, PAGE_SIZE);
#endif
}

int mm_flush_pgtbl_cache(pgd_t *raw_pgd, unsigned long addr, size_t size)
{
	pgd_t *pgd = NULL;
	pgd_t *pgd4k = NULL;
	unsigned long next;
	unsigned long end = addr + PAGE_ALIGN(size);

	if (!raw_pgd) {
		pr_err("%s, bad pgd, addr:0x%lx, size:0x%zx\n",
			__func__, addr, size);
		return -EINVAL;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	pgd = pgd_offset_raw(raw_pgd, addr);
#else
	pgd = pgd_offset_pgd(raw_pgd, addr);
#endif
	pgd4k = (pgd_t *)round_down((u64)pgd, PAGE_SIZE);

	do {
		next = pgd_addr_end(addr, end);
		pud_flush_range(pgd, addr, next);
		pgd++;
		addr = next;
	} while (addr != end);

	__flush_dcache_area(pgd4k, PAGE_SIZE);
	return 0;
}

int mm_svm_flush_cache(struct mm_struct *mm, unsigned long addr, size_t size)
{
	if (!mm) {
		pr_err("%s, bad mm, addr:0x%lx, size:0x%zx\n",
			__func__, addr, size);
		return -EINVAL;
	}
	(void)mm_flush_pgtbl_cache(mm->pgd, addr, size);
	arm_smmu_svm_tlb_inv_context(mm); /* flush tlb when mmap */

	return 0;
}

int mm_smmu_evt_register_notify(struct device *dev, struct notifier_block *n)
{
	return arm_smmu_evt_register_notify(dev, n);
}

int mm_smmu_evt_unregister_notify(struct device *dev,
				struct notifier_block *n)
{
	return arm_smmu_evt_unregister_notify(dev, n);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static noinline u64 smc_smmu_sec_config(u64 fid, int a1, int a2, int a3)
{
	struct arm_smccc_res res;

	arm_smccc_smc(fid, a1, a2, a3, 0, 0, 0, 0, &res);

	return res.a0;
}
#else
static noinline u64 smc_smmu_sec_config(u64 fid, int a1, int a2, int a3)
{
	register u64 x0 asm("x0") = fid;
	register u64 x1 asm("x1") = a1;
	register u64 x2 asm("x2") = a2;
	register u64 x3 asm("x3") = a3;

	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")

		"smc    #0\n"
		: "+r" (x0)
		: "r" (x1), "r" (x2), "r"(x3));

	return x0;
}
#endif

u64 smc_tcu_ops(int smmuid, enum mm_smmu_smc_ops opcode)
{
	u64 ret;

	if (opcode >= MM_TCU_OPS_MAX) {
		pr_err("%s opcode error: smmuid = %d, opcode = %d\n", __func__, smmuid, opcode);
		return (u64)-EINVAL;
	}

	ret = smc_smmu_sec_config(smc_ops_list[opcode], smmuid, 0, 0);
	if (opcode != MM_TCU_NODE_STATUS) {
		if (ret)
			pr_err("%s fail: smmuid = %d, opcode = %d, ret = %llu\n", __func__, smmuid, opcode, ret);
	}
	return ret;
}

int mm_smmu_nonsec_tcuctl(int smmuid)
{
	u64 ret;

	ret = smc_tcu_ops(smmuid, MM_TCU_CTL_UNSEC);
	pr_info("%s, smmuid:%d, ret:0x%llx\n", __func__, smmuid, ret);
	return 0;
}

int mm_smmu_tcu_node_status(int smmuid)
{
	u64 ret;

	ret = smc_tcu_ops(smmuid, MM_TCU_NODE_STATUS);
	pr_info("%s, smmuid:%d, ret:0x%llx\n", __func__, smmuid, ret);
	return ret;
}

void mm_smmu_dump_tbu_status(struct device *dev)
{
	if (!dev) {
		pr_err("%s input invalid\n", __func__);
		return;
	}
	arm_smmu_tbu_status_print(dev);
}

bool mm_get_smmu_info(struct device *dev, char *buffer, int length)
{
	int ret;

	if (!dev || !buffer || !length) {
		pr_err("%s input invalid\n", __func__);
		return false;
	}
	ret = arm_smmu_get_reg_info(dev, buffer, length);
	if (ret)
		return false;
	else
		return true;
}

#ifdef CONFIG_MM_IOMMU_DMD_REPORT
#define MAX_NAME_LENGTH		20
#define MAX_DMD_REG_INFO	2
static DEFINE_SPINLOCK(g_smmu_dmd_lock);
struct dmd_info {
	bool is_init;
	unsigned int err_id;
	char err_name[MAX_NAME_LENGTH];
	unsigned int reg_info[MAX_DMD_REG_INFO];
	void __iomem *reg_base[MAX_DMD_REG_INFO];
};

static struct dmd_info g_smmu_dmd_info[SMMU_INDEX_MAX] = {
	/* PCTRL PERI_CTRL2, ACTRL AO_MEM_CTRL10 */
	{false, SMMU_NPU_NOC_INFO, {"NPU_TCU"}, {0xFEC3E00C, 0xFA894848}, {NULL, NULL}},
};

void mm_smmu_noc_dmd_upload(enum smmu_index index)
{
	int ret;
	unsigned long flags;
	unsigned int stat0;
	unsigned int stat1;

	if (index >= SMMU_INDEX_MAX) {
		pr_err("%s err input index %d\n", __func__, index);
		return;
	}

	spin_lock_irqsave(&g_smmu_dmd_lock, flags);

	if (g_smmu_dmd_info[index].is_init == false) {
		pr_err("%s %d dmd info is uninit\n", __func__, index);
		spin_unlock_irqrestore(&g_smmu_dmd_lock, flags);
		return;
	}
	/* reg_base has 2 members */
	stat0 = readl(g_smmu_dmd_info[index].reg_base[0]);
	stat1 = readl(g_smmu_dmd_info[index].reg_base[1]);

	spin_unlock_irqrestore(&g_smmu_dmd_lock, flags);

	ret = bbox_diaginfo_record(g_smmu_dmd_info[index].err_id, NULL,
		"%s [0x%x][0x%x]",
		g_smmu_dmd_info[index].err_name, stat0, stat1);
	if (ret)
		pr_err("%s diaginfo record err\n", __func__);
}

static int __init mm_smmu_dmd_init(void)
{
	unsigned long flags;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_smmu_dmd_info); i++) {
		 /* reg_base has 2 members, first init reg_base[0] */
		g_smmu_dmd_info[i].reg_base[0] = ioremap(g_smmu_dmd_info[i].reg_info[0], sizeof(u32));/*lint !e446*/
		if (!g_smmu_dmd_info[i].reg_base[0]) {
			pr_err("%s %d reg[0] ioremap failed\n", __func__, i);
			continue;
		}
		/* init reg_base[1] */
		g_smmu_dmd_info[i].reg_base[1] = ioremap(g_smmu_dmd_info[i].reg_info[1], sizeof(u32));/*lint !e446*/
		if (!g_smmu_dmd_info[i].reg_base[1]) {
			pr_err("%s %d reg[1] ioremap failed\n", __func__, i);
			iounmap(g_smmu_dmd_info[i].reg_base[0]);
			continue;
		}

		spin_lock_irqsave(&g_smmu_dmd_lock, flags);
		g_smmu_dmd_info[i].is_init = true;
		spin_unlock_irqrestore(&g_smmu_dmd_lock, flags);
	}

	return 0;
}

static void __exit mm_smmu_dmd_exit(void)
{
	unsigned long flags;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_smmu_dmd_info); i++) {
		if (g_smmu_dmd_info[i].is_init == true) {
			spin_lock_irqsave(&g_smmu_dmd_lock, flags);
			g_smmu_dmd_info[i].is_init = false;
			spin_unlock_irqrestore(&g_smmu_dmd_lock, flags);

			/* reg_base has 2 members, they need be iounmap when exit */
			iounmap(g_smmu_dmd_info[i].reg_base[0]);
			iounmap(g_smmu_dmd_info[i].reg_base[1]);
		}
	}
}

module_init(mm_smmu_dmd_init);
module_exit(mm_smmu_dmd_exit);
#endif

MODULE_DESCRIPTION(
	"SVM Base on ARM architected SMMUv3 implementations");
MODULE_LICENSE("GPL v2");
