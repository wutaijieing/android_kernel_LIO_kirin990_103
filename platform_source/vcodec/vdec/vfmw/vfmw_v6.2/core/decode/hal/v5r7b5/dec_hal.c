/*
 * dec_hal.c
 *
 * This is for dec hal module proc.
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

#include "dec_hal.h"
#include "dec_dev.h"
#include "dec_reg.h"
#include "vfmw_pdt.h"

#define DEC_IRQ_NUM 2

typedef struct {
	int32_t irq_handle[DEC_IRQ_NUM];
	dec_dev_vdh vdh[VDH_NUM];
} dec_hal_ctx;

static dec_hal_ctx g_dec_hal_ctx;

static void dec_hal_create_vdh_inst(dec_dev_info *dev)
{
	void *vir = VCODEC_NULL;

	vir = &g_dec_hal_ctx.vdh[dev->dev_id];

	dev->handle = vir;
	(void)memset_s(vir, sizeof(dec_dev_vdh), 0, sizeof(dec_dev_vdh));
}

static void dec_hal_destroy_vdh_inst(dec_dev_info *dev)
{
	dev->handle = VCODEC_NULL;
}

static int32_t dec_hal_config_vdh(dec_dev_info *dev)
{
	uint32_t i;
	UADDR phy_addr;
	uint8_t *base_vir_addr = VCODEC_NULL;
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	uint32_t is_sec = (get_current_sec_mode() == NO_SEC_MODE) ? 0 : 1;

	phy_addr = dev->reg_phy_addr;
	base_vir_addr = (uint8_t *)OS_KMAP_REG(phy_addr, dev->reg_size);
	if (!base_vir_addr)
		return DEC_ERR;
	/* init vdh work clock */
	wr_vreg(base_vir_addr, VREG_OFS_DOMN_CLK_CNT, 0xAAAAAAAA);

	vdh->vdh_reg_pool.reg_num = VDH_REG_NUM;
	OS_SPIN_LOCK_INIT(&vdh->vdh_reg_pool.spin_lock);

	for (i = 0; i < vdh->vdh_reg_pool.reg_num; i++) {
		vdh->vdh_reg_pool.vdh_reg[i].used = VCODEC_FALSE;
		vdh->vdh_reg_pool.vdh_reg[i].reg_id = i;
		vdh->vdh_reg_pool.vdh_reg[i].is_sec = is_sec;
		vdh->vdh_reg_pool.vdh_reg[i].reg_phy_addr = phy_addr;
		vdh->vdh_reg_pool.vdh_reg[i].reg_vir_addr = base_vir_addr;
		vdh->vdh_reg_pool.vdh_reg[i].base_ofs[0] = OFS_BASE1;
		vdh->vdh_reg_pool.vdh_reg[i].base_ofs[1] = OFS_BASE2;

		dprint(PRN_DBG, "base 0x%pK ofs1 0x%x ofs2 0x%x\n",
			base_vir_addr, vdh->vdh_reg_pool.vdh_reg[i].base_ofs[0],
			vdh->vdh_reg_pool.vdh_reg[i].base_ofs[1]);
	}

	return DEC_OK;
}

static void dec_hal_clear_vdh(dec_dev_info *dev)
{
	uint32_t i;
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	uint8_t *base_vir_addr = VCODEC_NULL;

	base_vir_addr = vdh->vdh_reg_pool.vdh_reg[0].reg_vir_addr;
	if (base_vir_addr)
		OS_KUNMAP_REG(base_vir_addr);

	for (i = 0; (i < vdh->vdh_reg_pool.reg_num) && (i < VDH_REG_NUM); i++) {
		vdh->vdh_reg_pool.vdh_reg[i].used = VCODEC_FALSE;
		vdh->vdh_reg_pool.vdh_reg[i].reg_phy_addr = 0;
		vdh->vdh_reg_pool.vdh_reg[i].reg_vir_addr = VCODEC_NULL;
	}

	vdh->vdh_reg_pool.reg_num = 0;
	OS_SPIN_LOCK_EXIT(vdh->vdh_reg_pool.spin_lock);
}

void dec_hal_get_active_reg_id(void *dec_dev, uint16_t *reg_id)
{
	uint32_t pxp_reg_id = 0;
	vdh_reg_info *reg = VCODEC_NULL;
	dec_dev_info *dev = (dec_dev_info *)dec_dev;

	dev->ops->get_reg(dev, 0, &reg);
	rd_vreg(reg->reg_vir_addr + reg->base_ofs[0], VREG_OFS_BSP_CUR_REG, pxp_reg_id);
	*reg_id = pxp_reg_id;
}

