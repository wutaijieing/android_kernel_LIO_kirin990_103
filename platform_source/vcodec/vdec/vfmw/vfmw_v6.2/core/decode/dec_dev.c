/*
 * dec_dev.c
 *
 * This is for dec dev module proc.
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

#include "dec_dev.h"
#include "dec_hal.h"
#include "vfmw_pdt.h"

dec_dev_pool g_dev_pool_entry;

static int32_t dec_dev_get_reg_vdh(void *dev, uint16_t reg_id, void *reg);

static dec_dev_ops g_dec_dev_ops[] = {
	/* MFDE */
	{
		.open = dec_hal_open_vdh,
		.close = dec_hal_close_vdh,
		.reset = dec_hal_reset_vdh,
		.get_active_reg = dec_hal_get_active_reg_id,
		.start_reg = dec_hal_start_dec,
		.cancel_reg = dec_hal_cancel_vdh,
		.config_reg = dec_hal_config_reg_vdh,
		.check_int  = dec_hal_check_int,
		.get_reg = dec_dev_get_reg_vdh,
		.dump_reg = dec_hal_dump_reg,
		.set_pid_info = dec_hal_set_pid_info,
		.get_pid_info = dec_hal_get_pid_info,
	},
};

static void dec_dev_init_dev_pool(const void *args)
{
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t id = (get_current_sec_mode() == SEC_MODE) ? 1 : 0;
	vfmw_module_reg_info *reg_info = (vfmw_module_reg_info *)args;

	if (dec_dev_get_dev(id, &dev) == DEC_OK) {
		dev->dev_id = id;
		dev->type = DEC_DEV_TYPE_VDH;
		dev->reg_phy_addr = reg_info->reg_phy_addr;
		dev->reg_size = reg_info->reg_range;
		dev->ops = &g_dec_dev_ops[(int16_t)DEC_DEV_TYPE_VDH - 1];
		dev->poll_irq_enable = 0;
		OS_SEMA_INIT(&(dev->sema));
	}
}

static void dec_dev_deinit_dev_pool(void)
{
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t id = (get_current_sec_mode() == SEC_MODE) ? 1 : 0;
	if (dec_dev_get_dev(id, &dev) == DEC_OK) {
		OS_SEMA_EXIT(dev->sema);
		(void)memset_s(dev, sizeof(dec_dev_info), 0, sizeof(dec_dev_info));
	}
}

static int32_t dec_dev_get_reg_vdh(void *dec_dev, uint16_t reg_id, void *reg)
{
	dec_dev_info *dev = (dec_dev_info *)dec_dev;
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	vdh_reg_pool_info *reg_pool = &vdh->vdh_reg_pool;

	vdh_reg_info **vdh_reg = (vdh_reg_info **)reg;

	if (reg_id >= reg_pool->reg_num)
		return DEC_ERR;

	*vdh_reg = &reg_pool->vdh_reg[reg_id];

	return DEC_OK;
}

static void dec_dev_get_active_reg_id(dec_dev_info *dev)
{
	uint16_t reg_id = 0;
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	vdh_reg_pool_info *reg_pool = VCODEC_NULL;

	vfmw_assert(vdh);

	dev->ops->get_active_reg(dev, &reg_id);
	if (reg_id < VDH_REG_NUM) {
		reg_pool = &vdh->vdh_reg_pool;
		reg_pool->next_cfg_reg = reg_id;
		reg_pool->next_isr_reg = reg_id;
	}

	vfmw_assert(reg_id < VDH_REG_NUM);
}

static void dec_dev_init_reg_id(dec_dev_info *dev)
{
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	vdh_reg_pool_info *reg_pool = VCODEC_NULL;

	vfmw_assert(vdh);

	reg_pool = &vdh->vdh_reg_pool;
	reg_pool->next_cfg_reg = 0;
	reg_pool->next_isr_reg = 0;
}

