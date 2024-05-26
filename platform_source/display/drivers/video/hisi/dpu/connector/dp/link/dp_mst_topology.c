/*
 * Copyright (c) 2016 Synopsys, Inc.
 *
 * Synopsys DP TX Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTerrUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/*
 * Copyright (c) 2017 Hisilicon Tech. Co., Ltd. Integrated into the Hisilicon display system.
 */

#include "dp_mst_topology.h"
#include "controller/dp_core_interface.h"
#include "dp_edid.h"
#include "hisi_drv_dp.h"

/*lint -save -e* */
#define MST_MSG_BUF_LENGTH 256

struct mst_msg_info g_mst_msg;
static int dptx_aux_msg_allocate_payload(struct dp_ctrl *dptx, uint8_t port, uint8_t vcpid,
	uint16_t pbn, int port1);
static int dptx_sideband_get_down_rep(struct dp_ctrl *dptx, uint8_t request_id);

static void dptx_set_vcpid_table_range(struct dp_ctrl *dptx, uint32_t start, uint32_t count, uint32_t stream)
{
	int i;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");
	disp_check_and_no_retval(((start + count) > 64), err, "[DP] Invalid slot number > 63!\n");

	for (i = 0; i < count; i++) {
		disp_pr_debug("[DP] setting slot %d for stream %d\n",
			start + i, stream);
		dptx_set_vcpid_table_slot(dptx, start + i, stream);
	}
}

static void dptx_dpcd_clear_vcpid_table(struct dp_ctrl *dptx)
{
	uint8_t bytes[] = { 0x00, 0x00, 0x3f, };
	uint8_t status;
	int count = 0;
	int retval;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	retval = dptx_read_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
	if (retval)
		disp_pr_err("[DP] Read DPCD error\n");

	retval = dptx_write_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, DP_PAYLOAD_TABLE_UPDATED);
	if (retval)
		disp_pr_err("[DP] Write DPCD error\n");

	retval = dptx_read_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
	if (retval)
		disp_pr_err("[DP] Read DPCD error\n");

	retval = dptx_write_bytes_to_dpcd(dptx, DP_PAYLOAD_ALLOCATE_SET, bytes, 3);
	if (retval)
		disp_pr_err("[DP] dptx_write_bytes_to_dpcd DPCD error\n");

	status = 0;
	while (!(status & DP_PAYLOAD_TABLE_UPDATED)) {
		retval = dptx_read_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
		if (retval)
			disp_pr_err("[DP] Read DPCD error\n");

		count++;
		if (WARN(count > 2000, "Timeout waiting for DPCD VCPID table update\n"))
			break;

		udelay(1000);
	}

	retval = dptx_write_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, DP_PAYLOAD_TABLE_UPDATED);
	if (retval)
		disp_pr_err("[DP] Write DPCD error\n");

	retval = dptx_read_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
	if (retval)
		disp_pr_err("[DP] Read DPCD error\n");
}

static void dptx_dpcd_set_vcpid_table(struct dp_ctrl *dptx, uint32_t start, uint32_t count, uint32_t stream)
{
	uint8_t bytes[3];
	uint8_t status = 0;
	int tries = 0;
	int retval;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	bytes[0] = stream;
	bytes[1] = start;
	bytes[2] = count;

	dptx_write_bytes_to_dpcd(dptx, DP_PAYLOAD_ALLOCATE_SET, bytes, 3);

	while (!(status & DP_PAYLOAD_TABLE_UPDATED)) {
		retval = dptx_read_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
		if (retval)
			disp_pr_err("[DP] Read DPCD error\n");

		tries++;
		if (WARN(tries > 2000, "Timeout waiting for DPCD VCPID table update\n"))
			break;
	}

	retval = dptx_write_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, DP_PAYLOAD_TABLE_UPDATED);
	if (retval)
		disp_pr_err("[DP] Write DPCD error\n");

	retval = dptx_read_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
	if (retval)
		disp_pr_err("[DP] Read DPCD error\n");
}

static int print_buf(uint8_t *buf, int len)
{
	int i;
	uint32_t print_buf_size = 1024;
	char str[print_buf_size];
	int written = 0;

	disp_check_and_return((buf == NULL), -EINVAL, err, "[DP] buf is NULL!\n");

	written += snprintf(&str[written], print_buf_size - written, "Buffer:");

	for (i = 0; i < len; i++) {
		if (!(i % 16)) {
			written += snprintf(&str[written],
					    print_buf_size - written,
					    "\n%04x:", i);

			if (written >= print_buf_size)
				break;

		}

		written += snprintf(&str[written],
				    print_buf_size - written,
				    " %02x", buf[i]);

		if (written >= print_buf_size)
			break;
	}

	disp_pr_debug("%s\n\n", str);

	return 0;
}

static void dptx_print_vcpid_table(struct dp_ctrl *dptx)
{
	uint8_t bytes[64] = { 0, };
	int retval;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	retval = dptx_read_bytes_from_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, bytes, 64);
	if (retval)
		disp_pr_err("failed to dptx_read_bytes_from_dpcd Binfo, retval=%d.\n", retval);

	print_buf(bytes, 64);
}