static void dec_hal_write_reg(vdh_reg_info *reg, dec_dev_cfg *dec_vdh_cfg)
{
	int32_t ofs;
	uint32_t d32;
	uint32_t std_cfg;
	uint16_t dev_id = 0;
	uint32_t pxp_num = dec_vdh_cfg->use_pxp_num;
	uint32_t *vir = VCODEC_NULL;
	dec_dev_info *dev = VCODEC_NULL;

	(void)dec_dev_get_dev(dev_id, &dev);

	vir = (uint32_t *)(reg->reg_vir_addr + reg->base_ofs[0]);

	d32 = (pxp_num - 1) << 4;
	wr_vreg(reg->reg_vir_addr, VREG_OFS_PXP_CORE_EN, d32);

	d32 = 1;
	wr_vreg(vir, VREG_OFS_BSP_INTR_MSKSTATE, d32);

	d32 = 1;
	wr_vreg(vir, VREG_OFS_BSP_INTR_MASK, d32);

	d32 = dev->poll_irq_enable ? 1 : 0;
	wr_vreg(vir, VREG_OFS_PXP_INTR_MASK, d32);

	d32 = 0;
	if (!dev->smmu_bypass)
		((vreg_coeff *)(&d32))->mfde_mmu_en = 1;
	else
		((vreg_coeff *)(&d32))->mfde_mmu_en = 0;

	((vreg_coeff *)(&d32))->wireless_lowdelay_en = dec_vdh_cfg->wireless_ldy_en;
	wr_vreg(vir, VREG_OFS_VDH_COEFF, d32);

	ofs = VREG_OFS_BETWEEN_REG * reg->reg_id;

	vir = (uint32_t *)(reg->reg_vir_addr + reg->base_ofs[1]);
	d32 = 0;
	((vreg_info_state *)(&d32))->slice_low_dly = dec_vdh_cfg->is_slc_ldy;
	((vreg_info_state *)(&d32))->vdh_mdma_sel = dec_vdh_cfg->is_frm_cpy;
	((vreg_info_state *)(&d32))->int_report_en = 1;
	((vreg_info_state *)(&d32))->bsp_internal_ram = dec_vdh_cfg->bsp_internal_ram;
	((vreg_info_state *)(&d32))->pxp_internal_ram = dec_vdh_cfg->pxp_internal_ram;
	((vreg_info_state *)(&d32))->frame_indicator = dec_vdh_cfg->cfg_id;
	((vreg_info_state *)(&d32))->use_bsp_num = dec_vdh_cfg->list_num - 1;
	((vreg_info_state *)(&d32))->vdh_safe_flag = (reg->is_sec) ? 1 : 0;
	((vreg_info_state *)(&d32))->av1_intrabc = dec_vdh_cfg->av1_intrabc;
	wr_vreg(vir, VREG_OFS_INFO_STATE + ofs, d32);

	d32 = hw_addr_shift(dec_vdh_cfg->pub_msg_phy_addr);
	wr_vreg(vir, VREG_OFS_AVM_ADDR_CNT + ofs, d32);

	d32 = 0;
	std_cfg = dec_vdh_cfg->std_cfg;
	((vreg_proc_state *)(&d32))->vid_std = std_cfg;
	((vreg_proc_state *)(&d32))->pxp_core_len = (pxp_num - 1);
	((vreg_proc_state *)(&d32))->filmgrain_caltab_en = dec_vdh_cfg->apply_grain;
	((vreg_proc_state *)(&d32))->enhance_layer_valid_num = dec_vdh_cfg->enhance_layer_valid_num;
	wr_vreg(vir, VREG_OFS_PROCESS_STATE + ofs, d32);
}

int32_t dec_hal_open_vdh(void *dec_dev)
{
	dec_dev_info *dev = (dec_dev_info *)dec_dev;

	dec_hal_create_vdh_inst(dev);

	if (dec_hal_config_vdh(dev) != DEC_OK)
		goto config_vdh_error;

	return DEC_OK;

config_vdh_error:
	dec_hal_destroy_vdh_inst(dev);

	dprint(PRN_ERROR, "failed\n");

	return DEC_ERR;
}

void dec_hal_close_vdh(void *dec_dev)
{
	dec_dev_info *dev = (dec_dev_info *)dec_dev;

	dec_hal_clear_vdh(dev);

	dec_hal_destroy_vdh_inst(dev);
}