static int32_t dec_dev_get_cur_isr_reg(dec_dev_info *dev, uint16_t *reg_id, uint32_t *instance_id)
{
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	vdh_reg_pool_info *reg_pool = &vdh->vdh_reg_pool;
	uint16_t next_isr_reg = reg_pool->next_isr_reg;
	vdh_reg_info *reg = VCODEC_NULL;

	vfmw_assert_ret(next_isr_reg < VDH_REG_NUM, DEC_ERR);

	reg = &reg_pool->vdh_reg[next_isr_reg];
	if (reg->used == VCODEC_FALSE)
		return DEC_ERR;

	reg->decode_time = OS_GET_TIME_US() - reg->reg_start_time;
	*reg_id = next_isr_reg;
	*instance_id = reg->instance_id;
	return DEC_OK;
}

static int32_t dec_dev_update_isr_reg(dec_dev_info *dev)
{
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	vdh_reg_pool_info *reg_pool = &vdh->vdh_reg_pool;
	uint16_t next_isr_reg = reg_pool->next_isr_reg;
	vdh_reg_info *reg = VCODEC_NULL;

	vfmw_assert_ret(next_isr_reg < VDH_REG_NUM, DEC_ERR);

	reg = &reg_pool->vdh_reg[next_isr_reg];
	if (reg->used == VCODEC_FALSE)
		return DEC_ERR;

	reg_pool->next_isr_reg = (next_isr_reg + 1) % reg_pool->reg_num;
	return DEC_OK;
}

static int32_t dec_dev_get_cfg_reg_id(dec_dev_info *dev, uint16_t *reg_id)
{
	vdh_reg_info *reg = VCODEC_NULL;
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	vdh_reg_pool_info *reg_pool = &vdh->vdh_reg_pool;
	uint16_t next_cfg_reg = reg_pool->next_cfg_reg;

	vfmw_assert_ret(next_cfg_reg < VDH_REG_NUM, DEC_ERR);

	reg = &reg_pool->vdh_reg[next_cfg_reg];
	if (reg->used == VCODEC_TRUE)
		return DEC_ERR;
	*reg_id = reg->reg_id;

	return DEC_OK;
}

static void dec_dev_update_cfg_reg(dec_dev_info *dev, uint32_t instance_id, void *dev_cfg)
{
	unsigned long lock_flag;
	vdh_reg_info *reg = VCODEC_NULL;
	dec_dev_cfg *cfg = (dec_dev_cfg *)dev_cfg;
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	vdh_reg_pool_info *reg_pool = &vdh->vdh_reg_pool;
	uint16_t next_cfg_reg = reg_pool->next_cfg_reg;
	vfmw_assert(next_cfg_reg < VDH_REG_NUM);

	OS_SPIN_LOCK(reg_pool->spin_lock, &lock_flag);
	reg = &reg_pool->vdh_reg[next_cfg_reg];
	reg->used = VCODEC_TRUE;
	reg->instance_id = instance_id;
	reg->chan_id = cfg->dev_cfg_info.chan_id;
	reg_pool->next_cfg_reg = (next_cfg_reg + 1) % reg_pool->reg_num;
	OS_SPIN_UNLOCK(reg_pool->spin_lock, &lock_flag);

	return;
}

static dec_back_up_list *dec_dev_get_backup_reglist(uint32_t instance_id)
{
	dec_dev_pool *dev_pool = dec_dev_get_pool();
	uint32_t instance_index;

	if (get_current_sec_mode() == NO_SEC_MODE)
		instance_index = instance_id;
	else
		instance_index = DEC_SEC_INSTANCE_ID;

	vfmw_assert_ret(instance_index < VFMW_CHAN_NUM, VCODEC_NULL);

	return &dev_pool->dev_irq_backup_list[instance_index];
}

