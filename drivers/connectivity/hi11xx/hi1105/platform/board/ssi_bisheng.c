

#ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG
#include "ssi_bisheng.h"

#include "plat_debug.h"
#include "plat_pm.h"
#include "ssi_common.h"
#include "plat_parse_changid.h"

#define TCXO_32K_DET_VALUE    10
#define TCXO_LIMIT_THRESHOLD 5
#define TCXO_NOMAL_CKL  38400000
#define RTC_32K_NOMAL_CKL  32768

/* 以下寄存器是bisheng device定义 */
#define BISHENG_GLB_CTL_BASE                      0x40000000
#define BISHENG_GLB_CTL_SOFT_RST_BCPU_REG         (BISHENG_GLB_CTL_BASE + 0x70)
#define BISHENG_GLB_CTL_SOFT_RST_GCPU_REG         (BISHENG_GLB_CTL_BASE + 0x74)
#define BISHENG_GLB_CTL_SYS_TICK_CFG_W_REG        (BISHENG_GLB_CTL_BASE + 0x100) /* 写1清零systick，写0无效 */
#define BISHENG_GLB_CTL_SYS_TICK_VALUE_W_0_REG    (BISHENG_GLB_CTL_BASE + 0x104)
#define BISHENG_GLB_CTL_SYS_TICK_CFG_B_REG        (BISHENG_GLB_CTL_BASE + 0x114) /* 写1清零systick，写0无效 */
#define BISHENG_GLB_CTL_SYS_TICK_VALUE_B_0_REG    (BISHENG_GLB_CTL_BASE + 0x118)
#define BISHENG_GLB_CTL_SYS_TICK_CFG_GF_REG       (BISHENG_GLB_CTL_BASE + 0x180) /* 写1清零systick，写0无效 */
#define BISHENG_GLB_CTL_SYS_TICK_VALUE_GF_0_REG   (BISHENG_GLB_CTL_BASE + 0x184)
#define BISHENG_GLB_CTL_SYS_TICK_CFG_GLE_REG      (BISHENG_GLB_CTL_BASE + 0x194) /* 写1清零systick，写0无效 */
#define BISHENG_GLB_CTL_SYS_TICK_VALUE_GLE_0_REG  (BISHENG_GLB_CTL_BASE + 0x198)
#define BISHENG_GLB_CTL_PWR_ON_LABLE_REG          (BISHENG_GLB_CTL_BASE + 0x200) /* 芯片上电标记寄存器 */
#define BISHENG_GLB_CTL_BCPU_LOAD_REG             (BISHENG_GLB_CTL_BASE + 0x1080) /* BCPU_LOAD */
#define BISHENG_GLB_CTL_BCPU_PC_L_REG             (BISHENG_GLB_CTL_BASE + 0x1084) /* BCPU_PC低16bit */
#define BISHENG_GLB_CTL_BCPU_PC_H_REG             (BISHENG_GLB_CTL_BASE + 0x1088) /* BCPU_PC高16bit */
#define BISHENG_GLB_CTL_BCPU_LR_L_REG             (BISHENG_GLB_CTL_BASE + 0x108C) /* BCPU_LR低16bit */
#define BISHENG_GLB_CTL_BCPU_LR_H_REG             (BISHENG_GLB_CTL_BASE + 0x1090) /* BCPU_LR高16bit */
#define BISHENG_GLB_CTL_BCPU_SP_L_REG             (BISHENG_GLB_CTL_BASE + 0x1094) /* BCPU_SP低16bit */
#define BISHENG_GLB_CTL_BCPU_SP_H_REG             (BISHENG_GLB_CTL_BASE + 0x1098) /* BCPU_SP高16bit */
#define BISHENG_GLB_CTL_GCPU_LOAD_REG             (BISHENG_GLB_CTL_BASE + 0x109c) /* GCPU_LOAD */
#define BISHENG_GLB_CTL_GCPU_PC_L_REG             (BISHENG_GLB_CTL_BASE + 0x10A0) /* GCPU_PC低16bit */
#define BISHENG_GLB_CTL_GCPU_PC_H_REG             (BISHENG_GLB_CTL_BASE + 0x10A4) /* GCPU_PC高16bit */
#define BISHENG_GLB_CTL_GCPU_LR_L_REG             (BISHENG_GLB_CTL_BASE + 0x10A8) /* GCPU_LR低16bit */
#define BISHENG_GLB_CTL_GCPU_LR_H_REG             (BISHENG_GLB_CTL_BASE + 0x10AC) /* GCPU_LR高16bit */
#define BISHENG_GLB_CTL_GCPU_SP_L_REG             (BISHENG_GLB_CTL_BASE + 0x10B0) /* GCPU_SP低16bit */
#define BISHENG_GLB_CTL_GCPU_SP_H_REG             (BISHENG_GLB_CTL_BASE + 0x10B4) /* GCPU_SP高16bit */
#define BISHENG_GLB_CTL_GLE_LOAD_REG              (BISHENG_GLB_CTL_BASE + 0x10B8) /* GLE_LOAD */
#define BISHENG_GLB_CTL_GLE_PC_L_REG              (BISHENG_GLB_CTL_BASE + 0x10BC) /* GLE_PC低16bit */
#define BISHENG_GLB_CTL_GLE_PC_H_REG              (BISHENG_GLB_CTL_BASE + 0x10C0) /* GLE_PC高16bit */
#define BISHENG_GLB_CTL_GLE_LR_L_REG              (BISHENG_GLB_CTL_BASE + 0x10C4) /* GLE_LR低16bit */
#define BISHENG_GLB_CTL_GLE_LR_H_REG              (BISHENG_GLB_CTL_BASE + 0x10C8) /* GLE_LR高16bit */
#define BISHENG_GLB_CTL_WCPU_LOAD_REG            (BISHENG_GLB_CTL_BASE + 0x10CC) /* WCPU_LOAD */
#define BISHENG_GLB_CTL_WCPU_PC_L_REG            (BISHENG_GLB_CTL_BASE + 0x10D0) /* WCPU_PC低16bit */
#define BISHENG_GLB_CTL_WCPU_PC_H_REG            (BISHENG_GLB_CTL_BASE + 0x10D4) /* WCPU_PC高16bit */
#define BISHENG_GLB_CTL_WCPU_LR_L_REG            (BISHENG_GLB_CTL_BASE + 0x10D8) /* WCPU_LR低16bit */
#define BISHENG_GLB_CTL_WCPU_LR_H_REG            (BISHENG_GLB_CTL_BASE + 0x10Dc) /* WCPU_LR高16bit */
#define BISHENG_GLB_CTL_WCPU_SP_L_REG            (BISHENG_GLB_CTL_BASE + 0x10E0) /* WCPU_SP低16bit */
#define BISHENG_GLB_CTL_WCPU_SP_H_REG            (BISHENG_GLB_CTL_BASE + 0x10E4) /* WCPU_SP高16bit */
#define BISHENG_GLB_CTL_TCXO_DET_CTL_REG          (BISHENG_GLB_CTL_BASE + 0x550) /* TCXO时钟检测控制寄存器 */
#define BISHENG_GLB_CTL_TCXO_32K_DET_CNT_REG      (BISHENG_GLB_CTL_BASE + 0x554) /* TCXO时钟检测控制寄存器 */
#define BISHENG_GLB_CTL_TCXO_32K_DET_RESULT_REG   (BISHENG_GLB_CTL_BASE + 0x558) /* TCXO时钟检测控制寄存器 */
#define BISHENG_GLB_CTL_WCPU_WAIT_CTL_REG         (BISHENG_GLB_CTL_BASE + 0x85C)
#define BISHENG_GLB_CTL_BCPU_WAIT_CTL_REG         (BISHENG_GLB_CTL_BASE + 0x860)
#define BISHENG_GLB_CTL_GFCPU_WAIT_CTL_REG        (BISHENG_GLB_CTL_BASE + 0x864)

