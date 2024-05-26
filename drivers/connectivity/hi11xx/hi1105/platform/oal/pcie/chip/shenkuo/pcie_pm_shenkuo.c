

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#define HI11XX_LOG_MODULE_NAME "[PCIE_H]"
#define HISI_LOG_TAG           "[PCIE]"
#include "pcie_host.h"
#include "pcie_linux.h"
#include "shenkuo/pcie_ctrl_rb_regs.h"
#include "shenkuo/pcie_pcs_rb_regs.h"
#include "oam_ext_if.h"
#include "pcie_pm.h"
#include "pcie_pcs_trace.h"
#include "pcie_pcs_host.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_PCIE_PM_HI1106_C

/* mac优化寄存器 */
#define PCI_EXP_LNKCTL2_SPEED_MASK 0xf
#define PCI_EXP_SPEED_CHANGE_MASK  0x20000
#define LINK_RATE_MASK      0x300000
#define LTSSM_STATE_MASK    0x7e00

int32_t oal_pcie_device_auxclk_init_hi1103(oal_pcie_res *pst_pci_res);
int32_t oal_pcie_device_aspm_init_hi1103(oal_pcie_res *pst_pci_res);
void oal_pcie_device_aspm_ctrl_hi1103(oal_pcie_res *pst_pci_res, oal_bool_enum clear);

static uintptr_t oal_pcie_get_pcs_paddr(oal_pci_dev_stru *pst_pci_dev, oal_pcie_res *pst_pci_res, uint32_t dump_type)
{
    int32_t ret;
    pci_addr_map st_map;
    uint32_t phy_paddr = (oal_pcie_is_master_ip(pst_pci_dev) == OAL_TRUE) ? SHENKUO_PA_PCIE0_PHY_BASE_ADDRESS :
            SHENKUO_PA_PCIE1_PHY_BASE_ADDRESS;
    if (dump_type == PCIE_PCS_DUMP_BY_SSI) {
        return (uintptr_t)phy_paddr;
    }
    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, phy_paddr, &st_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "get dev address 0x%x failed", phy_paddr);
        return (uintptr_t)NULL;
    }
    /* memport virtual address */
    return (uintptr_t)st_map.va;
}

#define SHENKUO_PA_PCIE1_CTRL_BASE_ADDR      0x4010E000 /* 对应PCIE_CTRL页 */
static void* oal_pcie_get_ctrl_address(oal_pci_dev_stru *pst_pci_dev, oal_pcie_res *pst_pci_res)
{
    if (oal_pcie_is_master_ip(pst_pci_dev) == OAL_TRUE) {
        /* ep0 */
        return pst_pci_res->pst_pci_ctrl_base;
    } else {
        /* ep1 */
        int32_t ret;
        pci_addr_map st_map;
        ret = oal_pcie_inbound_ca_to_va(pst_pci_res, SHENKUO_PA_PCIE1_CTRL_BASE_ADDR, &st_map);
        if (ret != OAL_SUCC) {
            pci_print_log(PCI_LOG_ERR, "get dev address 0x%x failed", SHENKUO_PA_PCIE1_CTRL_BASE_ADDR);
            return NULL;
        }
        pst_pci_res->pst_pci_ctrl2_base = (void *)st_map.va;
        return pst_pci_res->pst_pci_ctrl2_base;
    }
}

