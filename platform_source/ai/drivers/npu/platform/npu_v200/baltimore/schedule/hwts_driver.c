/*
 * hwts_driver.c
 *
 * about hwts driver
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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

#include <asm/io.h>
#include <linux/bitmap.h>
#include "hwts_driver.h"
#include "npu_hwts_plat.h"
#include "npu_log.h"

#define reg_mask(len, offset) ((64 == (len)) ? 0xFFFFFFFFFFFFFFFF : (((1ULL << (len)) - 1) << (offset)))

#define reg_field_make(val, len, offset) ((((uint64_t)(val)) << (offset)) & reg_mask((len), (offset)))

#define reg_field_extract(reg, len, offset) (((reg)&reg_mask((len), (offset))) >> (offset))

#define reg_field_insert(reg, len, offset, val) \
	((reg) = ((reg) & (~reg_mask((len), (offset)))) | reg_field_make((val), (len), (offset)))

void hwts_driver_reset_cq_head(uint16_t cq_idx)
{
	uint64_t hwts_reg_addr;
	uint16_t cq_tail;
	uint64_t hwts_base_addr = npu_hwts_get_base_addr();

	SOC_NPU_HWTS_HWTS_CQ_CFG_UNION cq_cfg = { 0 };

	hwts_reg_addr = SOC_NPU_HWTS_HWTS_CQ_CFG_ADDR(hwts_base_addr, cq_idx);
	cq_cfg.value = readq(hwts_reg_addr);
	cq_tail = cq_cfg.reg.cq_tail;

	SOC_NPU_HWTS_HWTS_CQ_DB_UNION cq_db = { 0 };
	hwts_reg_addr = SOC_NPU_HWTS_HWTS_CQ_DB_ADDR(hwts_base_addr, cq_idx);
	cq_db.value = readq(hwts_reg_addr);
	cq_db.reg.cq_head = cq_tail;
	writeq(cq_db.value, hwts_reg_addr);
}

void hwts_driver_clear_channel_sq_en(uint16_t channel_id)
{
	uint64_t hwts_reg_addr;
	uint64_t reg_val;
	uint64_t hwts_base_addr = npu_hwts_get_base_addr();

	hwts_reg_addr = SOC_NPU_HWTS_HWTS_SQ_CFG0_ADDR(hwts_base_addr, channel_id);
	reg_val = readq(hwts_reg_addr);
	bitmap_clear(&reg_val, SOC_NPU_HWTS_HWTS_SQ_CFG0_sq_en_START, 1);
	writeq(reg_val, hwts_reg_addr);
}

void hwts_driver_reset_event_table(uint16_t event_id)
{
	uint64_t hwts_reg_addr;
	uint64_t hwts_base_addr = npu_hwts_get_base_addr();

	hwts_reg_addr = SOC_NPU_HWTS_HWTS_EVENT_TABLE_ADDR(hwts_base_addr, event_id);
	writeq(0, hwts_reg_addr);
	return;
}

void hwts_driver_schedule_sq(struct hwts_sq_cq *sqcq_info, uint16_t channel_id)
{
	uint64_t reg_val;
	uint64_t hwts_reg_addr;
	uint64_t hwts_base_addr = npu_hwts_get_base_addr();
	uint16_t priority = sqcq_info->priority;

	writeq(0xffffffffffffffff, SOC_NPU_HWTS_HWTS_L2_NORMAL_SQ_DONE_NS_MASK0_SET_ADDR(hwts_base_addr, 0));
	reg_val = readq(hwts_base_addr + 0x3480);

	struct schedule_sq_cq *sq_info = hwts_sq_cq_get_sq(sqcq_info);
	struct schedule_sq_cq *cq_info = hwts_sq_cq_get_cq(sqcq_info);

	npu_drv_info("channel_id=%u, sq id=%u, cq id=%u, priority=%u", channel_id, sq_info->id, cq_info->id, priority);
	npu_drv_info("sq head=%u, sq tail=%u, sq_length = %u, cq head=%u, cq tail=%u, cq_length = %u",
		sq_info->head, sq_info->tail, sq_info->length, cq_info->head, cq_info->tail, cq_info->length);

	/* step 1. write hwts sq, write sq base addr, using physical address */
	hwts_reg_addr = SOC_NPU_HWTS_HWTS_SQ_BASE_ADDR_ADDR(hwts_base_addr, channel_id);
	reg_val = (sq_info->base_addr << SOC_NPU_HWTS_HWTS_SQ_BASE_ADDR_sq_base_addr_START) |
		(1ULL << SOC_NPU_HWTS_HWTS_SQ_BASE_ADDR_sq_base_addr_is_virtual_START);
	writeq(reg_val, hwts_reg_addr);

	/* step 2. Set the info of corresponding cq, associate cq buffer with sq */
	hwts_reg_addr = SOC_NPU_HWTS_HWTS_SQ_CFG1_ADDR(hwts_base_addr, channel_id);
	reg_val = readq(hwts_reg_addr);
	reg_field_insert(reg_val,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_cqid_END - SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_cqid_START + 1,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_cqid_START, channel_id);

	reg_field_insert(reg_val,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_priority_END - SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_priority_START + 1,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_priority_START, priority);

	reg_field_insert(reg_val,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_aic_poolid_END - SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_aic_poolid_START + 1,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_aic_poolid_START, 0); /* default aicore pool 0 */

	reg_field_insert(reg_val,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_smmu_substream_id_END - SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_smmu_substream_id_START + 1,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_smmu_substream_id_START, sqcq_info->smmu_substream_id); /* smmu substream id */

	reg_field_insert(reg_val,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_log_en_END - SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_log_en_START + 1,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_log_en_START, 0); /* sq_log_disable */

	reg_field_insert(reg_val,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_profile_en_END - SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_profile_en_START + 1,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_sq_profile_en_START, 0); /* sq_profiling_disable */
	writeq(reg_val, hwts_reg_addr);

	/* write cq base addr */
	hwts_reg_addr = SOC_NPU_HWTS_HWTS_CQ_BASE_ADDR_ADDR(hwts_base_addr, channel_id); /* channel_id/cq_id */
	reg_val = (cq_info->base_addr << SOC_NPU_HWTS_HWTS_CQ_BASE_ADDR_cq_base_addr_START) |
		(1ULL << SOC_NPU_HWTS_HWTS_CQ_BASE_ADDR_cq_base_addr_is_virtual_START);
	writeq(reg_val, hwts_reg_addr);

	/* set cq length */
	hwts_reg_addr = SOC_NPU_HWTS_HWTS_CQ_CFG_ADDR(hwts_base_addr, channel_id);
	reg_val = readq(hwts_reg_addr);
	reg_val |= (1024ULL << SOC_NPU_HWTS_HWTS_CQ_CFG_cq_length_START);
	writeq(reg_val, hwts_reg_addr);

	/* set sq tail */
	hwts_reg_addr = SOC_NPU_HWTS_HWTS_SQ_DB_ADDR(hwts_base_addr, channel_id);
	reg_val = ((uint64_t)sq_info->tail << SOC_NPU_HWTS_HWTS_SQ_DB_sq_tail_START);
	writeq(reg_val, hwts_reg_addr);

	/* sq head length info and sq en */
	hwts_reg_addr = SOC_NPU_HWTS_HWTS_SQ_CFG0_ADDR(hwts_base_addr, channel_id);
	reg_val = ((uint64_t)sq_info->head << SOC_NPU_HWTS_HWTS_SQ_CFG0_sq_head_START) |
		((uint64_t)sq_info->length << SOC_NPU_HWTS_HWTS_SQ_CFG0_sq_length_START) |
		(1ULL << SOC_NPU_HWTS_HWTS_SQ_CFG0_sq_en_START);
	writeq(reg_val, hwts_reg_addr);
}
