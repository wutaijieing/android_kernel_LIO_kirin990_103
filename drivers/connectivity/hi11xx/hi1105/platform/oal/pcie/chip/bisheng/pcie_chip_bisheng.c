

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#define HI11XX_LOG_MODULE_NAME "[PCIEC]"
#define HISI_LOG_TAG           "[PCIEC]"

#include "bisheng/host_ctrl_rb_regs.h"
#include "bisheng/pcie_ctrl_rb_regs.h"

#include "chip/bisheng/pcie_soc_bisheng.h"
#include "pcie_chip.h"

/* bisheng Registers */
#define BISHENG_PA_PCIE_CTRL_BASE_ADDR      PCIE_CTRL_RB_BASE /* 对应PCIE_CTRL页 */
#define BISHENG_PA_PCIE_DBI_BASE_ADDRESS    0x40107000
#define BISHENG_PA_ETE_CTRL_BASE_ADDRESS    HOST_CTRL_RB_BASE
#define BISHENG_PA_GLB_CTL_BASE_ADDR        0x40000000
#define BISHENG_PA_PMU_CMU_CTL_BASE         0x40002000
#define BISHENG_PA_PMU2_CMU_IR_BASE         0x40012000
#define BISHENG_PA_W_CTL_BASE               0x40105000
#define BISHENG_PWR_ON_LABLE_REG            0X40000200
#define DEADBEAF_RESET_VAL                 ((uint16_t)0x5757)
#define BISHENG_DEV_VERSION_CPU_ADDR 0x00000180
int32_t oal_pcie_get_bar_region_info_bisheng(oal_pcie_res *pst_pci_res, oal_pcie_region **, uint32_t *);
int32_t oal_pcie_set_outbound_membar_bisheng(oal_pcie_res *pst_pci_res, oal_pcie_iatu_bar* pst_iatu_bar);

static uintptr_t oal_pcie_get_test_ram_address(void)
{
    return 0x2000000; // pkt mem for test, by acp port
}

static int32_t oal_pcie_voltage_bias_init_bisheng(oal_pcie_res *pst_pci_res)
{
    oal_print_hi11xx_log(HI11XX_LOG_ERR, "oal_pcie_chip_info_init_bisheng is not implement!");
    return OAL_SUCC;
}

static int32_t oal_pcie_host_slave_address_switch_bisheng(oal_pcie_res *pst_pci_res, uint64_t src_addr,
                                                          uint64_t* dst_addr, int32_t is_host_iova)
{
    if (is_host_iova == OAL_TRUE) {
        if (oal_likely((src_addr < (HISI_PCIE_MASTER_END_ADDRESS)))) {
            *dst_addr = src_addr + HISI_PCIE_IP_REGION_OFFSET;
            pci_print_log(PCI_LOG_DBG, "pcie_if_hostca_to_devva ok, hostca=0x%llx\n", *dst_addr);
            return OAL_SUCC;
        }
    } else {
        if (oal_likely((((src_addr >= HISI_PCIE_SLAVE_START_ADDRESS)
                       && (src_addr < (HISI_PCIE_SLAVE_END_ADDRESS)))))) {
            *dst_addr = src_addr - HISI_PCIE_IP_REGION_OFFSET;
            pci_print_log(PCI_LOG_DBG, "pcie_if_devva_to_hostca ok, devva=0x%llx\n", *dst_addr);
            return OAL_SUCC;
        }
    }

    pci_print_log(PCI_LOG_ERR, "pcie_slave_address_switch %s failed, src_addr=0x%llx\n",
                  (is_host_iova == OAL_TRUE) ? "iova->slave" : "slave->iova", src_addr);
    return -OAL_EFAIL;
}

