

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include "pcie_pcs_trace.h"
#include "oal_util.h"

int32_t pcie_pcs_regtable_check_para(pcs_trace_conf *trace)
{
    if (trace->msb < trace->lsb) {
        return -OAL_EINTR;
    }
    if ((trace->msb > PCS_REG_MAX_BIT_POS) || (trace->msb > PCS_REG_MAX_BIT_POS)) {
        return -OAL_EINVAL;
    }
    return OAL_SUCC;
}

int32_t pcie_pcs_write_regtable(uintptr_t base_addr, pcs_trace_conf *trace_conf, pcs_set_bits32 set_bits32)
{
    int32_t i;
    int32_t ret;
    uint32_t pos, bits;
    pcs_trace_conf *trace = NULL;
    for (i = 0;; i++) {
        trace = trace_conf + i;
        if (trace->offset == PCS_TRACE_INFO_END_FLAG) {
            break; // find the last trace config
        }
        ret = pcie_pcs_regtable_check_para(trace);
        if (ret != OAL_SUCC) {
            return ret;
        }
        pos = trace->lsb;
        bits = trace->msb - trace->lsb + 1;
        set_bits32(base_addr + trace->offset, pos, bits, trace->value);
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "0x%x set pos=%u bits=%u value=0x%x",
            (uint32_t)(base_addr + trace->offset), pos, bits, trace->value);
    }
    return OAL_SUCC;
}

int32_t pcie_pcs_trace_enable(uintptr_t base_addr, pcs_trace_conf *trace_conf,
                              pcs_set_bits32 set_bits32, uint32_t lane)
{
    if (oal_warn_on(set_bits32 == NULL)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "set_bits32 is null");
        return -OAL_ENODEV;
    }
    set_bits32(base_addr + 0x900, 0x2, 0x2, lane);
    return pcie_pcs_write_regtable(base_addr, trace_conf, set_bits32);
}

int32_t pcie_pcs_get_traceinfo(uintptr_t base_addr, pcs_set_bits32 set_bits32,
                               pcs_get_bits32 get_bits32, uint32_t lane,
                               uint8_t *buff, int32_t size)
{
    int32_t ret;
    int32_t offset;
    uint32_t i, st0, st1, st2, st3, last_addr;
    if (oal_warn_on(buff == NULL)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "buff is null");
        return -OAL_ENODEV;
    }

    if ((set_bits32 == NULL) || (get_bits32 == NULL)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bits32 ops is null");
        return -OAL_ENODEV;
    }

    offset = 0;
    /* 8bits for tracer address, max 128 items */
    last_addr = get_bits32((base_addr + 0x914), 0, 8);
    ret = snprintf_s((char*)(buff + offset), size - offset, size - offset - 1,
                     "trace_write_last_addr=0x%x", last_addr); // 8bits inner addr
    if (ret < 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, " mem buff overrun, size=%d", size);
        return -OAL_ENOMEM;
    }
    offset += ret;

    ret = snprintf_s((char*)(buff + offset), size - offset, size - offset - 1,
                     "      NO.     [97:96][95:64 ][63:32 ][31:0 ] \n");
    if (ret < 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, " mem buff overrun, size=%d", size);
        return -OAL_ENOMEM;
    }
    offset += ret;

    for (i = 0; i < 128; i++) { // mod 128
        set_bits32(base_addr + 0x924, 0, 7, i); // 7bits, slect tracer read addr
        st0 = get_bits32(base_addr + 0x928, 0, 32); // 32bits, whole reg
        st1 = get_bits32(base_addr + 0x92c, 0, 32); // 32bits, whole reg
        st2 = get_bits32(base_addr + 0x930, 0, 32); // 32bits, whole reg
        st3 = get_bits32(base_addr + 0x914, 9, 2); //  pos bit 9, 2bits
        ret = snprintf_s((char *)(buff + offset), size - offset, size - offset - 1,
            "trace_%03u[97:0],98'h%x%08x%08x%08x\n", i, st3, st2, st1, st0);
        if (ret < 0) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, " mem buff overrun, size=%d", size);
            return -OAL_ENOMEM;
        }
        offset += ret;
    }
    return OAL_SUCC;
}
#endif