static uint8_t drm_dp_msg_header_crc4(const uint8_t *data, size_t num_nibbles)
{
	uint8_t bitmask = 0x80;
	uint8_t bitshift = 7;
	uint8_t array_index = 0;
	int number_of_bits = num_nibbles * 4;
	uint8_t remainder = 0;

	if (data == NULL) {
		disp_pr_err("[DP] NULL Pointer\n");
		return  0;
	}

	while (number_of_bits != 0) {
		number_of_bits--;
		remainder <<= 1;
		remainder |= (data[array_index] & bitmask) >> bitshift;
		bitmask >>= 1;
		bitshift--;
		if (bitmask == 0) {
			bitmask = 0x80;
			bitshift = 7;
			array_index++;
		}
		if ((remainder & 0x10) == 0x10)
			remainder ^= 0x13;
	}

	number_of_bits = 4;
	while (number_of_bits != 0) {
		number_of_bits--;
		remainder <<= 1;
		if ((remainder & 0x10) != 0)
			remainder ^= 0x13;
	}

	return remainder;
}

static uint8_t drm_dp_msg_data_crc4(const uint8_t *data, uint8_t number_of_bytes)
{
	uint8_t bitmask = 0x80;
	uint8_t bitshift = 7;
	uint8_t array_index = 0;
	int number_of_bits = number_of_bytes * 8;
	uint16_t remainder = 0;

	disp_check_and_return((data == NULL), -EINVAL, err, "[DP] data is NULL!\n");

	while (number_of_bits != 0) {
		number_of_bits--;
		remainder <<= 1;
		remainder |= (data[array_index] & bitmask) >> bitshift;
		bitmask >>= 1;
		bitshift--;
		if (bitmask == 0) {
			bitmask = 0x80;
			bitshift = 7;
			array_index++;
		}
		if ((remainder & 0x100) == 0x100)
			remainder ^= 0xd5;
	}

	number_of_bits = 8;
	while (number_of_bits != 0) {
		number_of_bits--;
		remainder <<= 1;
		if ((remainder & 0x100) != 0)
			remainder ^= 0xd5;
	}

	return remainder & 0xff;
}

static void drm_dp_encode_sideband_msg_hdr(struct drm_dp_sideband_msg_hdr *hdr, uint8_t *buf, int *len)
{
	int idx = 0;
	int i;
	uint8_t crc4;

	disp_check_and_no_retval((hdr == NULL), err, "[DP] hdr is NULL!\n");

	buf[idx++] = ((hdr->lct & 0xf) << 4) | (hdr->lcr & 0xf);
	for (i = 0; i < (hdr->lct / 2); i++)
		buf[idx++] = hdr->rad[i];
	buf[idx++] = ((uint32_t)hdr->broadcast << 7) | ((uint32_t)hdr->path_msg << 6) |
		(hdr->msg_len & 0x3f);
	buf[idx++] = ((uint32_t)hdr->somt << 7) | ((uint32_t)hdr->eomt << 6) | ((uint32_t)hdr->seqno << 4);

	crc4 = drm_dp_msg_header_crc4(buf, (idx * 2) - 1);
	buf[idx - 1] |= (crc4 & 0xf);

	*len = idx;
}

static void drm_dp_crc_sideband_chunk_req(uint8_t *msg, uint8_t len)
{
	uint8_t crc4;

	disp_check_and_no_retval((msg == NULL), err, "[DP] msg is NULL!\n");

	crc4 = drm_dp_msg_data_crc4(msg, len);
	msg[len] = crc4;
}

static bool drm_dp_decode_sideband_msg_hdr(struct drm_dp_sideband_msg_hdr *hdr, uint8_t *buf,
	int buflen, uint8_t *hdrlen)
{
	uint8_t crc4;
	uint8_t len;
	int i;
	uint8_t idx;

	disp_check_and_return((hdr == NULL), false, err, "[DP] hdr is NULL!\n");

	if (buf[0] == 0)
		return false;

	len = 3;
	len += ((buf[0] & 0xf0) >> 4) / 2;
	if (len > buflen)
		return false;
	crc4 = drm_dp_msg_header_crc4(buf, (len * 2) - 1);

	if ((crc4 & 0xf) != (buf[len - 1] & 0xf)) {
		/* DRM_DEBUG_KMS("crc4 mismatch 0x%x 0x%x\n", crc4, buf[len - 1]); */
		return false;
	}

	hdr->lct = (buf[0] & 0xf0) >> 4;
	hdr->lcr = (buf[0] & 0xf);
	idx = 1;
	for (i = 0; i < (hdr->lct / 2); i++)
		hdr->rad[i] = buf[idx++];
	hdr->broadcast = (buf[idx] >> 7) & 0x1;
	hdr->path_msg = (buf[idx] >> 6) & 0x1;
	hdr->msg_len = buf[idx] & 0x3f;
	idx++;
	hdr->somt = (buf[idx] >> 7) & 0x1;
	hdr->eomt = (buf[idx] >> 6) & 0x1;
	hdr->seqno = (buf[idx] >> 4) & 0x1;
	idx++;
	*hdrlen = idx;
	return true;
}