int32_t dec_hal_reset_vdh(void *dec_dev)
{
	uint32_t i;
	uint32_t d32;
	uint32_t int_mask[2];
	vdh_reg_info *reg = VCODEC_NULL;
	uint8_t *rst_vir_addr = VCODEC_NULL;
	uint8_t *reg_vir_addr = VCODEC_NULL;
	uint32_t pid_store;

	dec_dev_info *dev = (dec_dev_info *)dec_dev;
	struct vfmw_global_info *glb_info = pdt_get_glb_info();

	rst_vir_addr = glb_info->crg_reg_vaddr;
	vfmw_assert_ret(rst_vir_addr != VCODEC_NULL, DEC_ERR);

	dev->ops->get_reg(dev, 0, &reg);
	reg_vir_addr = (uint8_t *)(reg->reg_vir_addr + reg->base_ofs[0]);
	rd_vreg(reg_vir_addr, VREG_OFS_BSP_INTR_MASK, int_mask[0]);
	rd_vreg(reg_vir_addr, VREG_OFS_PXP_INTR_MASK, int_mask[1]);

	pid_store = dec_hal_get_pid_info(dec_dev);

	rd_vreg(rst_vir_addr, VDH_SRST_REQ_REG_OFS, d32);
	((struct vcrg_vdh_srst_req *)(&d32))->vdh_mfde_srst_req = 1;
	wr_vreg(rst_vir_addr, VDH_SRST_REQ_REG_OFS, d32);

	for (i = 0; i < 1000; i++) {
		OS_UDELAY(1);
		rd_vreg(rst_vir_addr, VDH_SRST_OK_REG_OFS, d32);
		if (((struct vcrg_vdh_srst_ok *)(&d32))->vdh_mfde_srst_ok == 1)
			break;
	}

	if (i == 1000)
		dprint(PRN_ERROR, "reset vdh failed\n");

	rd_vreg(rst_vir_addr, VDH_SRST_REQ_REG_OFS, d32);

	((struct vcrg_vdh_srst_req *)(&d32))->vdh_mfde_srst_req = 0;

	wr_vreg(rst_vir_addr, VDH_SRST_REQ_REG_OFS, d32);

	OS_MB();

	wr_vreg(reg_vir_addr, VREG_OFS_BSP_INTR_MASK, int_mask[0]);
	wr_vreg(reg_vir_addr, VREG_OFS_PXP_INTR_MASK, int_mask[1]);

	rd_vreg(reg_vir_addr, VREG_OFS_BSP_CUR_REG, d32);
	if (d32 != 0)
		dprint(PRN_ERROR, "reset already, but cur reg id not init to 0\n");

	if (pid_store != 0)
		dec_hal_set_pid_info(dev, pid_store);

	return DEC_OK;
}

void dec_hal_set_pid_info(void *dec_dev, uint32_t pid)
{
	dec_dev_info *dev = (dec_dev_info *)dec_dev;
	vdh_reg_info *reg = VCODEC_NULL;
	uint8_t *reg_vir_addr = VCODEC_NULL;

	dev->ops->get_reg(dev, 0, &reg);
	reg_vir_addr = reg->reg_vir_addr;

	wr_vreg(reg_vir_addr, VREG_OFS_DEC_STORE, pid);
}

uint32_t dec_hal_get_pid_info(void *dec_dev)
{
	dec_dev_info *dev = (dec_dev_info *)dec_dev;
	uint32_t pid = 0;
	vdh_reg_info *reg = VCODEC_NULL;
	uint8_t *reg_vir_addr = VCODEC_NULL;

	dev->ops->get_reg(dev, 0, &reg);
	reg_vir_addr = reg->reg_vir_addr;

	rd_vreg(reg_vir_addr, VREG_OFS_DEC_STORE, pid);
	return pid;
}

int32_t dec_hal_config_reg_vdh(void *dec_dev, void *dec_vdh_cfg, uint16_t reg_id)
{
	int32_t ret;
	unsigned long lock_flag;
	dec_dev_info *dev = (dec_dev_info *)dec_dev;
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	dec_dev_cfg *dev_cfg = (dec_dev_cfg *)dec_vdh_cfg;
	vdh_reg_info *reg = VCODEC_NULL;

	dprint(PRN_DBG, "reg_id is %hu, std_cfg %u, list_num %hu\n",
		reg_id, dev_cfg->std_cfg, dev_cfg->list_num);
	ret = dev->ops->get_reg(dev, reg_id, &reg);
	vfmw_assert_ret(ret == DEC_OK, DEC_ERR);

	OS_SPIN_LOCK(vdh->vdh_reg_pool.spin_lock, &lock_flag);
	dec_hal_write_reg(reg, dev_cfg);
	OS_SPIN_UNLOCK(vdh->vdh_reg_pool.spin_lock, &lock_flag);

	return DEC_OK;
}