/* device intx/msi init */
static int32_t oal_ete_intr_init(oal_pcie_res *pst_pci_res)
{
    hreg_host_intr_mask intr_mask;
#ifdef _PRE_COMMENT_CODE_
    hreg_pcie_ctrl_ete_ch_dr_empty_intr_mask host_dre_mask;
#endif

    /* enable interrupts */
    if (pst_pci_res->pst_pci_ctrl_base == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pst_pci_ctrl_base is null");
        return -OAL_ENODEV;
    }

    hreg_get_val(intr_mask, pst_pci_res->pst_pci_ctrl_base + PCIE_CTRL_RB_HOST_INTR_MASK_OFF);

    intr_mask.bits.device2host_tx_intr_mask = 0;
    intr_mask.bits.device2host_rx_intr_mask = 0;
    intr_mask.bits.device2host_intr_mask = 0;

    /* WiFi中断需要在WiFi回调注册后Unmask */
    intr_mask.bits.mac_n1_intr_mask = 0;
    intr_mask.bits.mac_n2_intr_mask = 0;
    intr_mask.bits.phy_n1_intr_mask = 0;
    intr_mask.bits.phy_n2_intr_mask = 0;

    hreg_set_val(intr_mask, pst_pci_res->pst_pci_ctrl_base + PCIE_CTRL_RB_HOST_INTR_MASK_OFF);
    hreg_get_val(intr_mask, pst_pci_res->pst_pci_ctrl_base + PCIE_CTRL_RB_HOST_INTR_MASK_OFF);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie intr mask=0x%x", intr_mask.as_dword);

    /* mask:1 for mask, 0 for unmask */
#ifdef _PRE_COMMENT_CODE_
    hreg_get_val(host_dre_mask, pst_pci_res->pst_pci_ctrl_base + PCIE_CTRL_RB_PCIE_CTRL_ETE_CH_DR_EMPTY_INTR_MASK_OFF);
    host_dre_mask.bits.host_rx_ch_dr_empty_intr_mask = 0; /* unmask rx dre int */
    hreg_set_val(host_dre_mask,  pst_pci_res->pst_pci_ctrl_base +
                 PCIE_CTRL_RB_PCIE_CTRL_ETE_CH_DR_EMPTY_INTR_MASK_OFF);
#endif

    /* unmask all ete host int */
    oal_writel(0x0, pst_pci_res->ete_info.reg.ete_intr_mask_addr);

    return OAL_SUCC;
}

static void oal_ete_res_init(oal_pcie_res *pst_pci_res)
{
    oal_ete_reg *reg = &pst_pci_res->ete_info.reg;

    reg->host_intr_sts_addr = pst_pci_res->pst_pci_ctrl_base +
                              PCIE_CTRL_RB_HOST_INTR_STATUS_OFF;
    reg->host_intr_clr_addr = pst_pci_res->pst_pci_ctrl_base +
                              PCIE_CTRL_RB_HOST_INTR_CLR_OFF;
    reg->ete_intr_sts_addr = pst_pci_res->pst_ete_base +
                             HOST_CTRL_RB_PCIE_CTL_ETE_INTR_STS_OFF;
    reg->ete_intr_clr_addr = pst_pci_res->pst_ete_base +
                             HOST_CTRL_RB_PCIE_CTL_ETE_INTR_CLR_OFF;
    reg->ete_intr_mask_addr = pst_pci_res->pst_ete_base +
                              HOST_CTRL_RB_PCIE_CTL_ETE_INTR_MASK_OFF;
    reg->ete_dr_empty_sts_addr = pst_pci_res->pst_ete_base +
                                     HOST_CTRL_RB_PCIE_CTRL_ETE_CH_DR_EMPTY_INTR_STS_OFF;
    reg->ete_dr_empty_clr_addr = pst_pci_res->pst_ete_base +
                                 HOST_CTRL_RB_PCIE_CTRL_ETE_CH_DR_EMPTY_INTR_CLR_OFF;
    reg->h2d_intr_addr = pst_pci_res->pst_pci_ctrl_base + PCIE_CTRL_RB_HOST2DEVICE_INTR_SET_OFF;
    reg->host_intr_mask_addr = pst_pci_res->pst_pci_ctrl_base + PCIE_CTRL_RB_HOST_INTR_MASK_OFF;
}

