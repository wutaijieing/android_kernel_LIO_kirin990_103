/*
 * smmu.c
 *
 * This is for smmu driver.
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

#ifdef ENV_ARMLINUX_KERNEL
#include <asm/memory.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/delay.h>
#endif
#include <linux/slab.h>
#include <linux/mm_iommu.h>
#include "dbg.h"
#include "smmu.h"
#include "smmu_regs.h"
#include "smmu_cfg.h"
#include "vfmw_ext.h"
#include "vcodec_vdec_regulator.h"
#include "vcodec_vdec_plat.h"
#ifdef PCIE_FPGA_VERIFY
#include <linux/platform_drivers/mm_svm.h>
#endif

#define TBU_ACK_TIMEOUT 1000

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#define IO_REMAP_NO_CACHE ioremap
#else
#define IO_REMAP_NO_CACHE ioremap_nocache
#endif

struct smmu_entry g_smmu_entry;

struct smmu_entry *smmu_get_entry(void)
{
	return &g_smmu_entry;
}

#define tbu_reg_vir() (smmu_get_entry()->reg_info.smmu_tbu_reg_vir)
#define sid_reg_vir() (smmu_get_entry()->reg_info.smmu_sid_reg_vir)

#define rd_smmu_tbu_vreg(reg, dat) (dat = *((volatile uint32_t *)(tbu_reg_vir() + (reg))))
#define wr_smmu_tbu_vreg(reg, dat) (*((volatile uint32_t *)(tbu_reg_vir() + (reg))) = dat)

#define rd_smmu_sid_vreg(reg, dat) (dat = *((volatile uint32_t *)(sid_reg_vir() + (reg))))
#define wr_smmu_sid_vreg(reg, dat) (*((volatile uint32_t *)(sid_reg_vir() + (reg))) = dat)

static void set_tbu_reg(int32_t addr, uint32_t val, uint32_t bw, uint32_t bs)
{
	uint32_t mask = (1UL << bw) - 1UL;
	uint32_t tmp = 0;

	rd_smmu_tbu_vreg(addr, tmp);
	tmp &= ~(mask << bs);
	wr_smmu_tbu_vreg(addr, tmp | ((val & mask) << bs));
}

static void set_sid_reg(int32_t addr, uint32_t val, uint32_t bw, uint32_t bs)
{
	uint32_t mask = (1UL << bw) - 1UL;
	uint32_t tmp = 0;

	rd_smmu_sid_vreg(addr, tmp);
	tmp &= ~(mask << bs);
	wr_smmu_sid_vreg(addr, tmp | ((val & mask) << bs));
}

void smmu_cfg_one_sid(uint32_t reg_base_offset)
{
	set_sid_reg(reg_base_offset + SMMU_NORM_RSID, SMMU_NORM_RSID_VALUE, 8, 0); /* 8: bit width */
	set_sid_reg(reg_base_offset + SMMU_NORM_WSID, SMMU_NORM_WSID_VALUE, 8, 0); /* 8: bit width */

	set_sid_reg(reg_base_offset + SMMU_NORM_RSSID, 0, 1, 0);
	set_sid_reg(reg_base_offset + SMMU_NORM_WSSID, 0, 1, 0);

	set_sid_reg(reg_base_offset + SMMU_NORM_RSSIDV, 1, 1, 0);
	set_sid_reg(reg_base_offset + SMMU_NORM_WSSIDV, 1, 1, 0);
}

int32_t smmu_power_on_tcu(void)
{
#ifndef PCIE_FPGA_VERIFY
	struct regulator *tcu_regulator =
		vdec_plat_get_entry()->regulator_info.regulators[SMMU_TCU_REGULATOR];

	return regulator_enable(tcu_regulator);
#else
	return mm_smmu_poweron(vdec_plat_get_entry()->dev);
#endif
}

