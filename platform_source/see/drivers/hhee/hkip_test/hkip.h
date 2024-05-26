/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: HKIP general communication and test driver
 * Create : 2016/12/6
 */

#ifndef __HKIPT_H__
#define __HKIPT_H__

#include <linux/arm-smccc.h>
#include <linux/genalloc.h>
#include <platform_include/see/hkip_hhee.h>
#include <linux/version.h>
/*
 * Defines for HKIP HVC function ids.
 */
#define HKIP_HVC_VERSION	       HHEE_HVC_VERSION
#define HKIP_HVC_NOTIFY		       HHEE_HVC_NOTIFY
#define HVC_REGISTER_RO_MEMORY	       HKIP_HVC_RO_REGISTER
#define HVC_REGISTER_DRRO_MEMORY       HKIP_HVC_RO_MOD_REGISTER
#define HVC_REVOKE_DRRO_MEMORY	       HKIP_HVC_RO_MOD_UNREGISTER
#define HVC_REGISTER_BIT_TABLE	       HKIP_HVC_ROWM_REGISTER
#define HVC_SET_BIT		       HKIP_HVC_ROWM_SET_BIT
#define HVC_LKM			       HHEE_LKM_UPDATE
#define HVC_LIVEPATCH		       HHEE_HVC_LIVEPATCH
#define HVC_GET_TOKEN		       HHEE_HVC_TOKEN
#define HVC_ENABLE_TVM		       HHEE_HVC_ENABLE_TVM
#define HVC_DISABLE_LIVEPATCH	       HHEE_HVC_DISABLE_LIVEPATCH

/*
 * Defines for el2 register read
 */
#define HKIP_HCR_EL2		       HHEE_HCR_EL2
#define HKIP_VTTBR_EL2		       HHEE_VTTBR_EL2

/*
 * Defines for security level settings
 */
#define HKIP_PERMISSIVE		       HHEE_PERMISIVE

/*
 * Defines for logging functionality
 */
#define HKIP_INIT_LOGBUF	       HHEE_INIT_LOGBUF
#define HKIP_LOGBUF_INFO	       HHEE_LOGBUF_INFO
#define HKIP_CRASHLOG_INFO	       HHEE_CRASHLOG_INFO
#define HKIP_MONITORLOG_INFO	       HHEE_MONITORLOG_INFO
#define HKIP_MONITORLOG_RESET_COUNTERS HHEE_MONITORLOG_RESET_COUNTERS
#define HKIP_RETURN_FUNCTION_ADDRESS   HHEE_RETURN_FUNCTION_ADDRESS
#define HKIP_RETURN_VAR_ADDRESS	       HHEE_RETURN_VAR_ADDRESS
#define HKIP_RETURN_STAGE2_INFO	       HHEE_RETURN_STAGE2_INFO

/*
 * Undefined HVC
 */
#define HKIP_UNKNOWN_HVC	       HHEE_UNKNOWN_HVC

/*
 *  vmalloc_user addr MASK
 */
#define HKIP_VADDR_ADDMASK	       (0x000000ffffffffff)
#define HKIP_VADDR_RESMASK	       (0xffffff0000000000)

#define HKIPT_IOC_MAGIC		       'h'

#define HKIPT_IOCALLOCVMEM	       _IOWR(HKIPT_IOC_MAGIC, 1, struct hkipt_vmem)
#define HKIPT_IOCFREEVMEM	       _IOW(HKIPT_IOC_MAGIC, 2, struct hkipt_vmem)

struct hkipt_vmem {
	size_t size;
	uintptr_t va;
};

#define HKIPT_IOCGETPGTABLEDESCS                                               \
	_IOWR(HKIPT_IOC_MAGIC, 3, struct hkipt_pgtable_descs)
#define HKIPT_IOCSETPGTABLEDESCS                                               \
	_IOW(HKIPT_IOC_MAGIC, 4, struct hkipt_pgtable_descs)

