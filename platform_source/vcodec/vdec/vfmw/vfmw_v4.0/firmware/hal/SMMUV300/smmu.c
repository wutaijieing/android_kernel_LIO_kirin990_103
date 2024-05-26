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
#include <asm/io.h>
#include <linux/delay.h>
#endif
#include <linux/slab.h>
#include <linux/mm_iommu.h>
#include <linux/regulator/consumer.h>
#include "smmu.h"
#include "smmu_regs.h"
#include "public.h"
#include "regulator.h"

#define TBU_ACK_TIMEOUT 1000

struct smmu_entry {
	struct smmu_reg_info reg_info;
	struct smmu_cfg_info cfg_info;
	uint8_t smmu_init;
};

struct smmu_entry g_smmu_entry;
struct smmu_entry *smmu_get_entry(void)
{
	return &g_smmu_entry;
}

#define tbu_reg_vir() (smmu_get_entry()->reg_info.smmu_tbu_reg_vir)
#define sid_reg_vir() (smmu_get_entry()->reg_info.smmu_sid_reg_vir)

#define rd_smmu_tbu_vreg(reg, dat) \
	(dat = *((volatile uint32_t *)((uint8_t *)tbu_reg_vir() + (reg))))
#define wr_smmu_tbu_vreg(reg, dat) \
	(*((volatile uint32_t *)((uint8_t *)tbu_reg_vir() + (reg))) = dat)

#define rd_smmu_sid_vreg(reg, dat) \
	(dat = *((volatile uint32_t *)((uint8_t *)sid_reg_vir() + (reg))))
#define wr_smmu_sid_vreg(reg, dat) \
	(*((volatile uint32_t *)((uint8_t *)sid_reg_vir() + (reg))) = dat)

// mfde regs r/w
#define mfde_reg_vir() (smmu_get_entry()->reg_info.smmu_mfde_reg_vir)
#define rd_smmu_mfde_vreg(reg, dat) \
	dat = readl(((volatile uint32_t *)((uint8_t *)mfde_reg_vir() + (reg))));

#define wr_smmu_mfde_vreg(reg, dat) \
	writel(dat, ((volatile uint32_t *)((uint8_t *)mfde_reg_vir() + (reg))));

// scd regs r/w
#define scd_reg_vir() (smmu_get_entry()->reg_info.smmu_scd_reg_vir)
#define rd_smmu_scd_vred(reg, dat) \
	dat = readl(((volatile uint32_t *)((uint8_t *)scd_reg_vir() + (reg))));

#define wr_smmu_scd_vreg(reg, dat) \
	writel(dat, ((volatile uint32_t *)((uint8_t *)scd_reg_vir() + (reg))));

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

void smmu_cfg_sid(void)
{
	struct smmu_entry *entry = smmu_get_entry();
	set_sid_reg(SMMU_NORM_RSID, entry->cfg_info.smmu_norm_sid, 8, 0); // 8: bit width
	set_sid_reg(SMMU_NORM_WSID, entry->cfg_info.smmu_norm_sid, 8, 0); // 8: bit width

	set_sid_reg(SMMU_NORM_RSSID, 0, 1, 0);
	set_sid_reg(SMMU_NORM_WSSID, 0, 1, 0);

	set_sid_reg(SMMU_NORM_RSSIDV, 1, 1, 0);
	set_sid_reg(SMMU_NORM_WSSIDV, 1, 1, 0);
}

int32_t smmu_power_on_tcu(void)
{
	struct regulator *tcu_regulator = get_tcu_regulator();

	return regulator_enable(tcu_regulator);
}

void smmu_power_off_tcu(void)
{
	int32_t ret;
	struct regulator *tcu_regulator = get_tcu_regulator();

	ret = regulator_disable(tcu_regulator);
	if (ret)
		dprint(PRN_ERROR, "power off tcu failed\n");
}

static int32_t update_smmu_reg_vir(void)
{
	struct smmu_entry *entry = smmu_get_entry();

	entry->reg_info.smmu_mfde_reg_vir = (uint32_t *) mem_phy_2_vir(g_vdh_reg_base_addr);
	if (!entry->reg_info.smmu_mfde_reg_vir) {
		dprint(PRN_FATAL, "%s smmu_mfde_reg_vir is NULL, SMMU Init failed\n", __func__);
		return SMMU_ERR;
	}

	entry->reg_info.smmu_scd_reg_vir = (uint32_t *) mem_phy_2_vir(g_scd_reg_base_addr);
	if (!entry->reg_info.smmu_scd_reg_vir) {
		dprint(PRN_FATAL, "%s smmu_scd_reg_vir is NULL, SMMU Init failed\n", __func__);
		return SMMU_ERR;
	}

	entry->reg_info.smmu_tbu_reg_vir =
		(uint32_t *) mem_phy_2_vir(g_vdh_reg_base_addr + entry->cfg_info.smmu_tbu_offset);
	if (!entry->reg_info.smmu_tbu_reg_vir) {
		dprint(PRN_FATAL, "%s smmu_tbu_reg_vir is NULL, SMMU Init failed\n", __func__);
		return SMMU_ERR;
	}

	entry->reg_info.smmu_sid_reg_vir =
		(uint32_t *) mem_phy_2_vir(g_vdh_reg_base_addr + entry->cfg_info.smmu_sid_offset);
	if (!entry->reg_info.smmu_sid_reg_vir) {
		dprint(PRN_FATAL, "%s smmu_sid_reg_vir is NULL, SMMU Init failed\n", __func__);
		return SMMU_ERR;
	}

	return SMMU_OK;
}

