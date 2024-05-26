/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description:dft channel.c
 * Create:2019.09.22
 */
#include "dft_channel.h"

#include <securec.h>

#include <linux/types.h>
#include <linux/errno.h>

#include <platform_include/smart/linux/base/ap/protocol.h>
#include "iomcu_ipc.h"
#include "iomcu_log.h"


/*
 * Function    : iomcu_dft_data_fetch
 * Description : request data from contexthub
 * Input       : [event_id] event id
 *               [length] data length
 * Output      : [data] output date
 * Return      : 0 OK, other error
 */
int iomcu_dft_data_fetch(u32 event_id, void *data, u32 length)
{
	struct write_info pkg_ap;
	struct read_info pkg_mcu;
	int copy_len;
	int ret;

	if (event_id > DFT_EVENT_MAX || event_id < DFT_EVENT_CPU_USE) {
		ctxhub_err("dft log fetch event_id: %d illegal\n",
			event_id);
		return -EINVAL;
	}

	(void)memset_s(&pkg_ap, sizeof(struct write_info), 0, sizeof(struct write_info));
	(void)memset_s(&pkg_mcu, sizeof(struct read_info), 0, sizeof(struct read_info));

	pkg_ap.tag = TAG_DFT;
	pkg_ap.cmd = CMD_DFT_REQUEST_DATA;
	pkg_ap.wr_buf = &event_id;
	pkg_ap.wr_len = sizeof(event_id);

	ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);
	if (ret != 0) {
		ctxhub_err("send iomcu dft fetch req type = %d fail\n", event_id);
		return -EINVAL;
	} else if (pkg_mcu.errno != 0) {
		ctxhub_err("iomcu dft fetch req to mcu fail errno = %d\n", pkg_mcu.errno);
		return -EINVAL;
	}
	copy_len = (length < sizeof(pkg_mcu.data)) ? length :
		sizeof(pkg_mcu.data);
	ret = memcpy_s(data, length, pkg_mcu.data, copy_len);
	if (ret != EOK) {
		ctxhub_err("%s memcpy_s error\n", __func__);
		return ret;
	}
	return 0;
}

/*
 * Function    : iomcu_dft_flush
 * Description : set dft cmd to contexthub
 * Input       : [event_id] event id
 * Output      : None
 * Return      : None
 */
void iomcu_dft_flush(u32 event_id)
{
	struct write_info pkg_ap;
	int ret;

	if (event_id > DFT_EVENT_MAX || event_id < DFT_EVENT_CPU_USE) {
		ctxhub_err("%s, dft event_id: %d illegal\n", __func__, event_id);
		return;
	}

	(void)memset_s(&pkg_ap, sizeof(struct write_info), 0, sizeof(struct write_info));

	pkg_ap.tag = TAG_DFT;
	pkg_ap.cmd = CMD_DFT_FLUSH;
	pkg_ap.wr_buf = &event_id;
	pkg_ap.wr_len = sizeof(event_id);

	ret = write_customize_cmd(&pkg_ap, NULL, false);
	if (ret != 0)
		ctxhub_err("%s set cmd failed, ret = %d\n", __func__, ret);
}