struct hkipt_pgtable_descs {
	uintptr_t va;
	uintptr_t desc_addr[4];
	uintptr_t desc_val[4];
};

#define HKIPT_IOCGETEVENTLOGINFO                                               \
	_IOR(HKIPT_IOC_MAGIC, 5, struct hkipt_eventlog_info)

struct hkipt_eventlog_info {
	uintptr_t base;
	size_t size;
};

#define HKIPT_IOCMAPPHYMEM   _IOWR(HKIPT_IOC_MAGIC, 6, hkipt_mem_area_t)
#define HKIPT_IOCUNMAPPHYMEM _IOW(HKIPT_IOC_MAGIC, 7, hkipt_mem_area_t)

typedef struct hkipt_eventlog_info hkipt_mem_area_t;

#define HKIPT_IOCPATCH _IOW(HKIPT_IOC_MAGIC, 8, struct hkipt_patch)

struct hkipt_patch {
	uintptr_t va;
	size_t size;
	uint64_t low;
	uint64_t high;
	uint64_t token;
};

#define HKIPT_IOCHVC _IOWR(HKIPT_IOC_MAGIC, 9, struct hkipt_hvc)

struct hkipt_hvc {
	uint64_t call_id, x0, x1, x2, x3, x4;
};

#define HKIPT_IOCREAD  _IOWR(HKIPT_IOC_MAGIC, 10, struct hkipt_read_write)
#define HKIPT_IOCWRITE _IOW(HKIPT_IOC_MAGIC, 11, struct hkipt_read_write)

struct hkipt_read_write {
	uintptr_t va;
	uint8_t size;
	union {
		uint64_t x64;
		uint32_t x32;
		uint16_t x16;
		uint8_t x8;
	} data;
};

#define HKIPT_IOCSETPTE	  _IOW(HKIPT_IOC_MAGIC, 12, struct hkipt_pgtable_descs)

#define HKIPT_IOCGETTOKEN _IOWR(HKIPT_IOC_MAGIC, 14, struct hkipt_token)
struct hkipt_token {
	uint64_t token;
	uint8_t refetch;
	uint64_t error;
};

#define HKIPT_IOCPMALLOC_CREATE	 _IOWR(HKIPT_IOC_MAGIC, 15, struct hkipt_pmalloc)
#define HKIPT_IOCPMALLOC_ALLOC	 _IOWR(HKIPT_IOC_MAGIC, 16, struct hkipt_pmalloc)
#define HKIPT_IOCPMALLOC_PROTECT _IOW(HKIPT_IOC_MAGIC, 17, struct hkipt_pmalloc)
#define HKIPT_IOCPMALLOC_DESTROY _IOW(HKIPT_IOC_MAGIC, 18, struct hkipt_pmalloc)
#define HKIPT_IOCPMALLOC_PROTECTED                                             \
	_IOWR(HKIPT_IOC_MAGIC, 19, struct hkipt_pmalloc)
#define HKIPT_IOCPMALLOC_FREE	_IOW(HKIPT_IOC_MAGIC, 20, struct hkipt_pmalloc)
#define HKIPT_IOCPMALLOC_ZALLOC _IOWR(HKIPT_IOC_MAGIC, 21, struct hkipt_pmalloc)
#define HKIPT_IOCPMALLOC_STRDUP _IOWR(HKIPT_IOC_MAGIC, 22, struct hkipt_pmalloc)
#define HKIPT_IOCPMALLOC_ARRAY	_IOWR(HKIPT_IOC_MAGIC, 23, struct hkipt_pmalloc)
#define HKIPT_IOCPMALLOC_CALLOC _IOWR(HKIPT_IOC_MAGIC, 24, struct hkipt_pmalloc)
#define HKIPT_IOCPMALLOC_PREALLOC                                              \
	_IOWR(HKIPT_IOC_MAGIC, 25, struct hkipt_pmalloc)