void smmu_power_off_tcu(void)
{
#ifndef PCIE_FPGA_VERIFY
	int32_t ret;
	struct regulator *tcu_regulator =
		vdec_plat_get_entry()->regulator_info.regulators[SMMU_TCU_REGULATOR];

	ret = regulator_disable(tcu_regulator);
	if (ret)
		dprint(PRN_ERROR, "power off tcu failed\n");
#else
	mm_smmu_poweroff(vdec_plat_get_entry()->dev);
#endif
}

int32_t smmu_map_reg(void)
{
	UADDR reg_phy;
	uint8_t *reg_vaddr = VCODEC_NULL;
	vdec_dts *dts_info = &(vdec_plat_get_entry()->dts_info);
	struct smmu_entry *entry = smmu_get_entry();

	(void)memset_s(entry, sizeof(*entry), 0, sizeof(*entry));

	reg_phy = dts_info->module_reg[MMU_SID_MOUDLE].reg_phy_addr;
	reg_vaddr = (uint8_t *)IO_REMAP_NO_CACHE(reg_phy, dts_info->module_reg[MMU_SID_MOUDLE].reg_range);
	if (!reg_vaddr) {
		dprint(PRN_ERROR, "map mmu sid reg failed\n");
		return SMMU_ERR;
	}

	entry->reg_info.smmu_sid_reg_vir = reg_vaddr;

	reg_phy = dts_info->module_reg[MMU_TBU_MODULE].reg_phy_addr;
	reg_vaddr = (uint8_t *)IO_REMAP_NO_CACHE(reg_phy, dts_info->module_reg[MMU_TBU_MODULE].reg_range);
	if (!reg_vaddr) {
		dprint(PRN_ERROR, "map mmu tbu reg failed\n");
		iounmap(entry->reg_info.smmu_sid_reg_vir);
		entry->reg_info.smmu_sid_reg_vir = VCODEC_NULL;
		return SMMU_ERR;
	}

	entry->reg_info.smmu_tbu_reg_vir = reg_vaddr;

	return SMMU_OK;
}

void smmu_unmap_reg(void)
{
	struct smmu_entry *entry = smmu_get_entry();

	if (entry->reg_info.smmu_tbu_reg_vir) {
		iounmap(entry->reg_info.smmu_tbu_reg_vir);
		entry->reg_info.smmu_tbu_reg_vir = VCODEC_NULL;
	}

	if (entry->reg_info.smmu_sid_reg_vir) {
		iounmap(entry->reg_info.smmu_sid_reg_vir);
		entry->reg_info.smmu_sid_reg_vir = VCODEC_NULL;
	}
}

static int32_t smmu_cfg_one_tbu(uint32_t reg_base_offset)
{
	uint32_t i;
	uint32_t rel;
	uint32_t rel_cr;
	uint32_t ack_timeout;
	vdec_plat_ops * plat_ops = NULL;

	vdec_dts *dts_info = &(vdec_plat_get_entry()->dts_info);
	ack_timeout = dts_info->is_fpga ? TBU_ACK_TIMEOUT * 10 : TBU_ACK_TIMEOUT;

	plat_ops = get_vdec_plat_ops();
	if (plat_ops->smmu_need_cfg_max_tok_trans())
		set_tbu_reg(reg_base_offset + SMMU_TBU_CR, 0x17, 8, 8);
	set_tbu_reg(reg_base_offset + SMMU_TBU_CR, 0x1, 1, 0);

	for (i = 0; i < ack_timeout; i++) {
		udelay(1);
		rd_smmu_tbu_vreg((reg_base_offset + SMMU_TBU_CRACK), rel);
		if ((rel & 0x3) == 0x3)
			break;
	}

	if (i == ack_timeout)
		return SMMU_ERR;

	rd_smmu_tbu_vreg((reg_base_offset + SMMU_TBU_CR), rel_cr);
	if ((rel & 0xff00) < (rel_cr & 0xff00)) {
		dprint(PRN_FATAL, "check tok_trans_gnt failed\n");
		return SMMU_ERR;
	}

	return SMMU_OK;
}

