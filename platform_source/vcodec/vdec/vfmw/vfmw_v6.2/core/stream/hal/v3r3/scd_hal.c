/*
 * scd_hal.c
 *
 * This is for scd hal.
 *
 * Copyright (c) 2012-2020 Huawei Technologies CO., Ltd.
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

#include "stm_base.h"
#include "stm_dev.h"
#include "scd_hal.h"
#include "vfmw_pdt.h"

int32_t scd_hal_open(stm_dev *dev)
{
	uint8_t *vir = VCODEC_NULL;

	vir = (uint8_t *)OS_KMAP_REG(dev->reg_phy, dev->reg_size);
	if (!vir) {
		dprint(PRN_ERROR, "map scd reg failed\n");
		return STM_ERR;
	}

	dev->reg_vir = vir;

	scd_hal_clear_int();
	scd_hal_enable_int();

	return STM_OK;
}

void scd_hal_close(stm_dev *dev)
{
	if (!dev->reg_vir)
		return;

	OS_KUNMAP_REG(dev->reg_vir);

	dev->reg_phy = 0;
	dev->reg_vir = VCODEC_NULL;
	dev->reg_size = 0;
}

void scd_hal_reset(stm_dev *dev)
{
	uint32_t d32;
	uint32_t i;
	uint32_t rst_rq_reg_ofs;
	uint32_t rst_ok_reg_ofs;
	uint32_t mask;

	struct vfmw_global_info *glb_info = pdt_get_glb_info();
	rst_rq_reg_ofs = VDH_SRST_REQ_REG_OFS;
	rst_ok_reg_ofs = VDH_SRST_OK_REG_OFS;

	rd_vreg(dev->reg_vir, REG_SCD_INT_MASK, mask);

	rd_vreg(glb_info->crg_reg_vaddr, rst_rq_reg_ofs, d32);
	((struct vcrg_vdh_srst_req *)(&d32))->vdh_scd_srst_req = 1;
	wr_vreg(glb_info->crg_reg_vaddr, rst_rq_reg_ofs, d32);

	for (i = 0; i < RESET_SCD_COUNT; i++) {
		OS_UDELAY(1);
		rd_vreg(glb_info->crg_reg_vaddr, rst_ok_reg_ofs, d32);
		if (((struct vcrg_vdh_srst_ok *)(&d32))->vdh_scd_srst_ok == 1)
			break;
	}

	if (i == RESET_SCD_COUNT)
		dprint(PRN_ERROR, "reset scd fail\n");
	else
		dprint(PRN_ERROR, "reset scd success\n");

	rd_vreg(glb_info->crg_reg_vaddr, rst_rq_reg_ofs, d32);
	((struct vcrg_vdh_srst_req *)(&d32))->vdh_scd_srst_req = 0;
	wr_vreg(glb_info->crg_reg_vaddr, rst_rq_reg_ofs, d32);

	OS_MB();

	wr_vreg(glb_info->crg_reg_vaddr, REG_SCD_INT_MASK, mask);

	dev->state = STM_DEV_STATE_IDLE;
}

void scd_hal_start(stm_dev *dev)
{
	OS_MB();
	wr_vreg(dev->reg_vir, REG_SCD_START, 0);
	wr_vreg(dev->reg_vir, REG_SCD_START, 1);
}

void scd_hal_enable_int(void)
{
	stm_dev *dev = VCODEC_NULL;

	dev = stm_dev_get_dev();
	wr_vreg(dev->reg_vir, REG_SCD_INT_MASK, 0);
}

void scd_hal_clear_int(void)
{
	stm_dev *dev = VCODEC_NULL;

	dev = stm_dev_get_dev();
	wr_vreg(dev->reg_vir, REG_SCD_INT_CLR, 1);
}

void scd_hal_print_reg(const scd_reg *reg, uint8_t isr_before)
{
	dprint(PRN_ALWS, "dump %s begin...\n",
	       (isr_before == 1) ? "usr config reg" : "isr read reg");
	if (isr_before) {
		dprint(PRN_ALWS, "dn_msg_phy = %pK\n", reg->dn_msg_phy);
		dprint(PRN_ALWS, "up_msg_phy = %pK\n", reg->up_msg_phy);
		dprint(PRN_ALWS, "up_len = %x\n", reg->up_len);
		dprint(PRN_ALWS, "buffer_first = %pK\n", reg->buffer_first);
		dprint(PRN_ALWS, "buffer_last = %pK\n", reg->buffer_last);
		dprint(PRN_ALWS, "buffer_init = %pK\n", reg->buffer_init);
		dprint(PRN_ALWS, "pre_lsb = %x\n", reg->pre_lsb);
		dprint(PRN_ALWS, "pre_msb = %x\n", reg->pre_msb);
		dprint(PRN_ALWS, "int_mask = %x\n", reg->int_mask);
		dprint(PRN_ALWS, "scd_start = %x\n", reg->scd_start);
		dprint(PRN_ALWS, "vdh_mmu_en = %x\n", reg->vdh_mmu_en);
		dprint(PRN_ALWS, "scd_mmu_en = %x\n", reg->scd_mmu_en);
		dprint(PRN_ALWS, "safe_flag = %x\n", reg->safe_flag);
		dprint(PRN_ALWS, "std_type = %x\n", reg->std_type);
	} else {
		dprint(PRN_ALWS, "scd_over = %x\n", reg->scd_over);
		dprint(PRN_ALWS, "pre_lsb = %x\n", reg->pre_lsb);
		dprint(PRN_ALWS, "pre_msb = %x\n", reg->pre_msb);
		dprint(PRN_ALWS, "byte_valid = %x\n", reg->byte_valid);
		dprint(PRN_ALWS, "short_num = %x\n", reg->short_num);
		dprint(PRN_ALWS, "start_num = %x\n", reg->start_num);
		dprint(PRN_ALWS, "roll_addr = %pK\n", reg->roll_addr);
		dprint(PRN_ALWS, "src_eaten = %x\n", reg->src_eaten);
		dprint(PRN_ALWS, "next_addr = %pK\n", reg->next_addr);
		dprint(PRN_ALWS, "up_len = %x\n", reg->up_len);
	}
	dprint(PRN_ALWS, "dump %s end\n",
		(isr_before == 1) ? "usr config reg" : "isr read reg");
}

void scd_hal_read_reg(stm_dev *dev)
{
	uint8_t *reg_vir = VCODEC_NULL;
	scd_reg *reg = VCODEC_NULL;
	uint32_t data;

	reg = &(dev->reg);
	reg_vir = dev->reg_vir;

	rd_vreg(reg_vir, REG_SCD_INT_FLAG, data);
	reg->scd_over = data & 0x1;

	rd_vreg(reg_vir, REG_PRE_BYTE_LSB, reg->pre_lsb);
	rd_vreg(reg_vir, REG_PRE_BYTE_MSB, reg->pre_msb);
	rd_vreg(reg_vir, REG_BYTE_VALID, reg->byte_valid);

	rd_vreg(reg_vir, REG_SCD_NUM, data);
	reg->short_num = (data >> 22) & 0x3FF;
	reg->start_num = data & 0x3FFFFF;

	rd_vreg(reg_vir, REG_ROLL_ADDR_MSB, data);
	reg->roll_addr = (uint64_t)data << 32;
	rd_vreg(reg_vir, REG_ROLL_ADDR, data);
	reg->roll_addr += data;

	rd_vreg(reg_vir, REG_SRC_EATEN, reg->src_eaten);
	rd_vreg(reg_vir, REG_SCD_UP_LEN, reg->up_len);

	rd_vreg(reg_vir, REG_SEG_NEXT_ADDR_MSB, data);
	reg->next_addr = (uint64_t)data << 32;
	rd_vreg(reg_vir, REG_SEG_NEXT_ADDR, data);
	reg->next_addr += data;

	rd_vreg(reg_vir, REG_SCD_PROTOCOL, data);
	reg->int_unid = data >> 16;

	if (default_print_check(PRN_SCD_REGMSG))
		scd_hal_print_reg(reg, 0);
}

void scd_hal_write_reg(scd_reg_ioctl *reg_cfg_info)
{
	uint32_t data;
	stm_dev *dev = VCODEC_NULL;
	scd_reg *reg = VCODEC_NULL;

	dev = stm_dev_get_dev();
	reg = &(reg_cfg_info->reg);

	wr_vreg(dev->reg_vir, REG_LIST_ADDRESS,
		(uint32_t)(reg->dn_msg_phy >> SCD_ADDR_SHIFT));
	wr_vreg(dev->reg_vir, REG_UP_ADDRESS,
		(uint32_t)(reg->up_msg_phy >> SCD_ADDR_SHIFT));
	wr_vreg(dev->reg_vir, REG_SCD_UP_LEN, reg->up_len);
	wr_vreg(dev->reg_vir, REG_BUFFER_FIRST,
		(uint32_t)(reg->buffer_first >> SCD_ADDR_SHIFT));
	wr_vreg(dev->reg_vir, REG_BUFFER_LAST,
		(uint32_t)(reg->buffer_last >> SCD_ADDR_SHIFT));
	wr_vreg(dev->reg_vir, REG_BUFFER_INIT, reg->buffer_init);
	wr_vreg(dev->reg_vir, REG_BUFFER_INIT_MSB, 0);

	if (dev->smmu_bypass) {
		reg->vdh_mmu_en = 0;
		reg->scd_mmu_en = 0;
	}

	data = (reg->cfg_unid << 16) |
		(reg->vdh_mmu_en << 13) |
		(reg->scd_mmu_en << 9) |
		(reg->safe_flag << 7) |
		(reg->slice_check << 4) |
		(reg->std_type & 0xf);
	wr_vreg(dev->reg_vir, REG_SCD_PROTOCOL, data);
	wr_vreg(dev->reg_vir, REG_PRE_BYTE_LSB, reg->pre_lsb);
	wr_vreg(dev->reg_vir, REG_PRE_BYTE_MSB, reg->pre_msb);
	data = dev->poll_irq_enable ? 1 : reg->int_mask;
	wr_vreg(dev->reg_vir, REG_SCD_INT_MASK, data);

	dev->state = STM_DEV_STATE_BUSY;
	if (default_print_check(PRN_SCD_REGMSG))
		scd_hal_print_reg(reg, 1);
}

int32_t scd_hal_isr_state(void)
{
	stm_dev *dev = VCODEC_NULL;
	uint32_t data;

	dev = stm_dev_get_dev();
	rd_vreg(dev->reg_vir, REG_SCD_INT_FLAG, data);
	if ((data & 0x1) == 1)
		return STM_OK;

	return STM_ERR;
}

int32_t scd_hal_cancel(void)
{
	uint32_t i;
	uint32_t data;
	uint32_t loop = 1000;
	uint32_t delay = 30;
	stm_dev *dev = VCODEC_NULL;

	dev = stm_dev_get_dev();

	wr_vreg(dev->reg_vir, REG_SCD_DESTORY, 1);

	OS_MB();

	for (i = 0; i < loop; i++) {
		rd_vreg(dev->reg_vir, REG_SCD_INT_FLAG, data);
		if ((data >> 2) & 1)
			return STM_OK;
		else
			OS_UDELAY(delay);
	}

	dprint(PRN_ERROR, "scd cancel failed\n");

	return STM_ERR;
}