static int32_t oal_pcie_poweroff_complete(oal_pcie_res *pst_pci_res)
{
    pci_addr_map st_map; /* DEVICE power_label地址 */
    uint16_t value;
    int32_t ret;

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, BISHENG_PWR_ON_LABLE_REG, &st_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "invalid device address 0x%x, map failed\n", BISHENG_PWR_ON_LABLE_REG);
        return -OAL_ENOMEM;
    }
    value = oal_pcie_read_mem16(st_map.va);
    pci_print_log(PCI_LOG_WARN, "power_label 0x%x = 0x%x", BISHENG_PWR_ON_LABLE_REG, value);
    if (value == DEADBEAF_RESET_VAL) {
        return OAL_SUCC;
    } else if (value == 0xffff) {
        // check pcie link state
        if (oal_pcie_check_link_state(pst_pci_res) == OAL_FALSE) {
            pci_print_log(PCI_LOG_ERR, "pcie_detect_linkdown");
            return -OAL_ENODEV;
        }

        // retry & check device power_on_label
        value = oal_pcie_read_mem16(st_map.va);
        pci_print_log(PCI_LOG_WARN, "[retry]power_label 0x%x = 0x%x", BISHENG_PWR_ON_LABLE_REG, value);
        return -OAL_ENODEV;
    }
    return -OAL_EFAIL;
}

static void oal_pcie_chip_info_cb_init(pcie_chip_cb *cb, int32_t device_id)
{
    cb->get_test_ram_address = oal_pcie_get_test_ram_address;
    cb->pcie_voltage_bias_init = oal_pcie_voltage_bias_init_bisheng;
    cb->pcie_get_bar_region_info = oal_pcie_get_bar_region_info_bisheng;
    cb->pcie_set_outbound_membar = oal_pcie_set_outbound_membar_bisheng;
    cb->pcie_host_slave_address_switch = oal_pcie_host_slave_address_switch_bisheng;
    cb->pcie_poweroff_complete = oal_pcie_poweroff_complete;
    cb->ete_address_init = oal_ete_res_init;
    cb->ete_intr_init = oal_ete_intr_init;
}

int32_t oal_pcie_chip_info_init_bisheng(oal_pcie_res *pst_pci_res, int32_t device_id)
{
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "oal_pcie_chip_info_init_bisheng");
    pst_pci_res->chip_info.dual_pci_support = OAL_FALSE;
    pst_pci_res->chip_info.ete_support = OAL_TRUE;
    pst_pci_res->chip_info.membar_support = OAL_TRUE;
    pst_pci_res->chip_info.addr_info.pcie_ctrl = BISHENG_PA_PCIE_CTRL_BASE_ADDR;
    pst_pci_res->chip_info.addr_info.dbi = BISHENG_PA_PCIE_DBI_BASE_ADDRESS;
    pst_pci_res->chip_info.addr_info.ete_ctrl = BISHENG_PA_ETE_CTRL_BASE_ADDRESS;
    pst_pci_res->chip_info.addr_info.glb_ctrl = BISHENG_PA_GLB_CTL_BASE_ADDR;
    pst_pci_res->chip_info.addr_info.pmu_ctrl = BISHENG_PA_PMU_CMU_CTL_BASE;
    pst_pci_res->chip_info.addr_info.pmu2_ctrl = BISHENG_PA_PMU2_CMU_IR_BASE;
    pst_pci_res->chip_info.addr_info.boot_version = BISHENG_DEV_VERSION_CPU_ADDR;
    pst_pci_res->chip_info.addr_info.sharemem_addr = PCIE_CTRL_RB_HOST_DEVICE_REG1_REG;
    oal_pcie_chip_info_cb_init(&pst_pci_res->chip_info.cb, device_id);
    return OAL_SUCC;
}

static int32_t oal_pcie_linkup_prepare_fixup(int32_t ep_idx)
{
    return OAL_SUCC;
}

static int32_t oal_pcie_linkup_fail_fixup(int32_t ep_idx)
{
    return OAL_SUCC;
}

static int32_t oal_pcie_linkup_succ_fixup(int32_t ep_idx)
{
    return OAL_SUCC;
}

int32_t oal_pcie_board_init_bisheng(linkup_fixup_ops *ops)
{
    ops->link_prepare_fixup = oal_pcie_linkup_prepare_fixup;
    ops->link_fail_fixup = oal_pcie_linkup_fail_fixup;
    ops->link_succ_fixup = oal_pcie_linkup_succ_fixup;
    return OAL_SUCC;
}
#endif
