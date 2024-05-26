

#include "host_tx_shenkuo.h"
#include "hal_common.h"
#include "oam_ext_if.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HOST_HAL_TX_SHENKUO_C

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
uint32_t shenkuo_host_tx_clear_msdu_padding(oal_netbuf_stru *netbuf)
{
    uint32_t msdu_len = oal_netbuf_len(netbuf);
    uint8_t padding_len = (HAL_TX_MSDU_MAX_PADDING_LEN - (msdu_len & 0x7)) & 0x7;

    if (oal_unlikely(padding_len > oal_netbuf_tailroom(netbuf) &&
        pskb_expand_head(netbuf, 0, padding_len, GFP_ATOMIC))) {
        oam_error_log3(0, 0, "{shenkuo_host_tx_clear_msdu_padding::clear failed! msdu[%d] padding[%d] tailroom[%d]}",
            msdu_len, padding_len, oal_netbuf_tailroom(netbuf));
        return OAL_FAIL;
    }

    return memset_s(oal_netbuf_data(netbuf) + msdu_len, padding_len, 0, padding_len) == EOK ? OAL_SUCC : OAL_FAIL;
}
#else
uint32_t shenkuo_host_tx_clear_msdu_padding(oal_netbuf_stru *netbuf)
{
    return OAL_SUCC;
}
#endif

#define SHENKUO_DEVICE_LARGE_TX_RING_NUM 21
#define SHENKUO_DEVICE_SMALL_TX_RING_NUM 0
#define SHENKUO_DEVICE_TX_RING_TOTAL_NUM (SHENKUO_DEVICE_LARGE_TX_RING_NUM + SHENKUO_DEVICE_SMALL_TX_RING_NUM)
uint8_t shenkuo_host_get_device_tx_ring_num(void)
{
    return SHENKUO_DEVICE_TX_RING_TOTAL_NUM;
}
