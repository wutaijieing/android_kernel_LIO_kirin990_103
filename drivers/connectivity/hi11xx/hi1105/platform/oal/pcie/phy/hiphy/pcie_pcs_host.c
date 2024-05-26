

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include "pcie_pcs_host.h"
#include "oal_util.h"
#include "pcie_pcs_regs_ver01.h"
#include "oal_list.h"


/* FFE 配置档位 */
#define PCIE_PHY_PRST_P0 0
#define PCIE_PHY_PRST_P1 1
#define PCIE_PHY_PRST_P2 2
#define PCIE_PHY_PRST_P3 3
#define PCIE_PHY_PRST_P4 4
#define PCIE_PHY_PRST_P5 5
#define PCIE_PHY_PRST_P6 6
#define PCIE_PHY_PRST_P7 7
#define PCIE_PHY_PRST_P8 8
#define PCIE_PHY_PRST_P9 9
#define PCIE_PHY_PRST_P10 10
#define PCIE_PHY_PRST_MAX 11

#define PCIE_PHY_COL_PRE  0
#define PCIE_PHY_COL_POST 1

#define PCIE_PHY_DEFALUT_FLAG 12345678

typedef struct _pcie_phy_fixup_reg_ {
    oal_list_entry_stru list;
    uint32_t offset;
    uint32_t pos;
    uint32_t bits;
    uint32_t value;
} pcie_phy_fixup_reg;

int32_t g_pcie_phy_ffe = 0x7fffffff; // defualt, invalid value
oal_debug_module_param(g_pcie_phy_ffe, int, S_IRUGO | S_IWUSR);

int32_t g_pcs_sel_r = 0x9;
oal_debug_module_param(g_pcs_sel_r, int, S_IRUGO | S_IWUSR);

int32_t g_pcs_atten = PCIE_PHY_DEFALUT_FLAG;
oal_debug_module_param(g_pcs_atten, int, S_IRUGO | S_IWUSR);

uint32_t g_pcie_phy_fixup = 0;
oal_debug_module_param(g_pcie_phy_fixup, uint, S_IRUGO | S_IWUSR);

oal_define_spinlock(g_pcs_fixup_head_lock);
oal_list_create_head(g_pcs_fixup_head);


/* PHY FFE 配置, 切速之前配置, EP TX to RC RX
 * PHY团队给的固定值，这个数组不允许私自修改!!! */
static const uint32_t g_phy_preset[PCIE_PHY_PRST_MAX][2] = { // 2 cols
    [PCIE_PHY_PRST_P4] = {0, 0},
    [PCIE_PHY_PRST_P1] = {0, 6},
    [PCIE_PHY_PRST_P0] = {0, 9},
    [PCIE_PHY_PRST_P9] = {6, 0},
    [PCIE_PHY_PRST_P8] = {5, 4},
    [PCIE_PHY_PRST_P7] = {4, 7},
    [PCIE_PHY_PRST_P5] = {4, 0},
    [PCIE_PHY_PRST_P6] = {5, 0},
    [PCIE_PHY_PRST_P3] = {0, 4},
    [PCIE_PHY_PRST_P2] = {0, 7},
    [PCIE_PHY_PRST_P10] = {0, 12}
};

static int32_t oal_pcie_device_phy_ffe_para_check(void)
{
    if ((g_pcie_phy_ffe < 0) || (g_pcie_phy_ffe >= PCIE_PHY_PRST_MAX)) {
        pci_print_log(PCI_LOG_INFO, "ffe bypass, value=%d", g_pcie_phy_ffe);
        return -OAL_EINVAL;
    }
    pci_print_log(PCI_LOG_INFO, "ffe sel=P%d", g_pcie_phy_ffe);
    return OAL_SUCC;
}

