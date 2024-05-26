

#ifndef __PCIE_PCS_TRACE_H__
#define __PCIE_PCS_TRACE_H__

#include "oal_types.h"
#include "pcie_pcs_regs.h"

#define PCS_REG_MAX_BIT_POS 31

#define PCS_TRACE_INFO_END_FLAG 0xffffffff
#define PCS_TRACE_INFO_LAST              \
    {                                    \
        PCS_TRACE_INFO_END_FLAG, 0, 0, 0 \
    }

typedef enum _pcs_tracer_dfx_type_ {
    PCS_TRACER_LINK, /* 首次建链时数采 */
    PCS_TRACER_SPEED_CHANGE, /* 切速时数采 */
    PCS_TRACER_BUTT
} pcs_tracer_dfx_type;

typedef struct _pcs_trace_conf_ {
    uint32_t offset; // phy register offset
    uint32_t msb;    // phy register high bit pos
    uint32_t lsb;    // phy register low bit pos, pos = lsb, bits= msb - lsb + 1
    uint32_t value;  // the bits's value to config
} pcs_trace_conf;


int32_t pcie_pcs_get_traceinfo(uintptr_t base_addr, pcs_set_bits32 set_bits32,
                               pcs_get_bits32 get_bits32, uint32_t lane,
                               uint8_t *buff, int32_t size);
int32_t pcie_pcs_trace_enable(uintptr_t base_addr, pcs_trace_conf *trace_conf,
                              pcs_set_bits32 set_bits32, uint32_t lane);
/* tracer interfaces */
int32_t pcie_pcs_tracer_start(uintptr_t base_addr, pcs_tracer_dfx_type type, uint32_t lane, uint32_t dump_type);
void pcie_pcs_tracer_dump(uintptr_t base_addr, uint32_t lane,  uint32_t dump_type);
#endif