static int32_t dec_dev_insert_backup_reglist(dec_back_up *back_up, uint32_t instance_id)
{
	int32_t ret;
	unsigned long lock_flag;
	dec_back_up_list *dev_irq_back_up = dec_dev_get_backup_reglist(instance_id);

	vfmw_assert_ret(dev_irq_back_up, DEC_ERR);

	OS_SPIN_LOCK(dev_irq_back_up->lock, &lock_flag);
	if (dev_irq_back_up->insert_index >= VDH_REG_NUM) {
		dprint(PRN_ERROR, "failed, insert_index = %hu\n",
			dev_irq_back_up->insert_index);
		ret = DEC_ERR;
		goto exit;
	}

	if (dev_irq_back_up->dev_back_up_list[dev_irq_back_up->insert_index].used == 1) {
		dprint(PRN_ERROR, "failed, list full\n");
		ret = DEC_ERR;
		goto exit;
	}

	dev_irq_back_up->dev_back_up_list[dev_irq_back_up->insert_index].used = 1;
	(void)memcpy_s(&dev_irq_back_up->dev_back_up_list[dev_irq_back_up->insert_index].back_up,
		sizeof(dec_back_up), back_up, sizeof(dec_back_up));
	dev_irq_back_up->insert_index =
		(dev_irq_back_up->insert_index + 1) % VDH_REG_NUM;
	ret = DEC_OK;

exit:
	OS_SPIN_UNLOCK(dev_irq_back_up->lock, &lock_flag);
	if (ret == DEC_OK)
		OS_EVENT_GIVE(dev_irq_back_up->event);

	return ret;
}

static int32_t dec_dev_reglist_get_back_up(dec_back_up *back_up, dec_back_up_list *dev_irq_back_up)
{
	int32_t ret;
	unsigned long lock_flag;

	OS_SPIN_LOCK(dev_irq_back_up->lock, &lock_flag);
	if (dev_irq_back_up->get_index >= VDH_REG_NUM) {
		dprint(PRN_ERROR, "failed, get_index = %hu\n", dev_irq_back_up->get_index);
		ret = DEC_ERR;
		goto exit;
	}

	if (dev_irq_back_up->dev_back_up_list[dev_irq_back_up->get_index].used == 0) {
		ret = DEC_ERR;
		goto exit;
	}

	(void)memcpy_s(back_up, sizeof(dec_back_up),
		&dev_irq_back_up->dev_back_up_list[dev_irq_back_up->get_index].back_up,
		sizeof(dec_back_up));
	dev_irq_back_up->dev_back_up_list[dev_irq_back_up->get_index].used = 0;
	dev_irq_back_up->get_index = (dev_irq_back_up->get_index + 1) % VDH_REG_NUM;
	ret = DEC_OK;

exit:
	OS_SPIN_UNLOCK(dev_irq_back_up->lock, &lock_flag);

	return ret;
}

static int32_t dec_dev_release_reg(dec_dev_info *dev, uint16_t reg_id)
{
	dec_dev_vdh *vdh = VCODEC_NULL;
	dec_dev_mdma *mdma = VCODEC_NULL;
	unsigned long lock_flag;

	if (dev->type == DEC_DEV_TYPE_MDMA) {
		mdma = (dec_dev_mdma *)dev->handle;
		mdma->used = 0;
		return DEC_OK;
	}

	vdh = (dec_dev_vdh *)dev->handle;
	OS_SPIN_LOCK(vdh->vdh_reg_pool.spin_lock, &lock_flag);
	vdh->vdh_reg_pool.vdh_reg[reg_id].used = VCODEC_FALSE;
	OS_SPIN_UNLOCK(vdh->vdh_reg_pool.spin_lock, &lock_flag);

	return DEC_OK;
}