static int32_t oal_pcie_device_phy_ffe_check_match_info(oal_pcie_res *pst_pci_res, uint32_t base_addr,
                                                        uint32_t pre, uint32_t post)
{
    int32_t ret;
    uint32_t pre_r, post_r;

    /* read post, reg_da_tx_eq_post_ctrl_sts, pos: 3, bits: 4 */
    ret = oal_pcie_device_phy_read_reg(pst_pci_res, base_addr + PCIE_PHY_DA_0X_EQ_0_STS_OFFSET, 3, 4, &post_r);
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* read post, reg_da_tx_eq_pre_ctrl_sts, pos: 0, bits: 3 */
    ret = oal_pcie_device_phy_read_reg(pst_pci_res, base_addr + PCIE_PHY_DA_0X_EQ_0_STS_OFFSET, 0, 3, &pre_r);
    if (ret != OAL_SUCC) {
        return ret;
    }

    if ((pre == pre_r) && (post == post_r)) {
        pci_print_log(PCI_LOG_INFO, "phy ffe gear set succ![pre:%d, post:%d]", pre, post);
        return OAL_SUCC;
    }
    pci_print_log(PCI_LOG_ERR, "unkown ffe gear pre:%d post:%d", pre, post);
    return -OAL_EFAIL;
}

/* 调整FFE档位对GEN1也有影响，必须保证在GEN1上 EP TX补偿也是OK的！ */
static int32_t oal_pcie_device_phy_set_ffe(oal_pcie_res *pst_pci_res, uint32_t base_addr,
                                           uint32_t pre, uint32_t post)
{
    /* set 0x1b18 bits[3,2] to 0  reg_da_tx_eq_post_en_ctrl_en, reg_da_tx_eq_pre_en_ctrl_en */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_0X_EQ_0_OFFSET), 2, 2, 0x0);

    /* set 0x1b5c bits[8,8] to 1 TX EQ POST EN, reg_da_tx_eq_post_en_ctrl_val, pos=8, bits=1 */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_FOM_0_OFFSET), 8, 1, 0x1);
    /* set tx eq post value, reg_da_tx_eq_post_ctrl_en,  pos=1, bits=1 */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_0X_EQ_0_OFFSET), 1, 1, 0x1);
    /* set 0x1b5c bits[6,3] to post, reg_da_tx_eq_post_ctrl_val,  pos=3, bits=4 */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_FOM_0_OFFSET), 3, 4, post);

    /* set 0x1b5c bits[7,7] to 1 , reg_da_tx_eq_pre_en_ctrl_val, pos=7, bits=1 */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_FOM_0_OFFSET), 7, 1, 0x1);
    /* 0x1b18 tx eq pre set, reg_da_tx_eq_pre_ctrl_en, pos=0, bits=1 */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_0X_EQ_0_OFFSET), 0, 1, 0x1);
    /* 0x1b5c tx eq pre write,  reg_da_tx_eq_pre_ctrl_val, pos=0, bits=3 */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_FOM_0_OFFSET), 0, 3, pre);

    /* set 0x1b18 bits[3,2] to 0x3  reg_da_tx_eq_post_en_ctrl_en, reg_da_tx_eq_pre_en_ctrl_en, pos=2, bits=2 */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_0X_EQ_0_OFFSET), 2, 2, 0x3);

    /* dump post & pre value */
    return oal_pcie_device_phy_ffe_check_match_info(pst_pci_res, base_addr, pre, post);
}

static int32_t oal_pcie_device_phy_ffe_config(oal_pcie_res *pst_pci_res, uint32_t base_addr)
{
    int32_t ret;

    ret = oal_pcie_device_phy_ffe_para_check();
    if (ret != OAL_SUCC) {
        return OAL_SUCC; // ffe not set, used default phy para
    }

    return oal_pcie_device_phy_set_ffe(pst_pci_res, base_addr,
                                       g_phy_preset[g_pcie_phy_ffe][PCIE_PHY_COL_PRE],
                                       g_phy_preset[g_pcie_phy_ffe][PCIE_PHY_COL_POST]);
}

