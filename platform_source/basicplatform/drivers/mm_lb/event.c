/*
 * event.c
 *
 * pmu config for pmu event statistics
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define WAY_MASK           0xffff

#ifdef CONFIG_MM_LB_GEMINI
#define PMU_STEP           1
#else
#define PMU_STEP           2
#endif

static void pmu_slc_no_cfg(u32 r_reg0, u32 f_reg1, u32 f_reg2, u32 f_reg3)
{
	lb_print(LB_INFO, "%s 0x%x\n", __func__, r_reg0);
}

static void pmu_slc_bp5_cfg(u32 r_reg0, u32 f_reg1, u32 f_reg2, u32 f_reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, r_reg0);

		writel(r_reg0, man.lbd->base + SLC_ADDR400(SLC_BP5_CFG, i));
	}
}

static void pmu_slc_bp6_cfg(u32 r_reg0, u32 f_reg1, u32 f_reg2, u32 f_reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, r_reg0);

		writel(r_reg0, man.lbd->base + SLC_ADDR400(SLC_BP6_CFG, i));
	}
}

static void pmu_slc_bp7_cfg(u32 r_reg0, u32 r_reg1, u32 r_reg2, u32 f_reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x\n",
				__func__, r_reg0, r_reg1, r_reg2);

		writel(r_reg0, man.lbd->base + SLC_ADDR400(SLC_BP7_CFG_0, i));
		writel(r_reg1, man.lbd->base + SLC_ADDR400(SLC_BP7_CFG_1, i));
		writel(r_reg2, man.lbd->base + SLC_ADDR400(SLC_BP7_CFG_2, i));
	}
}

static void pmu_slc_bp8_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x 0x%x\n",
				__func__, reg0, reg1, reg2, reg3);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_BP8_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_BP8_CFG_1, i));
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_BP8_CFG_3, i));
		writel(reg3, man.lbd->base + SLC_ADDR400(SLC_BP8_CFG_4, i));
	}
}

static void pmu_slc_bp9_cfg(u32 reg, u32 f_reg1, u32 f_reg2, u32 f_reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(SLC_BP9_CFG_0, i));
	}
}

static void pmu_slc_bp10_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x\n",
				__func__, reg0, reg1, reg2);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_BP10_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_BP10_CFG_1, i));
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_BP10_CFG_2, i));
	}
}

static void pmu_slc_bp11_cfg(u32 reg, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(SLC_BP11_CFG_0, i));
	}
}

static void pmu_slc_bp12_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x\n",
				__func__, reg0, reg1, reg2);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_BP12_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_BP12_CFG_1, i));
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_BP12_CFG_2, i));
	}
}

static void pmu_slc_bp14_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x 0x%x\n",
				__func__, reg0, reg1, reg2, reg3);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_BP14A_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_BP14A_CFG_1, i));
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_BP14B_CFG_0, i));
		writel(reg3, man.lbd->base + SLC_ADDR400(SLC_BP14B_CFG_1, i));
	}
}

static void pmu_slc_bp15_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, reg0);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_BP15_CFG_0, i));
	}
}

static void pmu_slc_tcp0_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x\n",
				__func__, reg0, reg1, reg2);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP0_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_TCP0_CFG_1, i));
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_TCP0_CFG_2, i));
	}
}

static void pmu_slc_tcp1_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x\n",
				__func__, reg0, reg1, reg2);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP1_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_TCP1_CFG_1, i));
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_TCP1_CFG_2, i));
	}
}

static void pmu_slc_tcp2_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x 0x%x\n",
				__func__, reg0, reg1, reg2, reg3);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP2A_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_TCP2A_CFG_1, i));
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_TCP2B_CFG_0, i));
		writel(reg3, man.lbd->base + SLC_ADDR400(SLC_TCP2B_CFG_1, i));
	}
}

static void pmu_slc_tcp3_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x 0x%x\n",
				__func__, reg0, reg1, reg2, reg3);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP3A_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_TCP3A_CFG_1, i));
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_TCP3B_CFG_0, i));
		writel(reg3, man.lbd->base + SLC_ADDR400(SLC_TCP3B_CFG_1, i));
	}
}

static void pmu_slc_tcp4_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, reg0);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP4_CFG_0, i));
	}
}

static void pmu_slc_tcp5_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x\n", __func__, reg0, reg1);

#ifdef CONFIG_MM_LB_GEMINI
		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP5A_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_TCP5A_CFG_1, i));
		writel(WAY_MASK,
			man.lbd->base + SLC_ADDR400(SLC_TCP5A_CFG_2, i));

		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_TCP5B_CFG_0, i));
		writel(reg3, man.lbd->base + SLC_ADDR400(SLC_TCP5B_CFG_1, i));
		writel(WAY_MASK,
			man.lbd->base + SLC_ADDR400(SLC_TCP5B_CFG_2, i));
#else
		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP5_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_TCP5_CFG_1, i));
#endif
	}
}

static void pmu_slc_tcp6_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x 0x%x\n",
			__func__, reg0, reg1, reg2, reg3);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP6A_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_TCP6A_CFG_1, i));
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_TCP6B_CFG_0, i));
		writel(reg3, man.lbd->base + SLC_ADDR400(SLC_TCP6B_CFG_1, i));
	}
}

static void pmu_slc_tcp7_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x 0x%x\n",
			__func__, reg0, reg1, reg2, reg3);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP7A_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_TCP7A_CFG_1, i));
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_TCP7B_CFG_0, i));
		writel(reg3, man.lbd->base + SLC_ADDR400(SLC_TCP7B_CFG_1, i));
	}

	writel(WAY_MASK, man.lbd->base + GLB_PMU_CNT8_11_MODE_SEL);
}

static void pmu_slc_tcp8_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x 0x%x 0x%x\n",
			__func__, reg0, reg1, reg2, reg3);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP8A_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_TCP8A_CFG_1, i));
#ifdef CONFIG_MM_LB_GEMINI
		writel(WAY_MASK,
			man.lbd->base + SLC_ADDR400(SLC_TCP8A_CFG_2, i));
#endif
		writel(reg2, man.lbd->base + SLC_ADDR400(SLC_TCP8B_CFG_0, i));
		writel(reg3, man.lbd->base + SLC_ADDR400(SLC_TCP8B_CFG_1, i));
#ifdef CONFIG_MM_LB_GEMINI
		writel(WAY_MASK,
			man.lbd->base + SLC_ADDR400(SLC_TCP8B_CFG_2, i));
#endif
	}
}

static void pmu_slc_tcp9_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x 0x%x\n", __func__, reg0, reg1);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP9_CFG_0, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_TCP9_CFG_1, i));
	}
}

static void pmu_slc_tcp10_cfg(u32 reg0, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, reg0);

		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_TCP10_CFG_0, i));
	}
}

static void pmu_slc_dcp03_cfg_0(u32 reg, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(SLC_DCP03_CFG_0, i));
	}
}

static void pmu_slc_dcp6_cfg_0(u32 reg, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(SLC_DCP6_CFG_0, i));
	}
}

static void pmu_slc_dcp7_cfg_0(u32 reg, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(SLC_DCP7_CFG_0, i));
	}
}

static void pmu_slc_dcp8_cfg_0(u32 reg, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(SLC_DCP8_CFG_0, i));
	}
}

static void pmu_slc_dcp9_cfg_0(u32 reg, u32 reg1, u32 reg2, u32 reg3)
{
	int i;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		lb_print(LB_INFO, "%s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(SLC_DCP9_CFG_0, i));
	}
}

static void pmu_start_count(int mode_tp)
{
	int i;
	u32 reg;

	if (mode_tp >= LB_PMU_STATGO_MODE_MAX || mode_tp < 0) {
		lb_print(LB_ERROR, "invalid mode type\n");
		return;
	}

#ifdef CONFIG_MM_LB_GEMINI
	for (i = 0; i < SLC_IDX; i++) {
		reg = readl(man.lbd->base + SLC_ADDR2000(SLC_PMU_CTRL, i));
		writel((reg | lb_pmu_statgo_enable(mode_tp)),
				man.lbd->base + SLC_ADDR2000(SLC_PMU_CTRL, i));
	}

	reg = readl(man.lbd->base + GLB_PMU_CTRL);
	writel((reg | BIT(mode_tp)), man.lbd->base + GLB_PMU_CTRL);
#else
	reg = readl(man.lbd->base + GLB_PMU_CTRL);
	writel((reg | lb_pmu_statgo_enable(mode_tp)),
			man.lbd->base + GLB_PMU_CTRL);
#endif
}

static void pmu_end_count(int mode_tp)
{
	int i;
	u32 reg;

	if (mode_tp >= LB_PMU_STATGO_MODE_MAX || mode_tp < 0) {
		lb_print(LB_ERROR, "invalid mode type\n");
		return;
	}

#ifdef CONFIG_MM_LB_GEMINI
	for (i = 0; i < SLC_IDX; i++)
		writel(lb_pmu_statgo_disable(mode_tp),
				man.lbd->base + SLC_ADDR2000(SLC_PMU_CTRL, i));

	reg = readl(man.lbd->base + GLB_PMU_CTRL);
	writel((reg & ~(0x1 << mode_tp)), man.lbd->base + GLB_PMU_CTRL);
#else
	writel(lb_pmu_statgo_disable(mode_tp),
			man.lbd->base + GLB_PMU_CTRL);
#endif
}

static void pmu_map_event(int cnt, int event, int mode)
{
	int i;
	u32 reg;

	if (cnt < 0 || cnt >= LB_PMU_CNT_IDX) {
		lb_print(LB_ERROR, "%s, invalid param %d\n", __func__, cnt);
		return;
	}

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		reg = readl(man.lbd->base + SLC_ADDR400(SLC_CM_CNT_EN, i));
		reg |= (0x1 << cnt);

		lb_print(LB_INFO, "in1 %s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(SLC_CM_CNT_EN, i));
	}

	if (cnt >= LB_PMU_COMMON_COUNTER_MAX)
		return;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		reg = readl(man.lbd->base + SLC_ADDR400(get_cnt_mux(cnt), i));
		reg &= ~(PMU_CNT_MUX_MASK << pmu_cnt_mux_shift(cnt));
		reg |= event << pmu_cnt_mux_shift(cnt);

		lb_print(LB_INFO, "in2 %s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(get_cnt_mux(cnt), i));
	}

	if (cnt >= LB_PMU_CNT_8 && cnt <= LB_PMU_CNT_11) {
		reg = readl(man.lbd->base + GLB_PMU_CNT8_11_MODE_SEL);

		if (mode == STATGO_EVENT_1)
			reg |= CNT8_11_MODE_SEL << (cnt - LB_PMU_CNT_8);
		else
			reg &= ~(CNT8_11_MODE_SEL << (cnt - LB_PMU_CNT_8));

		writel(reg, man.lbd->base + GLB_PMU_CNT8_11_MODE_SEL);
	}
}

static void pmu_unmap_event(int cnt)
{
	int i;
	u32 reg;

	if (cnt < 0 || cnt >= LB_PMU_CNT_IDX) {
		lb_print(LB_ERROR, "%s, invalid param %d\n", __func__, cnt);
		return;
	}

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		reg = readl(man.lbd->base + SLC_ADDR400(SLC_CM_CNT_EN, i));
		reg &= ~BIT(cnt);

		lb_print(LB_INFO, "in1 %s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(SLC_CM_CNT_EN, i));
	}

	if (cnt >= LB_PMU_COMMON_COUNTER_MAX)
		return;

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		reg = readl(man.lbd->base + SLC_ADDR400(get_cnt_mux(cnt), i));
		reg &= ~(PMU_CNT_MUX_MASK << pmu_cnt_mux_shift(cnt));

		lb_print(LB_INFO, "in2 %s 0x%x\n", __func__, reg);

		writel(reg, man.lbd->base + SLC_ADDR400(get_cnt_mux(cnt), i));
	}
}

static u64 pmu_show(int cnt)
{
	u32 v1, v2;
	u64 v3;

	if (cnt < 0 || cnt >= LB_PMU_EVENT_CNT_IDX) {
		lb_print(LB_ERROR, "%s, invalid param %d\n", __func__, cnt);
		return 0;
	}

	v1 =  readl(man.lbd->base + SLC_PMU_CNT0_L + PMU_CNT_STEP * cnt);
	v2 =  readl(man.lbd->base + SLC_PMU_CNT0_H + PMU_CNT_STEP * cnt);
	/* v2 return as high 32 bit */
	v3 = (u64)v1 | ((u64)v2 << 32);

	lb_print(LB_INFO, "%s l:0x%x h:0x%x cnt:%lu\n", __func__, v1, v2, v3);

	return v3;
}

#ifdef CONFIG_MM_LB_TST
void pmu_set_wtmk(int cnt, u32 reg0, u32 reg1)
{
	int i;

	if (cnt < 0 || cnt >= LB_PMU_CNT_IDX) {
		lb_print(LB_ERROR, "%s, invalid param %d\n", __func__, cnt);
		return;
	}

	for (i = 0; i < SLC_IDX; i += PMU_STEP) {
		writel(reg0, man.lbd->base + SLC_ADDR400(SLC_PMU_CNT0_TH_L +
			PMU_CNT_STEP * cnt, i));
		writel(reg1, man.lbd->base + SLC_ADDR400(SLC_PMU_CNT0_TH_H +
			PMU_CNT_STEP * cnt, i));
	}
}
#endif
