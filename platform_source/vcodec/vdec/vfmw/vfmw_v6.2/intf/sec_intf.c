/*
 * sec_intf.c
 *
 * This is for vdec driver interface.
 *
 * Copyright (c) 2017-2020 Huawei Technologies CO., Ltd.
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

#include "sec_intf.h"
#include "vfmw_osal.h"
#include "vfmw_pdt.h"
#include "dec_dev.h"
#include "stm_dev.h"
#include "scd_hal.h"
#include "smmu.h"
#include "smmu_regs.h"
#include "sre_dev_relcb.h"

#define VFMW_OK 0
#define VFMW_ERR (-1)

#define unused(x) (void)(x)

/* irq num */
uint32_t g_vdec_irq_num_norm = 322;
uint32_t g_vdec_irq_num_prot = 323;
uint32_t g_vdec_irq_num_safe = 324;

/* only dev_id[0->VDH; 1->DRM] enabled */
const uint32_t g_irq_handled = 1;
uint32_t g_irq_num_norm;
vdec_plat g_vdec_plat_entry;
static uint32_t g_sec_store_info = 0;
static int32_t g_vdec_sec_drv_init = 0;

int32_t vfmw_sec_map_cfg_to_std(uint32_t std_cfg)
{
	int32_t idx;
	int32_t num;
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

int32_t vfmw_sec_dec_param_check(void *dev_cfg)
{
	dec_dev_cfg *cfg = (dec_dev_cfg *)dev_cfg;
	int32_t std_vid = vfmw_sec_map_cfg_to_std(cfg->std_cfg);

	vfmw_check_return(std_vid != VFMW_ERR, VFMW_ERR);
	vfmw_check_return(cfg->vdh_mmu_en == 1, VFMW_ERR);
	vfmw_check_return(cfg->vdh_safe_flag == 1, VFMW_ERR);
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
	vfmw_check_return(cfg->dev_cfg_info.dev_id == 1, VFMW_ERR);
	vfmw_check_return(cfg->dev_cfg_info.type == DEC_DEV_TYPE_VDH, VFMW_ERR);

	return VFMW_OK;
}

int32_t vfmw_sec_stm_param_check(void *stm_cfg)
{
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

	return VFMW_OK;
}

vdec_plat *vdec_plat_get_entry(void)
{
	return &g_vdec_plat_entry;
}

typedef int32_t (*vctrl_handler)(void *, void *);

typedef struct {
	vfmw_cmd_type cmd;
	vctrl_handler handler;
} vctrl_case;

static int32_t vfmw_isr(int irq, void *id)
{
	unused(irq);
	unused(id);
	uint16_t dev_id = 1;

	if (dev_id > DEC_DEV_NUM)
		return 0;

	if (scd_hal_isr_state() == STM_OK)
		stm_dev_isr();

	if (dec_dev_isr_state(dev_id) == DEC_OK)
		dec_dev_isr(dev_id);

	return 1;
}

static int32_t vfmw_request_irq(uint32_t irq_num_norm)
{
	vfmw_assert_ret(irq_num_norm != 0, VFMW_ERR);

	if (OS_REQUEST_IRQ(irq_num_norm, (vfmw_irq_handler_t)vfmw_isr) != 0) {
		dprint(PRN_FATAL, "Request vdec norm irq %u failed\n", irq_num_norm);
		return VFMW_ERR;
	}
	g_irq_num_norm = irq_num_norm;

	return VFMW_OK;
}

int32_t vfmw_acquire_back_up_fifo(uint32_t instance_id)
{
	int32_t ret;

	ret = dec_dev_acquire_back_up_fifo(instance_id);
	if (ret != DEC_OK)
		return VFMW_ERR;

	return VFMW_OK;
}

void vfmw_release_back_up_fifo(uint32_t instance_id)
{
	dec_dev_release_back_up_fifo(instance_id);

	return;
}

static void vfmw_free_irq(uint32_t irq_num_norm)
{
	vfmw_assert(irq_num_norm != 0);
	OS_FREE_IRQ(irq_num_norm);
	g_irq_num_norm = 0;
}

static vfmw_module_reg_info g_module_reg_info[MAX_INNER_MODULE] = {
	{ VDM_REG_PHY_ADDR, VDH_REG_RANGE },
	{ SCD_REG_SEC_PHY_ADDR, SCD_REG_RANGE },
	{ VDH_CRG_REG_PHY_ADDR, VDH_CRG_REG_RANGE },
	{ SMMU_SID_REG_PHY_ADDR, SMMU_SID_REG_RANGE },
	{ DP_MONIT_REG_PHY_ADDR, DP_MONIT_REG_RANGE },
	{ SMMU_TBU_REG_PHY_ADDR, SMMU_TBU_REG_RANGE },
};

static int32_t vdec_plat_init(vdec_plat *plt_info)
{
	(void)memset_s(&plt_info->dts_info, sizeof(vdec_dts), 0, sizeof(vdec_dts));
	plt_info->dts_info.irq_norm = g_vdec_irq_num_norm;
	plt_info->dts_info.irq_safe = g_vdec_irq_num_safe;

	(void)memcpy_s(plt_info->dts_info.module_reg,
		sizeof(plt_info->dts_info.module_reg), g_module_reg_info,
		sizeof(g_module_reg_info));

	plt_info->dts_info.is_fpga = 0;

	return 0;
}

static void vdec_plat_deinit(void)
{
	return;
}

static int32_t vfmw_deinit(void)
{
	dec_dev_deinit();

	stm_dev_deinit();

	pdt_deinit();

	vfmw_free_irq(g_irq_num_norm);

	return VFMW_OK;
}

static int32_t vfmw_init(void *args)
{
	int32_t ret;
	vdec_dts *dts_info = (vdec_dts *)args;
	struct pdt_config cfg = {0};

	ret = vfmw_request_irq(g_vdec_irq_num_safe);
	if (ret != VFMW_OK) {
		dprint(PRN_FATAL, "vfmw_request_irq failed\n");
		goto request_irq_failure;
	}

	ret = dec_dev_init(&dts_info->module_reg[MFDE_MODULE]);
	if (ret != DEC_OK) {
		dprint(PRN_FATAL, "dec_dev init failed\n");
		goto dec_dev_init_failure;
	}

	cfg.is_fpga = dts_info->is_fpga;
	ret = pdt_init(&dts_info->module_reg[CRG_MOUDLE],
		&cfg, (void *)dts_info->dev);
	if (ret) {
		dprint(PRN_FATAL, "glb reset failed\n");
		goto pdt_init_failure;
	}

	ret = stm_dev_init(&dts_info->module_reg[SCD_MOUDLE]);
	if (ret != STM_OK) {
		dprint(PRN_FATAL, "stm_dev init failed\n");
		goto stm_dev_init_failure;
	}

	return ret;

stm_dev_init_failure:
	pdt_deinit();
pdt_init_failure:
	dec_dev_deinit();
dec_dev_init_failure:
	vfmw_free_irq(g_irq_num_norm);
request_irq_failure:

	return ret;
}

int32_t vfmw_reset_vdh(void *dev_cfg)
{
	unused(dev_cfg);

	uint16_t dev_id = 1;
	dec_dev_info *dev = VCODEC_NULL;
	int32_t ret = dec_dev_get_dev(dev_id, &dev);

	vfmw_assert_ret(ret == VFMW_OK, VFMW_ERR);
	vfmw_assert_ret(dev->state == DEC_DEV_STATE_RUN, VFMW_ERR);
	if (VDH_REG_NUM == 1) {
		if (dec_dev_reset(dev) != DEC_OK) {
			dprint(PRN_FATAL, "reset vdh failed\n");
			return VFMW_ERR;
		}
	}

	return VFMW_OK;
}

static void vfmw_dec_store_info(int pid)
{
	dec_dev_write_store_info(pid);
}

int32_t tee_vdec_drv_init(int pid)
{
	vdec_plat *plt = VCODEC_NULL;

	if (g_vdec_sec_drv_init != 0) {
		dprint(PRN_FATAL, "init already\n");
		return 0;
	}

	plt = vdec_plat_get_entry();
	if (vdec_plat_init(plt) != 0) {
		dprint(PRN_FATAL, "vdec_plat_init failed\n");
		return VCODEC_FAILURE;
	}

	if (vfmw_init(&plt->dts_info) != 0) {
		dprint(PRN_FATAL, "vfmw init failed\n");
		vdec_plat_deinit();
		return VCODEC_FAILURE;
	}

	if (smmu_init() != 0) {
		dprint(PRN_FATAL, "vfmw init failed\n");
		vfmw_deinit();
		vdec_plat_deinit();
		return VCODEC_FAILURE;
	}

	int ret = (int)SRE_TaskRegister_DevRelCb((DEV_RELEASE_CALLBACK)tee_vdec_drv_exit,
		(int *)EXIT_ABNORMAL);
	if (ret != 0) {
		dprint(PRN_ALWS, "SRE_TaskRegister_DevRelCb failed\n");
		return VCODEC_FAILURE;
	}

	vfmw_dec_store_info(pid);

	g_vdec_sec_drv_init = 1;

	return 0;
}

int32_t tee_vdec_drv_exit(int exit_type)
{
	vfmw_assert_ret(g_vdec_sec_drv_init == 1, VCODEC_FAILURE);

	if (exit_type == EXIT_ABNORMAL) {
		for (uint32_t i = 0; i < 10; i++)
			OS_MSLEEP(10); // 10: ms
		dprint(PRN_ALWS, "vdec exit abnormal\n");
	}

	SRE_TaskUnRegister_DevRelCb((DEV_RELEASE_CALLBACK)tee_vdec_drv_exit, NULL);

	smmu_deinit();
	vfmw_deinit();

	g_vdec_sec_drv_init = 0;

	return 0;
}

int32_t tee_vdec_drv_scd_start(void *stm_cfg, int cfg_size,
	void *scd_state_reg, int reg_size)
{
	vfmw_assert_ret(g_vdec_sec_drv_init == 1, VCODEC_FAILURE);
	int32_t ret;
	vfmw_check_return(stm_cfg != NULL, VCODEC_FAILURE);
	vfmw_check_return((cfg_size == (int)sizeof(scd_reg_ioctl)), VCODEC_FAILURE);
	vfmw_check_return(scd_state_reg != NULL, VCODEC_FAILURE);
	vfmw_check_return((reg_size == (int)sizeof(scd_reg)), VCODEC_FAILURE);
	vfmw_check_return(vfmw_sec_stm_param_check(stm_cfg) == VFMW_OK, VFMW_ERR);
	ret = stm_dev_config_reg(stm_cfg, scd_state_reg);
	return ret;
}

int32_t tee_vdec_drv_iommu_map(void *mem_param, int in_size,
	void *out_mem_param, int out_size)
{
	vfmw_assert_ret(mem_param != NULL, VCODEC_FAILURE);
	vfmw_assert_ret((in_size== (int)sizeof(mem_record)), VCODEC_FAILURE);
	vfmw_assert_ret(out_mem_param != NULL, VCODEC_FAILURE);
	vfmw_assert_ret((out_size== (int)sizeof(mem_record)), VCODEC_FAILURE);

	return tee_mem_ionmap(mem_param, out_mem_param);
}

int32_t tee_vdec_drv_iommu_unmap(void *mem_param, int in_size)
{
	vfmw_assert_ret(mem_param != NULL, VCODEC_FAILURE);
	vfmw_assert_ret((in_size== (int)sizeof(mem_record)), VCODEC_FAILURE);

	return tee_mem_ion_unmap(mem_param);
}

int32_t tee_vdec_drv_get_active_reg(void *dev_cfg, int cfg_size)
{
	vfmw_assert_ret(g_vdec_sec_drv_init == 1, VCODEC_FAILURE);
	vfmw_assert_ret(dev_cfg != NULL, VCODEC_FAILURE);
	vfmw_assert_ret((cfg_size == (int)sizeof(dec_dev_cfg)), VCODEC_FAILURE);

	return get_cur_active_reg(dev_cfg);
}

int32_t tee_vdec_drv_dec_start(void *dev_cfg, int cfg_size)
{
	vfmw_assert_ret(g_vdec_sec_drv_init == 1, VCODEC_FAILURE);
	int32_t ret;
	vfmw_check_return(dev_cfg != NULL, VCODEC_FAILURE);
	vfmw_check_return((cfg_size == (int)sizeof(dec_dev_cfg)), VCODEC_FAILURE);
	vfmw_check_return(vfmw_sec_dec_param_check(dev_cfg) == VFMW_OK, VFMW_ERR);
	ret = vfmw_reset_vdh(dev_cfg);
	vfmw_check_return(ret == VFMW_OK, VCODEC_FAILURE);

	return dec_dev_config_reg(DEC_SEC_INSTANCE_ID, dev_cfg);
}

int32_t tee_vdec_drv_irq_query(void *dev_cfg, int cfg_size,
	void *read_backup, int backup_size)
{
	vfmw_assert_ret(g_vdec_sec_drv_init == 1, VCODEC_FAILURE);
	vfmw_check_return(dev_cfg != NULL, VCODEC_FAILURE);
	vfmw_check_return((cfg_size == (int)sizeof(dec_dev_cfg)), VCODEC_FAILURE);
	vfmw_check_return(read_backup != NULL, VCODEC_FAILURE);
	vfmw_check_return((backup_size == (int)sizeof(dec_back_up)), VCODEC_FAILURE);
	vfmw_check_return(vfmw_sec_dec_param_check(dev_cfg) == VFMW_OK, VFMW_ERR);

	return dec_dev_get_backup(dev_cfg, DEC_SEC_INSTANCE_ID, read_backup);
}

int32_t tee_vdec_drv_set_dev_reg(uint32_t dev_state)
{
	vfmw_assert_ret(g_vdec_sec_drv_init == 1, VCODEC_FAILURE);

	vfmw_assert_ret((dev_state == 0 || dev_state == 1), VCODEC_FAILURE);

	int32_t val = (int32_t)dev_state;

	for (int32_t i = 0; i < 32; ++i) // 32: num of register
		set_tbu_reg(SMMU_TBU_PROT_ENN + i * 0x4, val, 1, 0);

	return 0;
}

int32_t tee_vdec_drv_resume(void)
{
	vfmw_assert_ret(g_vdec_sec_drv_init == 1, VCODEC_FAILURE);

	stm_dev_resume();
	dec_dev_resume();

	smmu_init();

	if (g_sec_store_info != 0) {
		dec_dev_write_store_info(g_sec_store_info);
		g_sec_store_info = 0;
	}

	return 0;
}

int32_t tee_vdec_drv_suspend(void)
{
	vfmw_assert_ret(g_vdec_sec_drv_init == 1, VCODEC_FAILURE);

	g_sec_store_info = dec_dev_read_store_info();

	stm_dev_suspend();
	dec_dev_suspend();

	smmu_deinit();

	return 0;
}

int32_t tee_intf_init(void)
{
	osal_intf_init();
	stm_dev_init_entry();
	dec_dev_init_entry();

	return OSAL_OK;
}

DECLARE_TC_DRV(SecureVfmw, 0, 0, 0, TC_DRV_MODULE_INIT,
	tee_intf_init, NULL, NULL, NULL, NULL);
