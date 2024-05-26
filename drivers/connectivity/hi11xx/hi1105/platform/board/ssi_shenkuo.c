

#ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG
#include "ssi_shenkuo.h"

#include "plat_debug.h"
#include "plat_pm.h"
#include "ssi_common.h"
#include "plat_parse_changid.h"
#include "ssi_spmi.h"

#define SHENKUO_SPMI_BASE_ADDR     0x40014000
#define TCXO_32K_DET_VALUE    10
#define TCXO_LIMIT_THRESHOLD 5
#define TCXO_NOMAL_CKL  38400000
#define RTC_32K_NOMAL_CKL  32768

/* hi6506 reg, reg_offset, default_value */
#define CHIP_VERSION_2                 0x2
#define CHIP_VERSION_2_DEFAULT_VALUE   0x30
/* IRQ MASK */
#define SCP0_MASK_REG                  0x5000
#define OCP0_MASK_REG                  0x5001
#define OCP1_MASK_REG                  0x5002
#define IRQ0_MASK_REG                  0x5003
#define OCP2_MASK_REG                  0X5004
/* IRQ */
#define SCP0_IRQ_REG                   0x6000
#define OCP0_IRQ_REG                   0x6001
#define OCP1_IRQ_REG                   0x6002
#define IRQ0_IRQ_REG                   0x6003
#define OCP2_IRQ_REG                   0X6004
/* NP_EVENT */
#define SCP0_NP_EVENT_REG              0x7000
#define OCP0_NP_EVENT_REG              0x7001
#define OCP1_NP_EVENT_REG              0x7002
#define IRQ0_NP_EVENT_REG              0x7003
#define OCP2_NP_EVENT_REG              0x7004

static shenkuo_ssi_cpu_infos g_ssi_cpu_infos;