void dec_hal_start_dec(void *dec_dev, uint16_t reg_id)
{
	uint32_t ofs;
	int32_t ret;
	dec_dev_info *dev = (dec_dev_info *)dec_dev;
	vdh_reg_info *reg = VCODEC_NULL;

	ret = dev->ops->get_reg(dev, reg_id, &reg);
	vfmw_assert(ret == DEC_OK);
	reg->reg_start_time = OS_GET_TIME_US();
	OS_MB();
	ofs = VREG_OFS_DEC_START + reg_id * VREG_OFS_BETWEEN_REG;
	wr_vreg(reg->reg_vir_addr + reg->base_ofs[1], ofs, 1);
	OS_MB();

	ofs = VREG_OFS_PXP_START + reg_id * VREG_OFS_BETWEEN_REG;
	wr_vreg(reg->reg_vir_addr + reg->base_ofs[1], ofs, 1);
	OS_MB();
}

int32_t dec_hal_check_int(void *dec_dev, uint16_t reg_id)
{
	uint32_t data;
	vdh_reg_info *reg = VCODEC_NULL;
	uint32_t *vir = VCODEC_NULL;
	dec_dev_info *dev = (dec_dev_info *)dec_dev;

	dev->ops->get_reg(dev, reg_id, &reg);
	vir = (uint32_t *)(reg->reg_vir_addr + reg->base_ofs[0]);

	rd_vreg(vir, VREG_OFS_PXP_INTR_STATE, data);
	if ((data >> 17) & 0x1)
		return DEC_OK;

	return DEC_ERR;
}

int32_t dec_hal_read_int_info(void *dec_dev, uint16_t reg_id)
{
	uint32_t data = 0;
	dec_dev_info *dev = VCODEC_NULL;
	vdh_reg_info *reg = VCODEC_NULL;
	mdma_reg_info *mdma = VCODEC_NULL;
	uint32_t *vir = VCODEC_NULL;

	dev = (dec_dev_info *)dec_dev;
	(void)memset_s(&dev->back_up, sizeof(dec_back_up), 0, sizeof(dec_back_up));

	if (dev->type == DEC_DEV_TYPE_MDMA) {
		dev->ops->get_reg(dev, reg_id, &mdma);
		rd_vreg(mdma->reg_vir_addr, VREG_OFS_MDMA_INTR_STATE, data);
		dev->back_up.int_flag = data & 0x1;

		rd_vreg(mdma->reg_vir_addr, VREG_OFS_MDMA_UNID, data);
		dev->back_up.int_unid = data & 0xffff;
	} else {
		dev->ops->get_reg(dev, reg_id, &reg);
		if (reg == VCODEC_NULL) {
			dprint(PRN_ERROR, "reg is null\n");
			return DEC_ERR;
		}
		vir = (uint32_t *)(reg->reg_vir_addr + reg->base_ofs[0]);
		rd_vreg(vir, VREG_OFS_PXP_INTR_STATE, data);
		dev->back_up.int_flag = (data >> 17) & 0x1;
		dev->back_up.int_flag |= ((data >> 19) & 0x1) << 1;
		dev->back_up.int_unid = data & 0xffff;
		dev->back_up.err_flag = (data >> 18) & 0x1;
		dev->back_up.int_reg = (data >> 24) & 0x7;

		if (dev->back_up.err_flag)
			dev->back_up.rd_slc_msg = 1;

		rd_vreg(vir, VREG_OFS_BSP_CYCLES, data);
		dev->back_up.bsp_cycle = data;
		rd_vreg(vir, VREG_OFS_PXP_CYCLES, data);
		dev->back_up.pxp_cycle = data;
		if ((dev->back_up.err_flag || dev->perf_enable))
			dprint(PRN_ALWS,
				"int: instance_id %u reg_id: %hu, dec_err: %u, frame number: %u, dec_time %u us\n",
				reg->instance_id, reg_id, dev->back_up.err_flag, dev->back_up.int_unid, reg->decode_time);
	}

	return DEC_OK;
}

