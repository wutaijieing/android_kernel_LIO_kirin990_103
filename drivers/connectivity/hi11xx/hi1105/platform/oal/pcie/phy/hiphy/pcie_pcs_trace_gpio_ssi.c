

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#define HI11XX_LOG_MODULE_NAME "[PCS_TRACE]"
#define HISI_LOG_TAG           "[PCS_TRACE]"
#define HI11XX_LOG_MODULE_NAME_VAR pcs_trace_loglevel
#include "pcie_pcs_trace.h"
#include "ssi_common.h"
#include "pcie_pcs.h"

#define PCIE_PCS_TRACER_LOGSIZE 8192

int32_t g_pcie_pcs_tracer_speed_idx = 0;
oal_debug_module_param(g_pcie_pcs_tracer_speed_idx, int, S_IRUGO | S_IWUSR);

int32_t g_pcie_pcs_tracer_link_idx = 0;
oal_debug_module_param(g_pcie_pcs_tracer_link_idx, int, S_IRUGO | S_IWUSR);

static pcs_trace_conf g_pcs_conf_table_v3_cut[] = {
    // reg_trace_trigger_port_sel lane num set by caller
    { 0x904, 19, 15, 0x00 },      // trigger_start_mask[31:0]
    { 0x9a4, 27, 16, 0xfff },     // trigger_start_mask[43:32]
    { 0x908, 19, 15, 0x00 },      // trigger_stop_mask[31:0]
    { 0x9a4, 11, 0, 0x00 },       // trigger_stop_mask[43:32]
    { 0x90c, 19, 15, 0x00 },      // start_trigger_value[31:0]
    { 0x9a8, 21, 16, 0x0d },      // start_trigger_value[43:32]
    { 0x9a8, 27, 22, 0x11 },      // start_trigger_value[43:32]
    { 0x910, 19, 15, 0x00 },      // stop_trigger_value[31:0]
    { 0x9a8, 11, 0, 0x00 },       // stop_trigger_value[43:32]
    { 0x900, 1, 1, 0x00 },        // reg_cap_mode
    { 0x918, 31, 0, 0xf0020fc0 }, // state_mask[31:0]
    { 0x91c, 31, 0, 0x41 },       // state_mask[63:32]
    { 0x920, 31, 0, 0xfff80000 }, // state_mask[95:64]
    { 0x924, 9, 8, 0x1 },         // state_mask[97:96]
    { 0x920, 11, 11, 0x1 },       // state_mask[75]
    { 0x900, 7, 4, 0xa },         // reg_state_cnt_pulse_sel
    { 0x900, 0, 0, 1 },           // set reg_trace_start : 1
    { 0x604, 2, 2, 0x0 },         // set reg_dfxclk_en : 0
    { 0x900, 10, 8, 0x00 },       // reg_trace_state_sel : 0
    { 0x604, 2, 2, 0x1 },         // set reg_dfxclk_en : 1
    PCS_TRACE_INFO_LAST
};

static pcs_trace_conf g_pcs_conf_table_v3_cut2[] = {
    // reg_trace_trigger_port_sel lane num set by caller
    { 0x904, 19, 15, 0x00 },      // trigger_start_mask[31:0]
    { 0x9a4, 27, 16, 0xfff },     // trigger_start_mask[43:32]
    { 0x908, 19, 15, 0x00 },      // trigger_stop_mask[31:0]
    { 0x9a4, 11, 0, 0x00 },       // trigger_stop_mask[43:32]
    { 0x90c, 19, 15, 0x00 },      // start_trigger_value[31:0]
    { 0x9a8, 21, 16, 0x0d },      // start_trigger_value[43:32]
    { 0x9a8, 27, 22, 0x11 },      // start_trigger_value[43:32]
    { 0x910, 19, 15, 0x00 },      // stop_trigger_value[31:0]
    { 0x9a8, 11, 0, 0x00 },       // stop_trigger_value[43:32]
    { 0x900, 1, 1, 0x00 },        // reg_cap_mode
    { 0x918, 31, 0, 0xf0020fc0 }, // state_mask[31:0]
    { 0x918, 27, 23, 0x1f },
    { 0x91c, 12, 6, 0x7f },
    { 0x91c, 31, 0, 0x41 },       // state_mask[63:32]
    { 0x920, 31, 0, 0xfff80000 }, // state_mask[95:64]
    { 0x924, 9, 8, 0x1 },         // state_mask[97:96]
    { 0x920, 11, 11, 0x1 },       // state_mask[75]
    { 0x900, 7, 4, 0xa },         // reg_state_cnt_pulse_sel
    { 0x900, 0, 0, 1 },           // set reg_trace_start : 1
    { 0x604, 2, 2, 0x0 },         // set reg_dfxclk_en : 0
    { 0x900, 10, 8, 0x00 },       // reg_trace_state_sel : 0
    { 0x604, 2, 2, 0x1 },         // set reg_dfxclk_en : 1
    PCS_TRACE_INFO_LAST
};

