

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include "pcie_pcs_deye_pattern_host.h"
#include "pcie_pcs_deye_pattern.h"
#include "pcie_pcs.h"
#include "oal_util.h"

/* 开始PHY电子眼图测试 */
int32_t pcie_pcs_deye_pattern_host_test(uintptr_t base_addr, uint32_t lane, uint32_t dump_type)
{
    int32_t ret;
    void *buff = NULL;
    pcs_reg_ops reg_ops = { 0 };
    errno_t ret_err;

    if (oal_warn_on(base_addr == (uintptr_t)NULL)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "base_addr is null");
        return -OAL_EINVAL;
    }

    buff = oal_memalloc(PCIE_PCS_DEYE_PARTTERN_SIZE);
    if (buff == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, " mem alloc failed size=%d", PCIE_PCS_DEYE_PARTTERN_SIZE);
        return -OAL_ENOMEM;
    }
    ret_err = memset_s(buff, PCIE_PCS_DEYE_PARTTERN_SIZE, 0, PCIE_PCS_DEYE_PARTTERN_SIZE);
    if (ret_err != EOK) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, " memset_s failed=%d", ret_err);
        return -OAL_EFAIL;
    }

    if (pcie_pcs_ref_get(dump_type) != OAL_SUCC) {
        return -OAL_EFAIL;
    }
    pcie_pcs_set_regs_ops(&reg_ops, dump_type);
    ret = pcie_pcs_deye_pattern_draw(&reg_ops, base_addr, lane, buff, PCIE_PCS_DEYE_PARTTERN_SIZE);
    pcie_pcs_ref_put(dump_type);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, " pcie_pcs_deye_pattern_draw failed=%d", ret);
        return ret;
    }

    pcie_pcs_print_split_log_info((char*)buff); // print as string

    oal_free(buff);
    return OAL_SUCC;
}
#endif