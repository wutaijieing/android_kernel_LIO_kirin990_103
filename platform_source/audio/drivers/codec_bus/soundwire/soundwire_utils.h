/*
 * soundwire_utils.h -- soundwire utils
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SOUNDWIRE_UTILS_H__
#define __SOUNDWIRE_UTILS_H__

enum slv_status {
	SLV_OFFLINE,
	SLV_ONLINE,
	SLV_ONLINE_HAVE_WARNING
};

struct soundwire_priv;

unsigned char mst_read_reg(struct soundwire_priv *priv, unsigned int reg_offset);
void mst_write_reg(struct soundwire_priv *priv, unsigned char value, unsigned int reg_offset);

unsigned char slv_read_reg(struct soundwire_priv *priv, unsigned int reg_offset);
void slv_write_reg(struct soundwire_priv *priv, unsigned char value, unsigned int reg_offset);

unsigned int asp_cfg_read_reg(struct soundwire_priv *priv, unsigned int reg_offset);
void asp_cfg_write_reg(struct soundwire_priv *priv, unsigned int value, unsigned int reg_offset);

void write_fifo(struct soundwire_priv *priv, unsigned int offset, unsigned int value);

void intercore_try_lock(struct soundwire_priv *priv);
void intercore_unlock(struct soundwire_priv *priv);

enum slv_status soundwire_get_slv_status(struct soundwire_priv *priv);

#endif
