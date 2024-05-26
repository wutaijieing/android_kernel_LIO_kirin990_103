/*
 * hmntn_hw_sp.c
 *
 * mntn for providing the information of pc and sp when abnormal
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

#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/thread_info.h>
#include <linux/kdebug.h>
#include <linux/stacktrace.h>
#include <linux/ftrace.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/of.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <asm/stacktrace.h>
#include <asm/smp_plat.h>
#include <securec.h>
#include <soc_fcm_local_ananke_core_ns_interface.h>
#include <soc_fcm_local_enyo_core_ns_interface.h>
#include <soc_crgperiph_interface.h>
#include <soc_acpu_baseaddr_interface.h>

/*
 * Sp_reg is indexed by MPIDR. Current platform supports 2 clusters,
 * each of which is composed of 4 cpus. So we should reserve 8 CPUS.
 */
#define PR_LOG_TAG MNTN_RECORD_SP_TAG

#define MAX_CPU 8
#define MAX_FUNC_CALL 0x50
#define PART_NUM_CPU_MASK 0xFFF0
#define AFF1_CPU_MASK 0xF00
#define CPU_LITTLE (SOC_CRGPERIPH_A53_COREPOWERSTAT_ADDR(SOC_ACPU_PERI_CRG_BASE_ADDR))
#define CPU_BIG (SOC_CRGPERIPH_A53_COREPOWERSTAT_ADDR(SOC_ACPU_PERI_CRG_BASE_ADDR) + 0xA0)
#define CPU_POWERSTART_MASK 0XF
#define MAX_CLUSTER_NR 2
#define MAX_CORE_NR_PER_CLUSTER 4
#define MAX_CORE_NR (MAX_CLUSTER_NR * MAX_CORE_NR_PER_CLUSTER)
#define BITS_PER_CORE 4
#define ACPU_A76 0xD40
#define ACPU_A55 0xD05
#define BIG_CORE 0x1
#define LITTLE_CORE 0x0
#define SIZE_4K 0x1000
#define INIT_END 0xaa55
#define INIT_BEGIN 0x55aa
#define MNTN_SP_FAIL (-1)
#define MNTN_SP_SUCCESS 0
#define MNTN_SP_PARTNUM_MASK 0xFFF0
#define MNTN_SP_PARTNUM_RIGHT 0x4
#define MNTN_SP_INIT_VALUE 0x00
#define CPU_DEBUG_ENABLE 0x1
#define CPU_MPIDR_MASK 0xFF00
#define CPU_MPIDR_BIT_REG 8
#define CURRENTEL_EL1 (1 << 2)
#define PDC_PWR_ON_STAT 0x6
#define CPU_ON_LINE_ONLYONE 0x1
#define KERNEL_STATCK_RANGE 0xFFFFFF0000000000
#define TEEOS_STATCK_RANGE 0xFFFFFF8000000000
#define TEEOS_STATCK_SIEZE 0x3000000
#define SP_LS_BIT_REG 32
#define FRAME_SP_REG_BITS 0x10
#define FRAME_PC_REG_BITS 8

enum {
	ACORE_TYPE_SP_LS0,
	ACORE_TYPE_SP_LS1,
	ACORE_TYPE_ENUM,
};

struct mntn_cpu_info {
	u64 fcm_base_addr[MAX_CORE_NR];
	u32 fcm_cpu_type[MAX_CORE_NR];
	u32 fcm_local_cpu_ns_addr;
	u32 bigcore_partnum;  /* for sw,we only care big.LITTLE */
	u32 littlecore_partnum;
	u32 reg_offset;
	u32 init_flag;
	u64 fcm_littlecore_power_addr;
	u64 fcm_bigcore_power_addr;
	u64 reserved;
};

static struct mntn_cpu_info g_cpu_mntn_info;

static u32 dfx_mntn_get_cputype(void)
{
#if defined(__arm__) || defined(__aarch64__)
	u32 val;
#if !defined(__aarch64__)
	asm volatile("mrc p15, 0, %0, c0, c0, 0" : "=r" (val));
#else
	asm volatile("mrs %0, midr_el1" : "=r" (val));
#endif
	return ((val & MNTN_SP_PARTNUM_MASK) >> MNTN_SP_PARTNUM_RIGHT);
#else
	return MNTN_SP_FAIL;
#endif
}