#define BISHENG_PMU_CMU_CTL_BASE                    0x40002000
#define BISHENG_PMU_CMU_CTL_SYS_STATUS_0_REG        (BISHENG_PMU_CMU_CTL_BASE + 0x1C0) /* 系统状态 */
#define BISHENG_PMU_CMU_CTL_CLOCK_TCXO_PRESENCE_DET (BISHENG_PMU_CMU_CTL_BASE + 0x040) /* TCXO时钟在位检测 */

static bisheng_ssi_cpu_infos g_ssi_cpu_infos;

/* dump 寄存器定义 */
static ssi_reg_info g_bisheng_ssi_master_reg_full = { 0x40, 0x30, SSI_RW_SSI_MOD };
static ssi_reg_info g_bisheng_glb_ctrl_full = { 0x40000000, 0x08e0, SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_glb_ctrl_extend1 = { 0x40001c00, 0x50,   SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_glb_ctrl_extend2 = { 0x40001f64, 0x50,   SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_pmu_cmu_ctrl_full = { 0x40002000, 0xf98,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_pmu2_cmu_ctrl_part1 = { 0x40012000, 0x684,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_pmu2_cmu_ctrl_part2 = { 0x40012ba0, 0x388,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_coex_ctl_part1 = { 0x40011000, 0x300,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_coex_ctl_part2 = { 0x40011600, 0x3c,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_coex_ctl_part3 = { 0x40011700, 0x34,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_coex_ctl_part4 = { 0x40011800, 0x28,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_coex_ctl_part5 = { 0x40011900, 0x2c,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_rf_tcxo_pll_ctrl_full = { 0x40014000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_xoadc_ctrl_full = { 0x40017000, 0x20,  SSI_RW_DWORD_MOD };
static ssi_reg_info g_bisheng_gf_ctrl_full = { 0x43000000, 0x488,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_gf_diag_ctrl_full = { 0x43005000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_gf_rf_abb_ctrl_full = { 0x43016000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_gf_patch_ctrl_full = { 0x43020010, 0x8,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_gcpu_trace_ctrl_full = { 0x43024010, 0x8,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_b_ctrl_full = { 0x41000000, 0x128,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_bf_diag_ctrl_full = { 0x41006000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_bf_rf_abb_ctrl_full = { 0x4101a000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_b_patch_ctrl_full = { 0x41020010, 0x8,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_fm_ctrl_full = { 0x43040000, 0x8,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_bcpu_trace_ctrl_full = { 0x41024010, 0x8,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_w_ctrl_part1 = { 0x40105000, 0x46c,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_w_ctrl_part2 = { 0x40106000, 0x97c,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_w_diag_ctrl_full = { 0x40120108, 0x10,  SSI_RW_DWORD_MOD };
static ssi_reg_info g_bisheng_w_patch_ctrl_full = { 0x400e0010, 0x8,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_wl_rf_abb_ch_full = { 0x40110000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_wl_rf_abb_com_ctrl_full = { 0x40119000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_wcpu_trace_ctrl_full = { 0x400e4010, 0x8,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_gle_ctrl_full = { 0x42000000, 0x494,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_gle_diag_ctrl_full = { 0x42005000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_gle_rf_abb_ctrl_full = { 0x42012000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_gle_trace_ctrl_full = { 0x42020010, 0x8,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_pcie_ctrl_full = { 0x40108000, 0x664,  SSI_RW_DWORD_MOD };
static ssi_reg_info g_bisheng_host_ctrl_full = { 0x4010d000, 0xf78,  SSI_RW_DWORD_MOD };
static ssi_reg_info g_bisheng_pcie_dbi_full = { 0x40107000, 0x1000,  SSI_RW_DWORD_MOD };     /* 没建链之前不能读 */
static ssi_reg_info g_bisheng_wifi_gpio_full = {0x40005000, 0x74, SSI_RW_WORD_MOD};
static ssi_reg_info g_bisheng_bfgx_gpio_full = {0x40006000, 0x74, SSI_RW_WORD_MOD};
static ssi_reg_info g_bisheng_gf_gpio_full = {0x40007000, 0x74, SSI_RW_WORD_MOD};
static ssi_reg_info g_bisheng_gle_gpio_full = {0x40008000, 0x74, SSI_RW_WORD_MOD};


static ssi_reg_info g_bisheng_w_key_mem = { 0x400e6000, 0x2000, SSI_RW_DWORD_MOD};
static ssi_reg_info g_bisheng_b_key_mem = { 0x41026000, 0x2000, SSI_RW_DWORD_MOD};
static ssi_reg_info g_bisheng_gf_key_mem = { 0x43026000, 0x2000, SSI_RW_DWORD_MOD};
static ssi_reg_info g_bisheng_gle_key_mem = { 0x42022000, 0x2000, SSI_RW_DWORD_MOD};

static ssi_reg_info g_bisheng_tcxo_detect_reg1 = { 0x40000024, 0x4,  SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_tcxo_detect_reg2 = { 0x40000100, 0x28, SSI_RW_WORD_MOD };
static ssi_reg_info g_bisheng_tcxo_detect_reg3 = { 0x40000550, 0xc,  SSI_RW_WORD_MOD };


/* bisheng 默认dump 所有寄存器 */
static ssi_reg_info *g_bisheng_aon_reg_full[] = {
    &g_bisheng_glb_ctrl_full,
    &g_bisheng_glb_ctrl_extend1,
    &g_bisheng_glb_ctrl_extend2,
    &g_bisheng_pmu_cmu_ctrl_full,
    &g_bisheng_pmu2_cmu_ctrl_part1,
    &g_bisheng_pmu2_cmu_ctrl_part2,
    &g_bisheng_coex_ctl_part1,
    &g_bisheng_coex_ctl_part2,
    &g_bisheng_coex_ctl_part3,
    &g_bisheng_coex_ctl_part4,
    &g_bisheng_coex_ctl_part5,
    &g_bisheng_wifi_gpio_full,
    &g_bisheng_bfgx_gpio_full,
    &g_bisheng_gf_gpio_full,
    &g_bisheng_gle_gpio_full,
    &g_bisheng_rf_tcxo_pll_ctrl_full,
    &g_bisheng_xoadc_ctrl_full,
    &g_bisheng_gf_ctrl_full,
    &g_bisheng_gf_diag_ctrl_full,
    &g_bisheng_gf_rf_abb_ctrl_full,
    &g_bisheng_gf_patch_ctrl_full,
    &g_bisheng_gcpu_trace_ctrl_full,
    &g_bisheng_b_ctrl_full,
    &g_bisheng_bf_diag_ctrl_full,
    &g_bisheng_bf_rf_abb_ctrl_full,
    &g_bisheng_b_patch_ctrl_full,
    &g_bisheng_fm_ctrl_full,
    &g_bisheng_bcpu_trace_ctrl_full,
    &g_bisheng_w_ctrl_part1,
    &g_bisheng_w_ctrl_part2,
    &g_bisheng_w_diag_ctrl_full,
    &g_bisheng_w_patch_ctrl_full,
    &g_bisheng_wl_rf_abb_ch_full,
    &g_bisheng_wl_rf_abb_com_ctrl_full,
    &g_bisheng_wcpu_trace_ctrl_full,
    &g_bisheng_gle_ctrl_full,
    &g_bisheng_gle_diag_ctrl_full,
    &g_bisheng_gle_rf_abb_ctrl_full,
    &g_bisheng_gle_trace_ctrl_full,
    &g_bisheng_pcie_ctrl_full,
    &g_bisheng_host_ctrl_full,
};

static ssi_reg_info *g_bisheng_tcxo_detect_regs[] = {
    &g_bisheng_tcxo_detect_reg1,
    &g_bisheng_tcxo_detect_reg2,
    &g_bisheng_tcxo_detect_reg3
};

int bisheng_ssi_check_tcxo_available(void)
{
    uint16_t w_cur_sts = ssi_read16(gpio_ssi_reg(SSI_SSI_RPT_STS_1));
    uint16_t b_cur_sts = ssi_read16(gpio_ssi_reg(SSI_SSI_RPT_STS_3));
    uint16_t g_cur_sts = ssi_read16(gpio_ssi_reg(SSI_SSI_RPT_STS_5));

    ssi_read_master_regs(&g_bisheng_ssi_master_reg_full, NULL, 0, g_ssi_is_logfile);

    /* w_cur_sts/b_cur_sts/g_cur_sts中的bit[12:10]有一个为0x3, 表示TCXO在位 */
    if (((w_cur_sts & 0xC00) == 0xC00) || ((b_cur_sts & 0xC00) == 0xC00) || ((g_cur_sts & 0xC00) == 0xC00)) {
        ps_print_info("ssi tcxo is available, switch clk to ssi bypass\n");
        return BOARD_SUCC;
    }

    ps_print_info("ssi tcxo not available, switch clk to ssi\n");
    return BOARD_FAIL;
}

static void bisheng_dsm_cpu_info_dump(void)
{
    int32_t i;
    int32_t ret;
    int32_t count = 0;
    char buf[DSM_CPU_INFO_SIZE];
    /* dsm cpu信息上报 */
    if (g_halt_det_cnt || g_hi11xx_kernel_crash) {
        ps_print_info("g_halt_det_cnt=%u g_hi11xx_kernel_crash=%d dsm_cpu_info_dump return\n",
                      g_halt_det_cnt, g_hi11xx_kernel_crash);
        return;
    }

    /* 没有检测到异常，上报记录的CPU信息 */
    memset_s((void *)buf, sizeof(buf), 0, sizeof(buf));
    ret = snprintf_s(buf + count, sizeof(buf) - count, sizeof(buf) - count - 1,
                     "wcpu_state=0x%x %s, bcpu_state=0x%x %s, gcpu_state=0x%x %s, gle_state=0x%x %s",
                     g_ssi_cpu_infos.wcpu_info.cpu_state,
                     (g_ssi_cpu_st_str[g_ssi_cpu_infos.wcpu_info.cpu_state & 0x7]),
                     g_ssi_cpu_infos.bcpu_info.cpu_state,
                     (g_ssi_cpu_st_str[g_ssi_cpu_infos.bcpu_info.cpu_state & 0x7]),
                     g_ssi_cpu_infos.gcpu_info.cpu_state,
                     (g_ssi_cpu_st_str[g_ssi_cpu_infos.gcpu_info.cpu_state & 0x7]),
                     g_ssi_cpu_infos.gle_info.cpu_state,
                     (g_ssi_cpu_st_str[g_ssi_cpu_infos.gle_info.cpu_state & 0x7]));
    if (ret < 0) {
        goto done;
    }
    count += ret;

    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        if (g_ssi_cpu_infos.wcpu_info.reg_flag[i] == 0) {
            continue;
        }
        ret = snprintf_s(buf + count, sizeof(buf) - count, sizeof(buf) - count - 1,
                         "wcpu[%d] pc:0x%x lr:0x%x sp:0x%x ", i, g_ssi_cpu_infos.wcpu_info.pc[i],
                         g_ssi_cpu_infos.wcpu_info.lr[i], g_ssi_cpu_infos.wcpu_info.sp[i]);
        if (ret < 0) {
            goto done;
        }
        count += ret;
    }

done:
#ifdef CONFIG_HUAWEI_DSM
    hw_110x_dsm_client_notify(SYSTEM_TYPE_PLATFORM, DSM_1103_HALT, "%s\n", buf);
#else
    oal_io_print("log str format err [non-dsm]%s\n", buf);
#endif
}

static void bisheng_ssi_check_buck_scp_ocp_status(void)
{
    ps_print_info("bisheng_ssi_check_buck_scp_ocp_status bypass");
}

static int bisheng_ssi_check_wcpu_is_working(void)
{
    uint32_t reg, mask;

    /* pilot */
    reg = (uint32_t)ssi_read32(BISHENG_PMU_CMU_CTL_SYS_STATUS_0_REG);
    mask = reg & 0x7;
    ps_print_info("cpu state=0x%8x, wcpu is %s\n", reg, g_ssi_cpu_st_str[mask]);
    g_ssi_cpu_infos.wcpu_info.cpu_state = mask;
    if (mask == 0x5) {
        bisheng_ssi_check_buck_scp_ocp_status();
    }
    return (mask == 0x3);
}

static int bisheng_ssi_check_bcpu_is_working(void)
{
    uint32_t reg, mask;

    /* pilot */
    reg = (uint32_t)ssi_read32(BISHENG_PMU_CMU_CTL_SYS_STATUS_0_REG);
    mask = (reg >> 3) & 0x7; /* reg shift right by 3 bits and 0x7 bits */
    ps_print_info("cpu state=0x%8x, bcpu is %s\n", reg, g_ssi_cpu_st_str[mask]);
    g_ssi_cpu_infos.bcpu_info.cpu_state = mask;
    if (mask == 0x5) {
        bisheng_ssi_check_buck_scp_ocp_status();
    }
    return (mask == 0x3);
}

static int bisheng_ssi_check_gcpu_is_working(void)
{
    uint32_t reg, mask;

    /* pilot */
    reg = (uint32_t)ssi_read32(BISHENG_PMU_CMU_CTL_SYS_STATUS_0_REG);
    mask = (reg >> 6) & 0x7; /* reg shift right by 3 bits and 0x7 bits */
    ps_print_info("cpu state=0x%8x, gcpu is %s\n", reg, g_ssi_cpu_st_str[mask]);
    g_ssi_cpu_infos.gcpu_info.cpu_state = mask;
    if (mask == 0x5) {
        bisheng_ssi_check_buck_scp_ocp_status();
    }
    return (mask == 0x3);
}

static int bisheng_ssi_check_gle_is_working(void)
{
    uint32_t reg, mask;

    /* pilot */
    reg = (uint32_t)ssi_read32(BISHENG_PMU_CMU_CTL_SYS_STATUS_0_REG);
    mask = (reg >> 9) & 0x7; /* reg shift right by 3 bits and 0x7 bits */
    ps_print_info("cpu state=0x%8x, gle is %s\n", reg, g_ssi_cpu_st_str[mask]);
    g_ssi_cpu_infos.gle_info.cpu_state = mask;
    if (mask == 0x5) {
        bisheng_ssi_check_buck_scp_ocp_status();
    }
    return (mask == 0x3);
}

int bisheng_ssi_read_wcpu_pc_lr_sp(void)
{
    int i;
    uint32_t reg_low, reg_high, load, pc, lr, sp;

    /* read pc twice check whether cpu is runing */
    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        // pc锁存 使能位:bit0
        ssi_write32(BISHENG_GLB_CTL_WCPU_LOAD_REG, 0x81);
        oal_mdelay(2);
        load = (uint32_t)ssi_read32(BISHENG_GLB_CTL_WCPU_LOAD_REG);

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_WCPU_PC_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_WCPU_PC_H_REG);
        pc = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_WCPU_LR_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_WCPU_LR_H_REG);
        lr = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_WCPU_SP_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_WCPU_SP_H_REG);
        sp = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        // pc锁存 清除使能位:bit1
        ssi_write32(BISHENG_GLB_CTL_WCPU_LOAD_REG, 0x20);
        oal_mdelay(1);

        ps_print_info("gpio-ssi:load_sts:0x%x, read wcpu[%i], pc:0x%x, lr:0x%x, sp:0x%x \n",
                      load, i, pc, lr, sp);

        if (!pc) {
            ps_print_info("wcpu pc all zero\n");
        } else {
            if (g_ssi_cpu_infos.wcpu_info.reg_flag[i] == 0) {
                g_ssi_cpu_infos.wcpu_info.reg_flag[i] = 1;
                g_ssi_cpu_infos.wcpu_info.pc[i] = pc;
                g_ssi_cpu_infos.wcpu_info.lr[i] = lr;
                g_ssi_cpu_infos.wcpu_info.sp[i] = sp;
            }
        }
        oal_mdelay(10); /* delay 10 ms */
    }

    return 0;
}

int bisheng_ssi_read_bpcu_pc_lr_sp(void)
{
    int i;
    uint32_t reg_low, reg_high, load, pc, lr, sp;

    /* read pc twice check whether cpu is runing */
    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        ssi_write32(BISHENG_GLB_CTL_BCPU_LOAD_REG, 0x1);
        oal_mdelay(1);
        load = (uint32_t)ssi_read32(BISHENG_GLB_CTL_BCPU_LOAD_REG);

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_BCPU_PC_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_BCPU_PC_H_REG);
        pc = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_BCPU_LR_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_BCPU_LR_H_REG);
        lr = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_BCPU_SP_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_BCPU_SP_H_REG);
        sp = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        ps_print_info("gpio-ssi:read bcpu[%i], load_sts:0x%x, pc:0x%x, lr:0x%x, sp:0x%x \n", i, load, pc, lr, sp);
        if (!pc && !lr && !sp) {
            ps_print_info("bcpu pc lr sp all zero\n");
        } else {
            if (g_ssi_cpu_infos.bcpu_info.reg_flag[i] == 0) {
                g_ssi_cpu_infos.bcpu_info.reg_flag[i] = 1;
                g_ssi_cpu_infos.bcpu_info.pc[i] = pc;
                g_ssi_cpu_infos.bcpu_info.lr[i] = lr;
                g_ssi_cpu_infos.bcpu_info.sp[i] = sp;
            }
        }
        oal_mdelay(10); /* delay 10 ms */
    }

    return 0;
}

int bisheng_ssi_read_gpcu_pc_lr_sp(void)
{
    int i;
    uint32_t reg_low, reg_high, load, pc, lr, sp;

    /* read pc twice check whether cpu is runing */
    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        ssi_write32(BISHENG_GLB_CTL_GCPU_LOAD_REG, 0x1);
        oal_mdelay(1);
        load = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GCPU_LOAD_REG);

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GCPU_PC_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GCPU_PC_H_REG);
        pc = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GCPU_LR_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GCPU_LR_H_REG);
        lr = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GCPU_SP_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GCPU_SP_H_REG);
        sp = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        ps_print_info("gpio-ssi:read gcpu[%i], load_sts:0x%x pc:0x%x, lr:0x%x, sp:0x%x \n", i, load, pc, lr, sp);
        if (!pc && !lr && !sp) {
            ps_print_info("gcpu pc lr sp all zero\n");
        } else {
            if (g_ssi_cpu_infos.gcpu_info.reg_flag[i] == 0) {
                g_ssi_cpu_infos.gcpu_info.reg_flag[i] = 1;
                g_ssi_cpu_infos.gcpu_info.pc[i] = pc;
                g_ssi_cpu_infos.gcpu_info.lr[i] = lr;
                g_ssi_cpu_infos.gcpu_info.sp[i] = sp;
            }
        }
        oal_mdelay(10); /* delay 10 ms */
    }

    return 0;
}

int bisheng_ssi_read_gle_pc_lr_sp(void)
{
    int i;
    uint32_t reg_low, reg_high, load, pc, lr;

    /* read pc twice check whether cpu is runing */
    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        ssi_write32(BISHENG_GLB_CTL_GLE_LOAD_REG, 0x1);
        oal_mdelay(1);
        load = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GLE_LOAD_REG);

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GLE_PC_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GLE_PC_H_REG);
        pc = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GLE_LR_L_REG);
        reg_high = (uint32_t)ssi_read32(BISHENG_GLB_CTL_GLE_LR_H_REG);
        lr = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        ps_print_info("gpio-ssi:read gcpu[%i], load_sts:0x%x pc:0x%x, lr:0x%x\n", i, load, pc, lr);
        if (!pc && !lr) {
            ps_print_info("gcpu pc lr all zero\n");
        } else {
            if (g_ssi_cpu_infos.gle_info.reg_flag[i] == 0) {
                g_ssi_cpu_infos.gle_info.reg_flag[i] = 1;
                g_ssi_cpu_infos.gle_info.pc[i] = pc;
                g_ssi_cpu_infos.gle_info.lr[i] = lr;
            }
        }
        oal_mdelay(10); /* delay 10 ms */
    }

    return 0;
}

int bisheng_ssi_read_device_arm_register(void)
{
    int32_t ret;

    ret = bisheng_ssi_check_wcpu_is_working();
    if (ret < 0) {
        return ret;
    }
    if (ret) {
        bisheng_ssi_read_wcpu_pc_lr_sp();
    }
    bfgx_print_subsys_state();
    ret = bisheng_ssi_check_bcpu_is_working();
    if (ret < 0) {
        return ret;
    }
    if (ret) {
        bisheng_ssi_read_bpcu_pc_lr_sp();
    }
    ret = bisheng_ssi_check_gcpu_is_working();
    if (ret < 0) {
        return ret;
    }
    if (ret) {
        bisheng_ssi_read_gpcu_pc_lr_sp();
    }
    ret = bisheng_ssi_check_gle_is_working();
    if (ret < 0) {
        return ret;
    }
    if (ret) {
        bisheng_ssi_read_gle_pc_lr_sp();
    }

    // 打印device版本
    plat_changid_print();

    return 0;
}

static int bisheng_ssi_dump_device_aon_regs(unsigned long long module_set)
{
    int ret;
    if (module_set & SSI_MODULE_MASK_AON) {
        ret = ssi_read_reg_info_arry(g_bisheng_aon_reg_full, sizeof(g_bisheng_aon_reg_full) / sizeof(ssi_reg_info *),
                                     g_ssi_is_logfile);
        if (ret) {
            return -OAL_EFAIL;
        }
    }

    /* 建链之前不能读 */
    if (module_set & SSI_MODULE_MASK_PCIE_DBI) {
        if (bisheng_ssi_check_wcpu_is_working()) {
            ret = ssi_read_reg_info(&g_bisheng_pcie_dbi_full, NULL, 0, g_ssi_is_logfile);
            if (ret) {
                ps_print_info("pcie dbi regs read failed, continue try aon\n");
            }
        } else {
            ps_print_info("pcie dbi regs can't dump, wcpu down\n");
        }
    }

    return OAL_SUCC;
}

static int bisheng_ssi_dump_device_arm_regs(unsigned long long module_set)
{
    int ret;

    if (module_set & SSI_MODULE_MASK_ARM_REG) {
        ret = bisheng_ssi_read_device_arm_register();
        if (ret) {
            goto ssi_fail;
        }
    }

    return 0;
ssi_fail:
    return ret;
}

static int bisheng_ssi_dump_device_wcpu_key_mem(unsigned long long module_set)
{
    int ret;

    if (module_set & SSI_MODULE_MASK_WCPU_KEY_DTCM) {
        /* 仅debug版本, 通过ssi获取cpu_trace结果 */
        if (is_hi110x_debug_type() != OAL_TRUE) {
            ps_print_info("user mode or maybe beta user,ssi dump bypass\n");
            return 0;
        }

        if (bisheng_ssi_check_wcpu_is_working()) {
            ret = ssi_read_reg_info(&g_bisheng_w_key_mem, NULL, 0, g_ssi_is_logfile);
            if (ret) {
                ps_print_info("wcpu key mem read failed, continue try aon\n");
            }
        } else {
            ps_print_info("wcpu key mem can't dump, wcpu down\n");
        }
    }
    return 0;
}

static int bisheng_ssi_dump_device_bcpu_key_mem(unsigned long long module_set)
{
    int ret;

    if (module_set & SSI_MODULE_MASK_WCPU_KEY_DTCM) {
        /* 仅debug版本, 通过ssi获取cpu_trace结果 */
        if (is_hi110x_debug_type() != OAL_TRUE) {
            ps_print_info("user mode or maybe beta user,ssi dump bypass\n");
            return 0;
        }

        if (bisheng_ssi_check_bcpu_is_working()) {
            ret = ssi_read_reg_info(&g_bisheng_b_key_mem, NULL, 0, g_ssi_is_logfile);
            if (ret) {
                ps_print_info("bcpu key mem read failed, continue try aon\n");
            }
        } else {
            ps_print_info("bcpu key mem can't dump, bcpu down\n");
        }
    }
    return 0;
}

static int bisheng_ssi_dump_device_gcpu_key_mem(unsigned long long module_set)
{
    int ret;

    if (module_set & SSI_MODULE_MASK_WCPU_KEY_DTCM) {
        /* 仅debug版本, 通过ssi获取cpu_trace结果 */
        if (is_hi110x_debug_type() != OAL_TRUE) {
            ps_print_info("user mode or maybe beta user,ssi dump bypass\n");
            return 0;
        }

        if (bisheng_ssi_check_gcpu_is_working()) {
            ret = ssi_read_reg_info(&g_bisheng_gf_key_mem, NULL, 0, g_ssi_is_logfile);
            if (ret) {
                ps_print_info("gfcpu key mem read failed, continue try aon\n");
            }
        } else {
            ps_print_info("gfcpu key mem can't dump, gfcpu down\n");
        }
    }
    return 0;
}

static int bisheng_ssi_dump_device_gle_key_mem(unsigned long long module_set)
{
    int ret;

    if (module_set & SSI_MODULE_MASK_WCPU_KEY_DTCM) {
        /* 仅debug版本, 通过ssi获取cpu_trace结果 */
        if (is_hi110x_debug_type() != OAL_TRUE) {
            ps_print_info("user mode or maybe beta user,ssi dump bypass\n");
            return 0;
        }

        if (bisheng_ssi_check_gle_is_working()) {
            ret = ssi_read_reg_info(&g_bisheng_gle_key_mem, NULL, 0, g_ssi_is_logfile);
            if (ret) {
                ps_print_info("gle key mem read failed, continue try aon\n");
            }
        } else {
            ps_print_info("gle key mem can't dump, gle down\n");
        }
    }
    return 0;
}

static void bisheng_ssi_trigger_tcxo_detect(uint32_t *tcxo_det_value_target)
{
    if ((*tcxo_det_value_target) == 0) {
        ps_print_err("tcxo_det_value_target is zero, trigger failed\n");
        return;
    }

    /* 刚做过detect,改变det_value+2,观测值是否改变 */
    if ((*tcxo_det_value_target) == ssi_read32(BISHENG_GLB_CTL_TCXO_32K_DET_CNT_REG)) {
        (*tcxo_det_value_target) = (*tcxo_det_value_target) + 2;
    }

    ssi_write32(BISHENG_GLB_CTL_TCXO_32K_DET_CNT_REG, (*tcxo_det_value_target)); /* 设置计数周期 */
    ssi_write32(BISHENG_GLB_CTL_TCXO_DET_CTL_REG, 0x0);                          /* tcxo_det_en disable */

    /* to tcxo */
    ssi_switch_clk(SSI_AON_CLKSEL_TCXO);
    /* delay 150 us */
    oal_udelay(150);
    /* to ssi */
    ssi_switch_clk(SSI_AON_CLKSEL_SSI);

    ssi_write32(BISHENG_GLB_CTL_TCXO_DET_CTL_REG, 0x1); /* tcxo_det_en enable */

    /* to tcxo */
    ssi_switch_clk(SSI_AON_CLKSEL_TCXO);
    /* delay 31 * 2 * tcxo_det_value_taget us.wait detect done,根据设置的计数周期数 */
    oal_udelay(31 * (*tcxo_det_value_target) * 2);
    /* to ssi */
    ssi_switch_clk(SSI_AON_CLKSEL_SSI);
}

static int bisheng_ssi_detect_32k_handle(uint32_t sys_tick_old, uint64_t cost_time_s, uint32_t *clock_32k)
{
    uint32_t base_32k_clock = RTC_32K_NOMAL_CKL;
    uint32_t sys_tick_new;

    if (bisheng_ssi_check_wcpu_is_working()) {
        sys_tick_new = (uint32_t)ssi_read32(BISHENG_GLB_CTL_SYS_TICK_VALUE_W_0_REG);
    } else if (bisheng_ssi_check_bcpu_is_working()) {
        sys_tick_new = (uint32_t)ssi_read32(BISHENG_GLB_CTL_SYS_TICK_VALUE_B_0_REG);
    } else if (bisheng_ssi_check_gcpu_is_working()) {
        sys_tick_new = (uint32_t)ssi_read32(BISHENG_GLB_CTL_SYS_TICK_VALUE_GF_0_REG);
    } else {
        sys_tick_new = (uint32_t)ssi_read32(BISHENG_GLB_CTL_SYS_TICK_VALUE_GLE_0_REG);
    }

    if (sys_tick_new == sys_tick_old) {
        ps_print_err("32k sys_tick don't change after detect, 32k maybe abnormal, sys_tick=0x%x\n", sys_tick_new);
        return -OAL_EFAIL;
    } else {
        cost_time_s += 1446; /* 经验值,误差1446us */
        (*clock_32k) = (sys_tick_new * 1000) / (uint32_t)cost_time_s; /* 1000 is mean hz to KHz */
        ps_print_info("32k runtime:%llu us, sys_tick:%u\n", cost_time_s, sys_tick_new);
        ps_print_info("32k real= %u Khz[base=%u]\n", (*clock_32k), base_32k_clock);
    }
    return 0;
}

static int bisheng_ssi_detect_tcxo_handle(uint32_t tcxo_det_res_old, uint32_t tcxo_det_value_target, uint32_t clock_32k)
{
    uint64_t base_tcxo_clock = TCXO_NOMAL_CKL;
    uint32_t base_32k_clock = RTC_32K_NOMAL_CKL;
    uint64_t clock_tcxo, div_clock;
    uint64_t tcxo_limit_low, tcxo_limit_high, tcxo_tmp;
    uint32_t tcxo_det_res_new = (uint32_t)ssi_read32(BISHENG_GLB_CTL_TCXO_32K_DET_RESULT_REG);
    if (tcxo_det_res_new == tcxo_det_res_old) {
        ps_print_err("tcxo don't change after detect, tcxo or 32k maybe abnormal, tcxo_count=0x%x, 32k_clock=%u\n",
                     tcxo_det_res_new, clock_32k);
        return -OAL_EFAIL;
    }

    if (tcxo_det_value_target == 0) {
        ps_print_err("tcxo_det_value_target is zero\n");
        return -OAL_EFAIL;
    }

    /* 计算TCXO时钟误差范围 */
    tcxo_tmp = div_u64(base_tcxo_clock, 100); /* 100 is Percentage */
    tcxo_limit_low = (tcxo_tmp * (100 - TCXO_LIMIT_THRESHOLD)); /* TCXO Lower threshold 100 - 5 */
    tcxo_limit_high = (tcxo_tmp * (100 + TCXO_LIMIT_THRESHOLD)); /* TCXO Upper threshold 100 + 5 */

    /* 计算tcxo实际时钟 */
    clock_tcxo = (uint64_t)((tcxo_det_res_new * base_32k_clock) / (tcxo_det_value_target));
    div_clock = clock_tcxo;
    div_clock = div_u64(div_clock, 1000000); /* 1000000 is unit conversion hz to Mhz */
    if ((clock_tcxo < tcxo_limit_low) || (clock_tcxo > tcxo_limit_high)) {
        /* 时钟误差超过阈值 */
        ps_print_err("tcxo real=%llu hz,%llu Mhz[base=%llu][limit:%llu~%llu]\n",
                     clock_tcxo, div_clock, base_tcxo_clock, tcxo_limit_low, tcxo_limit_high);
        return -OAL_EFAIL;
    }

    ps_print_info("tcxo real=%llu hz,%llu Mhz[base=%llu][limit:%llu~%llu]\n",
                  clock_tcxo, div_clock, base_tcxo_clock, tcxo_limit_low, tcxo_limit_high);
    return 0;
}

static uint32_t bisheng_ssi_clr_systick(void)
{
    uint32_t sys_tick_old;
    if (bisheng_ssi_check_wcpu_is_working()) {
        sys_tick_old = (uint32_t)ssi_read32(BISHENG_GLB_CTL_SYS_TICK_VALUE_W_0_REG);
        ssi_write32(BISHENG_GLB_CTL_SYS_TICK_CFG_W_REG, 0x2); /* 清零w systick */
    } else if (bisheng_ssi_check_bcpu_is_working()) {
        sys_tick_old = (uint32_t)ssi_read32(BISHENG_GLB_CTL_SYS_TICK_VALUE_B_0_REG);
        ssi_write32(BISHENG_GLB_CTL_SYS_TICK_CFG_B_REG, 0x2); /* 清零b systick */
    } else if (bisheng_ssi_check_gcpu_is_working()) {
        sys_tick_old = (uint32_t)ssi_read32(BISHENG_GLB_CTL_SYS_TICK_VALUE_GF_0_REG);
        ssi_write32(BISHENG_GLB_CTL_SYS_TICK_CFG_GF_REG, 0x2); /* 清零gf systick */
    } else {
        sys_tick_old = (uint32_t)ssi_read32(BISHENG_GLB_CTL_SYS_TICK_VALUE_GLE_0_REG);
        ssi_write32(BISHENG_GLB_CTL_SYS_TICK_CFG_GLE_REG, 0x2); /* 清零gle systick */
    }
    return sys_tick_old;
}

static int bisheng_ssi_detect_tcxo_is_normal(void)
{
    /*
     * tcxo detect依赖tcxo和32k时钟
     * 如果在启动后tcxo异常, 那么tcxo_det_res为旧值
     * 如果在启动后32k异常, 那么sys_tick为旧值
     */
    int ret;
    uint32_t tcxo_det_present;
    uint32_t sys_tick_old, tcxo_det_res_old;
    uint32_t tcxo_det_value_target = TCXO_32K_DET_VALUE;
    uint32_t clock_32k;
    uint64_t us_to_s;

    declare_time_cost_stru(cost);

    /* 检测TCXO时钟是否在位 */
    tcxo_det_present = (uint32_t)ssi_read32(BISHENG_PMU_CMU_CTL_CLOCK_TCXO_PRESENCE_DET);
    if ((tcxo_det_present >> 8) && 0x1) {
        if ((tcxo_det_present >> 9) && 0x1) {
            ps_print_info("tcxo is present after detect\n");
        } else {
            ps_print_err("tcxo sts:0x%x maybe abnormal, not present after detect\n", tcxo_det_present);
            return -OAL_EFAIL;
        }
    } else {
        ps_print_err("tcxo sts:0x%x maybe abnormal, invalid detect result\n", tcxo_det_present);
        return -OAL_EFAIL;
    }

    /* 检测TCXO时钟精度是否准确 */
    tcxo_det_res_old = (uint32_t)ssi_read32(BISHENG_GLB_CTL_TCXO_32K_DET_RESULT_REG);
    sys_tick_old = bisheng_ssi_clr_systick();

    oal_get_time_cost_start(cost);

    /* 使能TCXO时钟精度检测 */
    bisheng_ssi_trigger_tcxo_detect(&tcxo_det_value_target);
    ret = ssi_read_reg_info_arry(g_bisheng_tcxo_detect_regs, sizeof(g_bisheng_tcxo_detect_regs) /
                                sizeof(ssi_reg_info *), g_ssi_is_logfile);
    if (ret) {
        return ret;
    }
    oal_udelay(1000); /* delay 1000 us,wait 32k count more */
    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);

    /* 32k detect by system clock */
    us_to_s = time_cost_var_sub(cost);
    ret = bisheng_ssi_detect_32k_handle(sys_tick_old, us_to_s, &clock_32k);
    if (ret) {
        return ret;
    }

    /* tcxo detect by 32k clock */
    return bisheng_ssi_detect_tcxo_handle(tcxo_det_res_old, tcxo_det_value_target, clock_32k);
}

static void bisheng_ssi_dump_device_tcxo_regs(unsigned long long module_set)
{
    int ret;
    if (module_set & (SSI_MODULE_MASK_AON | SSI_MODULE_MASK_AON_CUT)) {
        ret = bisheng_ssi_detect_tcxo_is_normal();
        if (ret) {
            ps_print_info("tcxo detect failed, continue dump\n");
        }
    }
}

static int bisheng_regs_dump(unsigned long long module_set)
{
    int ret;

    /* try to read pc&lr&sp regs */
    ret = bisheng_ssi_dump_device_arm_regs(module_set);
    if (ret) {
        return ret;
    }

    ret = ssi_check_device_isalive();
    if (ret) {
        return ret;
    }

    /* try to read all aon regs */
    ret = bisheng_ssi_dump_device_aon_regs(module_set);
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* try to read pc&lr&sp regs again */
    ret = bisheng_ssi_dump_device_arm_regs(module_set);
    if (ret) {
        return ret;
    }

    /* detect tcxo clock is normal, trigger */
    bisheng_ssi_dump_device_tcxo_regs(module_set);

    return OAL_SUCC;
}

int bisheng_ssi_device_regs_dump(unsigned long long module_set)
{
    int ret;
    g_halt_det_cnt = 0;

    memset_s(&g_ssi_cpu_infos, sizeof(g_ssi_cpu_infos), 0, sizeof(g_ssi_cpu_infos));

    ssi_read16(gpio_ssi_reg(SSI_SSI_CTRL));
    ssi_read16(gpio_ssi_reg(SSI_SEL_CTRL));

    ssi_switch_clk(SSI_AON_CLKSEL_SSI);

    ret = ssi_check_device_isalive();
    if (ret) {
        /* try to reset aon */
        ssi_force_reset_reg();
        ssi_switch_clk(SSI_AON_CLKSEL_SSI);
        if (ssi_check_device_isalive() < 0) {
            ps_print_info("after reset aon, ssi still can't work\n");
            goto ssi_fail;
        } else {
            ps_print_info("after reset aon, ssi ok, dump acp/ocp reg\n");
            bisheng_ssi_check_buck_scp_ocp_status();
            module_set = SSI_MODULE_MASK_COMM;
        }
    } else {
        bisheng_ssi_check_buck_scp_ocp_status();
    }

    ret = bisheng_regs_dump(module_set);
    if (ret != OAL_SUCC) {
        goto ssi_fail;
    }

    /* try to read wcpu key_mem */
    ret = bisheng_ssi_dump_device_wcpu_key_mem(module_set);
    if (ret != OAL_SUCC) {
        goto ssi_fail;
    }

    /* try to read bcpu key_mem */
    ret = bisheng_ssi_dump_device_bcpu_key_mem(module_set);
    if (ret != OAL_SUCC) {
        goto ssi_fail;
    }

    /* try to read gcpu key_mem */
    ret = bisheng_ssi_dump_device_gcpu_key_mem(module_set);
    if (ret != OAL_SUCC) {
        goto ssi_fail;
    }

    /* try to read gle key_mem */
    ret = bisheng_ssi_dump_device_gle_key_mem(module_set);
    if (ret != OAL_SUCC) {
        goto ssi_fail;
    }

    ssi_switch_clk(SSI_AON_CLKSEL_TCXO);
    bisheng_dsm_cpu_info_dump();

    return 0;

ssi_fail:
    ssi_switch_clk(SSI_AON_CLKSEL_TCXO);
    bisheng_dsm_cpu_info_dump();
    return ret;
}

#endif /* #ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG */
