/*
 * vfmw_intf_check.c
 *
 * This is for vfmw_intf_check.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/atomic.h>
#include "vfmw_intf.h"
#include "vfmw_osal.h"
#include "vfmw_pdt.h"
#include "dec_dev.h"
#include "stm_dev.h"
#include "dbg.h"
#include "vcodec_vdec_memory.h"
#include "vfmw_intf_check.h"
#include "vfmw_pdt.h"

#define VFMW_OK 0
#define VFMW_ERR (-1)

#define BYTE_LEN 6
#define BYTE_OFFSET 4

#define unused(x) (void)(x)

static struct vdec_address_info g_vdec_addr;

void vfmw_vdec_addr_init(void)
{
	int8_t idx;
	struct vdec_address_info *addr_info = &g_vdec_addr;

	for (idx = 0; idx < MAX_OPEN_COUNT; idx++) {
		addr_info->seg_buffer_addr[idx].file = NULL;
		addr_info->seg_buffer_addr[idx].seg_buffer_first = 0;
		addr_info->seg_buffer_addr[idx].seg_buffer_last = 0;
		addr_info->seg_buffer_addr[idx].pre_lsb = MAX_U32;
		addr_info->seg_buffer_addr[idx].pre_msb = MAX_U32;
	}
	addr_info->dec_pub_msg_phy_addr = 0;
	addr_info->scd_dn_msg_phy = 0;
	addr_info->scd_up_msg_phy = 0;
	mutex_init(&addr_info->vdec_check_mutex);
}

void vfmw_vdec_addr_clear(void)
{
	vfmw_vdec_addr_init();
}

static int32_t vfmw_map_cfg_to_std(uint32_t std_cfg)
{
	uint32_t idx;
	uint32_t num;
	const uint32_t map[][2] = {
		{ VFMW_H264,  0b00000 },
		{ VFMW_MPEG4, 0b00010 },
		{ VFMW_MPEG2, 0b00011 },
		{ VFMW_HEVC,  0b01101 },
		{ VFMW_VP9,   0b01110 },

	};

	num = sizeof(map) / sizeof(map[0]);
	for (idx = 0; idx < num; idx++)
		if (std_cfg == map[idx][1])
			return map[idx][0];

	return VFMW_ERR;
}

static int32_t vfmw_address_restore(void *dev, int32_t share_fd, UADDR *phy_addr,
	unsigned long *size)
{
	int32_t map_fd_ret;

	map_fd_ret = vfmw_get_vdec_glb_callback()->
		vdec_mem_iommu_map(dev, share_fd, phy_addr, size);
	vfmw_check_return((map_fd_ret != VCODEC_FAILURE), VFMW_ERR);
	map_fd_ret = vfmw_get_vdec_glb_callback()->
		vdec_mem_iommu_unmap(dev, share_fd, *phy_addr);
	vfmw_check_return((map_fd_ret != VCODEC_FAILURE), VFMW_ERR);

	return VFMW_OK;
}

static int32_t vfmw_address_check(UADDR addr_to_be_check, UADDR start_addr,
	UADDR end_addr)
{
	if ((addr_to_be_check == 0) || (addr_to_be_check < start_addr) ||
		(addr_to_be_check > end_addr))
		return VFMW_ERR;

	return VFMW_OK;
}

static int32_t vfmw_get_scd_msg_phy_addr(void *dev, UADDR *dn_phy_addr,
	UADDR *up_phy_addr, scd_reg *reg_in)
{
	int32_t ret;
	unsigned long size;
	struct vdec_address_info *addr_info = &g_vdec_addr;

	if ((addr_info->scd_dn_msg_phy != 0) &&
		(addr_info->scd_up_msg_phy != 0) &&
		(addr_info->scd_dn_msg_phy == reg_in->dn_msg_phy) &&
		(addr_info->scd_up_msg_phy == reg_in->up_msg_phy)) {
		*dn_phy_addr = addr_info->scd_dn_msg_phy;
		*up_phy_addr = addr_info->scd_up_msg_phy;
		return VFMW_OK;
	}

	ret = vfmw_address_restore(dev, reg_in->msg_share_fd, dn_phy_addr, &size);
	vfmw_check_return(ret == VFMW_OK, VFMW_ERR);
	(*up_phy_addr) = (*dn_phy_addr) + SCD_DMSG_SIZE;

	addr_info->scd_dn_msg_phy = (*dn_phy_addr);
	addr_info->scd_up_msg_phy = (*up_phy_addr);

	return VFMW_OK;
}

static int32_t vfmw_find_seg_buffer_idx(void *fd)
{
	int32_t idx;
	struct file *file = (struct file *)fd;

	for (idx = 0; idx < MAX_OPEN_COUNT; idx++)
		if (g_vdec_addr.seg_buffer_addr[idx].file == file)
			return idx;

	return VFMW_ERR;
}

static int32_t vfmw_find_idle_seg_buffer_idx(void *fd)
{
	int32_t idx;
	struct file *file = (struct file *)fd;
	struct vdec_address_info *addr_info = &g_vdec_addr;

	for (idx = 0; idx < MAX_OPEN_COUNT; idx++) {
		if (!addr_info->seg_buffer_addr[idx].file) {
			addr_info->seg_buffer_addr[idx].file = file;
			return idx;
		}
	}
	return VFMW_ERR;
}

static int32_t vfmw_get_seg_buffer_phy_addr(void *dev, scd_reg *reg_in,
	UADDR *phy_addr_first, UADDR *phy_addr_last, void *fd)
{
	int32_t ret;
	int32_t idx;
	uint32_t addr_offset;
	UADDR maped_buffer_addr;
	unsigned long buffer_size;
	struct vdec_address_info *addr_info = &g_vdec_addr;

	idx = vfmw_find_seg_buffer_idx(fd);
	if (idx != VFMW_ERR) {
		if ((addr_info->seg_buffer_addr[idx].seg_buffer_first != 0) &&
			(addr_info->seg_buffer_addr[idx].seg_buffer_last != 0) &&
			(addr_info->seg_buffer_addr[idx].seg_buffer_first ==
				reg_in->buffer_first) &&
			(addr_info->seg_buffer_addr[idx].seg_buffer_last ==
				reg_in->buffer_last)) {
			(*phy_addr_first) = addr_info->seg_buffer_addr[idx].seg_buffer_first;
			(*phy_addr_last) =
				addr_info->seg_buffer_addr[idx].seg_buffer_last;
			return VFMW_OK;
		}
	}

	ret = vfmw_address_restore(dev, reg_in->buffer_share_fd,
		&maped_buffer_addr, &buffer_size);
	vfmw_check_return(ret == VFMW_OK, VFMW_ERR);

	(*phy_addr_first) = align_up(maped_buffer_addr, 4) + SEG_TOP_BLANK_SIZE; // 4: 4-byte alignment
	addr_offset = (*phy_addr_first) - maped_buffer_addr;
	buffer_size = buffer_size - addr_offset - SEG_BTM_BLANK_SIZE;
	(*phy_addr_last) = (*phy_addr_first) + buffer_size;

	idx = (idx == VFMW_ERR) ? vfmw_find_idle_seg_buffer_idx(fd) : idx;
	vfmw_check_return(idx != VFMW_ERR, VFMW_ERR);
	addr_info->seg_buffer_addr[idx].seg_buffer_first = (*phy_addr_first);
	addr_info->seg_buffer_addr[idx].seg_buffer_last = (*phy_addr_last);

	return VFMW_OK;
}

static int32_t vfmw_stm_msg_address_check(void *fd, void *stm_cfg)
{
	int32_t ret;
	UADDR maped_dn_msg_phy;
	UADDR maped_up_msg_phy;
	scd_reg_ioctl *reg_config = (scd_reg_ioctl *)stm_cfg;
	scd_reg *reg_in = &(reg_config->reg);
	struct vfmw_global_info *glb_info = pdt_get_glb_info();
	struct device *dev = (struct device *)glb_info->dev;

	ret = vfmw_get_scd_msg_phy_addr(dev, &maped_dn_msg_phy,
		&maped_up_msg_phy, reg_in);
	vfmw_check_return(ret == VFMW_OK, VFMW_ERR);
	vfmw_check_return(maped_dn_msg_phy == reg_in->dn_msg_phy, VFMW_ERR);
	vfmw_check_return(maped_up_msg_phy == reg_in->up_msg_phy, VFMW_ERR);

	return VFMW_OK;
}

static int32_t vfmw_stm_seg_buffer_check(void *fd, void *stm_cfg)
{
	int32_t ret;
	UADDR maped_buffer_first;
	UADDR maped_buffer_last;
	scd_reg_ioctl *reg_config = (scd_reg_ioctl *)stm_cfg;
	scd_reg *reg_in = &(reg_config->reg);
	struct vfmw_global_info *glb_info = pdt_get_glb_info();
	struct device *dev = (struct device *)glb_info->dev;

	ret = vfmw_get_seg_buffer_phy_addr(dev, reg_in, &maped_buffer_first,
		&maped_buffer_last, fd);
	vfmw_check_return(ret == VFMW_OK, VFMW_ERR);
	vfmw_check_return(maped_buffer_first == reg_in->buffer_first, VFMW_ERR);
	vfmw_check_return(maped_buffer_last == reg_in->buffer_last, VFMW_ERR);
	vfmw_check_return(vfmw_address_check(reg_in->buffer_init,
		reg_in->buffer_first, reg_in->buffer_last) == VFMW_OK, VFMW_ERR);

	return VFMW_OK;
}

static int32_t vfmw_stm_address_check(void *fd, void *stm_cfg)
{
	vfmw_check_return(vfmw_stm_seg_buffer_check(fd, stm_cfg) == VFMW_OK,
		VFMW_ERR);
	vfmw_check_return(vfmw_stm_msg_address_check(fd, stm_cfg) == VFMW_OK,
		VFMW_ERR);

	return VFMW_OK;
}

static int32_t vfmw_check_pre_sb(void *fd, void *stm_cfg)
{
	scd_reg_ioctl *reg_config = (scd_reg_ioctl *)stm_cfg;
	scd_reg *reg_in = &(reg_config->reg);
	int32_t idx = vfmw_find_seg_buffer_idx(fd);

	vfmw_check_return(idx != VFMW_ERR, VFMW_ERR);

	if (((reg_in->pre_lsb == MAX_U32) ||
		(reg_in->pre_lsb == g_vdec_addr.seg_buffer_addr[idx].pre_lsb)) &&
		((reg_in->pre_msb == MAX_U32) || (reg_in->pre_msb == MAX_U16) ||
		(reg_in->pre_msb == g_vdec_addr.seg_buffer_addr[idx].pre_msb)))
		return VFMW_OK;

	return VFMW_ERR;
}

int32_t vfmw_stm_param_check(void *fd, void *stm_cfg)
{
#ifndef VFMW_INPUT_CHECK_SUPPORT
	return VFMW_OK;
#endif
	scd_reg_ioctl *reg_config = (scd_reg_ioctl *)stm_cfg;
	/* scd_id 1(tee),0(ree) */
	vfmw_check_return((reg_config->scd_id >= 0) &&
		(reg_config->scd_id < STM_DEV_NUM), VFMW_ERR);
	vfmw_check_return(reg_config->reg.scd_mmu_en == 1, VFMW_ERR);
	vfmw_check_return(reg_config->reg.vdh_mmu_en == 1, VFMW_ERR);
	vfmw_check_return(reg_config->reg.scd_start == 1, VFMW_ERR);
	vfmw_check_return((reg_config->reg.avs_flag == 0) ||
		(reg_config->reg.avs_flag == 1), VFMW_ERR);
	vfmw_check_return((reg_config->reg.std_type == VFMW_STM_AVC_HEVC) ||
		(reg_config->reg.std_type == VFMW_STM_MPEG4) ||
		(reg_config->reg.std_type == VFMW_STM_MPEG2) ||
		(reg_config->reg.std_type == VFMW_STM_VP9), VFMW_ERR);
	vfmw_check_return((reg_config->reg.slice_check == 0) ||
		(reg_config->reg.slice_check == 1), VFMW_ERR);
	vfmw_check_return((reg_config->reg.safe_flag == 0) ||
		(reg_config->reg.safe_flag == 1), VFMW_ERR);
	vfmw_check_return(reg_config->reg.up_step == SCD_UMSG_STEP, VFMW_ERR);
	vfmw_check_return(reg_config->reg.up_len <=
		SCD_UMSG_NUM * SCD_UMSG_STEP, VFMW_ERR);

	mutex_lock(&g_vdec_addr.vdec_check_mutex);
	if (vfmw_stm_address_check(fd, stm_cfg) != VFMW_OK) {
		mutex_unlock(&g_vdec_addr.vdec_check_mutex);
		return VFMW_ERR;
	}
	if (vfmw_check_pre_sb(fd, stm_cfg) != VFMW_OK) {
		mutex_unlock(&g_vdec_addr.vdec_check_mutex);
		return VFMW_ERR;
	}

	mutex_unlock(&g_vdec_addr.vdec_check_mutex);

	return VFMW_OK;
}