void smmu_init_entry(struct smmu_entry *entry)
{
	struct smmu_cfg_info *smmu_config = get_smmu_cfg_info();

	entry->cfg_info.smmu_tbu_offset = smmu_config->smmu_tbu_offset;
	entry->cfg_info.smmu_sid_offset = smmu_config->smmu_sid_offset;
	entry->cfg_info.smmu_norm_sid = smmu_config->smmu_norm_sid;
	entry->cfg_info.smmu_need_cfg = smmu_config->smmu_need_cfg;
}

int32_t smmu_init(void)
{
	return SMMU_OK;
}

int32_t smmu_v3_init(void)
{
	int32_t i;
	uint32_t rel;
	uint32_t rel_cr;
	struct smmu_entry *entry = smmu_get_entry();
	int32_t ret = memset_s(entry, sizeof(*entry), 0, sizeof(*entry));
	if (ret != EOK) {
		dprint(PRN_FATAL, " %s %d memset_s err in function\n", __func__, __LINE__);
		return SMMU_ERR;
	}
	uint32_t ack_timeout = g_is_fpga ? TBU_ACK_TIMEOUT * 10 : TBU_ACK_TIMEOUT;

	smmu_init_entry(entry);
	if (update_smmu_reg_vir()) {
		return SMMU_ERR;
	}

	if (smmu_power_on_tcu()) {
		dprint(PRN_FATAL, "power on tcu failed\n");
		return SMMU_ERR;
	}

	set_tbu_reg(SMMU_TBU_SCR, 0x1, 1, 0);
	if (entry->cfg_info.smmu_need_cfg)
		set_tbu_reg(SMMU_TBU_CR, 0x17, 8, 8); // 8: bit width, 8: bit start
	set_tbu_reg(SMMU_TBU_CR, 0x1, 1, 0);

	for (i = 0; i < ack_timeout; i++) {
		udelay(1);
		rd_smmu_tbu_vreg(SMMU_TBU_CRACK, rel);
		if ((rel & 0x3) == 0x3)
			break;
	}

	if (i == ack_timeout) {
		dprint(PRN_FATAL, "tbu and tcu connect failed\n");
		goto POWER_OFF_TCU;
	}

	dprint(PRN_ALWS, "tbu and tcu connect success\n");

	rd_smmu_tbu_vreg(SMMU_TBU_CR, rel_cr);
	if ((rel & 0xff00) < (rel_cr & 0xff00)) {
		dprint(PRN_FATAL, "check tok_trans_gnt failed\n");
		goto POWER_OFF_TCU;
	}

	entry->smmu_init = 1;
	smmu_cfg_sid();
	return SMMU_OK;

POWER_OFF_TCU:
	smmu_power_off_tcu();

	return SMMU_ERR;
}

void smmu_deinit(void)
{
	int32_t i;
	uint32_t rel;
	struct smmu_entry *entry = smmu_get_entry();
	uint32_t ack_timeout = g_is_fpga ? TBU_ACK_TIMEOUT * 10 : TBU_ACK_TIMEOUT;

	if (entry->smmu_init != 1) {
		dprint(PRN_ERROR, "smmu not init\n");
		return;
	}

	set_tbu_reg(SMMU_TBU_CR, 0x0, 1, 0);

	for (i = 0; i < ack_timeout; i++) {
		udelay(1);
		rd_smmu_tbu_vreg(SMMU_TBU_CRACK, rel);
		if ((rel & 0x3) == 0x1)
			break;
	}

	if (i == ack_timeout)
		dprint(PRN_ERROR, "tbu and tcu disconnect failed\n");

	dprint(PRN_CTRL, "tbu and tcu disconnect success\n");

	smmu_power_off_tcu();
	entry->smmu_init = 0;
}

static void set_vdh_master_reg(
	enum smmu_master_type master_type, UADDR addr,
	SINT32 val, SINT32 bw, SINT32 bs)
{
	SINT32 mask = (1UL << bw) - 1UL;
	SINT32 reg_value = 0;

	switch (master_type) {
	case MFDE:
		rd_smmu_mfde_vreg(addr, reg_value);
		reg_value &= ~(mask << bs); /*lint !e502*/
		wr_smmu_mfde_vreg(addr, reg_value | ((val & mask) << bs));
		break;
	case SCD:
		rd_smmu_scd_vred(addr, reg_value);
		reg_value &= ~(mask << bs); /*lint !e502*/
		wr_smmu_scd_vreg(addr, reg_value | ((val & mask) << bs));
		break;
	default:
		break;
	}
}

static void set_mmu_cfg_reg_mfde(
	enum smmu_master_type master_type, UINT32 secure_en,
	UINT32 mmu_en)
{
	if (mmu_en) {    // MMU enable
		// [12]mmu_en=1
		set_vdh_master_reg(master_type,
			REG_MFDE_MMU_CFG_EN, 0x1, 1, 12); // 1: bit width, 2: bit start.
		if (secure_en) {  // secure
			dprint(PRN_ALWS, "IN %s not support this mode: mmu_en:secure\n", __func__);
			dprint(PRN_ALWS, "%s, secure_en:%d, mmu_en:%d\n", __func__, secure_en, mmu_en);
		    // [31]secure_en=1
			set_vdh_master_reg(master_type,
				REG_MFDE_MMU_CFG_SECURE, 0x1, 1, 31); // 1: bit width, 31: bit start.
		} else { // non-secure
		    // [31]secure_en=0
			set_vdh_master_reg(master_type,
				REG_MFDE_MMU_CFG_SECURE, 0x0, 1, 31); // 1: bit width, 31: bit start.
		}
	} else {    // MMU disable
	    // [12]mmu_en=0
		set_vdh_master_reg(master_type,
			REG_MFDE_MMU_CFG_EN, 0x0, 1, 12); // 1: bit width, 12: bit start.
		if (secure_en) {    // secure
		    // [31]secure_en=1
			set_vdh_master_reg(master_type,
				REG_MFDE_MMU_CFG_SECURE, 0x1, 1, 31); // 1: bit width, 31: bit start.
		} else {    // non-secure
			dprint(PRN_ALWS, "IN %s not support this mode: non_mmu:non_secure\n", __func__);
			dprint(PRN_ALWS, "%s, secure_en:%d, mmu_en:%d\n", __func__, secure_en, mmu_en);
			// [31]secure_en=0
			set_vdh_master_reg(master_type,
				REG_MFDE_MMU_CFG_SECURE, 0x0, 1, 31); // 1: bit width, 31: bit start.
		}
	}
}

/*
 * function: set scd mmu cfg register
 */
static void set_mmu_cfg_reg_scd(
	enum smmu_master_type master_type, UINT32 secure_en, UINT32 mmu_en)
{
	if (mmu_en) {   // MMU enable
	    // [9]mmu_en=1
		set_vdh_master_reg(master_type, REG_SCD_MMU_CFG, 0x1, 1, 9);
		// [13]rdbuf_mmu_en=1
		set_vdh_master_reg(master_type, REG_SCD_MMU_CFG, 0x1, 1, 13);
		if (secure_en) {    // secure
			dprint(PRN_ALWS, "IN %s not support this mode: mmu_en:secure\n", __func__);
			dprint(PRN_ALWS, "%s, secure_en:%d, mmu_en:%d\n", __func__, secure_en, mmu_en);
			// [7]secure_en=1
			set_vdh_master_reg(master_type,
				REG_SCD_MMU_CFG, 0x1, 1, 7); // 1: bit width, 7: bit start.
		} else {    // non-secure //[7]secure_en=0
			set_vdh_master_reg(master_type,
				REG_SCD_MMU_CFG, 0x0, 1, 7); // 1: bit width, 7: bit start.
		}
	} else {    // MMU disable  [9]mmu_en=0
		set_vdh_master_reg(master_type, REG_SCD_MMU_CFG, 0x0, 1, 9);
		if (secure_en) {    // secure
			// [7]secure_en=1
			set_vdh_master_reg(master_type,
				REG_SCD_MMU_CFG, 0x1, 1, 7); // 1: bit width, 7: bit start.
		} else {    // non-secure
			dprint(PRN_ALWS, "IN %s not support this mode: non_mmu:non_secure\n", __func__);
			dprint(PRN_ALWS, "%s, secure_en:%d, mmu_en:%d\n", __func__, secure_en, mmu_en);
			// [7]secure_en=0
			set_vdh_master_reg(master_type,
				REG_SCD_MMU_CFG, 0x0, 1, 7); // 1: bit width, 7: bit start.
		}
	}
}


void smmu_set_master_reg(
	enum smmu_master_type master_type, UINT8 secure_en,
	UINT8 mmu_en)
{
	switch (master_type) {
	case MFDE:
		set_mmu_cfg_reg_mfde(master_type, secure_en, mmu_en);
		break;

	case SCD:
		set_mmu_cfg_reg_scd(master_type, secure_en, mmu_en);
		break;

	default:
		dprint(PRN_FATAL, "%s unkown master type %d\n", __func__, master_type);
		break;
	}
}

void smmu_init_global_reg(void)
{
}

void smmu_int_serv_proc(void)
{
}
