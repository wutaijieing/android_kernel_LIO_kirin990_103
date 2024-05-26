

#ifndef __HMAC_SOFT_RETRY_H__
#define __HMAC_SOFT_RETRY_H__

#include "hmac_user.h"
#include "hmac_tx_complete.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SOFT_RETRY_H

uint32_t hmac_tx_soft_retry_process(hmac_tid_info_stru *tx_tid_info, oal_netbuf_stru *netbuf);

#endif