/* dump 寄存器定义 */
static ssi_reg_info g_shenkuo_ssi_master_reg_full = { 0x40, 0x30, SSI_RW_SSI_MOD };
static ssi_reg_info g_shenkuo_glb_ctrl_full = { 0x40000000, 0x1000, SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_glb_ctrl_extend1 = { 0x40001400, 0x30,   SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_glb_ctrl_extend2 = { 0x40001d00, 0x60,   SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_pmu_cmu_ctrl_full = { 0x40002000, 0xb50,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_pmu2_cmu_ctrl_part1 = { 0x4000E000, 0x500,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_pmu2_cmu_ctrl_part2 = { 0x4000EB00, 0x3c8,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_coex_ctl_part1 = { 0x4000d000, 0x2b8,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_coex_ctl_part2 = { 0x4000d600, 0x38,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_rf_tcxo_pll_ctrl_full = { 0x40010000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_xoadc_ctrl_full = { 0x40013000, 0x20,  SSI_RW_DWORD_MOD };
static ssi_reg_info g_shenkuo_g_ctrl_full = { 0x40300000, 0x380,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_g_diag_ctrl_full = { 0x40305000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_g_rf_abb_ctrl_full = { 0x40307000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_g_patch_ctrl_full = { 0x40301000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_gcpu_trace_ctrl_full = { 0x40312000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_b_ctrl_full = { 0x40200000, 0x210,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_bf_diag_ctrl_full = { 0x40207000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_bf_rf_abb_ctrl_full = { 0x40209000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_b_patch_ctrl_full = { 0x40217000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_fm_ctrl_full = { 0x40231000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_bcpu_trace_ctrl_full = { 0x4021B000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_w_ctrl_part1 = { 0x40105100, 0x340,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_w_ctrl_part2 = { 0x40106000, 0x808,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_diag_ctrl_full = { 0x40124100, 0x20,  SSI_RW_DWORD_MOD };
static ssi_reg_info g_shenkuo_w_patch_ctrl_full = { 0x40126000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_w_cpu_ctrl_full = { 0x40100100, 0x1A8,  SSI_RW_DWORD_MOD };
static ssi_reg_info g_shenkuo_wl_rf_abb_ch_full = { 0x40114000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_wl_rf_abb_com_ctrl_full = { 0x40123000, 0x20,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_pcie_ctrl_full = { 0x40108000, 0x63c,  SSI_RW_DWORD_MOD };
static ssi_reg_info g_shenkuo_host_ctrl_full = { 0x40113000, 0xe80,  SSI_RW_DWORD_MOD };
static ssi_reg_info g_shenkuo_pcie_dbi_full = { 0x40107000, 0x1000,  SSI_RW_DWORD_MOD };     /* 没建链之前不能读 */
static ssi_reg_info g_shenkuo_wifi_gpio_full = {0x40005000, 0x74, SSI_RW_WORD_MOD};
static ssi_reg_info g_shenkuo_bfgx_gpio_full = {0x40006000, 0x74, SSI_RW_WORD_MOD};
static ssi_reg_info g_shenkuo_w_key_mem = { 0x20a0000, 0x2000, SSI_RW_DWORD_MOD};

static ssi_reg_info g_shenkuo_tcxo_detect_reg1 = { 0x40000040, 0x4,  SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_tcxo_detect_reg2 = { 0x400000c0, 0x40, SSI_RW_WORD_MOD };
static ssi_reg_info g_shenkuo_tcxo_detect_reg3 = { 0x40000800, 0xc,  SSI_RW_WORD_MOD };


/* shenkuo 默认dump 所有寄存器 */
static ssi_reg_info *g_shenkuo_aon_reg_full[] = {
    &g_shenkuo_glb_ctrl_full,
    &g_shenkuo_glb_ctrl_extend1,
    &g_shenkuo_glb_ctrl_extend2,
    &g_shenkuo_pmu_cmu_ctrl_full,
    &g_shenkuo_pmu2_cmu_ctrl_part1,
    &g_shenkuo_pmu2_cmu_ctrl_part2,
    &g_shenkuo_coex_ctl_part1,
    &g_shenkuo_coex_ctl_part2,
    &g_shenkuo_wifi_gpio_full,
    &g_shenkuo_bfgx_gpio_full,
    &g_shenkuo_rf_tcxo_pll_ctrl_full,
    &g_shenkuo_xoadc_ctrl_full,
    &g_shenkuo_g_ctrl_full,
    &g_shenkuo_g_diag_ctrl_full,
    &g_shenkuo_g_rf_abb_ctrl_full,
    &g_shenkuo_g_patch_ctrl_full,
    &g_shenkuo_gcpu_trace_ctrl_full,
    &g_shenkuo_b_ctrl_full,
    &g_shenkuo_bf_diag_ctrl_full,
    &g_shenkuo_bf_rf_abb_ctrl_full,
    &g_shenkuo_b_patch_ctrl_full,
    &g_shenkuo_fm_ctrl_full,
    &g_shenkuo_bcpu_trace_ctrl_full,
    &g_shenkuo_w_ctrl_part1,
    &g_shenkuo_w_ctrl_part2,
    &g_shenkuo_diag_ctrl_full,
    &g_shenkuo_w_patch_ctrl_full,
    &g_shenkuo_w_cpu_ctrl_full,
    &g_shenkuo_wl_rf_abb_ch_full,
    &g_shenkuo_wl_rf_abb_com_ctrl_full,
    &g_shenkuo_pcie_ctrl_full,
    &g_shenkuo_host_ctrl_full,
};

static ssi_reg_info *g_shenkuo_tcxo_detect_regs[] = {
    &g_shenkuo_tcxo_detect_reg1,
    &g_shenkuo_tcxo_detect_reg2,
    &g_shenkuo_tcxo_detect_reg3
};

uint32_t shenkuo_ssi_spmi_read(uint32_t addr)
{
    int32_t ret;
    uint32_t reg;
    uint32_t cur_channel;
    uint32_t data;

    /* first, reinit spmi_base_addr */
    reinit_spmi_base_addr(SHENKUO_SPMI_BASE_ADDR);

    /* pilot */
    reg = (uint32_t)ssi_read32(SHENKUO_PMU_CMU_CTL_SYS_STATUS_0_REG);
    if ((reg & 0x7) != 0) {
        cur_channel = SPMI_CHANNEL_WIFI;
    } else if (((reg >> 3) & 0x7) != 0) { // bcpu status at register [5:3] bits
        cur_channel = SPMI_CHANNEL_BFG;
    } else if (((reg >> 6) & 0x7) != 0) { // gnss status at register [8:6] bits
        cur_channel = SPMI_CHANNEL_GNSS;
    } else {
        ps_print_err("no cpu power on,please check wifi/bt/gnss is turned on?\n");
        return 0xFFFFFFFF;
    }

    ret = ssi_spmi_read(addr, cur_channel, SPMI_SLAVEID_HI6506, &data);
    if (ret != OAL_SUCC) {
        ps_print_err("Spmi read data through ssi fail\n");
        return 0xFFFFFFFF;
    }

    return data;
}

void shenkuo_ssi_spmi_write(uint32_t addr, uint32_t data)
{
    int32_t ret;
    uint32_t reg;
    uint32_t cur_channel;

    /* first, reinit spmi_base_addr */
    reinit_spmi_base_addr(SHENKUO_SPMI_BASE_ADDR);

    /* pilot */
    reg = (uint32_t)ssi_read32(SHENKUO_PMU_CMU_CTL_SYS_STATUS_0_REG);
    if ((reg & 0x7) != 0) {
        cur_channel = SPMI_CHANNEL_WIFI;
    } else if (((reg >> 3) & 0x7) != 0) { // bcpu status at register [5:3] bits
        cur_channel = SPMI_CHANNEL_BFG;
    } else if (((reg >> 6) & 0x7) != 0) { // gnss status at register [8:6] bits
        cur_channel = SPMI_CHANNEL_GNSS;
    } else {
        ps_print_err("no cpu power on,plese check wifi/bt/gnss is turned on?\n");
        return;
    }

    ret = ssi_spmi_write(addr, cur_channel, SPMI_SLAVEID_HI6506, data);
    if (ret != OAL_SUCC) {
        ps_print_err("Spmi write data through ssi fail\n");
    }
}

int shenkuo_ssi_check_tcxo_available(void)
{
    uint16_t w_cur_sts = ssi_read16(gpio_ssi_reg(SSI_SSI_RPT_STS_1));
    uint16_t b_cur_sts = ssi_read16(gpio_ssi_reg(SSI_SSI_RPT_STS_3));
    uint16_t g_cur_sts = ssi_read16(gpio_ssi_reg(SSI_SSI_RPT_STS_5));

    ssi_read_master_regs(&g_shenkuo_ssi_master_reg_full, NULL, 0, g_ssi_is_logfile);

    /* w_cur_sts/b_cur_sts/g_cur_sts中的bit[12:10]有一个为0x3, 表示TCXO在位 */
    if (((w_cur_sts & 0xC00) == 0xC00) || ((b_cur_sts & 0xC00) == 0xC00) || ((g_cur_sts & 0xC00) == 0xC00)) {
        ps_print_info("ssi tcxo is available, switch clk to ssi bypass\n");
        return BOARD_SUCC;
    }

    ps_print_info("ssi tcxo not available, switch clk to ssi\n");
    return BOARD_FAIL;
}

static void shenkuo_dsm_cpu_info_dump(void)
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
                     "wcpu_state=0x%x %s, bcpu_state=0x%x %s, gcpu_state=0x%x %s ",
                     g_ssi_cpu_infos.wcpu0_info.cpu_state,
                     (g_ssi_cpu_st_str[g_ssi_cpu_infos.wcpu0_info.cpu_state & 0x7]),
                     g_ssi_cpu_infos.bcpu_info.cpu_state,
                     (g_ssi_cpu_st_str[g_ssi_cpu_infos.bcpu_info.cpu_state & 0x7]),
                     g_ssi_cpu_infos.gcpu_info.cpu_state,
                     (g_ssi_cpu_st_str[g_ssi_cpu_infos.gcpu_info.cpu_state & 0x7]));
    if (ret < 0) {
        goto done;
    }
    count += ret;

    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        if (g_ssi_cpu_infos.wcpu0_info.reg_flag[i] == 0) {
            continue;
        }
        ret = snprintf_s(buf + count, sizeof(buf) - count, sizeof(buf) - count - 1,
                         "wcpu0[%d] pc:0x%x lr:0x%x sp:0x%x ", i, g_ssi_cpu_infos.wcpu0_info.pc[i],
                         g_ssi_cpu_infos.wcpu0_info.lr[i], g_ssi_cpu_infos.wcpu0_info.sp[i]);
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

static void shenkuo_ssi_check_buck_scp_ocp_status(void)
{
    ps_print_info("shenkuo_ssi_check_buck_scp_ocp_status bypass");
}

static int shenkuo_ssi_check_wcpu_is_working(void)
{
    uint32_t reg, mask;

    /* pilot */
    reg = (uint32_t)ssi_read32(SHENKUO_PMU_CMU_CTL_SYS_STATUS_0_REG);
    mask = reg & 0x7;
    ps_print_info("cpu state=0x%8x, wcpu0 is %s\n", reg, g_ssi_cpu_st_str[mask]);
    g_ssi_cpu_infos.wcpu0_info.cpu_state = mask;
    if (mask == 0x5) {
        shenkuo_ssi_check_buck_scp_ocp_status();
    }
    return (mask == 0x3);
}

static int shenkuo_ssi_check_bcpu_is_working(void)
{
    uint32_t reg, mask;

    /* pilot */
    reg = (uint32_t)ssi_read32(SHENKUO_PMU_CMU_CTL_SYS_STATUS_0_REG);
    mask = (reg >> 3) & 0x7; /* reg shift right by 3 bits and 0x7 bits */
    ps_print_info("cpu state=0x%8x, bcpu is %s\n", reg, g_ssi_cpu_st_str[mask]);
    g_ssi_cpu_infos.bcpu_info.cpu_state = mask;
    if (mask == 0x5) {
        shenkuo_ssi_check_buck_scp_ocp_status();
    }
    return (mask == 0x3);
}

static int shenkuo_ssi_check_gcpu_is_working(void)
{
    uint32_t reg, mask;

    /* pilot */
    reg = (uint32_t)ssi_read32(SHENKUO_PMU_CMU_CTL_SYS_STATUS_0_REG);
    mask = (reg >> 6) & 0x7; /* reg shift right by 3 bits and 0x7 bits */
    ps_print_info("cpu state=0x%8x, gcpu is %s\n", reg, g_ssi_cpu_st_str[mask]);
    g_ssi_cpu_infos.gcpu_info.cpu_state = mask;
    if (mask == 0x5) {
        shenkuo_ssi_check_buck_scp_ocp_status();
    }
    return (mask == 0x3);
}

int shenkuo_ssi_read_wcpu_pc_lr_sp(void)
{
    int i;
    uint32_t reg_low, reg_high, load, pc_core0, pc_core1;

    /* read pc twice check whether cpu is runing */
    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        // pc锁存 使能位:bit0
        ssi_write32(SHENKUO_GLB_CTL_WCPU0_LOAD_REG, 0x1);
        ssi_write32(SHENKUO_GLB_CTL_WCPU1_LOAD_REG, 0x1);
        oal_mdelay(1);
        load = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_WCPU0_LOAD_REG);

        reg_low = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_WCPU0_PC_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_WCPU0_PC_H_REG);
        pc_core0 = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_WCPU1_PC_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_WCPU1_PC_H_REG);
        pc_core1 = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        // pc锁存 清除使能位:bit1
        ssi_write32(SHENKUO_GLB_CTL_WCPU0_LOAD_REG, 0x2);
        ssi_write32(SHENKUO_GLB_CTL_WCPU1_LOAD_REG, 0x2);
        oal_mdelay(1);

        ps_print_info("gpio-ssi:load_sts:0x%x, read wcpu0[%i], pc:0x%x; wcpu1[%i], pc:0x%x \n",
                      load, i, pc_core0, i, pc_core1);

        if (!pc_core0 && !pc_core1) {
            ps_print_info("wcpu0 & wcpu1 pc all zero\n");
        } else {
            if (g_ssi_cpu_infos.wcpu0_info.reg_flag[i] == 0 && g_ssi_cpu_infos.wcpu1_info.reg_flag[i] == 0) {
                g_ssi_cpu_infos.wcpu0_info.reg_flag[i] = 1;
                g_ssi_cpu_infos.wcpu0_info.pc[i] = pc_core0;
                g_ssi_cpu_infos.wcpu1_info.reg_flag[i] = 1;
                g_ssi_cpu_infos.wcpu1_info.pc[i] = pc_core1;
            }
        }
        oal_mdelay(10); /* delay 10 ms */
    }

    return 0;
}

int shenkuo_ssi_read_bpcu_pc_lr_sp(void)
{
    int i;
    uint32_t reg_low, reg_high, load, pc, lr, sp;

    /* read pc twice check whether cpu is runing */
    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        ssi_write32(SHENKUO_GLB_CTL_BCPU_LOAD_REG, 0x1);
        oal_mdelay(1);
        load = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_BCPU_LOAD_REG);

        reg_low = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_BCPU_PC_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_BCPU_PC_H_REG);
        pc = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_BCPU_LR_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_BCPU_LR_H_REG);
        lr = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_BCPU_SP_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_BCPU_SP_H_REG);
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

static int shenkuo_ssi_read_bpcu_pc_lr_sp_direct(void)
{
    int i;
    uint32_t reg_low, reg_high, load, pc, lr, sp;
    int32_t ret;

    ret = shenkuo_ssi_check_bcpu_is_working();
    if (ret < 0) {
        return ret;
    }

    /* read pc twice check whether cpu is runing */
    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        ssi_write32(SHENKUO_DIRECT_BCPU_LOAD_REG, 0x1);
        oal_mdelay(1);
        load = (uint32_t)ssi_read32(SHENKUO_DIRECT_BCPU_LOAD_REG);

        reg_low = (uint32_t)ssi_read32(SHENKUO_DIRECT_BCPU_PC_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_DIRECT_BCPU_PC_H_REG);
        pc = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(SHENKUO_DIRECT_BCPU_LR_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_DIRECT_BCPU_LR_H_REG);
        lr = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(SHENKUO_DIRECT_BCPU_SP_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_DIRECT_BCPU_SP_H_REG);
        sp = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        ps_print_info("gpio-ssi:direct read bcpu[%i], load_sts:0x%x, pc:0x%x, lr:0x%x, sp:0x%x \n",
                      i, load, pc, lr, sp);
        oal_mdelay(10); /* delay 10 ms */
    }

    return 0;
}

int shenkuo_ssi_read_gpcu_pc_lr_sp(void)
{
    int i;
    uint32_t reg_low, reg_high, load, pc, lr, sp;

    /* read pc twice check whether cpu is runing */
    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        ssi_write32(SHENKUO_GLB_CTL_GCPU_LOAD_REG, 0x1);
        oal_mdelay(1);
        load = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_GCPU_LOAD_REG);

        reg_low = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_GCPU_PC_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_GCPU_PC_H_REG);
        pc = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_GCPU_LR_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_GCPU_LR_H_REG);
        lr = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_GCPU_SP_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_GCPU_SP_H_REG);
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

static int shenkuo_ssi_read_gpcu_pc_lr_sp_direct(void)
{
    int i;
    uint32_t reg_low, reg_high, load, pc, lr, sp;
    int32_t ret;

    ret = shenkuo_ssi_check_gcpu_is_working();
    if (ret < 0) {
        return ret;
    }

    /* read pc twice check whether cpu is runing */
    for (i = 0; i < SSI_CPU_ARM_REG_DUMP_CNT; i++) {
        ssi_write32(SHENKUO_DIRECT_GCPU_LOAD_REG, 0x1);
        oal_mdelay(1);
        load = (uint32_t)ssi_read32(SHENKUO_DIRECT_GCPU_LOAD_REG);

        reg_low = (uint32_t)ssi_read32(SHENKUO_DIRECT_GCPU_PC_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_DIRECT_GCPU_PC_H_REG);
        pc = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(SHENKUO_DIRECT_GCPU_LR_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_DIRECT_GCPU_LR_H_REG);
        lr = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        reg_low = (uint32_t)ssi_read32(SHENKUO_DIRECT_GCPU_SP_L_REG);
        reg_high = (uint32_t)ssi_read32(SHENKUO_DIRECT_GCPU_SP_H_REG);
        sp = reg_low | (reg_high << 16); /* Shift leftwards by 16 bits */

        ps_print_info("gpio-ssi:direct read gcpu[%i], load_sts:0x%x, pc:0x%x, lr:0x%x, sp:0x%x \n",
                      i, load, pc, lr, sp);
        oal_mdelay(10); /* delay 10 ms */
    }

    return 0;
}

int shenkuo_ssi_read_device_arm_register(void)
{
    int32_t ret;

    ret = shenkuo_ssi_check_wcpu_is_working();
    if (ret < 0) {
        return ret;
    }
    if (ret) {
        shenkuo_ssi_read_wcpu_pc_lr_sp();
    }
    bfgx_print_subsys_state();
    ret = shenkuo_ssi_check_bcpu_is_working();
    if (ret < 0) {
        return ret;
    }
    if (ret) {
        shenkuo_ssi_read_bpcu_pc_lr_sp();
    }
    ret = shenkuo_ssi_check_gcpu_is_working();
    if (ret < 0) {
        return ret;
    }
    if (ret) {
        shenkuo_ssi_read_gpcu_pc_lr_sp();
    }

    // 打印device版本
    plat_changid_print();

    return 0;
}

static int shenkuo_ssi_dump_device_aon_regs(unsigned long long module_set)
{
    int ret;
    if (module_set & SSI_MODULE_MASK_AON) {
        ret = ssi_read_reg_info_arry(g_shenkuo_aon_reg_full, sizeof(g_shenkuo_aon_reg_full) / sizeof(ssi_reg_info *),
                                     g_ssi_is_logfile);
        if (ret) {
            return -OAL_EFAIL;
        }
    }

    /* 建链之前不能读 */
    if (module_set & SSI_MODULE_MASK_PCIE_DBI) {
        if (shenkuo_ssi_check_wcpu_is_working()) {
            ret = ssi_read_reg_info(&g_shenkuo_pcie_dbi_full, NULL, 0, g_ssi_is_logfile);
            if (ret) {
                ps_print_info("pcie dbi regs read failed, continue try aon\n");
            }
        } else {
            ps_print_info("pcie dbi regs can't dump, wcpu down\n");
        }
    }

    return OAL_SUCC;
}

static int shenkuo_ssi_dump_device_arm_regs(unsigned long long module_set)
{
    int ret;

    if (module_set & SSI_MODULE_MASK_ARM_REG) {
        ret = shenkuo_ssi_read_device_arm_register();
        if (ret) {
            goto ssi_fail;
        }
    }

    return 0;
ssi_fail:
    return ret;
}

#define DIAG_CTL_GROUP2_SAMPLE_ENABLE_REG 0x40124C80
static int shenkuo_ssi_dump_device_wcpu_key_mem(unsigned long long module_set)
{
    int ret;

    if (module_set & SSI_MODULE_MASK_WCPU_KEY_DTCM) {
        /* 仅debug版本, 通过ssi获取cpu_trace结果 */
        if (is_hi110x_debug_type() != OAL_TRUE) {
            ps_print_info("user mode or maybe beta user,ssi dump bypass\n");
            return 0;
        }

        if (shenkuo_ssi_check_wcpu_is_working()) {
            ssi_write32(DIAG_CTL_GROUP2_SAMPLE_ENABLE_REG, 0x0);
            ret = ssi_read_reg_info(&g_shenkuo_w_key_mem, NULL, 0, g_ssi_is_logfile);
            if (ret) {
                ps_print_info("wcpu key mem read failed, continue try aon\n");
            }
        } else {
            ps_print_info("wcpu key mem can't dump, wcpu down\n");
        }
    }
    return 0;
}

static int32_t shenkuo_check_ssi_spmi_is_ok(void)
{
    uint32_t data = shenkuo_ssi_spmi_read(CHIP_VERSION_2);
    if (data != CHIP_VERSION_2_DEFAULT_VALUE) {
        ps_print_info("ssi_spmi interface is not normal\n");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

/* 检测6506电源异常，NP_EVENT在上电时读取 */
void shenkuo_ssi_dump_power_status_np_event(void)
{
    int32_t ret;
    uint32_t data;

    ps_print_err("6506 NP_EVENT DUMP\n");
    ret = shenkuo_check_ssi_spmi_is_ok();
    if (ret != OAL_SUCC) {
        return;
    }

    data = shenkuo_ssi_spmi_read(SCP0_NP_EVENT_REG);
    ps_print_err("reg0: 0x%x = 0x%x\n", SCP0_NP_EVENT_REG, data);

    data = shenkuo_ssi_spmi_read(OCP0_NP_EVENT_REG);
    ps_print_err("reg1: 0x%x = 0x%x\n", OCP0_NP_EVENT_REG, data);

    data = shenkuo_ssi_spmi_read(OCP1_NP_EVENT_REG);
    ps_print_err("reg2: 0x%x = 0x%x\n", OCP1_NP_EVENT_REG, data);

    data = shenkuo_ssi_spmi_read(IRQ0_NP_EVENT_REG);
    ps_print_err("reg3: 0x%x = 0x%x\n", IRQ0_NP_EVENT_REG, data);

    data = shenkuo_ssi_spmi_read(OCP2_NP_EVENT_REG);
    ps_print_err("reg4: 0x%x = 0x%x\n", OCP2_NP_EVENT_REG, data);
    return;
}

/* 检测6506电源异常, IRQ和IRQ_MASK在异常时读取 */
static void shenkuo_ssi_dump_power_status(void)
{
    int32_t ret;
    uint32_t data, data_mask;

    ps_print_err("6506 IRQ_STS and IRQ_MASK DUMP\n");
    ret = shenkuo_check_ssi_spmi_is_ok();
    if (ret != OAL_SUCC) {
        return;
    }

    data = shenkuo_ssi_spmi_read(SCP0_IRQ_REG);
    data_mask = shenkuo_ssi_spmi_read(SCP0_MASK_REG);
    ps_print_info("reg0: 0x%x = 0x%x reg_mask: 0x%x = 0x%x\n", SCP0_IRQ_REG, data, SCP0_MASK_REG, data_mask);

    data = shenkuo_ssi_spmi_read(OCP0_IRQ_REG);
    data_mask = shenkuo_ssi_spmi_read(OCP0_MASK_REG);
    ps_print_info("reg1: 0x%x = 0x%x reg_mask: 0x%x = 0x%x\n", OCP0_IRQ_REG, data, OCP0_MASK_REG, data_mask);

    data = shenkuo_ssi_spmi_read(OCP1_IRQ_REG);
    data_mask = shenkuo_ssi_spmi_read(OCP1_MASK_REG);
    ps_print_info("reg2: 0x%x = 0x%x reg_mask: 0x%x = 0x%x\n", OCP1_IRQ_REG, data, OCP1_MASK_REG, data_mask);

    data = shenkuo_ssi_spmi_read(IRQ0_IRQ_REG);
    data_mask = shenkuo_ssi_spmi_read(IRQ0_MASK_REG);
    ps_print_info("reg3: 0x%x = 0x%d reg_mask: 0x%x = 0x%x\n", IRQ0_IRQ_REG, data, IRQ0_MASK_REG, data_mask);

    data = shenkuo_ssi_spmi_read(OCP2_IRQ_REG);
    data_mask = shenkuo_ssi_spmi_read(OCP2_MASK_REG);
    ps_print_info("reg4: 0x%x = 0x%x reg_mask: 0x%x = 0x%x\n", OCP2_IRQ_REG, data, OCP2_MASK_REG, data_mask);

    return;
}

static void shenkuo_ssi_trigger_tcxo_detect(uint32_t *tcxo_det_value_target)
{
    if ((*tcxo_det_value_target) == 0) {
        ps_print_err("tcxo_det_value_target is zero, trigger failed\n");
        return;
    }

    /* 刚做过detect,改变det_value+2,观测值是否改变 */
    if ((*tcxo_det_value_target) == ssi_read32(SHENKUO_GLB_CTL_TCXO_32K_DET_CNT_REG)) {
        (*tcxo_det_value_target) = (*tcxo_det_value_target) + 2;
    }

    ssi_write32(SHENKUO_GLB_CTL_TCXO_32K_DET_CNT_REG, (*tcxo_det_value_target)); /* 设置计数周期 */
    ssi_write32(SHENKUO_GLB_CTL_TCXO_DET_CTL_REG, 0x0);                          /* tcxo_det_en disable */

    /* to tcxo */
    ssi_switch_clk(SSI_AON_CLKSEL_TCXO);
    /* delay 150 us */
    oal_udelay(150);
    /* to ssi */
    ssi_switch_clk(SSI_AON_CLKSEL_SSI);

    ssi_write32(SHENKUO_GLB_CTL_TCXO_DET_CTL_REG, 0x1); /* tcxo_det_en enable */

    /* to tcxo */
    ssi_switch_clk(SSI_AON_CLKSEL_TCXO);
    /* delay 31 * 2 * tcxo_det_value_taget us.wait detect done,根据设置的计数周期数 */
    oal_udelay(31 * (*tcxo_det_value_target) * 2);
    /* to ssi */
    ssi_switch_clk(SSI_AON_CLKSEL_SSI);
}

static int shenkuo_ssi_detect_32k_handle(uint32_t sys_tick_old, uint64_t cost_time_s, uint32_t *clock_32k)
{
    uint32_t base_32k_clock = RTC_32K_NOMAL_CKL;
    uint32_t sys_tick_new;

    if (shenkuo_ssi_check_wcpu_is_working()) {
        sys_tick_new = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_SYS_TICK_VALUE_W_0_REG);
    } else {
        sys_tick_new = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_SYS_TICK_VALUE_B_0_REG);
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

static int shenkuo_ssi_detect_tcxo_handle(uint32_t tcxo_det_res_old, uint32_t tcxo_det_value_target, uint32_t clock_32k)
{
    uint64_t base_tcxo_clock = TCXO_NOMAL_CKL;
    uint32_t base_32k_clock = RTC_32K_NOMAL_CKL;
    uint64_t clock_tcxo, div_clock;
    uint64_t tcxo_limit_low, tcxo_limit_high, tcxo_tmp;
    uint32_t tcxo_det_res_new = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_TCXO_32K_DET_RESULT_REG);
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

static int shenkuo_ssi_detect_tcxo_is_normal(void)
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
    tcxo_det_present = (uint32_t)ssi_read32(SHENKUO_PMU_CMU_CTL_CLOCK_TCXO_PRESENCE_DET);
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
    tcxo_det_res_old = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_TCXO_32K_DET_RESULT_REG);
    if (shenkuo_ssi_check_wcpu_is_working()) {
        sys_tick_old = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_SYS_TICK_VALUE_W_0_REG);
        ssi_write32(SHENKUO_GLB_CTL_SYS_TICK_CFG_W_REG, 0x2); /* 清零w systick */
    } else {
        sys_tick_old = (uint32_t)ssi_read32(SHENKUO_GLB_CTL_SYS_TICK_VALUE_B_0_REG);
        ssi_write32(SHENKUO_GLB_CTL_SYS_TICK_CFG_B_REG, 0x2); /* 清零b systick */
    }
    oal_get_time_cost_start(cost);

    /* 使能TCXO时钟精度检测 */
    shenkuo_ssi_trigger_tcxo_detect(&tcxo_det_value_target);
    ret = ssi_read_reg_info_arry(g_shenkuo_tcxo_detect_regs,
                                 sizeof(g_shenkuo_tcxo_detect_regs) / sizeof(ssi_reg_info *), g_ssi_is_logfile);
    if (ret) {
        return ret;
    }
    oal_udelay(1000); /* delay 1000 us,wait 32k count more */
    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);

    /* 32k detect by system clock */
    us_to_s = time_cost_var_sub(cost);
    ret = shenkuo_ssi_detect_32k_handle(sys_tick_old, us_to_s, &clock_32k);
    if (ret) {
        return ret;
    }

    /* tcxo detect by 32k clock */
    return shenkuo_ssi_detect_tcxo_handle(tcxo_det_res_old, tcxo_det_value_target, clock_32k);
}

static void shenkuo_ssi_dump_device_tcxo_regs(unsigned long long module_set)
{
    int ret;
    if (module_set & (SSI_MODULE_MASK_AON | SSI_MODULE_MASK_AON_CUT)) {
        ret = shenkuo_ssi_detect_tcxo_is_normal();
        if (ret) {
            ps_print_info("tcxo detect failed, continue dump\n");
        }
    }
}

static int shenkuo_regs_dump(unsigned long long module_set)
{
    int ret;

    /* try to read pc&lr&sp regs */
    ret = shenkuo_ssi_dump_device_arm_regs(module_set);
    if (ret) {
        return ret;
    }

    ret = ssi_check_device_isalive();
    if (ret) {
        return ret;
    }

    /* try to read all aon regs */
    ret = shenkuo_ssi_dump_device_aon_regs(module_set);
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* try to read pc&lr&sp regs again */
    ret = shenkuo_ssi_dump_device_arm_regs(module_set);
    if (ret) {
        return ret;
    }

    /* detect tcxo clock is normal, trigger */
    shenkuo_ssi_dump_device_tcxo_regs(module_set);

    return OAL_SUCC;
}

int shenkuo_ssi_device_regs_dump(unsigned long long module_set)
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
            shenkuo_ssi_check_buck_scp_ocp_status();
            module_set = SSI_MODULE_MASK_COMM;
        }
    } else {
        shenkuo_ssi_check_buck_scp_ocp_status();
    }

    ret = shenkuo_regs_dump(module_set);
    if (ret != OAL_SUCC) {
        goto ssi_fail;
    }

    /* try to read wcpu key_mem */
    ret = shenkuo_ssi_dump_device_wcpu_key_mem(module_set);
    if (ret != OAL_SUCC) {
        goto ssi_fail;
    }

    shenkuo_ssi_read_bpcu_pc_lr_sp_direct();
    shenkuo_ssi_read_gpcu_pc_lr_sp_direct();

    /* try to read pmu regs */
    shenkuo_ssi_dump_power_status_np_event();
    shenkuo_ssi_dump_power_status();

    ssi_switch_clk(SSI_AON_CLKSEL_TCXO);
    shenkuo_dsm_cpu_info_dump();

    return 0;

ssi_fail:
    ssi_switch_clk(SSI_AON_CLKSEL_TCXO);
    shenkuo_dsm_cpu_info_dump();
    return ret;
}

#endif /* #ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG */
