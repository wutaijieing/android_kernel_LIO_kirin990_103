
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include "pcie_pcs_deye_pattern.h"
#include "pcie_pcs.h"
#include "oal_util.h"
#include "pcie_pcs_regs.h"
#include "pcie_pcs_regs_ver01.h"

#define QUADRANT_1 0x21
#define QUADRANT_2 0x02
#define QUADRANT_3 0x14
#define QUADRANT_4 0x38
#define EYE_PATTERN_ARRY_SIZE 32
#define EYE_PATTERN_QUADRANT_SIZE 16
#define EYE_PATTERN_VALUE 255
#define EYE_PATTERN_INVALID_VALUE 0xFFFFFFFFU

/* 获取完成状态 */
uint32_t pcie_pcs_deye_test_status_polling(pcs_reg_ops *reg_ops, uintptr_t addr, uint32_t pos,
    uint32_t bits, uint32_t check_val)
{
    uint32_t polling_cnt = 0;
    uint32_t read_val = pcie_pcs_get_bits32(reg_ops, addr, pos, bits);

    while (read_val != check_val) {
        pcie_pcs_udelay(1);
        read_val = pcie_pcs_get_bits32(reg_ops, addr, pos, bits);
        polling_cnt++;
        /* wait 20us */
        if (polling_cnt > 20) {
            return read_val;
        }
    }

    return read_val;
}

uint32_t pcie_pcs_deye_pattern_test(pcs_reg_ops *reg_ops, uintptr_t base_addr, uint32_t lane, uint32_t quadrant,
    uint32_t pi, uint32_t vref)
{
    /* 初始化 */
    uint32_t counts;

    /* 关闭硬件FOM开关 */
    pcie_pcs_clr_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x152C), 11);
    pcie_pcs_clr_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x153C), 6);

    /* 设置象限 */
    pcie_pcs_set_bits32(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x153C), 0, 8, quadrant);

    /* 设置横纵坐标偏移 */
    pcie_pcs_set_bits32(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x153C), 12, 4, vref);
    pcie_pcs_set_bits32(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x153C), 8, 4, pi);

    /* 使能 */
    pcie_pcs_set_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x1538), 1);
    pcie_pcs_set_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x1B14), 6);
    pcie_pcs_set_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x1B58), 6);
    pcie_pcs_udelay(1);

    pcie_pcs_set_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x1538), 2);
    pcie_pcs_set_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x152C), 5);
    pcie_pcs_udelay(1);

    pcie_pcs_set_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x1538), 0);
    pcie_pcs_udelay(2);

    /* check status */
    if (pcie_pcs_deye_test_status_polling(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x1000), 18, 1, 0x1) !=
        0x1) {
        pcie_pcs_print("eye_pattern_test_status_polling fail quadrant[%d], pi[%d], vref[%d]\n", quadrant, pi, vref);
        return EYE_PATTERN_INVALID_VALUE;
    }

    /* 返回值 */
    counts = pcie_pcs_get_bits32(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x1528), 8, 8);

    pcie_pcs_set_bits32(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x1538), 0, 3, 0x0);
    pcie_pcs_clr_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x152C), 5);
    pcie_pcs_set_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x1B14), 6);
    pcie_pcs_clr_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x1B58), 6);

    /* 打开硬件FOM开关 */
    pcie_pcs_set_bit(reg_ops, base_addr + PCIE_PCLK_LANE_OFFSET(lane, 0x152C), 11);

    return counts;
}


uint32_t pcie_pcs_deye_cali_debug_read(pcs_reg_ops *reg_ops, uintptr_t base_addr, uint32_t result_sel)
{
    /* 初始化 */
    uint32_t result;

    /* RG_RX_EN_DEBUG */
    pcie_pcs_set_bit(reg_ops, base_addr + 0x1578, 0);

    /* TEST_RX_MODE定义 bit[3:0] H->L bits
     * 0000: SEL_C<4:0>,RX_ATT<2:0>
     * 0001: SEL_R<3:0>,SEL_GAIN<3:0>
     * 0010: EN_CTLE_VCM,EN_DFE,EN_OS_K,EN_GAIN_K,GEN,FORM_EN,EN_RX_LOS,LB_EN
     * 0011: RX_LOS,RX_OUT,EN_TERM,RG_TERM_R<4:0>
     * 0100: EQ_OS,E0B_OS_CAL,D1B_OS_CAL,E2B_OS_CAL,D3B_OS_CAL,GAIN_VTH_D1_DIG,GAIN_VTH_D3_DIG,GAIN_VREF_VTH
     * 0101: EN_OS_K,EQ_OS,OS_SWQP,RG_CTLE_OS<4:0>
     * 0110: EN_OS_K,OS_SEL,E0B_OS_CAL,E0_OS<4:0>
     * 0111: EN_GAIN_K,GS_CAL_EN,GAIN_VTH_D1_DIG,GS0_OS<4:0>
     * 1000: VREF_DFE<3:0>,DFE_TAP1<3:0>
     * 1001: DFE_CAL_EN,EN_DFE_K,HW_SW_EN_DFE_K,DFE_EN_DIG,DFE_EN,RG_RX_EN_DFE_K,RG_RX_DFE_TAP_OVRT,RG_RX_DFE_VREF_OVRT
     * 1010: DFE_VREF_DIG<3:0>,REG_DFE_VREF<3:0>
     * 1011: DFE_TP_DIG<3:0>,DFE_TP<3:0>
     * 1100: TIEL<3>*8
     * 1101: TIEL<2>*8
     * 1110: TIEL<1>*8
     * 1111: TIEL<0>*8
     */
    pcie_pcs_set_bits32(reg_ops, base_addr + 0x1544, 19, 4, result_sel);

    /* pmac_dbg_muxsel */
    pcie_pcs_set_bits32(reg_ops, base_addr + 0xAB0, 0, 9, 0xA0);
    (void)pcie_pcs_get_bits32(reg_ops, base_addr + 0xAB0, 0, 9);

    /* pmac_dbg_muxout */
    result = pcie_pcs_get_bits32(reg_ops, base_addr + 0xAB0, 16, 8);
    return result;
}

