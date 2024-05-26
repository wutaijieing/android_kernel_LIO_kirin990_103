#ifndef __MM_SVM_H
#define __MM_SVM_H
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mmu_notifier.h>
extern struct list_head mm_svm_list;

enum smmu_evt_type {
	PG_FAULT,
	HW_SERROR
};

enum smmu_index {
	SMMU_NPU_DMD,
	SMMU_INDEX_MAX
};

struct smmu_evt_msg {
	unsigned long fault_ss;
	unsigned long fault_reason;
	unsigned long fault_addr;
	unsigned long fault_resv;
};

struct mm_svm {
	char                *name;
	struct device       *dev;
	struct mm_struct    *mm;
	struct iommu_domain *dom;
	unsigned long        l2addr;
	struct dentry       *debug_root;
	pid_t                pid;
	struct list_head     list;
};

struct mm_aicpu_irq_info {
	u64 pgfault_asid_addr;
	u64 pgfault_va_addr;
	int (*callback)(void *); /* reserved */
	u64 cookie; /* reserved */
};

enum mm_smmu_smc_ops {
	MM_TCU_CTL_UNSEC,
	MM_TCU_NODE_STATUS,
	MM_TCU_CLK_ON,
	MM_TCU_CLK_OFF,
	MM_TCU_TTWC_BYPASS_ON,
	MM_TCU_OPS_MAX
};

#ifdef CONFIG_MM_SVM
int mm_smmu_poweron(int smmuid);
int mm_smmu_poweroff(int smmuid);
int mm_smmu_evt_register_notify(struct notifier_block *n);
int mm_smmu_evt_unregister_notify(struct notifier_block *n);
void mm_svm_unbind_task(struct mm_svm *svm);
struct mm_svm *mm_svm_bind_task(struct device *dev, struct task_struct *task);
void *mm_svm_get_l2buf_pte(struct mm_svm *svm, unsigned long addr);
void mm_svm_show_pte(struct mm_svm *svm, unsigned long addr, size_t size);
int mm_svm_get_ssid(struct mm_svm *svm, u16 *ssid,  u64 *ttbr, u64 *tcr);
int mm_svm_flush_cache(struct mm_struct *mm, unsigned long addr, size_t size);
int mm_aicpu_irq_offset_register(struct mm_aicpu_irq_info info);
int mm_svm_flag_set(struct task_struct *task, u32 flag);
int mm_smmu_nonsec_tcuctl(int smmuid);
#endif

#if defined(CONFIG_ARM_SMMU_V3) || defined(CONFIG_MM_SMMU_V3)
int mm_smmu_poweron(struct device *dev);
int mm_smmu_poweroff(struct device *dev);
int mm_smmu_evt_register_notify(struct device *dev, struct notifier_block *n);
int mm_smmu_evt_unregister_notify(struct device *dev, struct notifier_block *n);
void mm_svm_unbind_task(struct mm_svm *svm);
struct mm_svm *mm_svm_bind_task(struct device *dev, struct task_struct *task);
void *mm_svm_get_l2buf_pte(struct mm_svm *svm, unsigned long addr);
void mm_svm_show_pte(struct mm_svm *svm, unsigned long addr, size_t size);
int mm_svm_get_ssid(struct mm_svm *svm, u16 *ssid,  u64 *ttbr, u64 *tcr);
int mm_svm_flush_cache(struct mm_struct *mm, unsigned long addr, size_t size);
int mm_aicpu_irq_offset_register(struct mm_aicpu_irq_info info);
int mm_svm_flag_set(struct task_struct *task, u32 flag);
int mm_smmu_nonsec_tcuctl(int smmuid);
void mm_smmu_dump_tbu_status(struct device *dev);
u64 smc_tcu_ops(int smmuid, enum mm_smmu_smc_ops opcode);
#elif (!defined(CONFIG_MM_SVM))
static inline int mm_smmu_poweron(struct device *dev)
{
	return 0;
}
static inline int mm_smmu_poweroff(struct device *dev)
{
	return 0;
}
static inline int mm_smmu_evt_register_notify(struct device *dev,
	struct notifier_block *n)
{
	return 0;
}
static inline int mm_smmu_evt_unregister_notify(struct device *dev,
	struct notifier_block *n)
{
	return 0;
}
static inline void mm_svm_unbind_task(struct mm_svm *svm) { }
static inline struct mm_svm *mm_svm_bind_task(struct device *dev,
	struct task_struct *task)
{
	return NULL;
}
static inline void *mm_svm_get_l2buf_pte(struct mm_svm *svm, unsigned long addr)
{
	return NULL;
}
static inline void mm_svm_show_pte(struct mm_svm *svm,
	unsigned long addr, size_t size) { }
static inline int mm_svm_get_ssid(struct mm_svm *svm, u16 *ssid,  u64 *ttbr, u64 *tcr)
{
	return 0;
}
static inline int mm_svm_flush_cache(struct mm_struct *mm, unsigned long addr, size_t size)
{
	return 0;
}
static inline int mm_aicpu_irq_offset_register(struct mm_aicpu_irq_info info)
{
	return 0;
}
static inline int mm_svm_flag_set(struct task_struct *task, u32 flag)
{
	return 0;
}
static inline int mm_smmu_nonsec_tcuctl(int smmuid)
{
	return 0;
}
static inline void mm_smmu_dump_tbu_status(struct device *dev) { }
#endif /* CONFIG_ARM_SMMU_V3 */

#if defined(CONFIG_ARM_SMMU_V3) || defined(CONFIG_MM_SMMU_V3)
bool mm_get_smmu_info(struct device *dev, char *buffer, int length);
#if defined(CONFIG_MM_IOMMU_DMD_REPORT)
void mm_smmu_noc_dmd_upload(enum smmu_index index);
#else
static inline void mm_smmu_noc_dmd_upload(enum smmu_index index) { }
#endif
#else
static inline bool mm_get_smmu_info(struct device *dev, char *buffer, int length) { return false; }
#endif

#endif
