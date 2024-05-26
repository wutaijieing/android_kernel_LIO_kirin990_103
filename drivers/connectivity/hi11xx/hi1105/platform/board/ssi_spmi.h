
#ifndef __SSI_SPMI_H__
#define __SSI_SPMI_H__
#include "plat_debug.h"

/* master channel id */
typedef enum {
    SPMI_CHANNEL_WIFI = 0,
    SPMI_CHANNEL_BFG  = 1,
    SPMI_CHANNEL_GNSS = 2,
    SPMI_CHANNEL_GLE = 3,
    SPMI_CHANNEL_MAX
} spmi_channel_id;

/* slave id */
typedef enum {
    SPMI_SLAVEID_HI6506 = 0x1,
    SPMI_SLAVEID_HI6555 = 0x9,
    SPMI_SLAVEID_MAX = 0x10
} spmi_slave_id;

void reinit_spmi_base_addr(uint32_t spmi_base_addr);
int32_t ssi_spmi_read(uint32_t addr, uint32_t channel, uint32_t slave_id, uint32_t *value);
int32_t ssi_spmi_write(uint32_t addr, uint32_t channel, uint32_t slave_id, uint32_t data);

#endif