static void dec_dev_pxp_int_process(uint16_t dev_id)
{
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t reg_id = 0;
	uint32_t instance_id;

	if (dec_dev_get_dev(dev_id, &dev) != DEC_OK) {
		dprint(PRN_ERROR, "get_dev failed\n");
		return;
	}

	if (!dev->handle) {
		dec_hal_clear_int_state(dev);
		dprint(PRN_ERROR, "dev->handle null\n");
		return;
	}

	if (dec_dev_get_cur_isr_reg(dev, &reg_id, &instance_id) != DEC_OK) {
		dec_hal_clear_int_state(dev);
		dprint(PRN_ERROR, "get current pxp reg failed\n");
		return;
	}

	dec_hal_read_int_info(dev, reg_id);

	dec_hal_clear_int_state(dev);

	dec_dev_insert_backup_reglist(&dev->back_up, instance_id);
	dec_dev_update_isr_reg(dev);
	dec_dev_release_reg(dev, reg_id);
}

dec_dev_pool *dec_dev_get_pool(void)
{
	return &g_dev_pool_entry;
}

int32_t dec_dev_get_dev(uint16_t dev_id, dec_dev_info **dev)
{
	dec_dev_pool *dev_pool = VCODEC_NULL;

	if (dev_id >= DEC_DEV_NUM) {
		dprint(PRN_ERROR, "dev_id is error %hu\n", dev_id);
		return DEC_ERR;
	}

	dev_pool = dec_dev_get_pool();
	*dev = &(dev_pool->dev_inst[dev_id]);

	return DEC_OK;
}

static void dec_dev_init_source(void)
{
	dec_dev_pool *dev_pool = dec_dev_get_pool();
	dec_back_up_list *sec_back_up_list = VCODEC_NULL;

	if (get_current_sec_mode() == NO_SEC_MODE)
		return;

	sec_back_up_list = &dev_pool->dev_irq_backup_list[DEC_SEC_INSTANCE_ID];
	(void)memset_s(sec_back_up_list, sizeof(dec_back_up_list), 0, sizeof(dec_back_up_list));
	OS_EVENT_INIT(&sec_back_up_list->event, 0);
	OS_SPIN_LOCK_INIT(&sec_back_up_list->lock);
}

static void dec_dev_deinit_source(void)
{
	dec_dev_pool *dev_pool = dec_dev_get_pool();
	dec_back_up_list *sec_back_up_list = VCODEC_NULL;

	if (get_current_sec_mode() == NO_SEC_MODE)
		return;

	sec_back_up_list = &dev_pool->dev_irq_backup_list[DEC_SEC_INSTANCE_ID];
	OS_EVENT_EXIT(sec_back_up_list->event);
	OS_SPIN_LOCK_EXIT(sec_back_up_list->lock);
	(void)memset_s(sec_back_up_list, sizeof(dec_back_up_list), 0, sizeof(dec_back_up_list));
}

static int32_t dec_dev_open(void)
{
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t dev_id;
	int32_t ret;

	dev_id = (get_current_sec_mode() == NO_SEC_MODE) ? 0 : 1;

	ret = dec_dev_get_dev(dev_id, &dev);
	if (ret != DEC_OK) {
		dprint(PRN_ERROR, "get dec dev failed.");
		return DEC_ERR;
	}

	vfmw_assert_ret(dev->state == DEC_DEV_STATE_NULL, DEC_ERR);
	vfmw_assert_ret(dev->ops != VCODEC_NULL, DEC_ERR);
	vfmw_assert_ret(dev->ops->open != VCODEC_NULL, DEC_ERR);

	if (dev->ops->open(dev) != DEC_OK) {
		dprint(PRN_ERROR, "open dec dev failed.");
		return DEC_ERR;
	}

	dec_dev_init_reg_id(dev);

	dev->state = DEC_DEV_STATE_RUN;

	return DEC_OK;
}