static int32_t vfmw_dec_addr_check(dec_dev_cfg *cfg)
{
	int32_t ret;
	unsigned long size;
	UADDR maped_msg_addr;
	struct vfmw_global_info *glb_info = pdt_get_glb_info();
	struct device *dev = (struct device *)glb_info->dev;

	if ((g_vdec_addr.dec_pub_msg_phy_addr != 0) &&
		(g_vdec_addr.dec_pub_msg_phy_addr == cfg->pub_msg_phy_addr))
		return VFMW_OK;

	ret = vfmw_address_restore(dev, cfg->msg_shared_fd, &maped_msg_addr,
		&size);
	vfmw_check_return(ret == VFMW_OK, VFMW_ERR);

	g_vdec_addr.dec_pub_msg_phy_addr = maped_msg_addr;

	if (g_vdec_addr.dec_pub_msg_phy_addr == cfg->pub_msg_phy_addr)
		return VFMW_OK;

	return VFMW_ERR;
}

int32_t vfmw_dec_param_check(void *dev_cfg)
{
#ifndef VFMW_INPUT_CHECK_SUPPORT
	return VFMW_OK;
#endif

	dec_dev_cfg *cfg = (dec_dev_cfg *)dev_cfg;
	int32_t std_vid = vfmw_map_cfg_to_std(cfg->std_cfg);

	vfmw_check_return(std_vid != VFMW_ERR, VFMW_ERR);
	vfmw_check_return(cfg->vdh_mmu_en == 1, VFMW_ERR);
	vfmw_check_return(cfg->vdh_safe_flag == 0, VFMW_ERR);
	vfmw_check_return(cfg->is_slc_ldy == 0, VFMW_ERR);
	vfmw_check_return((std_vid > VFMW_START_RESERVED) &&
		(std_vid < VFMW_STD_MAX), VFMW_ERR);
	vfmw_check_return((cfg->bsp_internal_ram == 1) || (cfg->bsp_internal_ram == 0),
		VFMW_ERR);
	vfmw_check_return((cfg->pxp_internal_ram == 1) || (cfg->pxp_internal_ram == 0),
		VFMW_ERR);
	vfmw_check_return((cfg->is_frm_cpy == 0) ||
		((cfg->is_frm_cpy == 1) && ((std_vid == VFMW_VP9) || (std_vid == VFMW_MPEG4))),
		VFMW_ERR);
	vfmw_check_return(((cfg->list_num > 0) ||
		((cfg->list_num == 0) && (cfg->is_frm_cpy == 1))) &&
		(cfg->list_num <= VDH_BSP_NUM_IN_USE), VFMW_ERR);
	vfmw_check_return(cfg->dev_cfg_info.dev_id == 0, VFMW_ERR);
	vfmw_check_return(cfg->dev_cfg_info.type == DEC_DEV_TYPE_VDH, VFMW_ERR);

	mutex_lock(&g_vdec_addr.vdec_check_mutex);
	if (vfmw_dec_addr_check(cfg) != VFMW_OK) {
		mutex_unlock(&g_vdec_addr.vdec_check_mutex);
		return VFMW_ERR;
	}
	mutex_unlock(&g_vdec_addr.vdec_check_mutex);

	return VFMW_OK;
}

int32_t vfmw_reset_vdh(void *dev_cfg)
{
	uint16_t dev_id = 0;
	dec_dev_info *dev = VCODEC_NULL;

	unused(dev_cfg);
	vfmw_check_return(dec_dev_get_dev(dev_id, &dev) == VFMW_OK, VFMW_ERR);

	vfmw_check_return(dev->state == DEC_DEV_STATE_RUN, VFMW_ERR);
	if (VDH_REG_NUM == 1) {
		if (dec_dev_reset(dev) != DEC_OK) {
			dprint(PRN_FATAL, "reset vdh failed\n");
			return DEC_ERR;
		}
	}

	return VFMW_OK;
}

void vfmw_init_seg_buffer_addr(const struct file *file_handle)
{
	int8_t idx;
	struct vdec_address_info *addr_info = &g_vdec_addr;

	for (idx = 0; idx < MAX_OPEN_COUNT; idx++) {
		if (addr_info->seg_buffer_addr[idx].file == file_handle) {
			addr_info->seg_buffer_addr[idx].seg_buffer_first = 0;
			addr_info->seg_buffer_addr[idx].seg_buffer_last = 0;
			addr_info->seg_buffer_addr[idx].pre_lsb = MAX_U32;
			addr_info->seg_buffer_addr[idx].pre_msb = MAX_U32;
			addr_info->seg_buffer_addr[idx].file = NULL;
			return;
		}
	}
}
#ifdef ENABLE_VDEC_MODULE
EXPORT_SYMBOL(vfmw_init_seg_buffer_addr);
#endif