static pcs_trace_conf g_pcs_conf_table_v3_full[] = {
    // reg_trace_trigger_port_sel lane num set by caller
    { 0x904, 19, 15, 0x00 },      // trigger_start_mask[31:0]
    { 0x9a4, 27, 16, 0x0 },     // trigger_start_mask[43:32]
    { 0x908, 19, 15, 0x00 },      // trigger_stop_mask[31:0]
    { 0x9a4, 11, 0, 0x00 },       // trigger_stop_mask[43:32]
    { 0x90c, 19, 15, 0x00 },      // start_trigger_value[31:0]
    { 0x9a8, 21, 16, 0x20 },      // start_trigger_value[43:32]
    { 0x9a8, 27, 22, 0x0D },      // start_trigger_value[43:32]
    { 0x910, 19, 15, 0x00 },      // stop_trigger_value[31:0]
    { 0x9a8, 11, 0, 0x00 },       // stop_trigger_value[43:32]
    { 0x900, 1, 1, 0x00 },        // reg_cap_mode
    { 0x918, 31, 0, 0xf0800fc0 }, // state_mask[31:0]
    { 0x91c, 31, 0, 0x41 },       // state_mask[63:32]
    { 0x920, 31, 0, 0xf8000000 }, // state_mask[95:64]
    { 0x924, 9, 8, 0x1 },         // state_mask[97:96]
    { 0x920, 11, 11, 0x1 },       // state_mask[75]
    { 0x900, 7, 4, 0xa },         // reg_state_cnt_pulse_sel
    { 0x900, 0, 0, 1 },           // set reg_trace_start : 1
    { 0x604, 2, 2, 0x0 },         // set reg_dfxclk_en : 0
    { 0x900, 10, 8, 0x00 },       // reg_trace_state_sel : 0
    { 0x604, 2, 2, 0x1 },         // set reg_dfxclk_en : 1
    PCS_TRACE_INFO_LAST
};

static pcs_trace_conf g_pcs_conf_table_v3_dbg[] = {
    // reg_trace_trigger_port_sel lane num set by caller
    { 0x904, 19, 15, 0x00},      // trigger_start_mask[31:0]
    { 0x9a4, 27, 16, 0xfff},     // trigger_start_mask[43:32]
    { 0x908, 19, 15, 0x00 },      // trigger_stop_mask[31:0]
    { 0x9a4, 11, 0, 0x00 },       // trigger_stop_mask[43:32]
    { 0x90c, 19, 15, 0x00 },      // start_trigger_value[31:0]
    { 0x9a8, 21, 16, 0x0d },      // start_trigger_value[43:32]
    { 0x9a8, 27, 22, 0x11 },      // start_trigger_value[43:32]
    { 0x910, 19, 15, 0x00 },      // stop_trigger_value[31:0]
    { 0x9a8, 11, 0, 0x00 },       // stop_trigger_value[43:32]
    { 0x900, 1, 1, 0x00 },        // reg_cap_mode
    { 0x918, 31, 0, 0x0 }, // state_mask[31:0]
    { 0x91c, 31, 0, 0x0 },       // state_mask[63:32]
    { 0x920, 31, 0, 0xf8000000 }, // state_mask[95:64]
    { 0x924, 9, 8, 0x1 },         // state_mask[97:96]
    { 0x920, 11, 11, 0x1 },       // state_mask[75]
    { 0x900, 7, 4, 0xa },         // reg_state_cnt_pulse_sel
    { 0x900, 0, 0, 1 },           // set reg_trace_start : 1
    { 0x604, 2, 2, 0x0 },         // set reg_dfxclk_en : 0
    { 0x900, 10, 8, 0x00 },       // reg_trace_state_sel : 0
    { 0x604, 2, 2, 0x1 },         // set reg_dfxclk_en : 1
    PCS_TRACE_INFO_LAST
};

static pcs_trace_conf g_pcs_conf_table_linkup[] = {
    // reg_trace_trigger_port_sel lane num set by caller
    { 0x904, 19, 15, 0x00 },      // trigger_start_mask[31:0]
    { 0x9a4, 27, 16, 0xfff },     // trigger_start_mask[43:32]
    { 0x908, 19, 15, 0x00 },      // trigger_stop_mask[31:0]
    { 0x9a4, 11, 0, 0x00 },       // trigger_stop_mask[43:32]
    { 0x90c, 19, 15, 0x00 },      // start_trigger_value[31:0]
    { 0x9a8, 21, 16, 0x04 },      // start_trigger_value[43:32]
    { 0x9a8, 27, 22, 0x02 },      // start_trigger_value[43:32]
    { 0x910, 19, 15, 0x00 },      // stop_trigger_value[31:0]
    { 0x9a8, 11, 0, 0x00 },       // stop_trigger_value[43:32]
    { 0x900, 1, 1, 0x00 },        // reg_cap_mode
    { 0x918, 31, 0, 0xf0020fc0 }, // state_mask[31:0]
    { 0x918, 27, 23, 0x1f },
    { 0x91c, 12, 6, 0x7f },
    { 0x91c, 31, 0, 0x41 },       // state_mask[63:32]
    { 0x920, 31, 0, 0xfff80000 }, // state_mask[95:64]
    { 0x924, 9, 8, 0x1 },         // state_mask[97:96]
    { 0x920, 11, 11, 0x1 },       // state_mask[75]
    { 0x900, 7, 4, 0xa },         // reg_state_cnt_pulse_sel
    { 0x900, 0, 0, 1 },           // set reg_trace_start : 1
    { 0x604, 2, 2, 0x0 },         // set reg_dfxclk_en : 0
    { 0x900, 10, 8, 0x00 },       // reg_trace_state_sel : 0
    { 0x604, 2, 2, 0x1 },         // set reg_dfxclk_en : 1
    PCS_TRACE_INFO_LAST
};