static void dec_dev_close(void)
{
	uint16_t dev_id;
	dec_dev_info *dev = VCODEC_NULL;
	int32_t ret;

	dev_id = (get_current_sec_mode() == NO_SEC_MODE) ? 0 : 1;
	ret = dec_dev_get_dev(dev_id, &dev);
	if (ret != DEC_OK) {
		dprint(PRN_ERROR, "get dev failed.");
		return;
	}

	if (dev->state != DEC_DEV_STATE_RUN) {
		dprint(PRN_ERROR, "dec dev state error\n");
		return;
	}

	dev->ops->close(dev);
	dev->state = DEC_DEV_STATE_NULL;
}

void dec_dev_init_entry(void)
{
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t id = (get_current_sec_mode() == SEC_MODE) ? 1 : 0;
	if (dec_dev_get_dev(id, &dev) == DEC_OK) {
		(void)memset_s(dev, sizeof(dec_dev_info), 0, sizeof(dec_dev_info));
		dev->state = DEC_DEV_STATE_NULL;
	}
}

void dec_dev_deinit_entry(void)
{
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t id = (get_current_sec_mode() == SEC_MODE) ? 1 : 0;
	if (dec_dev_get_dev(id, &dev) == DEC_OK)
		(void)memset_s(dev, sizeof(dec_dev_info), 0, sizeof(dec_dev_info));
}

int32_t dec_dev_init(const void *args)
{
	int32_t ret = DEC_OK;

	dec_dev_init_dev_pool(args);
	dec_dev_init_source();

	if (dec_dev_open() != DEC_OK) {
		ret = DEC_ERR;
		goto out;
	}

	return ret;
out:
	dec_dev_deinit_source();
	dec_dev_deinit_dev_pool();

	return ret;
}

void dec_dev_deinit(void)
{
	dec_dev_close();
	dec_dev_deinit_source();
	dec_dev_deinit_dev_pool();
}

int32_t get_cur_active_reg(void *dev_cfg)
{
	uint16_t dev_id = (get_current_sec_mode() == SEC_MODE) ? 1 : 0;
	dec_dev_info *dev = VCODEC_NULL;
	dec_dev_cfg *cfg = (dec_dev_cfg *)dev_cfg;

	if (dec_dev_get_dev(dev_id, &dev))
		return DEC_ERR;

	vfmw_assert_ret(dev->state == DEC_DEV_STATE_RUN, DEC_ERR);
	dev->ops->get_active_reg(dev, &cfg->dev_cfg_info.reg_id);

	return DEC_OK;
}

int32_t dec_dev_config_reg(uint32_t instance_id, void *dev_cfg)
{
	uint16_t dev_id = (get_current_sec_mode() == SEC_MODE) ? 1 : 0;
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t reg_id;

	if (dec_dev_get_dev(dev_id, &dev))
		return DEC_ERR;

	vfmw_assert_ret(dev->state == DEC_DEV_STATE_RUN, DEC_ERR);
	if (dec_dev_get_cfg_reg_id(dev, &reg_id) != DEC_OK) {
		dprint(PRN_FATAL, "vdh cfg register error!\n");
		return DEC_ERR;
	}

	if (dev->ops->config_reg(dev, dev_cfg, reg_id)) {
		dprint(PRN_FATAL, "config vdh failed\n");
		return DEC_ERR;
	}

	dec_dev_update_cfg_reg(dev, instance_id, dev_cfg);

	dev->ops->start_reg(dev, reg_id);

	return DEC_OK;
}

static void dec_dev_cancel_reg_proc(dec_dev_info *dev, uint32_t instance_id, uint32_t chan_id)
{
	unsigned long lock_flag;
	uint32_t reg_id;
	vdh_reg_info *reg = VCODEC_NULL;
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	vdh_reg_pool_info *reg_pool = &vdh->vdh_reg_pool;

	OS_SPIN_LOCK(reg_pool->spin_lock, &lock_flag);
	for (reg_id = 0; (reg_id < reg_pool->reg_num) && (reg_id < VDH_REG_NUM); reg_id++) {
		reg = &reg_pool->vdh_reg[reg_id];
		if (reg->used == VCODEC_TRUE && reg->instance_id == instance_id && reg->chan_id == chan_id)
			dev->ops->cancel_reg(dev, reg->reg_id);
	}
	OS_SPIN_UNLOCK(reg_pool->spin_lock, &lock_flag);
}