#define LINK_SPEED_CHANGE_MAX_ROUND 2
void oal_pcie_link_speed_change(oal_pci_dev_stru *pst_pci_dev, oal_pcie_res *pst_pci_res)
{
    void *pcie_ctrl = NULL;
    oal_pci_dev_stru *pst_rc_dev = NULL;
    uint32_t val = 0;
    int32_t ep_l1ss_pm = 0;
    uint8_t speed_change_round = 1;
    uint8_t speed_change_succ = 0;
    uint32_t link_speed, link_sts;

    pcie_ctrl = oal_pcie_get_ctrl_address(pst_pci_dev, pst_pci_res);
    if (pcie_ctrl == NULL) {
        pci_print_log(PCI_LOG_ERR, "pcie_ctrl virtual address is null");
        return;
    }

    while ((speed_change_succ == 0) && (speed_change_round <= LINK_SPEED_CHANGE_MAX_ROUND)) {
        pci_print_log(PCI_LOG_INFO, "pcie link speed change, round:%d", speed_change_round);
        speed_change_round++;

        /* Step1: 如果目标速率等于当前速率bit[21:20], 不切速 */
        link_speed = oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STATUS2_OFF);
        if ((g_pcie_speed_change == PCIE_GEN1) || (g_pcie_speed_change == ((link_speed & LINK_RATE_MASK) >> 20))) {
            pci_print_log(PCI_LOG_WARN, "pcie link speed change bypass, target speed is GEN1 or same to current speed");
            continue;
        }

        /* Step2: 如果链路状态bit[14:9]非L0, 不切速 */
        link_sts = oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STATUS0_OFF);
        if (((link_sts & LTSSM_STATE_MASK) >> 9) != 0x11) {
            pci_print_log(PCI_LOG_ERR, "pcie link speed change bypass, link stats is not L0");
            continue;
        }

        /* Step3: 链路状态机数采维测 */
        pci_print_log(PCI_LOG_INFO,
                      "before: non_l1_rcvry_cnt:0x%x l1_rcvry_cnt:0x%x stat_monitor1:0x%x stat_monitor2:0x%x",
                      oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STAT_CNT_RCVRY_OFF),
                      oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STAT_CNT_L1_RCVRY_OFF),
                      oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STAT_MONITOR1_OFF),
                      oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STAT_MONITOR2_OFF));
        oal_writel(0xFFFFFFFF, (pcie_ctrl + PCIE_CTRL_RB_PCIE_STAT_MONITOR2_OFF));

        if (pcs_trace_speedchange_flag() != 0) {
            pcie_pcs_tracer_start(oal_pcie_get_pcs_paddr(pst_pci_dev, pst_pci_res, PCIE_PCS_DUMP_BY_MEMPORT),
                                  PCS_TRACER_SPEED_CHANGE, 0, PCIE_PCS_DUMP_BY_MEMPORT);
        }

        /* Step4: 配置rc侧mac寄存器 */
        pst_rc_dev = pci_upstream_bridge(pst_pci_dev);
        if (pst_rc_dev != NULL) {
            ep_l1ss_pm = oal_pci_pcie_cap(pst_rc_dev);
            if (!ep_l1ss_pm) {
                pci_print_log(PCI_LOG_ERR, "pcie link speed change fail, cannot get rc addr");
                continue;
            }

            /* debug mac reg before speed-change,bit[19:16]是软件切换后的链路速率 */
            oal_pci_read_config_dword(pst_rc_dev, ep_l1ss_pm + PCI_EXP_LNKCTL, &val);
            pci_print_log(PCI_LOG_INFO, "before: rc mac addr:0x%x PCI_EXP_LNKCTL:0x%x", PCI_EXP_LNKCTL, val);

            /* 配置TARGET_LINK_SPEED,bit[3:0]是软件切换配置的目标速率,bit[20:17]对应EQ状态 */
            oal_pci_read_config_dword(pst_rc_dev, ep_l1ss_pm + PCI_EXP_LNKCTL2, &val);
            pci_print_log(PCI_LOG_INFO, "before: rc mac addr:0x%x PCI_EXP_LNKCTL2:0x%x", PCI_EXP_LNKCTL2, val);
            val &= ~PCI_EXP_LNKCTL2_SPEED_MASK;
            val |= (uint32_t)(g_pcie_speed_change + 1);
            oal_pci_write_config_dword(pst_rc_dev, ep_l1ss_pm + PCI_EXP_LNKCTL2, val);

            /* 配置DIRECT_SPEED_CHANGE,bit[17]直接切速; 不用配置ep进行RETRAIN */
            oal_pci_read_config_dword(pst_rc_dev, ep_l1ss_pm + PCI_GEN2_CTRL, &val);
            pci_print_log(PCI_LOG_INFO, "before: rc mac addr:0x%x PCI_GEN2_CTRL:0x%x", PCI_GEN2_CTRL, val);
            val &= ~PCI_EXP_SPEED_CHANGE_MASK;
            val |= PCI_EXP_SPEED_CHANGE_MASK;
            oal_pci_write_config_dword(pst_rc_dev, ep_l1ss_pm + PCI_GEN2_CTRL, val);
        }

        /* Step5: 固定delay 1ms, EDA仿真不超过300us;并确认链路速度,bit[21:20] */
        msleep(1);
        link_speed = oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STATUS2_OFF);
        link_sts = oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STATUS0_OFF);
        if (((link_speed & LINK_RATE_MASK) >> 20) != g_pcie_speed_change) {
            oam_error_log1(0, OAM_SF_PWR, "speed change fail to %d[0:GEN1 1:GEN2 2:GEN3]", g_pcie_speed_change);
            pci_print_log(PCI_LOG_ERR, "speed change fail to %d[0:GEN1 1:GEN2 2:GEN3]", g_pcie_speed_change);
            declare_dft_trace_key_info("pcie_speed_change_fail", OAL_DFT_TRACE_FAIL);
            if (pcs_trace_speedchange_flag() & 0x1) {
                pcie_pcs_tracer_dump(oal_pcie_get_pcs_paddr(pst_pci_dev, pst_pci_res, PCIE_PCS_DUMP_BY_SSI),
                                     0, PCIE_PCS_DUMP_BY_SSI);
            }
        } else {
            speed_change_succ = 1;
            oam_warning_log1(0, OAM_SF_PWR, "speed change success to %d[0:GEN1 1:GEN2 2:GEN3]", g_pcie_speed_change);
            pci_print_log(PCI_LOG_INFO, "speed change success to %d[0:GEN1 1:GEN2 2:GEN3]", g_pcie_speed_change);
            declare_dft_trace_key_info("pcie_speed_change_succ", OAL_DFT_TRACE_SUCC);
            if (pcs_trace_speedchange_flag() & 0x2) {
                pcie_pcs_tracer_dump(oal_pcie_get_pcs_paddr(pst_pci_dev, pst_pci_res, PCIE_PCS_DUMP_BY_SSI),
                                     0, PCIE_PCS_DUMP_BY_SSI);
            }
        }

        /* Step6: 链路状态机数采维测，预期non_l1_rcvry_cnt=0x3, l1_rcvry_cnt=0x0 */
        pci_print_log(PCI_LOG_INFO,
                      "after: non_l1_rcvry_cnt:0x%x l1_rcvry_cnt:0x%x stat_monitor1:0x%x stat_monitor2:0x%x",
                      oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STAT_CNT_RCVRY_OFF),
                      oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STAT_CNT_L1_RCVRY_OFF),
                      oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STAT_MONITOR1_OFF),
                      oal_readl(pcie_ctrl + PCIE_CTRL_RB_PCIE_STAT_MONITOR2_OFF));
        /* debug mac reg after speed-change */
        oal_pci_read_config_dword(pst_rc_dev, ep_l1ss_pm + PCI_EXP_LNKCTL, &val);
        pci_print_log(PCI_LOG_INFO, "after: rc mac addr:0x%x PCI_EXP_LNKCTL:0x%x", PCI_EXP_LNKCTL, val);
        oal_pci_read_config_dword(pst_rc_dev, ep_l1ss_pm + PCI_EXP_LNKCTL2, &val);
        pci_print_log(PCI_LOG_INFO, "after: rc mac addr:0x%x PCI_EXP_LNKCTL2:0x%x", PCI_EXP_LNKCTL2, val);
        oal_pci_read_config_dword(pst_rc_dev, ep_l1ss_pm + PCI_GEN2_CTRL, &val);
        pci_print_log(PCI_LOG_INFO, "after: rc mac addr:0x%x PCI_GEN2_CTRL:0x%x", PCI_GEN2_CTRL, val);
        pci_print_log(PCI_LOG_INFO, "speed change result: link_sts = 0x%x, link_speed = 0x%x", link_sts, link_speed);
    }
}

