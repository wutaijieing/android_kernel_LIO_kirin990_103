

#ifndef __HOST_HAL_RING_H__
#define __HOST_HAL_RING_H__


#include "oal_types.h"
#include "host_hal_device.h"


/* �ж�ring�Ƿ�Ϊ���������������TRUE;��֮������FALSE */
static inline bool hal_ring_is_full(hal_host_ring_ctl_stru *ring_ctl)
{
    return (((ring_ctl)->un_write_ptr.st_write_ptr.bit_write_ptr == \
        (ring_ctl)->un_read_ptr.st_read_ptr.bit_read_ptr) && \
        ((ring_ctl)->un_write_ptr.st_write_ptr.bit_wrap_flag != (ring_ctl)->un_read_ptr.st_read_ptr.bit_wrap_flag));
}

/* �ж�ring�Ƿ�Ϊ�գ�����գ�����TRUE;��֮������FALSE */
static inline bool hal_ring_is_empty(hal_host_ring_ctl_stru *ring_ctl)
{
    return ((ring_ctl)->un_write_ptr.write_ptr == (ring_ctl)->un_read_ptr.read_ptr);
}

/* �ж�ring�Ƿ�ת�������ת������TRUE;��֮������FALSE */
static inline bool hal_ring_wrap_around(hal_host_ring_ctl_stru *ring_ctl)
{
    return ((ring_ctl)->un_read_ptr.st_read_ptr.bit_wrap_flag != (ring_ctl)->un_write_ptr.st_write_ptr.bit_wrap_flag);
}


typedef enum {
    HAL_RING_TYPE_INVALID,
    /* Free ring: �������write ptr, HW����read ptr */
    HAL_RING_TYPE_FREE_RING,
    /* Complete ring: �������read ptr, HW����write ptr */
    HAL_RING_TYPE_COMPLETE_RING,

    HAL_RING_TYPE_BUTT
} hal_ring_type_enum;

uint32_t hal_ring_init(hal_host_ring_ctl_stru *ring_ctl);
void hal_ring_get_hw2sw(hal_host_ring_ctl_stru *ring_ctl);
uint32_t hal_ring_set_sw2hw(hal_host_ring_ctl_stru *ring_ctl);
uint32_t hal_ring_get_entry_count(hal_host_ring_ctl_stru *ring_ctl, uint16_t *p_count);
uint32_t hal_comp_ring_wait_valid(hal_host_ring_ctl_stru *ring_ctl);
uint32_t hal_ring_get_entries(hal_host_ring_ctl_stru *ring_ctl, uint8_t *entries, uint16_t count);
uint32_t hal_ring_set_entries(hal_host_ring_ctl_stru *ring_ctl, uint64_t entry);
void hmac_update_free_ring_wptr(hal_host_ring_ctl_stru *ring_ctl, uint16_t count);
void hal_host_ring_tx_deinit(hal_host_device_stru *hal_device);

#endif /* __HAL_RING_H__ */
