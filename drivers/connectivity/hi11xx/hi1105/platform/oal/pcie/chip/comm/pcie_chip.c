

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#define HI11XX_LOG_MODULE_NAME     "[PCIEC]"
#define HI11XX_LOG_MODULE_NAME_VAR pciec_loglevel
#define HISI_LOG_TAG               "[PCIEC]"

#include "pcie_chip.h"
#include "chip/hi1103/pcie_chip_hi1103.h"
#include "chip/shenkuo/pcie_chip_shenkuo.h"
#include "chip/bisheng/pcie_chip_bisheng.h"
#include "chip/hi1161/pcie_chip_hi1161.h"
#include "pcie_linux.h"
#include "pcie_pm.h"
#include "board.h"

static pcie_chip_id_stru g_pcie_chipinfo_id[] = {
    {PCIE_HISI_DEVICE_ID_HI1103,     oal_pcie_chip_info_init_hi1103},
    {PCIE_HISI_DEVICE_ID_HI1105,     oal_pcie_chip_info_init_hi1103},
    {PCIE_HISI_DEVICE_ID_SHENKUO,     oal_pcie_chip_info_init_shenkuo},
    {PCIE_HISI_DEVICE_ID_SHENKUOFPGA, oal_pcie_chip_info_init_shenkuo},
    {PCIE_HISI_DEVICE_ID_BISHENG,    oal_pcie_chip_info_init_bisheng},
    {PCIE_HISI_DEVICE_ID_HI1161,     oal_pcie_chip_info_init_hi1161}
};

int32_t oal_pcie_board_init_shenkuo(linkup_fixup_ops *ops);
int32_t oal_pcie_board_init_bisheng(linkup_fixup_ops *ops);
static board_chip_id_stru g_board_chipinfo_id[] = {
    {BOARD_VERSION_SHENKUO, oal_pcie_board_init_shenkuo},
    {BOARD_VERSION_BISHENG, oal_pcie_board_init_bisheng},
};

static linkup_fixup_ops g_pcie_linkup_fixup_ops = { 0 };

static int32_t oal_pcie_chip_info_res_init(oal_pcie_res *pst_pci_res, int32_t device_id)
{
    int32_t i;
    int32_t chip_num = oal_array_size(g_pcie_chipinfo_id);
    for (i = 0; i < chip_num; i++) {
        if (g_pcie_chipinfo_id[i].device_id == device_id) {
            if (g_pcie_chipinfo_id[i].func != NULL) {
                return g_pcie_chipinfo_id[i].func(pst_pci_res, device_id);
            } else {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie chip init 0x%x init null func", device_id);
                return OAL_SUCC;
            }
        }
    }
    oal_print_hi11xx_log(HI11XX_LOG_ERR, "not implement chip info init device_id=0x%x", device_id);
    return -OAL_ENODEV;
}

static void oal_pcie_pcs_ini_config_init(void)
{
    int32_t l_cfg_value = 0;
    int32_t l_ret;

    /* 获取ini的配置值 */
    l_ret = get_cust_conf_int32(INI_MODU_PLAT, "pcs_trace_enable", &l_cfg_value);
    if (l_ret == INI_FAILED) {
        pci_print_log(PCI_LOG_INFO, "pcs_trace_enable not set, use defalut value=%d\n", g_pcie_pcs_tracer_enable);
        return;
    }

    /* 这里不判断有效性，在PHY初始化时判断，如果无效，不设置FFE */
    g_pcie_pcs_tracer_enable = (uint32_t)l_cfg_value;
    pci_print_log(PCI_LOG_INFO, "g_pcie_pcs_tracer_enable=0x%x", g_pcie_pcs_tracer_enable);
    return;
}

int32_t oal_pcie_board_init(void)
{
    uint32_t i;
    int32_t chip_id = get_hi110x_subchip_type();
    uint32_t chip_num = oal_array_size(g_board_chipinfo_id);
    oal_pcie_pcs_ini_config_init();
    for (i = 0; i < chip_num; i++) {
        if (g_board_chipinfo_id[i].chip_id == chip_id) {
            if (g_board_chipinfo_id[i].func != NULL) {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie board init 0x%x init succ", chip_id);
                return g_board_chipinfo_id[i].func(&g_pcie_linkup_fixup_ops);
            } else {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie board init 0x%x init null func", chip_id);
                return OAL_SUCC;
            }
        }
    }
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "not implement chip board init device_id=0x%x", chip_id);
    return -OAL_ENODEV;
}


int32_t oal_pcie_chip_info_init(oal_pcie_res *pst_pci_res)
{
    int32_t ret;
    uint32_t device_id;
    oal_pci_dev_stru *pst_pci_dev = pcie_res_to_dev(pst_pci_res);
    size_t len = sizeof(pst_pci_res->chip_info.addr_info);

    (void)memset_s(&pst_pci_res->chip_info.addr_info, len, 0, len);

    device_id = (uint32_t)pst_pci_dev->device;
    ret = oal_pcie_pm_chip_init(pst_pci_res, device_id);
    if (ret != OAL_SUCC) {
        return ret;
    }

    ret = oal_pcie_chip_info_res_init(pst_pci_res, device_id);
    if (ret != OAL_SUCC) {
        return ret;
    }

    return ret;
}

unsigned long oal_pcie_slt_get_deivce_ram_cpuaddr(oal_pcie_res *pst_pci_res)
{
    if (pst_pci_res->chip_info.cb.get_test_ram_address == NULL) {
        return 0xffffffff; // invalid address
    }
    return (unsigned long)pst_pci_res->chip_info.cb.get_test_ram_address();
}

int32_t oal_pcie_host_slave_address_switch(oal_pcie_res *pst_pci_res, uint64_t src_addr,
                                           uint64_t* dst_addr, int32_t is_host_iova)
{
    if (oal_warn_on(pst_pci_res->chip_info.cb.pcie_host_slave_address_switch == NULL)) {
        *dst_addr = 0;
        return -OAL_ENODEV; // needn't address switch
    }
    return pst_pci_res->chip_info.cb.pcie_host_slave_address_switch(pst_pci_res,
                                                                    src_addr, dst_addr, is_host_iova);
}

int32_t pcie_linkup_prepare_fixup(int32_t ep_idx)
{
    if (g_pcie_linkup_fixup_ops.link_prepare_fixup == NULL) {
        return -OAL_ENODEV;
    }
    return g_pcie_linkup_fixup_ops.link_prepare_fixup(ep_idx);
}

int32_t pcie_linkup_fail_fixup(int32_t ep_idx)
{
    if (g_pcie_linkup_fixup_ops.link_fail_fixup == NULL) {
        return -OAL_ENODEV;
    }
    return g_pcie_linkup_fixup_ops.link_fail_fixup(ep_idx);
}

int32_t pcie_linkup_succ_fixup(int32_t ep_idx)
{
    if (g_pcie_linkup_fixup_ops.link_succ_fixup == NULL) {
        return -OAL_ENODEV;
    }
    return g_pcie_linkup_fixup_ops.link_succ_fixup(ep_idx);
}
#endif
