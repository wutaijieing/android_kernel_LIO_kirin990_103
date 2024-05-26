/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "dp_aux.h"
#include "dp_ctrl.h"

int dptx_aux_rw_bytes(struct dp_ctrl *dptx,
	bool rw,
	bool i2c,
	uint32_t addr,
	uint8_t *bytes,
	uint32_t len)
{
	int retval;
	uint32_t i;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!");
	dpu_check_and_return(!bytes, -EINVAL, err, "[DP] bytes is NULL!");

	for (i = 0; i < len; ) {
		uint32_t curlen;

		curlen = min_t(uint32_t, len - i, 16);
		if (!i2c) {
			if (dptx->aux_rw) {
				retval = dptx->aux_rw(dptx, rw, i2c, true, false,
					addr + i, &bytes[i], curlen);
			} else {
				retval = -EINVAL;
			}
		} else {
			if (dptx->aux_rw) {
				retval = dptx->aux_rw(dptx, rw, i2c, true, false,
					addr, &bytes[i], curlen);
			} else {
				retval = -EINVAL;
			}
		}

		if (retval)
			return retval;

		i += curlen;
	}

	return 0;
}

int dptx_read_bytes_from_i2c(struct dp_ctrl *dptx,
	uint32_t device_addr,
	uint8_t *bytes,
	uint32_t len)
{
	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!");
	dpu_check_and_return(!bytes, -EINVAL, err, "[DP] bytes is NULL!");

	return dptx_aux_rw_bytes(dptx, true, true, device_addr, bytes, len);
}

int dptx_i2c_address_only(struct dp_ctrl *dptx, uint32_t device_addr)
{
	uint8_t bytes[1];

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!");

	if (dptx->aux_rw)
		return dptx->aux_rw(dptx, 0, true, false, true, device_addr, &bytes[0], 1);
	return -EINVAL;
}

int dptx_write_bytes_to_i2c(struct dp_ctrl *dptx,
	uint32_t device_addr,
	uint8_t *bytes,
	uint32_t len)
{
	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!");
	dpu_check_and_return(!bytes, -EINVAL, err, "[DP] bytes is NULL!");

	return dptx_aux_rw_bytes(dptx, false, true, device_addr, bytes, len);
}

int dptx_read_dpcd(struct dp_ctrl *dptx, uint32_t addr, uint8_t *byte)
{
	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!");
	dpu_check_and_return(!byte, -EINVAL, err, "[DP] byte is NULL!");

	return dptx_aux_rw_bytes(dptx, true, false, addr, byte, 1);
}

int dptx_write_dpcd(struct dp_ctrl *dptx, uint32_t addr, uint8_t byte)
{
	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!");

	return dptx_aux_rw_bytes(dptx, false, false, addr, &byte, 1);
}

int dptx_read_bytes_from_dpcd(struct dp_ctrl *dptx,
	uint32_t reg_addr,
	uint8_t *bytes,
	uint32_t len)
{
	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!");
	dpu_check_and_return(!bytes, -EINVAL, err, "[DP] bytes is NULL!");

	return dptx_aux_rw_bytes(dptx, true, false, reg_addr, bytes, len);
}

int dptx_write_bytes_to_dpcd(struct dp_ctrl *dptx,
	uint32_t reg_addr,
	uint8_t *bytes,
	uint32_t len)
{
	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!");
	dpu_check_and_return(!bytes, -EINVAL, err, "[DP] bytes is NULL!");

	return dptx_aux_rw_bytes(dptx, false, false, reg_addr, bytes, len);
}