struct hkipt_pmalloc {
	char name[256];
	struct gen_pool *pool;
	void *addr;
	int order;
	int atomic;
	size_t size;
	size_t n;
};

#define HKIPT_IOCLSM_GETHOOKS	_IOR(HKIPT_IOC_MAGIC, 26, hkipt_mem_area_t)
#define HKIPT_IOCLSM_REGHOOKS	_IO(HKIPT_IOC_MAGIC, 27)
#define HKIPT_IOCLSM_DEREGHOOKS _IO(HKIPT_IOC_MAGIC, 28)

#define HKIPT_IOCSEL_GETDB	_IOR(HKIPT_IOC_MAGIC, 29, hkipt_mem_area_t)

struct hkipt_stage2_info {
	uint64_t va;
	uint64_t flag;
	uint64_t pxn;
	uint64_t rd;
	uint64_t rw;
};
#define HKIPT_IOCGET_STAGE2 _IOWR(HKIPT_IOC_MAGIC, 30, struct hkipt_stage2_info)

#define HKIPT_IOCJUMP	    _IOWR(HKIPT_IOC_MAGIC, 31, struct hkipt_jump)
struct hkipt_jump {
	uint64_t addr;
	unsigned int in;
	unsigned int out;
};

/* Check TTHM extension is in use to manage access flag and dirty state */
#define HKIPT_IOCCHECK_HA	   _IOWR(HKIPT_IOC_MAGIC, 32, struct hkipt_pgtable_descs)
#define HKIPT_IOCCHECK_HD	   _IOWR(HKIPT_IOC_MAGIC, 33, struct hkipt_pgtable_descs)

#define HKIPT_IOCPGT_TCY	   _IOW(HKIPT_IOC_MAGIC, 34, struct hkipt_pgtable_descs)
#define HKIPT_IOCPGT_SETWP	   _IOW(HKIPT_IOC_MAGIC, 35, struct hkipt_pgtable_descs)

/* Enble HCR_EL2 TVM bit */
#define HKIPT_IOCENABLE_TVM	   _IO(HKIPT_IOC_MAGIC, 36)
/* Disable livepatch */
#define HKIPT_IOCDISABLE_LIVEPATCH _IO(HKIPT_IOC_MAGIC, 37)

#define HKIPT_IOCSMC		   _IOWR(HKIPT_IOC_MAGIC, 38, struct hkipt_smc)

struct hkipt_smc {
	uint64_t call_id, x0, x1, x2, x3, x4;
};

/* following three structs are copied from hhee */
struct hhee_event {
	uint16_t type; /* Event type */
	uint16_t seq_no; /* Event sequence number */
	uint32_t flags; /* Reserved for flags, write zero, read ignore */

	uint64_t link; /* Exception return address (ELR_EL2) */
	uint64_t virt_addr; /* Virtual address (FAR_EL2) */
	uint64_t rsvd1; /* Reserved for IPA, undefined */
	uint32_t syndrome; /* Exception syndrome (ESR_EL2) */
	uint32_t ctx_id; /* Context ID (CONTEXTIDR_EL1) */
	uint64_t tid_system; /* Kernel mode thread ID (TPIDR_EL1) */
	uint64_t tid_shared; /* Thread ID (TPIDRRO_EL0) */
	uint64_t tid_user; /* User space thread ID (TPIDR_EL0) */

	uint32_t __attribute__((aligned(128))) task_id; /* Process ID or -1 if unknown */

	char __attribute__((aligned(256))) description[256]; /* Human-readable description */
};

