

#ifndef __OAM_EVENT_H__
#define __OAM_EVENT_H__

/* ����ͷ�ļ����� */
#include "oal_ext_if.h"
#include "oam_ext_if.h"

/* �궨�� */
#define OAM_OTA_DATA_TO_STD_MAX_LEN  300
#define OAM_OTA_FRAME_TO_SDT_MAX_LEN 1200
#define OAM_SKB_CB_LEN               oal_netbuf_cb_size()

/* �������� */
extern uint32_t oam_event_init(void);
extern void oam_set_ota_board_type(oam_ota_stru *ota_data);

#endif /* end of oam_event.h */
