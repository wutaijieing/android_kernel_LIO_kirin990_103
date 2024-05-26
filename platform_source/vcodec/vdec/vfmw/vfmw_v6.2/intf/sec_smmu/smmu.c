/*
 * smmu.c
 *
 * This is for vdec smmu.
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

#include "vfmw_osal.h"
#include "dbg.h"
#include "smmu.h"
#include "smmu_regs.h"
#include <stdbool.h>
#include "sec_smmu_com.h"

#define SMMU_RWERRADDR_SIZE 128
#define unused(x) (void)(x)

typedef struct {
	uint32_t *smmu_tbu_base_viraddr;
	uint32_t *smmu_sid_cfg_viraddr;
} smmu_reg_vir_s;

smmu_reg_vir_s g_smmu_reg_vir;
static int32_t g_smmu_init_flag;

#define rd_smmu_tbu_vreg(reg, dat) \
	do { \
		dat = *((volatile int32_t *)((int8_t *)g_smmu_reg_vir.smmu_tbu_base_viraddr + (reg))); \
	} while (0)

#define wr_smmu_tbu_vreg(reg, dat) \
	do { \
		__asm__ volatile("dsb sy"); \
		*((volatile int32_t *)((int8_t *)g_smmu_reg_vir.smmu_tbu_base_viraddr + (reg))) = dat; \
		__asm__ volatile("dsb sy"); \
	} while (0)

#define rd_smmu_sid_vreg(reg, dat) \
	do { \
		dat = *((volatile int32_t *)((int8_t *)g_smmu_reg_vir.smmu_sid_cfg_viraddr + (reg))); \
	} while (0)

#define wr_smmu_sid_vreg(reg, dat) \
	do { \
		__asm__ volatile("dsb sy"); \
		*((volatile int32_t *)((int8_t *)g_smmu_reg_vir.smmu_sid_cfg_viraddr + (reg))) = dat; \
		__asm__ volatile("dsb sy"); \
	} while (0)

void set_tbu_reg(int32_t addr, uint32_t val, uint32_t bw, uint32_t bs)
{
	uint32_t mask = (1UL << bw) - 1UL;
	volatile uint32_t tmp = 0;

	rd_smmu_tbu_vreg(addr, tmp);
	tmp &= ~(mask << bs);
	wr_smmu_tbu_vreg(addr, tmp | ((val & mask) << bs));
}

static void set_sid_reg(int32_t addr, uint32_t val, uint32_t bw, uint32_t bs)
{
	uint32_t mask = (1UL << bw) - 1UL;
	volatile uint32_t tmp = 0;

	rd_smmu_sid_vreg(addr, tmp);
	tmp &= ~(mask << bs);
	wr_smmu_sid_vreg(addr, tmp | ((val & mask) << bs));
}

static int8_t *smmu_ioremap(UADDR phy_addr, uint32_t len)
{
	unused(len);
	return (int8_t *)phy_addr;
}

void smmu_iounmap(uint8_t *vir_addr)
{
	unused(vir_addr);
}

static void smmu_cfg_sid(void)
{
	/* sid set */
	set_sid_reg(SMMU_NORM_RSID, 9, 8, 0); // 9: regval, 8: regbw, 0:regbs
	set_sid_reg(SMMU_NORM_WSID, 9, 8, 0); // 9: regval, 8: regbw, 0:regbs

	/* ssid set */
	set_sid_reg(SMMU_NORM_RSSID, 0, 8, 0); // 0: regval, 8: regbw, 0:regbs
	set_sid_reg(SMMU_NORM_WSSID, 0, 8, 0); // 0: regval, 8: regbw, 0:regbs

	/* ssidv set */
	set_sid_reg(SMMU_NORM_RSSIDV, 1, 1, 0); // 1: regval, 1: regbw, 0:regbs
	set_sid_reg(SMMU_NORM_WSSIDV, 1, 1, 0); // 1: regval, 1: regbw, 0:regbs
}