static char const *dptx_sideband_header_rad_string(struct drm_dp_sideband_msg_hdr *header)
{
	disp_check_and_return((header == NULL), "none", err, "[DP] header is NULL!\n");

	if (header->lct >= 1)
		return "TODO";

	return "none";
}

static void dptx_print_sideband_header(struct dp_ctrl *dptx, struct drm_dp_sideband_msg_hdr *header)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");
	disp_check_and_no_retval((header == NULL), err, "[DP] header is NULL!\n");

	disp_pr_info(
		"[DP] SIDEBAND_MSG_HEADER: lct=%d, lcr=%d, rad=%s, bcast=%d, path=%d, msglen=%d, somt=%d, eomt=%d, seqno=%d\n",
		 header->lct, header->lcr, dptx_sideband_header_rad_string(header),
		 header->broadcast, header->path_msg, header->msg_len,
		 header->somt, header->eomt, header->seqno);
}

static void drm_dp_sideband_parse_connection_status_notify(struct drm_dp_sideband_msg_rx *raw,
	struct drm_dp_sideband_msg_req_body *msg)
{
	int idx = 1;

	disp_check_and_no_retval((raw == NULL), err, "[DP] raw is NULL!\n");
	disp_check_and_no_retval((msg == NULL), err, "[DP] msg is NULL!\n");

	msg->u.conn_stat.port_number = (raw->msg[idx] & 0xf0) >> 4;
	idx++;

	memcpy(msg->u.conn_stat.guid, &raw->msg[idx], 16);
	idx += 16;

	msg->u.conn_stat.legacy_device_plug_status = (raw->msg[idx] >> 6) & 0x1;
	msg->u.conn_stat.displayport_device_plug_status = (raw->msg[idx] >> 5) & 0x1;
	msg->u.conn_stat.message_capability_status = (raw->msg[idx] >> 4) & 0x1;
	msg->u.conn_stat.input_port = (raw->msg[idx] >> 3) & 0x1;
	msg->u.conn_stat.peer_device_type = (raw->msg[idx] & 0x7);
	idx++;
}

static int dptx_remote_i2c_read(struct dp_ctrl *dptx, uint8_t port, int port1)
{
	uint8_t buf[MST_MSG_BUF_LENGTH] = {0};
	int len = MST_MSG_BUF_LENGTH;
	int retval = 0;
	int edidblocknum = 0;
	uint8_t *msg = NULL;

	struct drm_dp_sideband_msg_hdr header = {
		.lct = 1,
		.lcr = 0,
		.rad = { 0, },
		.broadcast = false,
		.path_msg = 0,
		.msg_len = 9,
		.somt = 1,
		.eomt = 1,
		.seqno = 0,
	};

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	if ((port1 >= 0) && dptx->need_rad) {
		header.lct = 2;
		header.lcr = 1;
		header.rad[0] |= ((dptx->rad_port << 4) & 0xf0);
		header.rad[0] |= (0 & 0xf);
	}

	drm_dp_encode_sideband_msg_hdr(&header, buf, &len);

	print_buf(buf, len);

	if (len < (MST_MSG_BUF_LENGTH - 9)) {
		msg = &buf[len];
		msg[0] = DP_REMOTE_I2C_READ;
		msg[1] = ((port & 0xf) << 4);
		msg[1] |= 1 & 0x3;
		msg[2] = 0x50 & 0x7f;
		msg[3] = 1;
		msg[4] = 0 << 5;
		msg[5] = 0 & 0xf;
		msg[6] = 0x50 & 0x7f;
		msg[7] = 0x80;

		drm_dp_crc_sideband_chunk_req(msg, 8);

		len += 9;

		disp_pr_info("[DP] +\n");
		dptx_write_bytes_to_dpcd(dptx, DP_SIDEBAND_MSG_DOWN_REQ_BASE, buf, len);

		retval = dptx_sideband_get_down_rep(dptx, DP_REMOTE_I2C_READ);
		if (retval)
			return retval;

		disp_pr_info(" msg length = %d\n", g_mst_msg.msg_len);

		edidblocknum = ((g_mst_msg.msg_len - 3) / DPTX_DEFAULT_EDID_BUFLEN);

		if (edidblocknum == 1) {
			memcpy(dptx->edid, &(g_mst_msg.msg[3]), DPTX_DEFAULT_EDID_BUFLEN);
		} else if (edidblocknum == 2) {
			kfree(dptx->edid);
			dptx->edid = NULL;

			dptx->edid = kzalloc(EDID_BLOCK_LENGTH * 2, GFP_KERNEL);
			if (!dptx->edid) {
				disp_pr_err("[DP] Allocate edid buffer error!\n");
				return -EINVAL;
			}
			memcpy(dptx->edid, &(g_mst_msg.msg[3]), DPTX_DEFAULT_EDID_BUFLEN * 2);
		} else {
			disp_pr_err("[DP] MST get EDID fail\n");
		}

		release_edid_info(dptx);
		memset(&(dptx->edid_info), 0, sizeof(struct edid_information));

		retval = parse_edid(dptx, DPTX_DEFAULT_EDID_BUFLEN * edidblocknum);
		if (retval)
			disp_pr_err("[DP] EDID Parser fail, display safe mode\n");
	} else {
		return -EINVAL;
	}
	return 0;
}