static s32 dfx_mntn_sp_check(u64 sp)
{
	if (((sp & KERNEL_STATCK_RANGE) != KERNEL_STATCK_RANGE) ||
		((sp >= TEEOS_STATCK_RANGE) && (sp <= (TEEOS_STATCK_RANGE + TEEOS_STATCK_SIEZE)))) {
		pr_err("%s: sp is not in EL1_NSE\n", __func__);
		return MNTN_SP_FAIL;
	}

	return MNTN_SP_SUCCESS;
}

static void dfx_mntn_init_status(u32 init)
{
	g_cpu_mntn_info.init_flag = init;
}

static u32 dfx_mntn_get_initstatus(void)
{
	return g_cpu_mntn_info.init_flag;
}

static void dfx_mntn_init_vari(void)
{
	(void)memset_s(&g_cpu_mntn_info, sizeof(g_cpu_mntn_info),
		MNTN_SP_INIT_VALUE, sizeof(g_cpu_mntn_info));
}

static void dfx_mntn_read_cpuid(void *arg)
{
	u32 mpidr, core;

	const u32 cpuid = dfx_mntn_get_cputype();
	const int cpu = raw_smp_processor_id();

	/* logic to physical */
	mpidr = (unsigned int)__cpu_logical_map[cpu];
	pr_err("%s: mpidr is %x\n", __func__, mpidr);

	/* physical, get core num from MPIDR reg OF cpu */
	core = (mpidr & CPU_MPIDR_MASK) >> CPU_MPIDR_BIT_REG;

	/* physical pn */
	g_cpu_mntn_info.fcm_cpu_type[core] = cpuid;

	pr_err("%s: logic cpuid is %x, physical id is %u,id is %x\n",
		__func__, cpu, core, g_cpu_mntn_info.fcm_cpu_type[cpu]);
}

static void dfx_mntn_set_cpu_parnum_off(u32 bigcore, u32 littlecore)
{
	g_cpu_mntn_info.bigcore_partnum = bigcore;
	g_cpu_mntn_info.littlecore_partnum = littlecore;
	pr_err("%s: bigcore is %x, littlecore is %x\n", __func__, bigcore, littlecore);
}

static s32 dfx_mntn_set_reg_addr(u32 base_addr, u32 reg_offset)
{
	u32 i;

	for (i = 0; i < NR_CPUS; i++) {
		g_cpu_mntn_info.fcm_base_addr[i] = (uintptr_t)ioremap_nocache(
			(phys_addr_t)(base_addr + reg_offset * i), SIZE_4K);
		if (g_cpu_mntn_info.fcm_base_addr[i] == 0) {
			pr_err("%s: ioremap fail\n", __func__);
			return MNTN_SP_FAIL;
		}
	}

	g_cpu_mntn_info.fcm_littlecore_power_addr = (uintptr_t)ioremap_nocache(
		SOC_CRGPERIPH_A53_COREPOWERSTAT_ADDR(SOC_ACPU_PERI_CRG_BASE_ADDR), SIZE_4K);
	if (g_cpu_mntn_info.fcm_littlecore_power_addr == 0) {
		pr_err("%s: fcm_littlecore_power_addr ioremap fail\n", __func__);
		return MNTN_SP_FAIL;
	}

	g_cpu_mntn_info.fcm_bigcore_power_addr = (uintptr_t)ioremap_nocache(
		SOC_CRGPERIPH_A53_COREPOWERSTAT_ADDR(SOC_ACPU_PERI_CRG_BASE_ADDR) + 0xA0, SIZE_4K);
	if (g_cpu_mntn_info.fcm_bigcore_power_addr == 0) {
		pr_err("%s: fcm_bigcore_power_addr ioremap fail\n", __func__);
		return MNTN_SP_FAIL;
	}

	return MNTN_SP_SUCCESS;
}