/*
 * HHEE_EV_UNSET:Unused event buffer
 * HHEE_EV_BOOT: Hypervisor boot message
 * HHEE_EV_MMU_DISABLE:Attempt to disable stage 1 address translation
 * HHEE_EV_MMU_TABLE_OVERWRITE:Attempt to overwrite stage 1 top
 * level address translation page
 * HHEE_EV_WXN_DISABLE:Attempt to disable WXN
 * HHEE_EV_OS_TEXT_OVERWRITE:Attempt to overwrite kernel code
 * HHEE_EV_OS_DATA_OVERWRITE:Attempt to overwrite read-only kernel data
 * HHEE_EV_OS_MEDIATE_OVERWRITE:Attempt to overwrite write-mediated
 * data kernel data
 */
enum hhee_event_type {
	HHEE_EV_UNSET,
	HHEE_EV_BOOT,
	HHEE_EV_MMU_DISABLE = 0x100,
	HHEE_EV_MMU_TABLE_OVERWRITE,
	HHEE_EV_WXN_DISABLE,
	HHEE_EV_OS_TEXT_OVERWRITE = 0x201,
	HHEE_EV_OS_DATA_OVERWRITE = 0x202,
	HHEE_EV_OS_MEDIATE_OVERWRITE = 0x203,
};

struct hhee_event_header {
	uint64_t __attribute__((aligned(PAGE_SIZE)))
	magic; /* HHEE_EVENT_HEAD magic constant */
	uint64_t write_offset; /* Total number of messages ever written */
	uint64_t buffer_size; /* Size in bytes of entire buffer area
				(including header and footer) */
	uint64_t buffer_capacity; /* Capacity in messages
					of the circual buffer */
	uint64_t buffer_offset; /* Offset in bytes from &magic to &events[0] */
	uint64_t footer_offset; /* Offset in bytse from &magic to footer */
	struct hhee_event __attribute__((aligned(sizeof(struct hhee_event))))
	events[];
};
#define HHEE_EVENT_MAGIC 0x6851895ba852fb79UL

#define PGTABLE_LEVELS	 3

#if PGTABLE_LEVELS == 4
#define PGD_DESC 0
#define PUD_DESC 1
#define PMD_DESC 2
#define PTE_DESC 3
#elif PGTABLE_LEVELS == 3
#define PGD_DESC 0
#define PUD_DESC 0
#define PMD_DESC 1
#define PTE_DESC 2
#elif PGTABLE_LEVELS == 2
#define PGD_DESC 0
#define PUD_DESC 0
#define PMD_DESC 0
#define PTE_DESC 1
#else
#error
#endif

#ifdef __KERNEL__

#if PGTABLE_LEVELS != CONFIG_PGTABLE_LEVELS

#error Assumption in test code that there are 3 level page tables is no longer valid
#error revisit test code and then update PGTABLE_LEVELS to correct value

#endif /* PGTABLE_LEVELS != CONFIG_PGTABLE_LEVELS */

#define req_desc  req->desc_val
#define req_pte	  req_desc[PTE_DESC]
#define req_pmd	  req_desc[PMD_DESC]
#define req_pud	  req_desc[PUD_DESC]
#define req_pgd	  req_desc[PGD_DESC]

#define req_descp req->desc_addr
#define req_ptep  ((pte_t *)req_descp[PTE_DESC])
#define req_pmdp  ((pmd_t *)req_descp[PMD_DESC])
#define req_pudp  ((pud_t *)req_descp[PUD_DESC])
#define req_pgdp  ((pgd_t *)req_descp[PGD_DESC])

#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#include <asm/compiler.h>
#endif

static inline uint64_t do_hvc(uint64_t function_id, uint64_t *_arg0, uint64_t *_arg1,
			 uint64_t *_arg2, uint64_t *_arg3, uint64_t *_arg4)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct arm_smccc_res res;

	arm_smccc_hvc(function_id, *_arg0, *_arg1, *_arg2, *_arg3, *_arg4, 0, 0, &res);
	*_arg0 = res.a0;
	*_arg1 = res.a1;
	*_arg2 = res.a2;
	*_arg3 = res.a3;

	return (int)res.a0;