static int dptx_enum_path_resources(struct dp_ctrl *dptx, uint8_t port, int port1)
{
	uint8_t buf[256] = {0};
	int len = 256;
	int retval = 0;
	uint8_t *msg = NULL;

	struct drm_dp_sideband_msg_hdr header = {
		.lct = 1,
		.lcr = 0,
		.rad = { 0, },
		.broadcast = false,
		.path_msg = 0,
		.msg_len = 3,
		.somt = 1,
		.eomt = 1,
		.seqno = 0,
	};

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	if ((port1 >= 0) && dptx->need_rad) {
		header.lct = 2;
		header.lcr = 1;
		header.rad[0] |= ((dptx->rad_port << 4) & 0xf0);
		header.rad[0] |= (0 & 0xf);
	}

	drm_dp_encode_sideband_msg_hdr(&header, buf, &len);

	print_buf(buf, len);

	if (len < (MST_MSG_BUF_LENGTH - 3)) {
		msg = &buf[len];
		msg[0] = DP_ENUM_PATH_RESOURCES;
		msg[1] = ((port & 0xf) << 4);

		drm_dp_crc_sideband_chunk_req(msg, 2);

		len += 3;

		disp_pr_debug("Sending DOWN_REQ\n");
		retval = dptx_write_bytes_to_dpcd(dptx, DP_SIDEBAND_MSG_DOWN_REQ_BASE, buf, len);
		if (retval)
			disp_pr_err("[DP] dptx_write_bytes_to_dpcd fail\n");

		retval = dptx_sideband_get_down_rep(dptx, DP_ENUM_PATH_RESOURCES);
		if (retval)
			return retval;
	} else {
		return -EINVAL;
	}

	return 0;
}


int dptx_sideband_get_up_req(struct dp_ctrl *dptx)
{
	struct drm_dp_sideband_msg_hdr header;
	struct drm_dp_sideband_msg_rx raw;
	struct drm_dp_sideband_msg_req_body req_msg;

	uint8_t buf[256] = { 0 };
	uint8_t header_len;
	int retval;
	int first = 1;
	uint8_t msg[1024];
	uint8_t msg_len;
	int retries = 0;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	memset(&raw, 0, sizeof(raw));
	memset(&req_msg, 0, sizeof(req_msg));

again:
	memset(msg, 0, sizeof(msg));
	msg_len = 0;

	while (1) {
		retval = dptx_read_bytes_from_dpcd(dptx, DP_SIDEBAND_MSG_UP_REQ_BASE, buf, 256);
		if (retval) {
			disp_pr_err("[DP] Error reading up req %d\n", retval);
			return retval;
		}

		if (!drm_dp_decode_sideband_msg_hdr(&header, buf, 256, &header_len)) {
			disp_pr_err("[DP] Error decoding sideband header %d\n", retval);
			return -EINVAL;
		}

		dptx_print_sideband_header(dptx, &header);

		header.msg_len -= 1;
		memcpy(&msg[msg_len], &buf[header_len], header.msg_len);
		msg_len += header.msg_len;

		if (first && !header.somt) {
			disp_pr_err("[DP] SOMT not set\n");
			return -EINVAL;
		}
		first = 0;

		retval = dptx_write_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, DP_UP_REQ_MSG_RDY);
		if (retval)
			disp_pr_err("[DP] dptx_write_dpcd fail\n");

		if (header.eomt)
			break;
	}

	print_buf(msg, msg_len);
	if ((msg[0] & 0x7f) != DP_CONNECTION_STATUS_NOTIFY) {
		if (retries < 3) {
			disp_pr_err("[DP] request_id %d does not match expected %d, retrying\n",
				msg[0] & 0x7f, DP_UP_REQ_MSG_RDY);
			retries++;
			goto again;
		} else {
			disp_pr_err("[DP] request_id %d does not match expected %d, giving up\n",
				msg[0] & 0x7f, DP_UP_REQ_MSG_RDY);
			return -EINVAL;
		}
	}

	memcpy(raw.msg, &msg, msg_len);

	print_buf(buf, 256);
	print_buf(raw.msg, 256);
	drm_dp_sideband_parse_connection_status_notify(&raw, &req_msg);

	print_buf(msg, msg_len);
	if (req_msg.u.conn_stat.displayport_device_plug_status) {
		dptx->vcp_id++;
		dptx_aux_msg_allocate_payload(dptx, req_msg.u.conn_stat.port_number, dptx->vcp_id, dptx->pbn, -1);
	} else {
		dptx->vcp_id--;
	}

	return 0;
}

static int dptx_wait_down_rep(struct dp_ctrl *dptx)
{
	int count = 0;
	uint8_t vector = 0;
	int retval;
	uint8_t bytes[1] = {0};

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	while (1) {
		retval = dptx_read_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, &vector);
		if (retval)
			disp_pr_err("dptx_read_dpcd fail\n");

		retval = dptx_read_bytes_from_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR_ESI0, bytes, sizeof(bytes));
		if (retval)
			disp_pr_err("dptx_read_bytes_from_dpcd fail\n");

		if ((vector & DP_DOWN_REP_MSG_RDY) || (bytes[0] & DP_DOWN_REP_MSG_RDY)) {
			disp_pr_info("[DP] vector set\n");
			break;
		}

		count++;
		if (count > 1000) {
			disp_pr_err("[DP] Timed out\n");
			return -ETIMEDOUT;
		}

		udelay(100);
	}

	return 0;
}