int32_t oal_pcie_device_phy_config_shenkuo(oal_pcie_res *pst_pci_res)
{
    oal_pcie_linux_res *pst_pci_lres = (oal_pcie_linux_res *)oal_pci_get_drvdata(pcie_res_to_dev(pst_pci_res));

    /* pcie0 phy寄存器优化 */
    oal_pcie_device_phy_config_single_lane(pst_pci_res, SHENKUO_PA_PCIE0_PHY_BASE_ADDRESS);

    /* ASPM L1sub power_on_time bias */
    oal_pcie_config_l1ss_poweron_time(pst_pci_lres->pst_pcie_dev);

    /* GEN1->GENx link speed change */
    (void)oal_pcie_link_speed_change(pst_pci_lres->pst_pcie_dev, pst_pci_res);

    oal_pcie_device_phy_disable_l1ss_rekey(pst_pci_res, SHENKUO_PA_PCIE0_PHY_BASE_ADDRESS);

#ifdef _PRE_PLAT_FEATURE_HI110X_DUAL_PCIE
    /* dual pcie */
    if (pst_pci_lres->pst_pcie_dev_dual != NULL) {
        /* pcie1 phy寄存器优化 */
        oal_pcie_device_phy_config_single_lane(pst_pci_res, SHENKUO_PA_PCIE1_PHY_BASE_ADDRESS);

        /* ASPM L1sub power_on_time bias */
        oal_pcie_config_l1ss_poweron_time(pst_pci_lres->pst_pcie_dev_dual);

        /* GEN1->GENx link speed change */
        (void)oal_pcie_link_speed_change(pst_pci_lres->pst_pcie_dev_dual, pst_pci_res);

        oal_pcie_device_phy_disable_l1ss_rekey(pst_pci_res, SHENKUO_PA_PCIE1_PHY_BASE_ADDRESS);
    }
#endif

    return OAL_SUCC;
}

int32_t pcie_pm_chip_init_shenkuo(oal_pcie_res *pst_pci_res, int32_t device_id)
{
    pst_pci_res->chip_info.cb.pm_cb.pcie_device_aspm_init = oal_pcie_device_aspm_init_hi1103;
    pst_pci_res->chip_info.cb.pm_cb.pcie_device_auxclk_init = oal_pcie_device_auxclk_init_hi1103;
    pst_pci_res->chip_info.cb.pm_cb.pcie_device_aspm_ctrl = oal_pcie_device_aspm_ctrl_hi1103;
    pst_pci_res->chip_info.cb.pm_cb.pcie_device_phy_config = oal_pcie_device_phy_config_shenkuo;
    return OAL_SUCC;
}
#endif