/* 切速时数采配置 */
static pcs_trace_conf *g_pcs_conf_speed_array[] = {
    g_pcs_conf_table_v3_cut,
    g_pcs_conf_table_v3_cut2,
    g_pcs_conf_table_v3_dbg,
    g_pcs_conf_table_v3_full
};

/* 预留 */
static pcs_trace_conf *g_pcs_conf_link_array[] = {
    g_pcs_conf_table_linkup,
    g_pcs_conf_table_linkup
};

static pcs_trace_conf* pcie_pcs_get_tracer_conf(pcs_tracer_dfx_type type)
{
    switch (type) {
        case PCS_TRACER_SPEED_CHANGE:
            if ((uint32_t)g_pcie_pcs_tracer_speed_idx < oal_array_size(g_pcs_conf_speed_array)) {
                return g_pcs_conf_speed_array[g_pcie_pcs_tracer_speed_idx];
            }
            break;
        case PCS_TRACER_LINK:
            if ((uint32_t)g_pcie_pcs_tracer_link_idx < oal_array_size(g_pcs_conf_link_array)) {
                return g_pcs_conf_link_array[g_pcie_pcs_tracer_link_idx];
            }
            break;
        default:
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "unsupport pcs tracer type=%d", type);
    }
    oal_print_hi11xx_log(HI11XX_LOG_ERR, "mismatch pcs conf=%d", type);
    return NULL;
}


/* 使能PHY tracer, called by chip ops */
int32_t pcie_pcs_tracer_start(uintptr_t base_addr, pcs_tracer_dfx_type type, uint32_t lane,
                              uint32_t dump_type)
{
    int32_t ret;
    pcs_set_bits32 set_bits32 = NULL;
    pcs_get_bits32 get_bits32 = NULL;
    pcs_trace_conf *trace = NULL;

    if (oal_warn_on(base_addr == (uintptr_t)NULL)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "base_addr is null");
        return -OAL_EINVAL;
    }

    trace = pcie_pcs_get_tracer_conf(type);
    if (trace == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "unsupport pcs tracer type=%d", type);
        return -OAL_ENODEV;
    }
    if (pcie_pcs_ref_get(dump_type) != OAL_SUCC) {
        return -OAL_EFAIL;
    }
    pcie_pcs_get_bitops(&set_bits32, &get_bits32, dump_type);
    ret = pcie_pcs_trace_enable(base_addr, trace, set_bits32, lane);
    pcie_pcs_ref_put(dump_type);
    return ret;
}

/* 停止PHY tracer, 同时上报数采数据 */
void pcie_pcs_tracer_dump(uintptr_t base_addr, uint32_t lane, uint32_t dump_type)
{
    int32_t ret;
    void *buff = NULL;
    pcs_set_bits32 set_bits32 = NULL;
    pcs_get_bits32 get_bits32 = NULL;
    errno_t ret_err;

    if (oal_warn_on(base_addr == (uintptr_t)NULL)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "base_addr is null");
        return;
    }

    /* phy tracer dump info about 6024 bytes */
    buff = oal_memalloc(PCIE_PCS_TRACER_LOGSIZE);
    if (buff == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, " mem alloc failed size=%d", PCIE_PCS_TRACER_LOGSIZE);
        return;
    }
    ret_err = memset_s(buff, PCIE_PCS_TRACER_LOGSIZE, 0, PCIE_PCS_TRACER_LOGSIZE);
    if (ret_err != EOK) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, " memset_s failed=%d", ret_err);
        return;
    }

    if (pcie_pcs_ref_get(dump_type) != OAL_SUCC) {
        return;
    }
    pcie_pcs_get_bitops(&set_bits32, &get_bits32, dump_type);
    ret = pcie_pcs_get_traceinfo(base_addr, set_bits32, get_bits32, lane,
                                 buff, PCIE_PCS_TRACER_LOGSIZE);
    pcie_pcs_ref_put(dump_type);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, " pcie_pcs_get_traceinfo failed=%d", ret);
        return;
    }

    pcie_pcs_print_split_log_info((char*)buff); // print as string

    oal_free(buff);
}


#endif