static int dptx_clear_down_rep(struct dp_ctrl *dptx)
{
	int count = 0;
	uint8_t vector = 0;
	uint8_t bytes[1] = { 0 };
	int retval;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	while (1) {
		retval = dptx_read_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, &vector);
		if (retval)
			disp_pr_err("dptx_read_dpcd fail\n");

		retval = dptx_read_bytes_from_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR_ESI0, bytes, sizeof(bytes));
		if (retval)
			disp_pr_err("dptx_read_bytes_from_dpcd fail\n");

		if (!(vector & DP_DOWN_REP_MSG_RDY || bytes[0] & DP_DOWN_REP_MSG_RDY)) {
			disp_pr_info("[DP] vector clear\n");
			break;
		}

		retval = dptx_write_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, DP_DOWN_REP_MSG_RDY);
		if (retval)
			disp_pr_err("dptx_write_dpcd fail\n");

		retval = dptx_write_bytes_to_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR_ESI0, bytes, sizeof(bytes));
		if (retval)
			disp_pr_err("dptx_write_bytes_to_dpcd fail\n");

		count++;
		if (count > 2000) {
			disp_pr_err("[DP] Timed out\n");
			return -ETIMEDOUT;
		}

		udelay(1000);
	}

	return 0;
}

static int dptx_sideband_get_down_rep(struct dp_ctrl *dptx, uint8_t request_id)
{
	struct drm_dp_sideband_msg_hdr header;
	uint8_t buf[MST_MSG_BUF_LENGTH] = {0};
	uint8_t header_len;
	int retval;
	int first = 1;
	int retries = 0;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

again:
	memset(&g_mst_msg, 0x0, sizeof(g_mst_msg));

	while (1) {
		retval = dptx_wait_down_rep(dptx);
		if (retval) {
			disp_pr_err("[DP] Error waiting down rep %d\n", retval);
			return retval;
		}

		retval = dptx_read_bytes_from_dpcd(dptx, DP_SIDEBAND_MSG_DOWN_REP_BASE, buf, MST_MSG_BUF_LENGTH);
		if (retval) {
			disp_pr_err("[DP] Error reading down rep %d\n", retval);
			return retval;
		}

		if (!drm_dp_decode_sideband_msg_hdr(&header, buf, MST_MSG_BUF_LENGTH, &header_len)) {
			disp_pr_err("[DP] Error decoding sideband header %d\n", retval);
			return -EINVAL;
		}

		dptx_print_sideband_header(dptx, &header);

		/* check sideband msg body crc */
		header.msg_len -= 1;
		memcpy(&g_mst_msg.msg[g_mst_msg.msg_len], &buf[header_len], header.msg_len);
		g_mst_msg.msg_len += header.msg_len;

		if (first && !header.somt) {
			disp_pr_err("[DP] SOMT not set\n");
			return -EINVAL;
		}
		first = 0;

		retval = dptx_write_dpcd(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, DP_DOWN_REP_MSG_RDY);
		if (retval)
			disp_pr_err("[DP] dptx_write_dpcd fail");

		if (header.eomt)
			break;
	}

	print_buf(g_mst_msg.msg, g_mst_msg.msg_len);
	if ((g_mst_msg.msg[0] & 0x7f) != request_id) {
		if (retries < 3) {
			disp_pr_err("[DP] request_id %d does not match expected %d, retrying\n",
				g_mst_msg.msg[0] & 0x7f, request_id);
			retries++;
			goto again;
		} else {
			disp_pr_err("[DP] request_id %d does not match expected %d, giving up\n",
				g_mst_msg.msg[0] & 0x7f, request_id);
			return -EINVAL;
		}
	}

	retval = dptx_clear_down_rep(dptx);
	if (retval) {
		disp_pr_err("[DP] Error waiting down rep clear %d\n", retval);
		return retval;
	}

	return 0;
}

static int dptx_aux_msg_clear_payload_id_table(struct dp_ctrl *dptx)
{
	uint8_t buf[MST_MSG_BUF_LENGTH];
	int len = 0;
	int retval = 0;
	uint8_t *msg = NULL;

	struct drm_dp_sideband_msg_hdr header = {
		.lct = 1,
		.lcr = 6,
		.rad = { 0, },
		.broadcast = true,
		.path_msg = 1,
		.msg_len = 2,
		.somt = 1,
		.eomt = 1,
		.seqno = 0,
	};

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	drm_dp_encode_sideband_msg_hdr(&header, buf, &len);

	if (len < (MST_MSG_BUF_LENGTH - 2)) {
		msg = &buf[len];
		msg[0] = DP_CLEAR_PAYLOAD_ID_TABLE;
		drm_dp_crc_sideband_chunk_req(msg, 1);

		len += 2;

		disp_pr_debug("Sending DOWN_REQ\n");
		dptx_write_bytes_to_dpcd(dptx, DP_SIDEBAND_MSG_DOWN_REQ_BASE,
					 buf, len);

		retval = dptx_sideband_get_down_rep(dptx, DP_CLEAR_PAYLOAD_ID_TABLE);
		if (retval)
			return retval;
	} else {
		return -EINVAL;
	}

	return 0;
}

