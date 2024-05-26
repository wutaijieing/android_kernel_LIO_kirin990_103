/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description:dft_channel.h
 * Create:2019.09.22
 */

#ifndef _IOMCU_DFT_CHANNEL_H_
#define _IOMCU_DFT_CHANNEL_H_

#include <linux/types.h>

/*
 * Function    : iomcu_dft_data_fetch
 * Description : request data from contexthub
 * Input       : [event_id] event id
 *               [length] data length
 * Output      : [data] output date
 * Return      : 0 OK, other error
 */
int iomcu_dft_data_fetch(u32 event_type, void *data, u32 length);

/*
 * Function    : iomcu_dft_flush
 * Description : send dft event id to contexthub
 * Input       : [event_id] event id
 * Output      : None
 * Return      : None
 */
void iomcu_dft_flush(u32 event_id);

#endif /* _IOMCU_DFT_CHANNEL_H_ */