int32_t smmu_init(void)
{
	UADDR phy_addr;
	int8_t *vir_addr = NULL;

	int is_smmu_power_on = sec_smmu_poweron(SMMU_MEDIA2);
	if (is_smmu_power_on != 0) {
		dprint(PRN_ERROR, "smmu power on fail, ret = %d\n", is_smmu_power_on);
		return is_smmu_power_on;
	}

	/* 9: sid, 0: ssid, 0: SVM */
	int is_smmu_bind = sec_smmu_bind(SMMU_MEDIA2, 9, 0, 0);
	if (is_smmu_bind != 0) {
		dprint(PRN_ERROR, "smmu bind fail\n");
		sec_smmu_poweroff(SMMU_MEDIA2);
		return is_smmu_bind;
	}

	(void)memset_s(&g_smmu_reg_vir, sizeof(g_smmu_reg_vir), 0,
		sizeof(g_smmu_reg_vir));

	phy_addr = SMMU_SID_REG_PHY_ADDR; /* smmu sid */
	vir_addr = smmu_ioremap(phy_addr, 0x1000); // 0x1000: phy_addr length
	if (!vir_addr) {
		dprint(PRN_ERROR, "map tcu fail\n");
		sec_smmu_unbind(SMMU_MEDIA2, 9, 0); /* 9: sid, 0: ssid */
		sec_smmu_poweroff(SMMU_MEDIA2);
		return SMMU_ERR;
	}

	g_smmu_reg_vir.smmu_sid_cfg_viraddr = (uint32_t *)(vir_addr);

	phy_addr = SMMU_TBU_REG_PHY_ADDR; /* smmu tbu */
	vir_addr = smmu_ioremap(phy_addr, 0x1000); // 0x1000: phy_addr length
	if (!vir_addr) {
		dprint(PRN_ERROR, "map tbu fail\n");
		smmu_iounmap((uint8_t *)g_smmu_reg_vir.smmu_tbu_base_viraddr);
		g_smmu_reg_vir.smmu_tbu_base_viraddr = NULL;
		sec_smmu_unbind(SMMU_MEDIA2, 9, 0); /* 9: sid, 0: ssid */
		sec_smmu_poweroff(SMMU_MEDIA2);
		return SMMU_ERR;
	}
	g_smmu_reg_vir.smmu_tbu_base_viraddr = (uint32_t *)(vir_addr);

	set_tbu_reg(SMMU_TBU_SCR, 0x0, 1, 0);

	/* #### open tbu #### */
	smmu_cfg_sid();

	dprint(PRN_ERROR, "smmu connect sucess!\n");
	g_smmu_init_flag = 1;
	return SMMU_OK;
}

void smmu_deinit(void)
{
	if (g_smmu_init_flag != 1) {
		dprint(PRN_ALWS, "smmu not init\n");
		return;
	}

	set_tbu_reg(SMMU_TBU_SCR, 0x1, 1, 0);

	smmu_iounmap((uint8_t *)g_smmu_reg_vir.smmu_sid_cfg_viraddr);
	g_smmu_reg_vir.smmu_sid_cfg_viraddr = NULL;

	smmu_iounmap((uint8_t *)g_smmu_reg_vir.smmu_tbu_base_viraddr);
	g_smmu_reg_vir.smmu_tbu_base_viraddr = NULL;

	g_smmu_init_flag = 0;

	/* 9: sid, 0: ssid */
	int is_smmu_bind = sec_smmu_unbind(SMMU_MEDIA2, 9, 0);
	if (is_smmu_bind != 0)
		dprint(PRN_ERROR, "smmu unbind fail\n");
	int is_smmu_power_off = sec_smmu_poweroff(SMMU_MEDIA2);
	if (is_smmu_power_off != 0)
		dprint(PRN_ERROR, "smmu power on fail\n");
}