/* called by kernel interface, add fixup regiter configs */
void oal_pcie_device_phy_config_fixup_add(uint32_t offset, uint32_t pos, uint32_t bits, uint32_t value)
{
    unsigned long flags;
    pcie_phy_fixup_reg *pst_phy_reg = (pcie_phy_fixup_reg*)oal_memalloc(sizeof(pcie_phy_fixup_reg));
    if (pst_phy_reg == NULL) {
        pci_print_log(PCI_LOG_ERR, "mem alloc failed, size=%d", (int32_t)sizeof(pcie_phy_fixup_reg));
        return;
    }
    oal_spin_lock_irq_save(&g_pcs_fixup_head_lock, &flags);
    pst_phy_reg->offset = offset;
    pst_phy_reg->pos = pos;
    pst_phy_reg->bits = bits;
    pst_phy_reg->value = value;
    oal_list_add(&pst_phy_reg->list, &g_pcs_fixup_head);
    oal_spin_unlock_irq_restore(&g_pcs_fixup_head_lock, &flags);
}

void oal_pcie_device_phy_config_fixup_dumpinfo(void)
{
    int32_t i = 0;
    unsigned long flags;
    oal_list_entry_stru *pst_pos = NULL;
    pcie_phy_fixup_reg *pst_phy_reg = NULL;
    oal_io_print("pcs phy fixup dump info:\n");
    oal_spin_lock_irq_save(&g_pcs_fixup_head_lock, &flags);
    oal_list_search_for_each(pst_pos, &g_pcs_fixup_head)
    {
        oal_spin_unlock_irq_restore(&g_pcs_fixup_head_lock, &flags);
        pst_phy_reg = oal_list_get_entry(pst_pos, pcie_phy_fixup_reg, list);
        oal_io_print("[%d] offset:0x%8x, pos:%4u, bits:%4u, value:0x%8x\n", ++i,
                     pst_phy_reg->offset, pst_phy_reg->pos, pst_phy_reg->bits, pst_phy_reg->value);
        oal_spin_lock_irq_save(&g_pcs_fixup_head_lock, &flags);
    }
    oal_spin_unlock_irq_restore(&g_pcs_fixup_head_lock, &flags);
    oal_io_print("pcs phy fixup dump finish\n");
}

void oal_pcie_device_phy_config_fixup(oal_pcie_res *pst_pci_res, uint32_t base_addr)
{
    int32_t i = 0;
    unsigned long flags;
    oal_list_entry_stru *pst_pos = NULL;
    pcie_phy_fixup_reg *pst_phy_reg = NULL;
    if (g_pcie_phy_fixup == 0) {
        pci_print_log(PCI_LOG_INFO, "pcie device phy config fixup dbg bypass.");
        return;
    }
    oal_spin_lock_irq_save(&g_pcs_fixup_head_lock, &flags);
    oal_list_search_for_each(pst_pos, &g_pcs_fixup_head)
    {
        oal_spin_unlock_irq_restore(&g_pcs_fixup_head_lock, &flags);
        pst_phy_reg = oal_list_get_entry(pst_pos, pcie_phy_fixup_reg, list);
        oal_io_print("[%d] offset:0x%8x, pos:%4u, bits:%4u, value:0x%8x\n", ++i,
                     pst_phy_reg->offset, pst_phy_reg->pos, pst_phy_reg->bits, pst_phy_reg->value);
        oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + pst_phy_reg->offset),
                                       pst_phy_reg->pos, pst_phy_reg->bits, pst_phy_reg->value);
        oal_spin_lock_irq_save(&g_pcs_fixup_head_lock, &flags);
    }
    oal_spin_unlock_irq_restore(&g_pcs_fixup_head_lock, &flags);
}

static void oal_pcie_phy_ini_config_ffe(void)
{
    int32_t value = 0;
    int32_t ret;

    /* 获取ini的配置值 */
    ret = get_cust_conf_int32(INI_MODU_PLAT, "pcie_phy_ffe", &value);
    if (ret == INI_FAILED) {
        pci_print_log(PCI_LOG_INFO, "pcie_phy_ffe not set, use defalut value=%d\n", g_pcie_phy_ffe);
        return;
    }

    /* 这里不判断有效性，在PHY初始化时判断，如果无效，不设置FFE */
    g_pcie_phy_ffe = (uint16_t)value;
    pci_print_log(PCI_LOG_INFO, "g_pcie_phy_ffe=%d", g_pcie_phy_ffe);
}