int32_t dec_dev_cancel_reg(uint32_t instance_id, void *dev_cfg)
{
	dec_dev_info *dev = VCODEC_NULL;
	dec_dev_cfg *cfg = (dec_dev_cfg *)dev_cfg;
	uint16_t dev_id = (get_current_sec_mode() == SEC_MODE) ? 1 : 0;
	if (dec_dev_get_dev(dev_id, &dev) != DEC_OK) {
		dprint(PRN_FATAL, "get dec dev failed\n");
		return DEC_ERR;
	}
	vfmw_assert_ret(dev->state == DEC_DEV_STATE_RUN, DEC_ERR);

	dec_dev_cancel_reg_proc(dev, instance_id, cfg->dev_cfg_info.chan_id);
	return DEC_OK;
}

int32_t dec_dev_isr_state(uint16_t dev_id)
{
	dec_dev_info *dev = VCODEC_NULL;

	if (dec_dev_get_dev(dev_id, &dev))
		return DEC_ERR;

	return dev->ops->check_int(dev, 0);
}

int32_t dec_dev_isr(uint16_t dev_id)
{
	const int32_t irq_handled = 1;

	dec_dev_pxp_int_process(dev_id);

	return irq_handled;
}

int32_t dec_dev_reset(dec_dev_info *dev)
{
	int32_t ret;
	unsigned long lock_flag;
	uint32_t reg_id;
	vdh_reg_info *reg = VCODEC_NULL;
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	vdh_reg_pool_info *reg_pool = &vdh->vdh_reg_pool;

	/* hal reset */
	ret = dev->ops->reset(dev);
	if (ret != DEC_OK) {
		dprint(PRN_ERROR, "dev_id %d, reset failed.", dev->dev_id);
		return DEC_ERR;
	}

	/* dev reset */
	OS_SPIN_LOCK(reg_pool->spin_lock, &lock_flag);
	for (reg_id = 0; (reg_id < reg_pool->reg_num) && (reg_id < VDH_REG_NUM); reg_id++) {
		reg = &reg_pool->vdh_reg[reg_id];
		reg->used = VCODEC_FALSE;
	}

	dec_dev_init_reg_id(dev);
	OS_SPIN_UNLOCK(reg_pool->spin_lock, &lock_flag);

	return DEC_OK;
}

static void dec_dev_dump_reg_proc(dec_dev_info *dev)
{
	uint16_t reg_id;
	uint32_t instance_id;

	if (dec_dev_get_cur_isr_reg(dev, &reg_id, &instance_id) != DEC_OK) {
		dprint(PRN_ERROR, "get current pxp reg failed\n");
		return;
	}
	unused_param(instance_id);

	dev->ops->dump_reg(dev, reg_id);
}

static int32_t dec_dev_no_block_wait_vdh(dec_dev_cfg *cfg, dec_back_up *back_up,
	dec_dev_info *dev, dec_back_up_list *back_up_list)
{
	struct vfmw_global_info *glb_info = pdt_get_glb_info();
	uint32_t vdh_time_out = (glb_info->is_fpga ? VDH_FPGA_TIME_OUT : VDH_TIME_OUT);

	back_up->is_need_wait = 0;
	if (cfg->cur_time > vdh_time_out / 2) { // call delay 2ms
		dec_dev_reset(dev);
		return DEC_ERR;
	}
	if (dec_dev_reglist_get_back_up(back_up, back_up_list) != DEC_OK)
		back_up->is_need_wait = 1;

	return DEC_OK;
}