static bool drm_dp_sideband_parse_link_address(struct drm_dp_sideband_msg_rx *raw,
	struct drm_dp_sideband_msg_reply_body *repmsg)
{
	int idx = 1;
	int i;

	memcpy(repmsg->u.link_addr.guid, &raw->msg[idx], 16);
	idx += 16;
	repmsg->u.link_addr.nports = raw->msg[idx] & 0xf;
	idx++;

	if (idx > raw->curlen)
		goto fail_len;

	for (i = 0; i < repmsg->u.link_addr.nports; i++) {
		if (raw->msg[idx] & 0x80)
			repmsg->u.link_addr.ports[i].input_port = 1;

		repmsg->u.link_addr.ports[i].peer_device_type = (raw->msg[idx] >> 4) & 0x7;
		repmsg->u.link_addr.ports[i].port_number = (raw->msg[idx] & 0xf);

		idx++;
		if (idx > raw->curlen)
			goto fail_len;
		repmsg->u.link_addr.ports[i].mcs = (raw->msg[idx] >> 7) & 0x1;
		repmsg->u.link_addr.ports[i].ddps = (raw->msg[idx] >> 6) & 0x1;
		if (repmsg->u.link_addr.ports[i].input_port == 0)
			repmsg->u.link_addr.ports[i].legacy_device_plug_status = (raw->msg[idx] >> 5) & 0x1;
		idx++;
		if (idx > raw->curlen)
			goto fail_len;
		if (repmsg->u.link_addr.ports[i].input_port == 0) {
			repmsg->u.link_addr.ports[i].dpcd_revision = (raw->msg[idx]);
			idx++;
			if (idx > raw->curlen)
				goto fail_len;
			memcpy(repmsg->u.link_addr.ports[i].peer_guid, &raw->msg[idx], 16);
			idx += 16;
			if (idx > raw->curlen)
				goto fail_len;
			repmsg->u.link_addr.ports[i].num_sdp_streams = (raw->msg[idx] >> 4) & 0xf;
			repmsg->u.link_addr.ports[i].num_sdp_stream_sinks = (raw->msg[idx] & 0xf);
			idx++;

		}
		if (idx > raw->curlen)
			goto fail_len;
	}

	return true;
fail_len:
	disp_pr_err("link address reply parse length fail %d %d\n", idx, raw->curlen);
	return false;
}

static int dptx_aux_msg_link_address(struct dp_ctrl *dptx, struct drm_dp_sideband_msg_rx *raw,
	struct drm_dp_sideband_msg_reply_body *rep, int port)
{
	uint8_t buf[MST_MSG_BUF_LENGTH];
	int len = 0;
	uint8_t *msg = NULL;
	int retval = 0;

	struct drm_dp_sideband_msg_hdr header = {
		.lct = 1,
		.lcr = 0,
		.rad = { 0, },
		.broadcast = false,
		.path_msg = 0,
		.msg_len = 2,
		.somt = 1,
		.eomt = 1,
		.seqno = 0,
	};

	if (port >= 0) {
		header.lct = 2;
		header.lcr = 1;
		header.rad[0] |= (((uint32_t)port << 4) & 0xf0);
		header.rad[0] |= (0 & 0xf);
	}

	drm_dp_encode_sideband_msg_hdr(&header, buf, &len);