static void oal_pcie_phy_ini_config_atten(void)
{
    int32_t sel_r, atten;
    int32_t ret_sel, ret_att;

    /* 获取ini的配置值 */
    ret_sel = get_cust_conf_int32(INI_MODU_PLAT, "pcie_phy_sel_r", &sel_r);
    ret_att = get_cust_conf_int32(INI_MODU_PLAT, "pcie_phy_atten", &atten);
    if (ret_sel == INI_SUCC) {
        g_pcs_sel_r = sel_r;
    }
    if (ret_att == INI_SUCC) {
        g_pcs_atten = atten;
    }
    pci_print_log(PCI_LOG_INFO, "g_pcs_sel_r=0x%x", g_pcs_sel_r);
    pci_print_log(PCI_LOG_INFO, "g_pcs_atten=0x%x", g_pcs_atten);
}

void oal_pcie_phy_ini_config_init(void)
{
    oal_pcie_phy_ini_config_ffe();
    oal_pcie_phy_ini_config_atten();
}

/* ASPM L1sub reK bypass, otherwize pcie phy will reK when enter recovery */
void oal_pcie_device_phy_disable_l1ss_rekey(oal_pcie_res *pst_pci_res, uint32_t base_addr)
{
    oal_pcie_device_phy_config_reg(pst_pci_res,
                                   (base_addr + PCIE_PHY_UPDTCTRL_OFFSET), 8, 3, 0x0);
}

static void oal_pcie_device_phy_config_single_lane_from_ini(oal_pcie_res *pst_pci_res, uint32_t base_addr)
{
    /* 修改SEL_R默认值: default sel_r:0xb, 设置为0x9 */
    if (g_pcs_sel_r != PCIE_PHY_DEFALUT_FLAG) {
        oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_RX_AFE_CFG_REG_OFFSET), 9, 1, 0x1);
        oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_RXGSACTRL_OFFSET), 16, 4, g_pcs_sel_r);
    }

    /* atten reconfig */
    if (g_pcs_atten != PCIE_PHY_DEFALUT_FLAG) {
        oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_RX_AFE_CFG_REG_OFFSET), 4, 1, 0x1);
        oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_RXGSACTRL_OFFSET), 8, 3, g_pcs_atten);
    }
}

void oal_pcie_device_phy_config_single_lane(oal_pcie_res *pst_pci_res, uint32_t base_addr)
{
    /* 优化高温眼图效果: 关闭RX DFE数模接口使能 */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_DFE_EN_OFFSET), 0, 1, 0x1);
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_DFE_VAL_OFFSET), 0, 1, 0x0);

    /* 优化眼图margin: 增大RX Loading电阻 */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_RX0_GEN_EN_OFFSET), 1, 1, 0x1);
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_DA_RX0_GEN_VAL_OFFSET), 1, 1, 0x0);

    /* PLL TCLP Iref select en: bit[1] 0:disable(default) 1:enable */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_ICP_CTRL_EN_OFFSET), 1, 1, 0x1);
    /* PLL TCLP Iref config Value: bit[1:4] & & Scale bit[5]
     * I = (Value * 20uA + 200uA) * (Scale + 1) I:200uA(default) I:1mA(max) */
    oal_pcie_device_phy_config_reg(pst_pci_res, (base_addr + PCIE_PHY_ICP_CTRL_VAL_OFFSET), 1, 5, 0x1f);

    oal_pcie_device_phy_config_single_lane_from_ini(pst_pci_res, base_addr);

    /* fixup for debug */
    oal_pcie_device_phy_config_fixup(pst_pci_res, base_addr);

    oal_pcie_device_phy_ffe_config(pst_pci_res, base_addr);
}

#endif