static int32_t smmu_config_reg(struct smmu_entry *entry)
{
	int32_t ret;
	uint32_t reg_index;
	uint32_t reg_base_offset;
	uint32_t tbu_num = entry->tbu_info.mmu_tbu_num;
	uint32_t one_tbu_offset = entry->tbu_info.mmu_tbu_offset;
	uint32_t one_sid_offset = entry->tbu_info.mmu_sid_offset;

	dprint(PRN_CTRL, "tbu_num %u one_tbu_offset 0x%x one_sid_offset 0x%x\n", tbu_num, one_tbu_offset, one_sid_offset);

	for (reg_index = 0; reg_index < tbu_num; reg_index++) {
		reg_base_offset = reg_index * one_tbu_offset;
		ret = smmu_cfg_one_tbu(reg_base_offset);
		if (ret != SMMU_OK) {
			dprint(PRN_FATAL, "reg_index %u tbu and tcu connect failed\n", reg_index);
			return SMMU_ERR;
		}
		dprint(PRN_CTRL, "reg_index %u tbu and tcu connect success\n", reg_index);

		reg_base_offset = reg_index * one_sid_offset;
		smmu_cfg_one_sid(reg_base_offset);
	}

	return SMMU_OK;
}

static void smmu_init_entry(struct smmu_entry *entry)
{
	struct smmu_tbu_info *tbu_info = &(vdec_plat_get_entry()->smmu_info);

	entry->tbu_info.mmu_tbu_num = tbu_info->mmu_tbu_num;
	entry->tbu_info.mmu_tbu_offset = tbu_info->mmu_tbu_offset;
	entry->tbu_info.mmu_sid_offset = tbu_info->mmu_sid_offset;
}

int32_t smmu_init(void)
{
	struct smmu_entry *entry = smmu_get_entry();

	smmu_init_entry(entry);
	if (smmu_power_on_tcu()) {
		dprint(PRN_FATAL, "power on tcu failed\n");
		return SMMU_ERR;
	}
	if (smmu_config_reg(entry) != SMMU_OK) {
		smmu_power_off_tcu();
		return SMMU_ERR;
	}

	entry->smmu_init = 1;
	return SMMU_OK;
}

static int32_t smmu_deinit_one_tbu(uint32_t reg_base_offset)
{
	uint32_t i;
	uint32_t rel;
	uint32_t ack_timeout;

	vdec_dts *dts_info = &(vdec_plat_get_entry()->dts_info);
	ack_timeout = dts_info->is_fpga ? TBU_ACK_TIMEOUT * 10 : TBU_ACK_TIMEOUT;

	set_tbu_reg(reg_base_offset + SMMU_TBU_CR, 0x0, 1, 0);
	for (i = 0; i < ack_timeout; i++) {
		udelay(1);
		rd_smmu_tbu_vreg((reg_base_offset + SMMU_TBU_CRACK), rel);
		if ((rel & 0x3) == 0x1)
			break;
	}

	if (i == ack_timeout)
		return SMMU_ERR;

	return SMMU_OK;
}

static void smmu_deinit_tbu(struct smmu_entry *entry)
{
	int32_t ret;
	uint32_t tbu_index;
	uint32_t tbu_num = entry->tbu_info.mmu_tbu_num;
	uint32_t one_tbu_offset = entry->tbu_info.mmu_tbu_offset;
	uint32_t reg_base_offset;

	for (tbu_index = 0; tbu_index < tbu_num; tbu_index++) {
		reg_base_offset = tbu_index * one_tbu_offset;
		ret = smmu_deinit_one_tbu(reg_base_offset);
		if (ret != SMMU_OK)
			dprint(PRN_ERROR, "reg_index %u tbu and tcu disconnect failed\n", tbu_index);
		else
			dprint(PRN_CTRL, "reg_index %u tbu and tcu disconnect success\n", tbu_index);
	}

	return;
}

void smmu_deinit(void)
{
	struct smmu_entry *entry = smmu_get_entry();

	if (entry->smmu_init != 1) {
		dprint(PRN_ERROR, "smmu not init\n");
		return;
	}

	smmu_deinit_tbu(entry);
	smmu_power_off_tcu();

	entry->smmu_init = 0;
}