int32_t pcie_pcs_deye_cali_dfe_result(pcs_reg_ops *reg_ops, uintptr_t base_addr, uint8_t *buff, int32_t size)
{
    uint32_t value;
    int32_t ret = 0;
    int32_t offset = 0;
    uint32_t tp_dig, dfe_tp, dfe_tp1;
    uint32_t dfe_vref_dig, dfe_vref;

    value = pcie_pcs_deye_cali_debug_read(reg_ops, base_addr, 0xB);
    tp_dig = pcie_pcs_get_bits_from_value(value, 4, 4);
    dfe_tp = pcie_pcs_get_bits_from_value(value, 0, 4);

    value = pcie_pcs_deye_cali_debug_read(reg_ops, base_addr, 0x8);
    dfe_tp1 = pcie_pcs_get_bits_from_value(value, 0, 4);

    value = pcie_pcs_deye_cali_debug_read(reg_ops, base_addr, 0xD);
    dfe_vref_dig = pcie_pcs_get_bits_from_value(value, 0, 6);

    value = pcie_pcs_deye_cali_debug_read(reg_ops, base_addr, 0xC);
    dfe_vref = pcie_pcs_get_bits_from_value(value, 0, 6);

    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1,
        "********  PCIe idx:0x%x dfe cali result show  ********" NEWLINE, (uint32_t)base_addr);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "dfe_tp = 0x%x" NEWLINE, dfe_tp);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "tp_dig = 0x%x" NEWLINE, tp_dig);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "dfe_tp1 = 0x%x" NEWLINE, dfe_tp1);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "dfe_vref_dig = 0x%x" NEWLINE,
        dfe_vref_dig);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "dfe_vref = 0x%x" NEWLINE, dfe_vref);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    return offset;
}

int32_t pcie_pcs_deye_cali_ctle_result(pcs_reg_ops *reg_ops, uintptr_t base_addr, uint8_t *buff, int32_t size)
{
    uint32_t value;
    int32_t ret = 0;
    int32_t offset = 0;
    uint32_t sel_c, rx_addr, sel_r, sel_gain, gen;
    uint32_t ctle_os, e0_os, gs_os;

    value = pcie_pcs_deye_cali_debug_read(reg_ops, base_addr, 0x0);
    sel_c = pcie_pcs_get_bits_from_value(value, 3, 5);
    rx_addr = pcie_pcs_get_bits_from_value(value, 0, 3);

    value = pcie_pcs_deye_cali_debug_read(reg_ops, base_addr, 0x1);
    sel_r = pcie_pcs_get_bits_from_value(value, 4, 4);
    sel_gain = pcie_pcs_get_bits_from_value(value, 0, 4);

    value = pcie_pcs_deye_cali_debug_read(reg_ops, base_addr, 0x2);
    gen = pcie_pcs_get_bits_from_value(value, 3, 1);

    value = pcie_pcs_deye_cali_debug_read(reg_ops, base_addr, 0x5);
    ctle_os = pcie_pcs_get_bits_from_value(value, 0, 5);

    value = pcie_pcs_deye_cali_debug_read(reg_ops, base_addr, 0x6);
    e0_os = pcie_pcs_get_bits_from_value(value, 0, 5);

    value = pcie_pcs_deye_cali_debug_read(reg_ops, base_addr, 0x7);
    gs_os = pcie_pcs_get_bits_from_value(value, 0, 5);

    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1,
        "********  PCIe idx:0x%x ctle eq cali result show  ********" NEWLINE, base_addr);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "sel_c = 0x%x" NEWLINE, sel_c);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "rx_att = 0x%x" NEWLINE, rx_addr);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "sel_r = 0x%x" NEWLINE, sel_r);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "sel_gain = 0x%x" NEWLINE, sel_gain);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "gen = 0x%x" NEWLINE, gen);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "ctle_os = 0x%x" NEWLINE, ctle_os);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "e0_os = 0x%x" NEWLINE, e0_os);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "gs_os = 0x%x" NEWLINE, gs_os);
    offset += ret;
    if (ret <= 0) {
        return offset;
    }
    return offset;
}