static s32 dfx_mntn_cputype(u32 coreid)
{
	pr_err("%s: coreid is %x\n", __func__, coreid);
	if (coreid >= MAX_CPU) {
		pr_err("%s: coreid is over\n", __func__);
		return MNTN_SP_FAIL;
	}

	if (g_cpu_mntn_info.bigcore_partnum == g_cpu_mntn_info.fcm_cpu_type[coreid])
		return BIG_CORE;

	if (g_cpu_mntn_info.littlecore_partnum == g_cpu_mntn_info.fcm_cpu_type[coreid])
		return LITTLE_CORE;

	return MNTN_SP_FAIL;
}

static u64 dfx_cpu_mntn_get_sp_addr(u32 coreid)
{
	return g_cpu_mntn_info.fcm_base_addr[coreid];
}

static void dfx_mntn_acore_debug_enable(u64 core_debug_base)
{
	SOC_FCM_LOCAL_ENYO_CORE_NS_ENYO_DBG_CLR_CFG_REG_UNION clr_debug_en;

	clr_debug_en.reg.latch_debug_en = CPU_DEBUG_ENABLE;
	writel_relaxed(*((u32 *)&clr_debug_en),
		(u64 __iomem *)(uintptr_t)SOC_FCM_LOCAL_ENYO_CORE_NS_ENYO_DBG_CLR_CFG_REG_ADDR(core_debug_base));
}

static void dfx_mntn_acore_sp_ls0_read(u64 *sp_ls0_value, u64 core_debug_base)
{
	u64 sp_ls0_value_hi, sp_ls0_value_lo;

	sp_ls0_value_lo = ((SOC_FCM_LOCAL_ENYO_CORE_NS_ENYO_DBG_LS0_SP_LO_UNION);
	readl_relaxed((u64 __iomem *)(uintptr_t)SOC_FCM_LOCAL_ENYO_CORE_NS_ENYO_DBG_LS0_SP_LO_ADDR(core_debug_base)))
	.reg.core_ls0_sp_low;
	sp_ls0_value_hi = ((SOC_FCM_LOCAL_ENYO_CORE_NS_ENYO_DBG_LS0_SP_HI_UNION);
	readl_relaxed((u64 __iomem *)(uintptr_t)SOC_FCM_LOCAL_ENYO_CORE_NS_ENYO_DBG_LS0_SP_HI_ADDR(core_debug_base)));
	.reg.core_ls0_sp_high;

	*(sp_ls0_value) = (sp_ls0_value_hi << SP_LS_BIT_REG) + sp_ls0_value_lo;
}

static void dfx_mntn_acore_sp_ls1_read(u64 *sp_ls1_value, u64 core_debug_base)
{
	u64 sp_ls1_value_hi, sp_ls1_value_lo;

	sp_ls1_value_lo = ((SOC_FCM_LOCAL_ENYO_CORE_NS_ENYO_DBG_LS1_SP_LO_UNION);
	readl_relaxed((u64 __iomem *)(uintptr_t)SOC_FCM_LOCAL_ENYO_CORE_NS_ENYO_DBG_LS1_SP_LO_ADDR(core_debug_base)));
	.reg.core_ls1_sp_low;
	sp_ls1_value_hi = ((SOC_FCM_LOCAL_ENYO_CORE_NS_ENYO_DBG_LS1_SP_HI_UNION);
	readl_relaxed((u64 __iomem *)(uintptr_t)SOC_FCM_LOCAL_ENYO_CORE_NS_ENYO_DBG_LS1_SP_HI_ADDR(core_debug_base)));
	.reg.core_ls1_sp_high;

	*(sp_ls1_value) = (sp_ls1_value_hi << SP_LS_BIT_REG) + sp_ls1_value_lo;
}

static s32 dfx_mntn_cpu_powerstats(u8 coreid)
{
	u32 acpu_powerstat;
	u8 cluster_idx;
	u8 core_idx;

	cluster_idx = coreid / MAX_CORE_NR_PER_CLUSTER;
	core_idx = coreid % MAX_CORE_NR_PER_CLUSTER;

	acpu_powerstat = readl_relaxed((u64 __iomem *)(uintptr_t)
		(cluster_idx ? g_cpu_mntn_info.fcm_bigcore_power_addr : g_cpu_mntn_info.fcm_littlecore_power_addr));

	if (((acpu_powerstat >> (core_idx * BITS_PER_CORE)) & CPU_POWERSTART_MASK) == PDC_PWR_ON_STAT) {
		pr_err(" cpu %u is poweron\n", coreid);
		return MNTN_SP_SUCCESS;
	}

	pr_err(" cpu %u is powerdown\n", coreid);
	return MNTN_SP_FAIL;
}