#else
	register uint64_t arg0 asm("x0") = function_id;
	register uint64_t arg1 asm("x1") = *_arg0;
	register uint64_t arg2 asm("x2") = *_arg1;
	register uint64_t arg3 asm("x3") = *_arg2;
	register uint64_t arg4 asm("x4") = *_arg3;
	register uint64_t arg5 asm("x5") = *_arg4;
	asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x1")
			__asmeq("%2", "x2")
			__asmeq("%3", "x3")
			__asmeq("%4", "x4")
			__asmeq("%5", "x5")
			"hvc	#0\n"
		: "+r" (arg0), "+r" (arg1), "+r" (arg2),
		"+r" (arg3), "+r" (arg4), "+r" (arg5));

	*_arg0 = arg0;
	*_arg1 = arg1;
	*_arg2 = arg2;
	*_arg3 = arg3;
	*_arg4 = arg4;

	/*
	 * printk(KERN_ALERT "HVC RET %llx %llx %llx %llx %llx\n",
         * _arg0, *_arg1, *_arg2, *_arg3, *_arg4);
	 */
	return arg0;
#endif
}

static inline uint64_t do_smc(uint64_t function_id, uint64_t *_arg0, uint64_t *_arg1,
			 uint64_t *_arg2, uint64_t *_arg3, uint64_t *_arg4)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, *_arg0, *_arg1, *_arg2, *_arg3, *_arg4, 0, 0, &res);
	*_arg0 = res.a0;
	*_arg1 = res.a1;
	*_arg2 = res.a2;
	*_arg3 = res.a3;

	return (int)res.a0;
#else
	register uint64_t arg0 asm("x0") = function_id;
	register uint64_t arg1 asm("x1") = *_arg0;
	register uint64_t arg2 asm("x2") = *_arg1;
	register uint64_t arg3 asm("x3") = *_arg2;
	register uint64_t arg4 asm("x4") = *_arg3;
	register uint64_t arg5 asm("x5") = *_arg4;
	asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x1")
			__asmeq("%2", "x2")
			__asmeq("%3", "x3")
			__asmeq("%4", "x4")
			__asmeq("%5", "x5")
			"smc	#0\n"
		: "+r" (arg0), "+r" (arg1), "+r" (arg2),
		"+r" (arg3), "+r" (arg4), "+r" (arg5));

	*_arg0 = arg0;
	*_arg1 = arg1;
	*_arg2 = arg2;
	*_arg3 = arg3;
	*_arg4 = arg4;

	return arg0;
#endif
}

static inline uint64_t hkip_hvc(uint64_t function_id, uint64_t arg0, uint64_t arg1,
			   uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	return do_hvc(function_id, &arg0, &arg1, &arg2, &arg3, &arg4);
}

static inline void hkip_hvc_2(uint64_t function_id, uint64_t *arg0,
			      uint64_t *arg1, uint64_t arg2, uint64_t arg3,
			      uint64_t arg4)
{
	do_hvc(function_id, arg0, arg1, &arg2, &arg3, &arg4);
}

int hkip_get_monitorlog_info(uint64_t *base, uint64_t *size);
long hkip_unlocked_ioctl(struct file *file, unsigned int cmd,
			 unsigned long arg);
bool el1_addr_invalid(uint64_t addr, bool is_write);
int map_physical_area(struct vm_struct **varea, phys_addr_t pa, size_t size,
		      pgprot_t prot);
long el1_text_sandbox(void *in);
void *sandbox_try_execute(long (*func)(void *), void *data);

#if 1
#include <linux/delay.h>
#define DBG(fmt, ...)                                                          \
	do {                                                                   \
		printk(KERN_ERR "HKIPT " fmt, ##__VA_ARGS__);                  \
	} while (0)
#else
#define DBG(fmt, ...)                                                          \
	do {                                                                   \
		;                                                              \
	} while (0)
#endif

#endif /* __KERNEL__ */
#endif /* HKIPT_H */