static uint32_t vfmw_get_scd_valid_num(uint32_t valid_byte)
{
	uint32_t bit_shift;
	uint32_t count = 0;

	for (bit_shift = 0; bit_shift < BYTE_LEN; bit_shift++) {
		if (((valid_byte >> bit_shift) & 0x1) == 1)
			count++;
		else
			break;
	}

	return count;
}

int32_t vfmw_update_pre_sb(void *fd, void *scd_state_reg)
{
#ifndef VFMW_INPUT_CHECK_SUPPORT
	return VFMW_OK;
#endif
	int32_t idx;
	uint32_t valid_num;
	uint32_t cnt;
	uint8_t src_byte[BYTE_LEN];
	uint8_t dst_byte[BYTE_LEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	scd_reg *reg_back = VCODEC_NULL;
	struct vdec_address_info *addr_info = &g_vdec_addr;

	mutex_lock(&addr_info->vdec_check_mutex);

	idx = vfmw_find_seg_buffer_idx(fd);
	if (idx == VFMW_ERR) {
		mutex_unlock(&addr_info->vdec_check_mutex);
		return VFMW_ERR;
	}

	reg_back = (scd_reg *)scd_state_reg;
	valid_num = vfmw_get_scd_valid_num(reg_back->byte_valid);

	*(uint32_t *)src_byte = reg_back->pre_lsb;
	*(uint16_t *)(src_byte + BYTE_OFFSET) = reg_back->pre_msb & 0xffff;
	for (cnt = 0; cnt < valid_num; cnt++)
		dst_byte[cnt] = src_byte[cnt];

	addr_info->seg_buffer_addr[idx].pre_lsb = *(uint32_t *)dst_byte;
	addr_info->seg_buffer_addr[idx].pre_msb = *(uint16_t *)(dst_byte + BYTE_OFFSET);

	mutex_unlock(&addr_info->vdec_check_mutex);

	return VFMW_OK;
}