static uint32_t g_deye_pattern_result[EYE_PATTERN_ARRY_SIZE][EYE_PATTERN_ARRY_SIZE];
int32_t pcie_pcs_deye_pattern_draw(pcs_reg_ops *reg_ops, uintptr_t base_addr, uint32_t lane, uint8_t *buff,
    int32_t size)
{
    uint32_t pi, vref;
    int i, j, k;

    int32_t ret;
    int32_t offset = 0;

    (void)memset_s((void*)g_deye_pattern_result, sizeof(g_deye_pattern_result), 0, sizeof(g_deye_pattern_result));

    /*
     * 电子眼图4个象限
     * 每个象限16*16, 整个map是32*32
     * 纵坐标是vref电压, 横坐标是pi时间
     * 每次进入FOM模式, 读取一个值，代表某时间点的电压
     */
    for (vref = 0; vref < EYE_PATTERN_QUADRANT_SIZE; vref++) {
        for (pi = 0; pi < EYE_PATTERN_QUADRANT_SIZE; pi++) {
            /* 从中心向四周扩展 */
            g_deye_pattern_result[15 - vref][15 - pi] =
                pcie_pcs_deye_pattern_test(reg_ops, base_addr, lane, QUADRANT_1, pi, vref);
            g_deye_pattern_result[16 + vref][15 - pi] =
                pcie_pcs_deye_pattern_test(reg_ops, base_addr, lane, QUADRANT_2, pi, vref);
            g_deye_pattern_result[16 + vref][16 + pi] =
                pcie_pcs_deye_pattern_test(reg_ops, base_addr, lane, QUADRANT_3, pi, vref);
            g_deye_pattern_result[15 - vref][16 + pi] =
                pcie_pcs_deye_pattern_test(reg_ops, base_addr, lane, QUADRANT_4, pi, vref);
        }
    }

    /* 保证非pcie aspm状态，且dbg_uart打印正常 */
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1,
        "********  PCIe idx:0x%x lane:%d Eye Pattern  ********" NEWLINE, (uint32_t)base_addr, lane);
    if (ret <= 0) {
        return -OAL_ENOMEM;
    }
    offset += ret;

    /* 打印横坐标 */
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1,
        "  F E D C B A 9 8 7 6 5 4 3 2 1 0 0 1 2 3 4 5 6 7 8 9 A B C D E F" NEWLINE);
    if (ret <= 0) {
        return -OAL_ENOMEM;
    }
    offset += ret;

    for (i = 0; i < EYE_PATTERN_ARRY_SIZE; i++) {
        /* 打印纵坐标 */
        k = 0xF - i;
        if (k >= 0) {
        } else {
            k = -k - 1;
        }
        ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "%x ", k);
        if (ret <= 0) {
            return -OAL_ENOMEM;
        }
        offset += ret;

        /* 打印电压值 */
        for (j = 0; j < EYE_PATTERN_ARRY_SIZE; j++) {
            if (g_deye_pattern_result[i][j] > 0 && g_deye_pattern_result[i][j] < EYE_PATTERN_VALUE) {
                ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "+ "); // 眼皮
            } else if (g_deye_pattern_result[i][j] == 0) {
                ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "- "); // 眼外
            } else if (g_deye_pattern_result[i][j] == EYE_PATTERN_VALUE) {
                ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "  "); // 眼内
            } else if (g_deye_pattern_result[i][j] == EYE_PATTERN_INVALID_VALUE) {
                ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "@"); // 超时标记
            } else {
                ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, "#"); // 非法值
            }
            if (ret <= 0) {
                return -OAL_ENOMEM;
            }
            offset += ret;
        }
        ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, NEWLINE);
        if (ret <= 0) {
            return -OAL_ENOMEM;
        }
        offset += ret;
    }
    ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1, NEWLINE);
    if (ret <= 0) {
        return -OAL_ENOMEM;
    }
    offset += ret;

    /* 打印电子眼图 rx校准结果 */
    ret = pcie_pcs_deye_cali_ctle_result(reg_ops, base_addr, buff + offset, size - offset);
    if (ret <= 0) {
        return -OAL_ENOMEM;
    }
    offset += ret;

    ret = pcie_pcs_deye_cali_dfe_result(reg_ops, base_addr, buff + offset, size - offset);
    if (ret <= 0) {
        return -OAL_ENOMEM;
    }
    offset += ret;

    return OAL_SUCC;
}

#endif