static int32_t dec_dev_poll_irq(dec_dev_info *dev)
{
	struct vfmw_global_info *glb_info = pdt_get_glb_info();
	int32_t vdh_time_out = (glb_info->is_fpga ? VDH_FPGA_TIME_OUT : VDH_TIME_OUT);
	int32_t poll_interval_ms = (glb_info->is_fpga ? 50 : 5);
	int32_t sleep_time = 0;
	while (dec_dev_isr_state(dev->dev_id) != DEC_OK) {
		if (sleep_time >= vdh_time_out)
			return OSAL_ERR;

		sleep_time += poll_interval_ms;
		OS_MSLEEP(poll_interval_ms);
	}
	dec_dev_isr(dev->dev_id);

	return OSAL_OK;
}

static int32_t dec_dev_decode_status(dec_dev_info *dev, dec_back_up_list *back_up_list)
{
	struct vfmw_global_info *glb_info = pdt_get_glb_info();
	int32_t vdh_time_out = (glb_info->is_fpga ? VDH_FPGA_TIME_OUT : VDH_TIME_OUT);
	if (dev->poll_irq_enable)
		return dec_dev_poll_irq(dev);

	return OS_EVENT_WAIT(back_up_list->event, vdh_time_out);
}

static int32_t dec_dev_block_wait_vdh(dec_back_up *back_up,
	dec_dev_info *dev, dec_back_up_list *back_up_list)
{
	struct vfmw_global_info *glb_info = pdt_get_glb_info();
	back_up->is_need_wait = 0;
	while (dec_dev_reglist_get_back_up(back_up, back_up_list) != DEC_OK) {
		if (dec_dev_decode_status(dev, back_up_list) != OSAL_OK) {
			if (dev->reg_dump_enable || glb_info->is_fpga) {
				dprint(PRN_ERROR, "decoder timeout, dump reg[%d] now\n", back_up_list->get_index);
				dec_dev_dump_reg_proc(dev);
			}

			if (dev->reset_disable) {
				dprint(PRN_FATAL, "decoder timeout, not reset vdh\n");
				return DEC_ERR;
			}
			dprint(PRN_FATAL, "decoder timeout, dec reset now\n");
			dec_dev_reset(dev);

			return DEC_ERR;
		}
	}

	if (back_up->err_flag && (dev->reg_dump_enable || glb_info->is_fpga)) {
		dprint(PRN_ERROR, "decoder error, dump reg now\n");
		dec_dev_dump_reg_proc(dev);
	}

	return DEC_OK;
}

int32_t dec_dev_get_backup(void *dev_cfg, uint32_t instance_id, void *read_backup)
{
	dec_dev_cfg *cfg = (dec_dev_cfg *)dev_cfg;
	dec_dev_info *dev = VCODEC_NULL;
	dec_back_up_list *back_up_list = VCODEC_NULL;
	uint16_t dev_id = (get_current_sec_mode() == SEC_MODE) ? 1 : 0;
	if (dec_dev_get_dev(dev_id, &dev) != DEC_OK) {
		dprint(PRN_FATAL, "get dec dev failed\n");
		return DEC_ERR;
	}

	vfmw_assert_ret(dev->state == DEC_DEV_STATE_RUN, DEC_ERR);

	back_up_list = dec_dev_get_backup_reglist(instance_id);
	vfmw_assert_ret(back_up_list, DEC_ERR);
	if (get_current_sec_mode() == NO_SEC_MODE)
		return dec_dev_block_wait_vdh(read_backup, dev, back_up_list);
	else
		return dec_dev_no_block_wait_vdh(cfg, read_backup, dev, back_up_list);
}