	if (len < (MST_MSG_BUF_LENGTH - 2)) {
		msg = &buf[len];
		msg[0] = DP_LINK_ADDRESS;

		drm_dp_crc_sideband_chunk_req(msg, 1);

		len += 2;
		print_buf(buf, len);
		disp_pr_debug("Sending DOWN_REQ\n");
		dptx_write_bytes_to_dpcd(dptx, DP_SIDEBAND_MSG_DOWN_REQ_BASE,
			buf, len);

		retval = dptx_sideband_get_down_rep(dptx, DP_LINK_ADDRESS);
		if (retval)
			return retval;

		memcpy(raw->msg, g_mst_msg.msg, g_mst_msg.msg_len);
		raw->curlen = g_mst_msg.msg_len;
		disp_pr_debug("rawlen = %d\n", len);

		print_buf(raw->msg, raw->curlen);
		drm_dp_sideband_parse_link_address(raw, rep);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int dptx_aux_msg_allocate_payload(struct dp_ctrl *dptx, uint8_t port,
	uint8_t vcpid, uint16_t pbn, int port1)
{
	uint8_t buf[MST_MSG_BUF_LENGTH] = {0};
	int len = 0;
	uint8_t *msg = NULL;
	int retval = 0;

	struct drm_dp_sideband_msg_hdr header = {
		.lct = 1,
		.lcr = 0,
		.rad = { 0, },
		.broadcast = false,
		.path_msg = 1,
		.msg_len = 6,
		.somt = 1,
		.eomt = 1,
		.seqno = 0,
	};

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	disp_pr_debug("PBN=%d\n", pbn);
	if ((port1 >= 0) && dptx->need_rad) {
		header.lct = 2;
		header.lcr = 1;
		header.rad[0] |= ((dptx->rad_port << 4) & 0xf0);
		header.rad[0] |= (0 & 0xf);
	}

	drm_dp_encode_sideband_msg_hdr(&header, buf, &len);

	print_buf(buf, len);
	if (len < (MST_MSG_BUF_LENGTH - 6)) {
		msg = &buf[len];
		msg[0] = DP_ALLOCATE_PAYLOAD;
		msg[1] = ((port & 0xf) << 4);
		msg[2] = vcpid & 0x7f;
		msg[3] = pbn >> 8;
		msg[4] = pbn & 0xff;
		drm_dp_crc_sideband_chunk_req(msg, 5);

		len += 6;

		disp_pr_debug("Sending DOWN_REQ\n");
		dptx_write_bytes_to_dpcd(dptx, DP_SIDEBAND_MSG_DOWN_REQ_BASE,
			buf, len);

		retval = dptx_sideband_get_down_rep(dptx, DP_ALLOCATE_PAYLOAD);
		if (retval)
			return retval;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int dptx_calc_num_slots(struct dp_ctrl *dptx, int pbn, uint32_t *slots)
{
	int div;
	int dp_link_bw;
	int dp_link_count;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((slots == NULL), -EINVAL, err, "[DP] slots is NULL!\n");

	dp_link_bw = dptx_phy_rate_to_bw(dptx->link.rate);
	dp_link_count = dptx->link.lanes;

	switch (dp_link_bw) {
	case DP_LINK_BW_1_62:
		div = 3 * dp_link_count;
		break;
	case DP_LINK_BW_2_7:
		div = 5 * dp_link_count;
		break;
	case DP_LINK_BW_5_4:
		div = 10 * dp_link_count;
		break;
	case DP_LINK_BW_8_1:
		div = 15 * dp_link_count;
		break;
	default:
		return 0;
	}

	*slots = DIV_ROUND_UP(pbn, div);

	return 0;
}

int dptx_mst_initial(struct dp_ctrl *dptx)
{
	uint8_t byte = 0;
	int retval;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	if (dptx->mst) {
		retval = dptx_read_dpcd(dptx, DP_MSTM_CAP, &byte);
		if (retval)
			return retval;

		if (byte & DP_MST_CAP) {
			dptx->mst = true;
			/* this code will lead aux transfer fail. aux nack!! */
			retval = dptx_write_dpcd(dptx, DP_MSTM_CTRL, 0x07);
			if (retval)
				return retval;
		} else {
			dptx->mst = false;
		}
	} else {
		retval = dptx_read_dpcd(dptx, DP_MSTM_CAP, &byte);
		if (retval)
			return retval;

		if (byte & DP_MST_CAP) {
			retval = dptx_write_dpcd(dptx, DP_MSTM_CTRL, 0x00);
			if (retval)
				return retval;
		}
	}

	/* Ensure streams = 1 if no MST */
	if (!dptx->mst) {
		dptx->streams = 1;
	} else {
		dptx->streams = 2;
		memset(&g_mst_msg, 0x0, sizeof(g_mst_msg));
	}

	if (dptx->mst)
		dptx_mst_enable(dptx);

	return 0;
}

int dptx_get_topology(struct dp_ctrl *dptx)
{
	int retval;
	int i;
	int port_count = 0;
	struct drm_dp_sideband_msg_rx raw;
	struct drm_dp_sideband_msg_reply_body rep;

	memset(&raw, 0, sizeof(raw));
	memset(&rep, 0, sizeof(rep));

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	dptx->need_rad = false;

	retval = dptx_aux_msg_clear_payload_id_table(dptx);
	if (retval)
		return retval;

	/* LINK_ADDRESS FIRST MONITOR */
	disp_pr_debug("--------------- Sending link_address 0\n");
	retval = dptx_aux_msg_link_address(dptx, &raw, &rep, -1);
	if (retval)
		return retval;

	disp_pr_debug("NPORTS = %d\n", rep.u.link_addr.nports);
	for (i = 0; i < rep.u.link_addr.nports; i++)
		disp_pr_debug("input=%d, pdt=%d, pnum=%d, mcs=%d, ldps = %d, ddps=%d\n", __func__,
			rep.u.link_addr.ports[i].input_port,
			rep.u.link_addr.ports[i].peer_device_type,
			rep.u.link_addr.ports[i].port_number,
			rep.u.link_addr.ports[i].mcs,
			rep.u.link_addr.ports[i].legacy_device_plug_status,
			rep.u.link_addr.ports[i].ddps);

	huawei_dp_imonitor_set_param(DP_PARAM_TIME_START, NULL);

	for (i = 0; i < rep.u.link_addr.nports; i++) {
		if (rep.u.link_addr.ports[i].peer_device_type == 3 &&
			!rep.u.link_addr.ports[i].mcs && rep.u.link_addr.ports[i].ddps && port_count < 2) {
			dptx->logic_port = true;
			dptx->port[port_count] = rep.u.link_addr.ports[i].port_number;
			port_count++;
		}

		if (rep.u.link_addr.ports[i].peer_device_type == 2 &&
			rep.u.link_addr.ports[i].mcs && rep.u.link_addr.ports[i].ddps) {
			dptx->need_rad = true;
			dptx->rad_port = rep.u.link_addr.ports[i].port_number;

			retval = dptx_aux_msg_link_address(dptx, &raw, &rep, rep.u.link_addr.ports[i].port_number);
			if (retval)
				return retval;

			disp_pr_debug("NPORTS = %d\n", __func__, rep.u.link_addr.nports);
			for (i = 0; i < rep.u.link_addr.nports; i++) {
				disp_pr_debug("input=%d, pdt=%d, pnum=%d, mcs=%d, ldps = %d, ddps=%d\n", __func__,
					rep.u.link_addr.ports[i].input_port,
					rep.u.link_addr.ports[i].peer_device_type,
					rep.u.link_addr.ports[i].port_number,
					rep.u.link_addr.ports[i].mcs,
					rep.u.link_addr.ports[i].legacy_device_plug_status,
					rep.u.link_addr.ports[i].ddps);
				if ((rep.u.link_addr.ports[i].peer_device_type == 3) &&
					(!rep.u.link_addr.ports[i].mcs) &&
					(rep.u.link_addr.ports[i].ddps) &&
					(port_count < 2)) {
					dptx->port[port_count] = rep.u.link_addr.ports[i].port_number;
					port_count++;
				}
			}
		}
	}

	dptx_enum_path_resources(dptx, dptx->port[0], -1);
	dptx_remote_i2c_read(dptx, dptx->port[0], -1);

	dptx_enum_path_resources(dptx, dptx->port[1], 1);
	dptx_remote_i2c_read(dptx, dptx->port[1], 1);

	return retval;
}

int dptx_mst_payload_setting(struct dp_ctrl *dptx)
{
	int i;
	int retval;
	int tries = 0;
	uint8_t status = 0;
	uint16_t pbn;
	uint32_t slots;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	dptx->pbn = (uint16_t)dp_calc_pbn_mode(dptx->vparams.mdtd.pixel_clock, 24);

	pbn = dptx->pbn;
	retval = dptx_calc_num_slots(dptx, pbn, &slots);
	if (retval)
		return retval;

	disp_pr_info("NUM SLOTS = %d\n", __func__, slots);

	dptx_dpcd_clear_vcpid_table(dptx);
	dptx_clear_vcpid_table(dptx);

	for (i = 0; i < dptx->streams; i++)
		dptx_set_vcpid_table_range(dptx, slots * i + 1, slots, i + 1);

	for (i = 0; i < dptx->streams; i++)
		dptx_dpcd_set_vcpid_table(dptx, slots * i + 1, slots, i + 1);

	dptx_print_vcpid_table(dptx);

	dptx_initiate_mst_act(dptx);

	while (!(status & DP_PAYLOAD_ACT_HANDLED)) {
		retval = dptx_read_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
		if (retval)
			disp_pr_debug("[DP] dptx_read_dpcd fail");

		tries++;
		if (WARN(tries > 100, "Timeout waiting for ACT_HANDLED\n"))
			break;

		mdelay(20);
	}

	retval = dptx_write_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, 0x3);
	if (retval)
		disp_pr_err("[DP] dptx_write_dpcd fail");

	retval = dptx_read_dpcd(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
	if (retval)
		disp_pr_err("[DP] dptx_read_dpcd fail");

	dptx->vcp_id = 1;

	if (dptx->logic_port)
		dptx_aux_msg_allocate_payload(dptx, dptx->port[0], dptx->vcp_id, pbn, -1);
	else
		dptx_aux_msg_allocate_payload(dptx, dptx->port[0], dptx->vcp_id, pbn, 1);
	dptx->vcp_id++;

	dptx_aux_msg_allocate_payload(dptx, dptx->port[1], dptx->vcp_id, pbn, 1);

	return retval;
}

/**
 * dp_calc_pbn_mode() - Calculate the PBN for a mode.
 * @clock: dot clock for the mode
 * @bpp: bpp for the mode.
 *
 * This uses the formula in the spec to calculate the PBN value for a mode.
 */
int dp_calc_pbn_mode(int clock, int bpp)
{
	s64 kbps;
	s64 peak_kbps;
	u32 numerator;
	u32 denominator;

	kbps = (s64)clock * bpp;

	/*
	 * margin 5300ppm + 300ppm ~ 0.6% as per spec, factor is 1.006
	 * The unit of 54/64Mbytes/sec is an arbitrary unit chosen based on
	 * common multiplier to render an integer PBN for all link rate/lane
	 * counts combinations
	 * calculate
	 * peak_kbps *= (1006/1000)
	 * peak_kbps *= (64/54)
	 * peak_kbps *= 8    convert to bytes
	 */
	numerator = 64 * 1006;
	denominator = 54 * 8 * 1000 * 1000;

	kbps *= numerator;
	peak_kbps = drm_fixp_from_fraction(kbps, denominator);

	return drm_fixp2int_ceil(peak_kbps);
}

/*lint -restore*/