void dec_hal_clear_int_state(void *dec_dev)
{
	uint32_t data = 0;
	mdma_reg_info *mdma_reg = VCODEC_NULL;
	vdh_reg_info *reg = VCODEC_NULL;
	dec_dev_info *dev = (dec_dev_info *)dec_dev;

	if (dev->type == DEC_DEV_TYPE_MDMA) {
		data = 1;
		dev->ops->get_reg(dev, 0, &mdma_reg);
		wr_vreg(mdma_reg->reg_vir_addr, VREG_OFS_MDMA_INTR_CLR, data);
		OS_MB();
		return;
	}

	dev->ops->get_reg(dev, 0, &reg);

	if (dev->back_up.int_flag & 1)
		data |= 0x1;

	if (dev->back_up.int_flag & 2)
		data |= 0x1 << 2;

	wr_vreg(reg->reg_vir_addr + reg->base_ofs[0],
		VREG_OFS_PXP_INTR_MSKSTATE, data);
	OS_MB();
}

int32_t dec_hal_cancel_vdh(void *dec_dev, uint16_t reg_id)
{
	uint32_t i;
	uint32_t d32;
	dec_dev_info *dev = (dec_dev_info *)dec_dev;
	vdh_reg_info *reg = VCODEC_NULL;
	uint32_t ofs = VREG_OFS_INFO_STATE + reg_id * VREG_OFS_BETWEEN_REG;

	dev->ops->get_reg(dev, reg_id, &reg);

	ofs += reg->base_ofs[1];
	rd_vreg(reg->reg_vir_addr, ofs, d32);

	((vreg_info_state *)(&d32))->dec_cancel = 1;
	wr_vreg(reg->reg_vir_addr, ofs, d32);
	OS_MB();

	for (i = 0; i < DEC_CANCEL_ACK_TIMEOUT; i++) {
		rd_vreg(reg->reg_vir_addr, ofs, d32);
		if (((vreg_info_state *)(&d32))->bsp_state == 1 &&
			((vreg_info_state *)(&d32))->pxp_state == 1) {
			dprint(PRN_ALWS, "cancel ok\n");
			return DEC_OK;
		}

		OS_UDELAY(30); // 30: us
	}

	dprint(PRN_ERROR, "failed\n");

	return DEC_ERR;
}

int32_t dec_hal_dump_reg(void *dec_dev, uint16_t reg_id)
{
#ifdef VFMW_PROC_SUPPORT
	int32_t ret, i;
	uint32_t data[4];
	UADDR reg_phy_addr;
	uint8_t tbuf[DUMP_BUFFER_LEN] = {0};
	uint8_t *reg_vir_addr = VCODEC_NULL;
	os_file *reg_file = VCODEC_NULL;

	dec_dev_info *dev = (dec_dev_info *)dec_dev;
	dec_dev_vdh *vdh  = (dec_dev_vdh *)dev->handle;

	ret = snprintf_s(tbuf, sizeof(tbuf), sizeof(tbuf) - 1,
			"%s%s_reg_%hu.txt", VDH_REG_DUMP_DIR, "vdh_reg_dump", reg_id);
	if (ret < 0)
		return -1;

	reg_file = OS_FOPEN(tbuf, OS_RDWR | OS_CREATE | OS_TRUNC, 0);
	if (!reg_file) {
		dprint(PRN_ERROR, "open file %s failed\n", tbuf);
		return -1;
	}

	reg_vir_addr = vdh->vdh_reg_pool.vdh_reg[0].reg_vir_addr;
	reg_phy_addr = vdh->vdh_reg_pool.vdh_reg[0].reg_phy_addr;

	for (i = 0; i < dev->reg_size >> 2; i += 4) {
		rd_vreg(reg_vir_addr, i * 4 + 0x0, data[0]);
		rd_vreg(reg_vir_addr, i * 4 + 0x4, data[1]);
		rd_vreg(reg_vir_addr, i * 4 + 0x8, data[2]);
		rd_vreg(reg_vir_addr, i * 4 + 0xc, data[3]);
		ret = snprintf_s(tbuf, sizeof(tbuf), sizeof(tbuf) - 1, "0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", \
				(reg_phy_addr + i * 4), data[0], data[1], data[2], data[3]);
		if (ret < 0) {
			OS_FCLOSE(reg_file);
			return -1;
		}

		OS_FWRITE(tbuf, strlen(tbuf), reg_file);
	}

	OS_FCLOSE(reg_file);
	return 0;
#endif
	unused_param(dec_dev);
	unused_param(reg_id);
	return 0;
}