static void dec_dev_suspend_vdh(dec_dev_info *dec_dev)
{
	int32_t reg_id;
	unsigned long lock_flag;
	int32_t used_count = 0;
	int32_t wait_count = 0;
	dec_dev_info *dev = (dec_dev_info *)dec_dev;
	dec_dev_vdh *vdh = (dec_dev_vdh *)dev->handle;
	vdh_reg_pool_info *reg_pool = &vdh->vdh_reg_pool;

	do {
		if (wait_count > DEC_VDH_WAIT_COUNT)
			dec_dev_reset(dev);

		OS_SPIN_LOCK(reg_pool->spin_lock, &lock_flag);
		for (reg_id = 0, used_count = 0; reg_id < VDH_REG_NUM; reg_id++) {
			if (reg_pool->vdh_reg[reg_id].used == VCODEC_TRUE)
				used_count++;
		}
		OS_SPIN_UNLOCK(reg_pool->spin_lock, &lock_flag);

		if (used_count == 0)
			break;

		wait_count++;
		OS_MSLEEP(DEC_VDH_WAIT_TIME);
	} while (used_count);
}

void dec_dev_suspend(void)
{
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t dev_id;

	for (dev_id = 0; dev_id < DEC_DEV_NUM; dev_id++) {
		if (dec_dev_get_dev(dev_id, &dev) != DEC_OK)
			continue;

		if (dev->state == DEC_DEV_STATE_NULL)
			continue;

		dec_dev_suspend_vdh(dev);
		dev->state = DEC_DEV_STATE_SUSPEND;
	}
}

void dec_dev_resume(void)
{
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t dev_id;

	for (dev_id = 0; dev_id < DEC_DEV_NUM; dev_id++) {
		if (dec_dev_get_dev(dev_id, &dev) != DEC_OK)
			continue;

		if (dev->state == DEC_DEV_STATE_NULL)
			continue;

		dec_dev_get_active_reg_id(dev);
		dev->state = DEC_DEV_STATE_RUN;
	}
}

void dec_dev_write_store_info(uint32_t pid)
{
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t dev_id = (get_current_sec_mode() == SEC_MODE) ? 1 : 0;
	if (dec_dev_get_dev(dev_id, &dev) != DEC_OK) {
		dprint(PRN_FATAL, "get dec dev failed\n");
		return;
	}

	dev->ops->set_pid_info(dev, pid);
}

uint32_t dec_dev_read_store_info(void)
{
	dec_dev_info *dev = VCODEC_NULL;
	uint16_t dev_id = (get_current_sec_mode() == SEC_MODE) ? 1 : 0;
	if (dec_dev_get_dev(dev_id, &dev) != DEC_OK) {
		dprint(PRN_FATAL, "get dec dev failed\n");
		return 0;
	}

	return dev->ops->get_pid_info(dev);
}

int32_t dec_dev_acquire_back_up_fifo(uint32_t instance_id)
{
	dec_dev_pool *dev_pool = dec_dev_get_pool();
	dec_back_up_list *back_up_list = VCODEC_NULL;

	vfmw_assert_ret(instance_id < VFMW_CHAN_NUM, DEC_ERR);

	back_up_list = &dev_pool->dev_irq_backup_list[instance_id];
	(void)memset_s(back_up_list, sizeof(dec_back_up_list), 0, sizeof(dec_back_up_list));
	OS_EVENT_INIT(&back_up_list->event, 0);
	OS_SPIN_LOCK_INIT(&back_up_list->lock);

	return DEC_OK;
}

void dec_dev_release_back_up_fifo(uint32_t instance_id)
{
	dec_dev_pool *dev_pool = dec_dev_get_pool();
	dec_back_up_list *back_up_list = VCODEC_NULL;

	vfmw_assert(instance_id < VFMW_CHAN_NUM);

	back_up_list = &dev_pool->dev_irq_backup_list[instance_id];
	OS_EVENT_EXIT(back_up_list->event);
	OS_SPIN_LOCK_EXIT(back_up_list->lock);
	(void)memset_s(back_up_list, sizeof(dec_back_up_list), 0, sizeof(dec_back_up_list));

	return;
}