static s32 dfx_mntn_acore_sp(u64 *dst, u32 type, u8 coreid)
{
	u64 core_debug_base;
	s32 ret;

	if (dfx_mntn_cpu_powerstats(coreid) == MNTN_SP_FAIL)
		return MNTN_SP_FAIL;

	core_debug_base = dfx_cpu_mntn_get_sp_addr(coreid);
	dfx_mntn_acore_debug_enable(core_debug_base);

	switch (type) {
	case ACORE_TYPE_SP_LS0:
		dfx_mntn_acore_sp_ls0_read(dst, core_debug_base);
		break;
	case ACORE_TYPE_SP_LS1:
		dfx_mntn_acore_sp_ls1_read(dst, core_debug_base);
		break;
	default:
		return MNTN_SP_FAIL;
	}

	ret = dfx_mntn_sp_check(*dst);
	if (ret == MNTN_SP_FAIL)
		return MNTN_SP_FAIL;

	return MNTN_SP_SUCCESS;
}

static void __dfx_cpu_mntn_stack(u64 sp, u32 cpu)
{
	struct stackframe frame;
	u32 i;
	u64 top_st;

	top_st = ALIGN(sp, THREAD_SIZE);
	pr_err("mntn Call trace[CPU%u]: 0x%llx  ~~ 0x%llx\n", cpu, sp, top_st);
	sp = sp & (~(sizeof(char *)-1));

	frame.sp = sp + FRAME_SP_REG_BITS;
	frame.fp = frame.sp;
	frame.pc = READ_ONCE_NOCHECK(*(unsigned long *)(uintptr_t)(sp + FRAME_PC_REG_BITS));
	for (i = 0; i < MAX_FUNC_CALL; i++) {
		unsigned long where = frame.pc;
		int ret;
#if (KERNEL_VERSION(4, 4, 0) > LINUX_VERSION_CODE)
		ret = unwind_frame(&frame);
#else
		ret = unwind_frame(current, &frame);
#endif
		if (ret < 0) {
			pr_err("%s: this frame[%u] analysis finish!!!\n", __func__, i);
			break;
		}
		print_ip_sym(where);
	}

	pr_err("\n");

	if (i >= MAX_FUNC_CALL)
		pr_err("%s: call trace may be corrupted!!!\n", __func__);
}

static void dfx_cpu_mntn_stack(int cpu)
{
	u32 mpidr, core;
	u64 sp;
	s32 ret;
	s32 coretype;

	if (cpu >= MAX_CPU) {
		pr_err("%s: cpu logic num is over\n", __func__);
		return;
	}

	/* physical */
	mpidr = (unsigned int)__cpu_logical_map[cpu];
	pr_err("%s: mpidr is %x\n", __func__, mpidr);
	/*
	 * fcm way to get mpidr
	 */
	if (dfx_mntn_get_initstatus() != INIT_END) {
		pr_err("%s: hw isnot init,pls wait\n", __func__);
		return;
	}

	/* physical, get core num from MPIDR reg OF cpu */
	core = (mpidr & CPU_MPIDR_MASK) >> CPU_MPIDR_BIT_REG;
	if (core >= MAX_CPU) {
		pr_err("%s: CPU[%d] error mpidr =0x%x\n", __func__, cpu, mpidr);
		return;
	}

	coretype = dfx_mntn_cputype(core);
	if (coretype == MNTN_SP_FAIL) {
		pr_err("%s: dfx_mntn_acore_sp sp0 rturn null\n", __func__);
		return;
	}

	pr_err("%s: coretye is %x\n", __func__, (u32)coretype);

	ret = dfx_mntn_acore_sp(&sp, ACORE_TYPE_SP_LS0, core);
	if (ret == MNTN_SP_FAIL) {
		pr_err("%s: dfx_mntn_acore_sp sp0 rturn null\n", __func__);
		return;
	}

	__dfx_cpu_mntn_stack(sp, core);

	if (coretype == BIG_CORE) {
		ret = dfx_mntn_acore_sp(&sp, ACORE_TYPE_SP_LS1, core);
		if (ret == MNTN_SP_FAIL) {
			pr_err("%s: dfx_mntn_acore_sp sp1 rturn null\n", __func__);
			return;
		}

		pr_err("%s: big core stack begin: +++\n", __func__);
		__dfx_cpu_mntn_stack(sp, core);
		pr_err("%s: big core stack end: +++\n", __func__);
	}
}

/*
 * solve condi: For instance, a cpu is stall and not response for IPI stop,
 * we will print it's stack.
 */
void hisi_mntn_show_stack_cpustall(void)
{
	int cpu, i;

	if (dfx_mntn_get_initstatus() != INIT_END) {
		pr_err("%s: hw isnot init,pls wait\n", __func__);
		return;
	}

	if (num_online_cpus() == CPU_ON_LINE_ONLYONE)
		return;
	pr_info("[%s]:Some cpus are stall\n", __func__);
	cpu = raw_smp_processor_id();
	for (i = 0; i < NR_CPUS; i++) {
		if (!cpu_online(i))
			continue;
		if (cpu == i)
			continue;
		dfx_cpu_mntn_stack(i);
	}
}

/*
 * solve condi: For rcu stall, dump other cpus stack
 */
void hisi_mntn_show_stack_othercpus(int cpu)
{
	int curr_cpu;

	if (dfx_mntn_get_initstatus() != INIT_END) {
		pr_err("%s: hw isnot init,pls wait\n", __func__);
		return;
	}

	curr_cpu = raw_smp_processor_id();
	/* don't dump current cpu stack */
	if (curr_cpu == cpu)
		return;

	dfx_cpu_mntn_stack(cpu);
}

static s32 dfx_mntn_hw_init(void)
{
	s32 ret;
	u32 fcm_local_cpu_ns_addr = MNTN_SP_INIT_VALUE;
	u32 bigcore_partnum = MNTN_SP_INIT_VALUE;
	u32 littlecore_partnum = MNTN_SP_INIT_VALUE;
	u32 reg_offset = MNTN_SP_INIT_VALUE;

	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,sp_mntn");
	if (np == NULL) {
		pr_err("[%s], find node fail!\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "sp_mntn_fcm_baseaddr", &fcm_local_cpu_ns_addr);
	if (ret) {
		pr_err("[%s], cannot find sp_mntn_fcm_baseaddr in dts!\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "BIGCORE_PARTNUM", &bigcore_partnum);
	if (ret) {
		pr_err("[%s], cannot find BIGCORE_PARTNUM in dts!\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "LITTLECORE_PARTNUM", &littlecore_partnum);
	if (ret) {
		pr_err("[%s], cannot find LITTLECORE_PARTNUM in dts!\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "reg_offset", &reg_offset);
	if (ret) {
		pr_err("[%s], cannot find reg_offset in dts!\n", __func__);
		return -ENODEV;
	}

	dfx_mntn_init_vari();

	dfx_mntn_set_cpu_parnum_off(bigcore_partnum, littlecore_partnum);

	ret = dfx_mntn_set_reg_addr(fcm_local_cpu_ns_addr, reg_offset);
	if (ret == MNTN_SP_FAIL)
		return MNTN_SP_FAIL;

	ret = on_each_cpu(dfx_mntn_read_cpuid, NULL, 1);
	if (ret != MNTN_SP_SUCCESS) {
		pr_err("[%s], on_each_cpu fail!\n", __func__);
		return MNTN_SP_FAIL;
	}

	return MNTN_SP_SUCCESS;
}

static int __init dfx_cpu_mntn_init(void)
{
	s32 ret;

	pr_err("%s: hw stack init begin ++\n", __func__);

	dfx_mntn_init_status(INIT_BEGIN);

	ret = dfx_mntn_hw_init();
	if (ret != MNTN_SP_SUCCESS) {
		pr_err("%s: dfx_mntn_hw_init init fail\n", __func__);
		return MNTN_SP_FAIL;
	}

	dfx_mntn_init_status(INIT_END);

	pr_err("%s: hw stack init end ++\n", __func__);

	return MNTN_SP_SUCCESS;
}

core_initcall(dfx_cpu_mntn_init);
