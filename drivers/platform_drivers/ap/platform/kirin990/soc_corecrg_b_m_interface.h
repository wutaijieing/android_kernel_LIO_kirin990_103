#ifndef __SOC_CORECRG_B_M_INTERFACE_H__
#define __SOC_CORECRG_B_M_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#ifndef __SOC_H_FOR_ASM__
#define SOC_CORECRG_B_M_PEREN0_ADDR(base) ((base) + (0x000UL))
#define SOC_CORECRG_B_M_PERDIS0_ADDR(base) ((base) + (0x004UL))
#define SOC_CORECRG_B_M_PERCLKEN0_ADDR(base) ((base) + (0x008UL))
#define SOC_CORECRG_B_M_PERSTAT0_ADDR(base) ((base) + (0x00CUL))
#define SOC_CORECRG_B_M_CLKDIV0_ADDR(base) ((base) + (0x040UL))
#define SOC_CORECRG_B_M_PERI_STAT0_ADDR(base) ((base) + (0x060UL))
#define SOC_CORECRG_B_M_PMC_TO_NP0_ADDR(base) ((base) + (0x080UL))
#define SOC_CORECRG_B_M_PMC_TO_NP1_ADDR(base) ((base) + (0x084UL))
#define SOC_CORECRG_B_M_NP_TO_PMC_ADDR(base) ((base) + (0x088UL))
#define SOC_CORECRG_B_M_PIE_CTRL0_ADDR(base) ((base) + (0x100UL))
#define SOC_CORECRG_B_M_PIE_CTRL1_ADDR(base) ((base) + (0x104UL))
#define SOC_CORECRG_B_M_PIE_CTRL2_ADDR(base) ((base) + (0x108UL))
#define SOC_CORECRG_B_M_PIE_CTRL3_ADDR(base) ((base) + (0x10CUL))
#define SOC_CORECRG_B_M_PIE_CTRL4_ADDR(base) ((base) + (0x110UL))
#define SOC_CORECRG_B_M_PIE_CTRL5_ADDR(base) ((base) + (0x114UL))
#define SOC_CORECRG_B_M_PIE_CTRL6_ADDR(base) ((base) + (0x118UL))
#define SOC_CORECRG_B_M_PIE_STAT0_ADDR(base) ((base) + (0x130UL))
#define SOC_CORECRG_B_M_PIE_STAT1_ADDR(base) ((base) + (0x134UL))
#define SOC_CORECRG_B_M_PIE_STAT2_ADDR(base) ((base) + (0x138UL))
#define SOC_CORECRG_B_M_PIE_STAT3_ADDR(base) ((base) + (0x13CUL))
#define SOC_CORECRG_B_M_VS_CTRL_EN_BIG0_ADDR(base) ((base) + (0x150UL))
#define SOC_CORECRG_B_M_VS_CTRL_BYPASS_BIG0_ADDR(base) ((base) + (0x154UL))
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_ADDR(base) ((base) + (0x158UL))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_ADDR(base) ((base) + (0x15CUL))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_BIG0_ADDR(base) ((base) + (0x160UL))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_ADDR(base) ((base) + (0x164UL))
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_BIG0_ADDR(base) ((base) + (0x168UL))
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_ADDR(base) ((base) + (0x16CUL))
#define SOC_CORECRG_B_M_VS_CTRL_EN_MIDDLE0_ADDR(base) ((base) + (0x180UL))
#define SOC_CORECRG_B_M_VS_CTRL_BYPASS_MIDDLE0_ADDR(base) ((base) + (0x184UL))
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_ADDR(base) ((base) + (0x188UL))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_ADDR(base) ((base) + (0x18CUL))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_MIDDLE0_ADDR(base) ((base) + (0x190UL))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_ADDR(base) ((base) + (0x194UL))
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_MIDDLE0_ADDR(base) ((base) + (0x198UL))
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_ADDR(base) ((base) + (0x19CUL))
#define SOC_CORECRG_B_M_APLL3SSCCTRL_ADDR(base) ((base) + (0x1E0UL))
#define SOC_CORECRG_B_M_APLL5SSCCTRL_ADDR(base) ((base) + (0x1E4UL))
#define SOC_CORECRG_B_M_FCM_CLKEN_ADDR(base) ((base) + (0x200UL))
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_ADDR(base) ((base) + (0x210UL))
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_ADDR(base) ((base) + (0x214UL))
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_ADDR(base) ((base) + (0x218UL))
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_ADDR(base) ((base) + (0x220UL))
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_ADDR(base) ((base) + (0x22CUL))
#define SOC_CORECRG_B_M_FNPLL_CFG0_ADDR(base) ((base) + (0x240UL))
#define SOC_CORECRG_B_M_FNPLL_CFG1_ADDR(base) ((base) + (0x244UL))
#define SOC_CORECRG_B_M_FNPLL_CFG2_ADDR(base) ((base) + (0x248UL))
#define SOC_CORECRG_B_M_FNPLL_CFG3_ADDR(base) ((base) + (0x24CUL))
#define SOC_CORECRG_B_M_FNPLL_CFG4_ADDR(base) ((base) + (0x250UL))
#define SOC_CORECRG_B_M_FNPLL_CFG5_ADDR(base) ((base) + (0x254UL))
#define SOC_CORECRG_B_M_FNPLL_STAT_ADDR(base) ((base) + (0x270UL))
#else
#define SOC_CORECRG_B_M_PEREN0_ADDR(base) ((base) + (0x000))
#define SOC_CORECRG_B_M_PERDIS0_ADDR(base) ((base) + (0x004))
#define SOC_CORECRG_B_M_PERCLKEN0_ADDR(base) ((base) + (0x008))
#define SOC_CORECRG_B_M_PERSTAT0_ADDR(base) ((base) + (0x00C))
#define SOC_CORECRG_B_M_CLKDIV0_ADDR(base) ((base) + (0x040))
#define SOC_CORECRG_B_M_PERI_STAT0_ADDR(base) ((base) + (0x060))
#define SOC_CORECRG_B_M_PMC_TO_NP0_ADDR(base) ((base) + (0x080))
#define SOC_CORECRG_B_M_PMC_TO_NP1_ADDR(base) ((base) + (0x084))
#define SOC_CORECRG_B_M_NP_TO_PMC_ADDR(base) ((base) + (0x088))
#define SOC_CORECRG_B_M_PIE_CTRL0_ADDR(base) ((base) + (0x100))
#define SOC_CORECRG_B_M_PIE_CTRL1_ADDR(base) ((base) + (0x104))
#define SOC_CORECRG_B_M_PIE_CTRL2_ADDR(base) ((base) + (0x108))
#define SOC_CORECRG_B_M_PIE_CTRL3_ADDR(base) ((base) + (0x10C))
#define SOC_CORECRG_B_M_PIE_CTRL4_ADDR(base) ((base) + (0x110))
#define SOC_CORECRG_B_M_PIE_CTRL5_ADDR(base) ((base) + (0x114))
#define SOC_CORECRG_B_M_PIE_CTRL6_ADDR(base) ((base) + (0x118))
#define SOC_CORECRG_B_M_PIE_STAT0_ADDR(base) ((base) + (0x130))
#define SOC_CORECRG_B_M_PIE_STAT1_ADDR(base) ((base) + (0x134))
#define SOC_CORECRG_B_M_PIE_STAT2_ADDR(base) ((base) + (0x138))
#define SOC_CORECRG_B_M_PIE_STAT3_ADDR(base) ((base) + (0x13C))
#define SOC_CORECRG_B_M_VS_CTRL_EN_BIG0_ADDR(base) ((base) + (0x150))
#define SOC_CORECRG_B_M_VS_CTRL_BYPASS_BIG0_ADDR(base) ((base) + (0x154))
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_ADDR(base) ((base) + (0x158))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_ADDR(base) ((base) + (0x15C))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_BIG0_ADDR(base) ((base) + (0x160))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_ADDR(base) ((base) + (0x164))
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_BIG0_ADDR(base) ((base) + (0x168))
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_ADDR(base) ((base) + (0x16C))
#define SOC_CORECRG_B_M_VS_CTRL_EN_MIDDLE0_ADDR(base) ((base) + (0x180))
#define SOC_CORECRG_B_M_VS_CTRL_BYPASS_MIDDLE0_ADDR(base) ((base) + (0x184))
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_ADDR(base) ((base) + (0x188))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_ADDR(base) ((base) + (0x18C))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_MIDDLE0_ADDR(base) ((base) + (0x190))
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_ADDR(base) ((base) + (0x194))
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_MIDDLE0_ADDR(base) ((base) + (0x198))
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_ADDR(base) ((base) + (0x19C))
#define SOC_CORECRG_B_M_APLL3SSCCTRL_ADDR(base) ((base) + (0x1E0))
#define SOC_CORECRG_B_M_APLL5SSCCTRL_ADDR(base) ((base) + (0x1E4))
#define SOC_CORECRG_B_M_FCM_CLKEN_ADDR(base) ((base) + (0x200))
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_ADDR(base) ((base) + (0x210))
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_ADDR(base) ((base) + (0x214))
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_ADDR(base) ((base) + (0x218))
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_ADDR(base) ((base) + (0x220))
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_ADDR(base) ((base) + (0x22C))
#define SOC_CORECRG_B_M_FNPLL_CFG0_ADDR(base) ((base) + (0x240))
#define SOC_CORECRG_B_M_FNPLL_CFG1_ADDR(base) ((base) + (0x244))
#define SOC_CORECRG_B_M_FNPLL_CFG2_ADDR(base) ((base) + (0x248))
#define SOC_CORECRG_B_M_FNPLL_CFG3_ADDR(base) ((base) + (0x24C))
#define SOC_CORECRG_B_M_FNPLL_CFG4_ADDR(base) ((base) + (0x250))
#define SOC_CORECRG_B_M_FNPLL_CFG5_ADDR(base) ((base) + (0x254))
#define SOC_CORECRG_B_M_FNPLL_STAT_ADDR(base) ((base) + (0x270))
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0 : 1;
        unsigned int gt_clk_apll5_sscg : 1;
        unsigned int gt_clk_apll3_sscg : 1;
        unsigned int gt_clk_pie_trim : 1;
        unsigned int gt_clk_pll_noise_printer : 1;
        unsigned int reserved_1 : 1;
        unsigned int reserved_2 : 1;
        unsigned int reserved_3 : 1;
        unsigned int reserved_4 : 1;
        unsigned int reserved_5 : 1;
        unsigned int reserved_6 : 1;
        unsigned int reserved_7 : 1;
        unsigned int reserved_8 : 1;
        unsigned int reserved_9 : 1;
        unsigned int reserved_10 : 1;
        unsigned int reserved_11 : 1;
        unsigned int reserved_12 : 1;
        unsigned int reserved_13 : 1;
        unsigned int reserved_14 : 1;
        unsigned int reserved_15 : 1;
        unsigned int reserved_16 : 1;
        unsigned int reserved_17 : 1;
        unsigned int reserved_18 : 1;
        unsigned int reserved_19 : 1;
        unsigned int reserved_20 : 1;
        unsigned int reserved_21 : 7;
    } reg;
} SOC_CORECRG_B_M_PEREN0_UNION;
#endif
#define SOC_CORECRG_B_M_PEREN0_gt_clk_apll5_sscg_START (1)
#define SOC_CORECRG_B_M_PEREN0_gt_clk_apll5_sscg_END (1)
#define SOC_CORECRG_B_M_PEREN0_gt_clk_apll3_sscg_START (2)
#define SOC_CORECRG_B_M_PEREN0_gt_clk_apll3_sscg_END (2)
#define SOC_CORECRG_B_M_PEREN0_gt_clk_pie_trim_START (3)
#define SOC_CORECRG_B_M_PEREN0_gt_clk_pie_trim_END (3)
#define SOC_CORECRG_B_M_PEREN0_gt_clk_pll_noise_printer_START (4)
#define SOC_CORECRG_B_M_PEREN0_gt_clk_pll_noise_printer_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0 : 1;
        unsigned int gt_clk_apll5_sscg : 1;
        unsigned int gt_clk_apll3_sscg : 1;
        unsigned int gt_clk_pie_trim : 1;
        unsigned int gt_clk_pll_noise_printer : 1;
        unsigned int reserved_1 : 1;
        unsigned int reserved_2 : 1;
        unsigned int reserved_3 : 1;
        unsigned int reserved_4 : 1;
        unsigned int reserved_5 : 1;
        unsigned int reserved_6 : 1;
        unsigned int reserved_7 : 1;
        unsigned int reserved_8 : 1;
        unsigned int reserved_9 : 1;
        unsigned int reserved_10 : 1;
        unsigned int reserved_11 : 1;
        unsigned int reserved_12 : 1;
        unsigned int reserved_13 : 1;
        unsigned int reserved_14 : 1;
        unsigned int reserved_15 : 1;
        unsigned int reserved_16 : 1;
        unsigned int reserved_17 : 1;
        unsigned int reserved_18 : 1;
        unsigned int reserved_19 : 1;
        unsigned int reserved_20 : 1;
        unsigned int reserved_21 : 7;
    } reg;
} SOC_CORECRG_B_M_PERDIS0_UNION;
#endif
#define SOC_CORECRG_B_M_PERDIS0_gt_clk_apll5_sscg_START (1)
#define SOC_CORECRG_B_M_PERDIS0_gt_clk_apll5_sscg_END (1)
#define SOC_CORECRG_B_M_PERDIS0_gt_clk_apll3_sscg_START (2)
#define SOC_CORECRG_B_M_PERDIS0_gt_clk_apll3_sscg_END (2)
#define SOC_CORECRG_B_M_PERDIS0_gt_clk_pie_trim_START (3)
#define SOC_CORECRG_B_M_PERDIS0_gt_clk_pie_trim_END (3)
#define SOC_CORECRG_B_M_PERDIS0_gt_clk_pll_noise_printer_START (4)
#define SOC_CORECRG_B_M_PERDIS0_gt_clk_pll_noise_printer_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0 : 1;
        unsigned int gt_clk_apll5_sscg : 1;
        unsigned int gt_clk_apll3_sscg : 1;
        unsigned int gt_clk_pie_trim : 1;
        unsigned int gt_clk_pll_noise_printer : 1;
        unsigned int reserved_1 : 1;
        unsigned int reserved_2 : 1;
        unsigned int reserved_3 : 1;
        unsigned int reserved_4 : 1;
        unsigned int reserved_5 : 1;
        unsigned int reserved_6 : 1;
        unsigned int reserved_7 : 1;
        unsigned int reserved_8 : 1;
        unsigned int reserved_9 : 1;
        unsigned int reserved_10 : 1;
        unsigned int reserved_11 : 1;
        unsigned int reserved_12 : 1;
        unsigned int reserved_13 : 1;
        unsigned int reserved_14 : 1;
        unsigned int reserved_15 : 1;
        unsigned int reserved_16 : 1;
        unsigned int reserved_17 : 1;
        unsigned int reserved_18 : 1;
        unsigned int reserved_19 : 1;
        unsigned int reserved_20 : 1;
        unsigned int reserved_21 : 7;
    } reg;
} SOC_CORECRG_B_M_PERCLKEN0_UNION;
#endif
#define SOC_CORECRG_B_M_PERCLKEN0_gt_clk_apll5_sscg_START (1)
#define SOC_CORECRG_B_M_PERCLKEN0_gt_clk_apll5_sscg_END (1)
#define SOC_CORECRG_B_M_PERCLKEN0_gt_clk_apll3_sscg_START (2)
#define SOC_CORECRG_B_M_PERCLKEN0_gt_clk_apll3_sscg_END (2)
#define SOC_CORECRG_B_M_PERCLKEN0_gt_clk_pie_trim_START (3)
#define SOC_CORECRG_B_M_PERCLKEN0_gt_clk_pie_trim_END (3)
#define SOC_CORECRG_B_M_PERCLKEN0_gt_clk_pll_noise_printer_START (4)
#define SOC_CORECRG_B_M_PERCLKEN0_gt_clk_pll_noise_printer_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0 : 1;
        unsigned int st_clk_apll5_sscg : 1;
        unsigned int st_clk_apll3_sscg : 1;
        unsigned int st_clk_pie_trim : 1;
        unsigned int reserved_1 : 1;
        unsigned int reserved_2 : 1;
        unsigned int reserved_3 : 1;
        unsigned int reserved_4 : 1;
        unsigned int reserved_5 : 1;
        unsigned int reserved_6 : 1;
        unsigned int reserved_7 : 1;
        unsigned int reserved_8 : 1;
        unsigned int reserved_9 : 1;
        unsigned int reserved_10 : 1;
        unsigned int reserved_11 : 1;
        unsigned int reserved_12 : 1;
        unsigned int reserved_13 : 1;
        unsigned int reserved_14 : 1;
        unsigned int reserved_15 : 1;
        unsigned int reserved_16 : 1;
        unsigned int reserved_17 : 1;
        unsigned int reserved_18 : 1;
        unsigned int reserved_19 : 1;
        unsigned int reserved_20 : 1;
        unsigned int reserved_21 : 1;
        unsigned int reserved_22 : 7;
    } reg;
} SOC_CORECRG_B_M_PERSTAT0_UNION;
#endif
#define SOC_CORECRG_B_M_PERSTAT0_st_clk_apll5_sscg_START (1)
#define SOC_CORECRG_B_M_PERSTAT0_st_clk_apll5_sscg_END (1)
#define SOC_CORECRG_B_M_PERSTAT0_st_clk_apll3_sscg_START (2)
#define SOC_CORECRG_B_M_PERSTAT0_st_clk_apll3_sscg_END (2)
#define SOC_CORECRG_B_M_PERSTAT0_st_clk_pie_trim_START (3)
#define SOC_CORECRG_B_M_PERSTAT0_st_clk_pie_trim_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_enable_mode_mdm : 1;
        unsigned int cpub_pll_sel : 1;
        unsigned int reserved_0 : 5;
        unsigned int reserved_1 : 2;
        unsigned int reserved_2 : 2;
        unsigned int reserved_3 : 4;
        unsigned int reserved_4 : 1;
        unsigned int bitmasken : 16;
    } reg;
} SOC_CORECRG_B_M_CLKDIV0_UNION;
#endif
#define SOC_CORECRG_B_M_CLKDIV0_svfd_enable_mode_mdm_START (0)
#define SOC_CORECRG_B_M_CLKDIV0_svfd_enable_mode_mdm_END (0)
#define SOC_CORECRG_B_M_CLKDIV0_cpub_pll_sel_START (1)
#define SOC_CORECRG_B_M_CLKDIV0_cpub_pll_sel_END (1)
#define SOC_CORECRG_B_M_CLKDIV0_bitmasken_START (16)
#define SOC_CORECRG_B_M_CLKDIV0_bitmasken_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int st_clk_cpu_big0 : 1;
        unsigned int st_clk_cpu_middle0 : 1;
        unsigned int reserved_0 : 1;
        unsigned int reserved_1 : 2;
        unsigned int reserved_2 : 1;
        unsigned int reserved_3 : 26;
    } reg;
} SOC_CORECRG_B_M_PERI_STAT0_UNION;
#endif
#define SOC_CORECRG_B_M_PERI_STAT0_st_clk_cpu_big0_START (0)
#define SOC_CORECRG_B_M_PERI_STAT0_st_clk_cpu_big0_END (0)
#define SOC_CORECRG_B_M_PERI_STAT0_st_clk_cpu_middle0_START (1)
#define SOC_CORECRG_B_M_PERI_STAT0_st_clk_cpu_middle0_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int rd_wr_mode : 1;
        unsigned int wr_en : 1;
        unsigned int rd_en : 1;
        unsigned int gt_clk_np_sw : 1;
        unsigned int gt_clk_np_source : 1;
        unsigned int flag_mode : 1;
        unsigned int cpmdata_th : 6;
        unsigned int flag_wait_cfg_in : 3;
        unsigned int sel_np_cpu : 1;
        unsigned int reserved : 16;
    } reg;
} SOC_CORECRG_B_M_PMC_TO_NP0_UNION;
#endif
#define SOC_CORECRG_B_M_PMC_TO_NP0_rd_wr_mode_START (0)
#define SOC_CORECRG_B_M_PMC_TO_NP0_rd_wr_mode_END (0)
#define SOC_CORECRG_B_M_PMC_TO_NP0_wr_en_START (1)
#define SOC_CORECRG_B_M_PMC_TO_NP0_wr_en_END (1)
#define SOC_CORECRG_B_M_PMC_TO_NP0_rd_en_START (2)
#define SOC_CORECRG_B_M_PMC_TO_NP0_rd_en_END (2)
#define SOC_CORECRG_B_M_PMC_TO_NP0_gt_clk_np_sw_START (3)
#define SOC_CORECRG_B_M_PMC_TO_NP0_gt_clk_np_sw_END (3)
#define SOC_CORECRG_B_M_PMC_TO_NP0_gt_clk_np_source_START (4)
#define SOC_CORECRG_B_M_PMC_TO_NP0_gt_clk_np_source_END (4)
#define SOC_CORECRG_B_M_PMC_TO_NP0_flag_mode_START (5)
#define SOC_CORECRG_B_M_PMC_TO_NP0_flag_mode_END (5)
#define SOC_CORECRG_B_M_PMC_TO_NP0_cpmdata_th_START (6)
#define SOC_CORECRG_B_M_PMC_TO_NP0_cpmdata_th_END (11)
#define SOC_CORECRG_B_M_PMC_TO_NP0_flag_wait_cfg_in_START (12)
#define SOC_CORECRG_B_M_PMC_TO_NP0_flag_wait_cfg_in_END (14)
#define SOC_CORECRG_B_M_PMC_TO_NP0_sel_np_cpu_START (15)
#define SOC_CORECRG_B_M_PMC_TO_NP0_sel_np_cpu_END (15)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int mem_ctrl_s : 26;
        unsigned int reserved : 6;
    } reg;
} SOC_CORECRG_B_M_PMC_TO_NP1_UNION;
#endif
#define SOC_CORECRG_B_M_PMC_TO_NP1_mem_ctrl_s_START (0)
#define SOC_CORECRG_B_M_PMC_TO_NP1_mem_ctrl_s_END (25)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int mem_rdata : 8;
        unsigned int w_done : 1;
        unsigned int st_clk_cpu_np_mem : 1;
        unsigned int ona_clk_rd : 1;
        unsigned int onb_clk_cpu_np : 1;
        unsigned int reserved : 20;
    } reg;
} SOC_CORECRG_B_M_NP_TO_PMC_UNION;
#endif
#define SOC_CORECRG_B_M_NP_TO_PMC_mem_rdata_START (0)
#define SOC_CORECRG_B_M_NP_TO_PMC_mem_rdata_END (7)
#define SOC_CORECRG_B_M_NP_TO_PMC_w_done_START (8)
#define SOC_CORECRG_B_M_NP_TO_PMC_w_done_END (8)
#define SOC_CORECRG_B_M_NP_TO_PMC_st_clk_cpu_np_mem_START (9)
#define SOC_CORECRG_B_M_NP_TO_PMC_st_clk_cpu_np_mem_END (9)
#define SOC_CORECRG_B_M_NP_TO_PMC_ona_clk_rd_START (10)
#define SOC_CORECRG_B_M_NP_TO_PMC_ona_clk_rd_END (10)
#define SOC_CORECRG_B_M_NP_TO_PMC_onb_clk_cpu_np_START (11)
#define SOC_CORECRG_B_M_NP_TO_PMC_onb_clk_cpu_np_END (11)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ripple_trim_type : 1;
        unsigned int ripple_trim_en : 1;
        unsigned int ripple_trim_time : 8;
        unsigned int ripple_trim_th : 8;
        unsigned int reserved : 14;
    } reg;
} SOC_CORECRG_B_M_PIE_CTRL0_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_CTRL0_ripple_trim_type_START (0)
#define SOC_CORECRG_B_M_PIE_CTRL0_ripple_trim_type_END (0)
#define SOC_CORECRG_B_M_PIE_CTRL0_ripple_trim_en_START (1)
#define SOC_CORECRG_B_M_PIE_CTRL0_ripple_trim_en_END (1)
#define SOC_CORECRG_B_M_PIE_CTRL0_ripple_trim_time_START (2)
#define SOC_CORECRG_B_M_PIE_CTRL0_ripple_trim_time_END (9)
#define SOC_CORECRG_B_M_PIE_CTRL0_ripple_trim_th_START (10)
#define SOC_CORECRG_B_M_PIE_CTRL0_ripple_trim_th_END (17)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0 : 1;
        unsigned int ripple_trim_curs_set : 1;
        unsigned int ripple_trim_fine_in : 12;
        unsigned int ripple_trim_curs_in : 6;
        unsigned int ripple_trim_fine_set : 4;
        unsigned int reserved_1 : 8;
    } reg;
} SOC_CORECRG_B_M_PIE_CTRL1_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_CTRL1_ripple_trim_curs_set_START (1)
#define SOC_CORECRG_B_M_PIE_CTRL1_ripple_trim_curs_set_END (1)
#define SOC_CORECRG_B_M_PIE_CTRL1_ripple_trim_fine_in_START (2)
#define SOC_CORECRG_B_M_PIE_CTRL1_ripple_trim_fine_in_END (13)
#define SOC_CORECRG_B_M_PIE_CTRL1_ripple_trim_curs_in_START (14)
#define SOC_CORECRG_B_M_PIE_CTRL1_ripple_trim_curs_in_END (19)
#define SOC_CORECRG_B_M_PIE_CTRL1_ripple_trim_fine_set_START (20)
#define SOC_CORECRG_B_M_PIE_CTRL1_ripple_trim_fine_set_END (23)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pie_en : 1;
        unsigned int pie_reserved_in1 : 8;
        unsigned int reserved : 23;
    } reg;
} SOC_CORECRG_B_M_PIE_CTRL2_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_CTRL2_pie_en_START (0)
#define SOC_CORECRG_B_M_PIE_CTRL2_pie_en_END (0)
#define SOC_CORECRG_B_M_PIE_CTRL2_pie_reserved_in1_START (1)
#define SOC_CORECRG_B_M_PIE_CTRL2_pie_reserved_in1_END (8)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pie_ibias_trim : 4;
        unsigned int reserved_0 : 1;
        unsigned int pie_en_cmp_trim : 1;
        unsigned int pie_cmp3_trim : 4;
        unsigned int pie_cmp2_trim : 4;
        unsigned int pie_cmp1_trim : 4;
        unsigned int pie_cmp0_trim : 4;
        unsigned int pie_vtrig_trim : 4;
        unsigned int pie_cmp4_trim : 4;
        unsigned int reserved_1 : 2;
    } reg;
} SOC_CORECRG_B_M_PIE_CTRL3_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_ibias_trim_START (0)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_ibias_trim_END (3)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_en_cmp_trim_START (5)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_en_cmp_trim_END (5)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_cmp3_trim_START (6)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_cmp3_trim_END (9)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_cmp2_trim_START (10)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_cmp2_trim_END (13)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_cmp1_trim_START (14)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_cmp1_trim_END (17)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_cmp0_trim_START (18)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_cmp0_trim_END (21)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_vtrig_trim_START (22)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_vtrig_trim_END (25)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_cmp4_trim_START (26)
#define SOC_CORECRG_B_M_PIE_CTRL3_pie_cmp4_trim_END (29)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pie_ps_en : 8;
        unsigned int pie_vset_filter : 3;
        unsigned int pie_mon_en : 1;
        unsigned int pie_ib_cmp_set : 2;
        unsigned int pie_ripple_set : 3;
        unsigned int pie_mon_h_sel : 6;
        unsigned int pie_mon_l_sel : 6;
        unsigned int pie_vbos_set : 3;
    } reg;
} SOC_CORECRG_B_M_PIE_CTRL4_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_ps_en_START (0)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_ps_en_END (7)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_vset_filter_START (8)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_vset_filter_END (10)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_mon_en_START (11)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_mon_en_END (11)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_ib_cmp_set_START (12)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_ib_cmp_set_END (13)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_ripple_set_START (14)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_ripple_set_END (16)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_mon_h_sel_START (17)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_mon_h_sel_END (22)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_mon_l_sel_START (23)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_mon_l_sel_END (28)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_vbos_set_START (29)
#define SOC_CORECRG_B_M_PIE_CTRL4_pie_vbos_set_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pie_test_ctrl : 3;
        unsigned int pie_vset : 4;
        unsigned int pie_ocp_set : 8;
        unsigned int pie_pwr_main_set : 4;
        unsigned int reserved_0 : 3;
        unsigned int pie_pwr_ax_set : 3;
        unsigned int pie_pwr_ar_set : 3;
        unsigned int pie_ocp_enn : 1;
        unsigned int reserved_1 : 3;
    } reg;
} SOC_CORECRG_B_M_PIE_CTRL5_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_test_ctrl_START (0)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_test_ctrl_END (2)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_vset_START (3)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_vset_END (6)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_ocp_set_START (7)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_ocp_set_END (14)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_pwr_main_set_START (15)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_pwr_main_set_END (18)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_pwr_ax_set_START (22)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_pwr_ax_set_END (24)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_pwr_ar_set_START (25)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_pwr_ar_set_END (27)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_ocp_enn_START (28)
#define SOC_CORECRG_B_M_PIE_CTRL5_pie_ocp_enn_END (28)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pie_mon_cfg : 16;
        unsigned int pie_ps_cfg : 12;
        unsigned int reserved : 4;
    } reg;
} SOC_CORECRG_B_M_PIE_CTRL6_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_CTRL6_pie_mon_cfg_START (0)
#define SOC_CORECRG_B_M_PIE_CTRL6_pie_mon_cfg_END (15)
#define SOC_CORECRG_B_M_PIE_CTRL6_pie_ps_cfg_START (16)
#define SOC_CORECRG_B_M_PIE_CTRL6_pie_ps_cfg_END (27)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pie_cmp0_out_test : 1;
        unsigned int pie_cmp1_out_test : 1;
        unsigned int pie_cmp2_out_test : 1;
        unsigned int pie_cmp3_out_test : 1;
        unsigned int pie_cmp4_out_test : 1;
        unsigned int reserved_0 : 1;
        unsigned int pie_flag : 1;
        unsigned int pie_mon_l_flag : 1;
        unsigned int pie_mon_h_flag : 1;
        unsigned int pie_mon_l_data : 6;
        unsigned int pie_mon_h_data : 6;
        unsigned int pie_mon_l_hold_clk : 1;
        unsigned int pie_mon_h_hold_clk : 1;
        unsigned int pie_ps_pvalid : 1;
        unsigned int pie_ps_nvalid : 1;
        unsigned int pie_ps_clk_count : 1;
        unsigned int reserved_1 : 6;
    } reg;
} SOC_CORECRG_B_M_PIE_STAT0_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_STAT0_pie_cmp0_out_test_START (0)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_cmp0_out_test_END (0)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_cmp1_out_test_START (1)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_cmp1_out_test_END (1)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_cmp2_out_test_START (2)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_cmp2_out_test_END (2)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_cmp3_out_test_START (3)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_cmp3_out_test_END (3)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_cmp4_out_test_START (4)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_cmp4_out_test_END (4)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_flag_START (6)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_flag_END (6)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_l_flag_START (7)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_l_flag_END (7)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_h_flag_START (8)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_h_flag_END (8)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_l_data_START (9)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_l_data_END (14)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_h_data_START (15)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_h_data_END (20)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_l_hold_clk_START (21)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_l_hold_clk_END (21)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_h_hold_clk_START (22)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_mon_h_hold_clk_END (22)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_ps_pvalid_START (23)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_ps_pvalid_END (23)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_ps_nvalid_START (24)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_ps_nvalid_END (24)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_ps_clk_count_START (25)
#define SOC_CORECRG_B_M_PIE_STAT0_pie_ps_clk_count_END (25)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ripple_trim_fine_stat : 12;
        unsigned int ripple_trim_ok : 4;
        unsigned int ripple_trim_curs_stat : 6;
        unsigned int ripple_trim_curs_ok : 1;
        unsigned int reserved : 1;
        unsigned int pie_reserved_out1 : 8;
    } reg;
} SOC_CORECRG_B_M_PIE_STAT1_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_STAT1_ripple_trim_fine_stat_START (0)
#define SOC_CORECRG_B_M_PIE_STAT1_ripple_trim_fine_stat_END (11)
#define SOC_CORECRG_B_M_PIE_STAT1_ripple_trim_ok_START (12)
#define SOC_CORECRG_B_M_PIE_STAT1_ripple_trim_ok_END (15)
#define SOC_CORECRG_B_M_PIE_STAT1_ripple_trim_curs_stat_START (16)
#define SOC_CORECRG_B_M_PIE_STAT1_ripple_trim_curs_stat_END (21)
#define SOC_CORECRG_B_M_PIE_STAT1_ripple_trim_curs_ok_START (22)
#define SOC_CORECRG_B_M_PIE_STAT1_ripple_trim_curs_ok_END (22)
#define SOC_CORECRG_B_M_PIE_STAT1_pie_reserved_out1_START (24)
#define SOC_CORECRG_B_M_PIE_STAT1_pie_reserved_out1_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pie_ps_pdata : 16;
        unsigned int pie_ps_ndata : 16;
    } reg;
} SOC_CORECRG_B_M_PIE_STAT2_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_STAT2_pie_ps_pdata_START (0)
#define SOC_CORECRG_B_M_PIE_STAT2_pie_ps_pdata_END (15)
#define SOC_CORECRG_B_M_PIE_STAT2_pie_ps_ndata_START (16)
#define SOC_CORECRG_B_M_PIE_STAT2_pie_ps_ndata_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pie_mon_l_hold_data : 16;
        unsigned int pie_mon_h_hold_data : 16;
    } reg;
} SOC_CORECRG_B_M_PIE_STAT3_UNION;
#endif
#define SOC_CORECRG_B_M_PIE_STAT3_pie_mon_l_hold_data_START (0)
#define SOC_CORECRG_B_M_PIE_STAT3_pie_mon_l_hold_data_END (15)
#define SOC_CORECRG_B_M_PIE_STAT3_pie_mon_h_hold_data_START (16)
#define SOC_CORECRG_B_M_PIE_STAT3_pie_mon_h_hold_data_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int vs_ctrl_en_big0 : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_CORECRG_B_M_VS_CTRL_EN_BIG0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_CTRL_EN_BIG0_vs_ctrl_en_big0_START (0)
#define SOC_CORECRG_B_M_VS_CTRL_EN_BIG0_vs_ctrl_en_big0_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int vs_ctrl_bypass_big0 : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_CORECRG_B_M_VS_CTRL_BYPASS_BIG0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_CTRL_BYPASS_BIG0_vs_ctrl_bypass_big0_START (0)
#define SOC_CORECRG_B_M_VS_CTRL_BYPASS_BIG0_vs_ctrl_bypass_big0_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int vol_drop_en_big0 : 1;
        unsigned int big0_cpu0_wfi_wfe_bypass : 1;
        unsigned int big0_cpu1_wfi_wfe_bypass : 1;
        unsigned int big0_cpu2_wfi_wfe_bypass : 1;
        unsigned int big0_cpu3_wfi_wfe_bypass : 1;
        unsigned int big0_l2_idle_div_mod : 2;
        unsigned int big0_cfg_cnt_cpu_wake_quit : 16;
        unsigned int big0_cpu_wake_up_mode : 2;
        unsigned int big0_cpu_l2_idle_switch_bypass : 1;
        unsigned int big0_cpu_l2_idle_gt_en : 1;
        unsigned int big0_dvfs_div_en : 1;
        unsigned int reserved : 1;
        unsigned int sel_ckmux_big0_icg : 1;
        unsigned int cpu_clk_div_cfg_big0 : 2;
    } reg;
} SOC_CORECRG_B_M_VS_CTRL_BIG0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_vol_drop_en_big0_START (0)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_vol_drop_en_big0_END (0)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu0_wfi_wfe_bypass_START (1)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu0_wfi_wfe_bypass_END (1)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu1_wfi_wfe_bypass_START (2)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu1_wfi_wfe_bypass_END (2)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu2_wfi_wfe_bypass_START (3)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu2_wfi_wfe_bypass_END (3)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu3_wfi_wfe_bypass_START (4)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu3_wfi_wfe_bypass_END (4)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_l2_idle_div_mod_START (5)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_l2_idle_div_mod_END (6)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cfg_cnt_cpu_wake_quit_START (7)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cfg_cnt_cpu_wake_quit_END (22)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu_wake_up_mode_START (23)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu_wake_up_mode_END (24)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu_l2_idle_switch_bypass_START (25)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu_l2_idle_switch_bypass_END (25)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu_l2_idle_gt_en_START (26)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_cpu_l2_idle_gt_en_END (26)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_dvfs_div_en_START (27)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_big0_dvfs_div_en_END (27)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_sel_ckmux_big0_icg_START (29)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_sel_ckmux_big0_icg_END (29)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_cpu_clk_div_cfg_big0_START (30)
#define SOC_CORECRG_B_M_VS_CTRL_BIG0_cpu_clk_div_cfg_big0_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_data_limit_e_big0 : 1;
        unsigned int svfd_off_time_step_big0 : 1;
        unsigned int svfd_pulse_width_sel_big0 : 1;
        unsigned int reserved_0 : 1;
        unsigned int svfd_ulvt_ll_big0 : 4;
        unsigned int svfd_ulvt_sl_big0 : 4;
        unsigned int svfd_lvt_ll_big0 : 4;
        unsigned int svfd_lvt_sl_big0 : 4;
        unsigned int svfd_trim_big0 : 2;
        unsigned int reserved_1 : 2;
        unsigned int svfd_d_rate_big0 : 2;
        unsigned int svfd_off_mode_big0 : 1;
        unsigned int svfd_div64_en_big0 : 1;
        unsigned int svfd_cpm_period_big0 : 1;
        unsigned int svfd_edge_sel_big0 : 1;
        unsigned int svfd_cmp_data_mode_big0 : 2;
    } reg;
} SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_data_limit_e_big0_START (0)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_data_limit_e_big0_END (0)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_off_time_step_big0_START (1)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_off_time_step_big0_END (1)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_pulse_width_sel_big0_START (2)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_pulse_width_sel_big0_END (2)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_ulvt_ll_big0_START (4)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_ulvt_ll_big0_END (7)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_ulvt_sl_big0_START (8)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_ulvt_sl_big0_END (11)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_lvt_ll_big0_START (12)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_lvt_ll_big0_END (15)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_lvt_sl_big0_START (16)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_lvt_sl_big0_END (19)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_trim_big0_START (20)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_trim_big0_END (21)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_d_rate_big0_START (24)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_d_rate_big0_END (25)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_off_mode_big0_START (26)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_off_mode_big0_END (26)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_div64_en_big0_START (27)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_div64_en_big0_END (27)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_cpm_period_big0_START (28)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_cpm_period_big0_END (28)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_edge_sel_big0_START (29)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_edge_sel_big0_END (29)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_cmp_data_mode_big0_START (30)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_BIG0_svfd_cmp_data_mode_big0_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_glitch_test_1 : 1;
        unsigned int svfd_test_ffs_1 : 2;
        unsigned int svfd_test_cmp_1 : 7;
        unsigned int reserved_0 : 3;
        unsigned int reserved_1 : 19;
    } reg;
} SOC_CORECRG_B_M_VS_SVFD_CTRL1_BIG0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_BIG0_svfd_glitch_test_1_START (0)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_BIG0_svfd_glitch_test_1_END (0)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_BIG0_svfd_test_ffs_1_START (1)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_BIG0_svfd_test_ffs_1_END (2)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_BIG0_svfd_test_cmp_1_START (3)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_BIG0_svfd_test_cmp_1_END (9)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_enalbe_mode_1 : 2;
        unsigned int svfs_cpm_threshold_r_1 : 6;
        unsigned int svfs_cpm_threshold_f_1 : 6;
        unsigned int reserved : 2;
        unsigned int bitmasken : 16;
    } reg;
} SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_svfd_enalbe_mode_1_START (0)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_svfd_enalbe_mode_1_END (1)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_svfs_cpm_threshold_r_1_START (2)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_svfs_cpm_threshold_r_1_END (7)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_svfs_cpm_threshold_f_1_START (8)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_svfs_cpm_threshold_f_1_END (13)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_bitmasken_START (16)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_BIG0_bitmasken_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_cpm_data_r_big0 : 8;
        unsigned int svfd_cpm_data_f_big0 : 8;
        unsigned int svfd_dll_lock_big0 : 1;
        unsigned int reserved : 15;
    } reg;
} SOC_CORECRG_B_M_VS_CPM_DATA_STAT_BIG0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_BIG0_svfd_cpm_data_r_big0_START (0)
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_BIG0_svfd_cpm_data_r_big0_END (7)
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_BIG0_svfd_cpm_data_f_big0_START (8)
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_BIG0_svfd_cpm_data_f_big0_END (15)
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_BIG0_svfd_dll_lock_big0_START (16)
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_BIG0_svfd_dll_lock_big0_END (16)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_test_out_ffs_big0 : 4;
        unsigned int svfd_test_out_cpm_big0 : 4;
        unsigned int svfd_glitch_result_big0 : 1;
        unsigned int idle_low_freq_en_big0 : 1;
        unsigned int vbat_drop_protect_ind_big0 : 1;
        unsigned int reserved : 21;
    } reg;
} SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_svfd_test_out_ffs_big0_START (0)
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_svfd_test_out_ffs_big0_END (3)
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_svfd_test_out_cpm_big0_START (4)
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_svfd_test_out_cpm_big0_END (7)
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_svfd_glitch_result_big0_START (8)
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_svfd_glitch_result_big0_END (8)
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_idle_low_freq_en_big0_START (9)
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_idle_low_freq_en_big0_END (9)
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_vbat_drop_protect_ind_big0_START (10)
#define SOC_CORECRG_B_M_VS_TEST_STAT_BIG0_vbat_drop_protect_ind_big0_END (10)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int vs_ctrl_en_middle0 : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_CORECRG_B_M_VS_CTRL_EN_MIDDLE0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_CTRL_EN_MIDDLE0_vs_ctrl_en_middle0_START (0)
#define SOC_CORECRG_B_M_VS_CTRL_EN_MIDDLE0_vs_ctrl_en_middle0_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int vs_ctrl_bypass_middle0 : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_CORECRG_B_M_VS_CTRL_BYPASS_MIDDLE0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_CTRL_BYPASS_MIDDLE0_vs_ctrl_bypass_middle0_START (0)
#define SOC_CORECRG_B_M_VS_CTRL_BYPASS_MIDDLE0_vs_ctrl_bypass_middle0_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int vol_drop_en_middle0 : 1;
        unsigned int middle0_cpu0_wfi_wfe_bypass : 1;
        unsigned int middle0_cpu1_wfi_wfe_bypass : 1;
        unsigned int middle0_cpu2_wfi_wfe_bypass : 1;
        unsigned int middle0_cpu3_wfi_wfe_bypass : 1;
        unsigned int middle0_l2_idle_div_mod : 2;
        unsigned int middle0_cfg_cnt_cpu_wake_quit : 16;
        unsigned int middle0_cpu_wake_up_mode : 2;
        unsigned int middle0_cpu_l2_idle_switch_bypass : 1;
        unsigned int middle0_cpu_l2_idle_gt_en : 1;
        unsigned int middle0_dvfs_div_en : 1;
        unsigned int reserved : 1;
        unsigned int sel_ckmux_middle0_icg : 1;
        unsigned int cpu_clk_div_cfg_middle0 : 2;
    } reg;
} SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_vol_drop_en_middle0_START (0)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_vol_drop_en_middle0_END (0)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu0_wfi_wfe_bypass_START (1)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu0_wfi_wfe_bypass_END (1)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu1_wfi_wfe_bypass_START (2)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu1_wfi_wfe_bypass_END (2)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu2_wfi_wfe_bypass_START (3)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu2_wfi_wfe_bypass_END (3)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu3_wfi_wfe_bypass_START (4)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu3_wfi_wfe_bypass_END (4)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_l2_idle_div_mod_START (5)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_l2_idle_div_mod_END (6)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cfg_cnt_cpu_wake_quit_START (7)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cfg_cnt_cpu_wake_quit_END (22)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu_wake_up_mode_START (23)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu_wake_up_mode_END (24)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu_l2_idle_switch_bypass_START (25)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu_l2_idle_switch_bypass_END (25)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu_l2_idle_gt_en_START (26)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_cpu_l2_idle_gt_en_END (26)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_dvfs_div_en_START (27)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_middle0_dvfs_div_en_END (27)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_sel_ckmux_middle0_icg_START (29)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_sel_ckmux_middle0_icg_END (29)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_cpu_clk_div_cfg_middle0_START (30)
#define SOC_CORECRG_B_M_VS_CTRL_MIDDLE0_cpu_clk_div_cfg_middle0_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_data_limit_e_middle0 : 1;
        unsigned int svfd_off_time_step_middle0 : 1;
        unsigned int svfd_pulse_width_sel_middle0 : 1;
        unsigned int reserved_0 : 1;
        unsigned int svfd_ulvt_ll_middle0 : 4;
        unsigned int svfd_ulvt_sl_middle0 : 4;
        unsigned int svfd_lvt_ll_middle0 : 4;
        unsigned int svfd_lvt_sl_middle0 : 4;
        unsigned int svfd_trim_middle0 : 2;
        unsigned int reserved_1 : 2;
        unsigned int svfd_d_rate_middle0 : 2;
        unsigned int svfd_off_mode_middle0 : 1;
        unsigned int svfd_div64_en_middle0 : 1;
        unsigned int svfd_cpm_period_middle0 : 1;
        unsigned int svfd_edge_sel_middle0 : 1;
        unsigned int svfd_cmp_data_mode_middle0 : 2;
    } reg;
} SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_data_limit_e_middle0_START (0)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_data_limit_e_middle0_END (0)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_off_time_step_middle0_START (1)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_off_time_step_middle0_END (1)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_pulse_width_sel_middle0_START (2)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_pulse_width_sel_middle0_END (2)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_ulvt_ll_middle0_START (4)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_ulvt_ll_middle0_END (7)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_ulvt_sl_middle0_START (8)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_ulvt_sl_middle0_END (11)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_lvt_ll_middle0_START (12)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_lvt_ll_middle0_END (15)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_lvt_sl_middle0_START (16)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_lvt_sl_middle0_END (19)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_trim_middle0_START (20)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_trim_middle0_END (21)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_d_rate_middle0_START (24)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_d_rate_middle0_END (25)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_off_mode_middle0_START (26)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_off_mode_middle0_END (26)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_div64_en_middle0_START (27)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_div64_en_middle0_END (27)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_cpm_period_middle0_START (28)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_cpm_period_middle0_END (28)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_edge_sel_middle0_START (29)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_edge_sel_middle0_END (29)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_cmp_data_mode_middle0_START (30)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL0_MIDDLE0_svfd_cmp_data_mode_middle0_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_glitch_test_middle0 : 1;
        unsigned int svfd_test_ffs_middle0 : 2;
        unsigned int svfd_test_cmp_middle0 : 7;
        unsigned int reserved_0 : 3;
        unsigned int reserved_1 : 19;
    } reg;
} SOC_CORECRG_B_M_VS_SVFD_CTRL1_MIDDLE0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_MIDDLE0_svfd_glitch_test_middle0_START (0)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_MIDDLE0_svfd_glitch_test_middle0_END (0)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_MIDDLE0_svfd_test_ffs_middle0_START (1)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_MIDDLE0_svfd_test_ffs_middle0_END (2)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_MIDDLE0_svfd_test_cmp_middle0_START (3)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL1_MIDDLE0_svfd_test_cmp_middle0_END (9)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_enalbe_mode_middle0 : 2;
        unsigned int svfs_cpm_threshold_r_middle0 : 6;
        unsigned int svfs_cpm_threshold_f_middle0 : 6;
        unsigned int reserved : 2;
        unsigned int bitmasken : 16;
    } reg;
} SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_svfd_enalbe_mode_middle0_START (0)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_svfd_enalbe_mode_middle0_END (1)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_svfs_cpm_threshold_r_middle0_START (2)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_svfs_cpm_threshold_r_middle0_END (7)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_svfs_cpm_threshold_f_middle0_START (8)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_svfs_cpm_threshold_f_middle0_END (13)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_bitmasken_START (16)
#define SOC_CORECRG_B_M_VS_SVFD_CTRL2_MIDDLE0_bitmasken_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_cpm_data_r_middle0 : 8;
        unsigned int svfd_cpm_data_f_middle0 : 8;
        unsigned int svfd_dll_lock_middle0 : 1;
        unsigned int reserved : 15;
    } reg;
} SOC_CORECRG_B_M_VS_CPM_DATA_STAT_MIDDLE0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_MIDDLE0_svfd_cpm_data_r_middle0_START (0)
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_MIDDLE0_svfd_cpm_data_r_middle0_END (7)
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_MIDDLE0_svfd_cpm_data_f_middle0_START (8)
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_MIDDLE0_svfd_cpm_data_f_middle0_END (15)
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_MIDDLE0_svfd_dll_lock_middle0_START (16)
#define SOC_CORECRG_B_M_VS_CPM_DATA_STAT_MIDDLE0_svfd_dll_lock_middle0_END (16)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int svfd_test_out_ffs_middle0 : 4;
        unsigned int svfd_test_out_cpm_middle0 : 4;
        unsigned int svfd_glitch_result_middle0 : 1;
        unsigned int idle_low_freq_en_middle0 : 1;
        unsigned int vbat_drop_protect_ind_middle0 : 1;
        unsigned int reserved : 21;
    } reg;
} SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_UNION;
#endif
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_svfd_test_out_ffs_middle0_START (0)
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_svfd_test_out_ffs_middle0_END (3)
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_svfd_test_out_cpm_middle0_START (4)
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_svfd_test_out_cpm_middle0_END (7)
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_svfd_glitch_result_middle0_START (8)
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_svfd_glitch_result_middle0_END (8)
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_idle_low_freq_en_middle0_START (9)
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_idle_low_freq_en_middle0_END (9)
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_vbat_drop_protect_ind_middle0_START (10)
#define SOC_CORECRG_B_M_VS_TEST_STAT_MIDDLE0_vbat_drop_protect_ind_middle0_END (10)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int apll3_ssc_reset : 1;
        unsigned int apll3_ssc_disable : 1;
        unsigned int apll3_ssc_downspread : 1;
        unsigned int apll3_ssc_spread : 3;
        unsigned int apll3_ssc_divval : 4;
        unsigned int reserved : 22;
    } reg;
} SOC_CORECRG_B_M_APLL3SSCCTRL_UNION;
#endif
#define SOC_CORECRG_B_M_APLL3SSCCTRL_apll3_ssc_reset_START (0)
#define SOC_CORECRG_B_M_APLL3SSCCTRL_apll3_ssc_reset_END (0)
#define SOC_CORECRG_B_M_APLL3SSCCTRL_apll3_ssc_disable_START (1)
#define SOC_CORECRG_B_M_APLL3SSCCTRL_apll3_ssc_disable_END (1)
#define SOC_CORECRG_B_M_APLL3SSCCTRL_apll3_ssc_downspread_START (2)
#define SOC_CORECRG_B_M_APLL3SSCCTRL_apll3_ssc_downspread_END (2)
#define SOC_CORECRG_B_M_APLL3SSCCTRL_apll3_ssc_spread_START (3)
#define SOC_CORECRG_B_M_APLL3SSCCTRL_apll3_ssc_spread_END (5)
#define SOC_CORECRG_B_M_APLL3SSCCTRL_apll3_ssc_divval_START (6)
#define SOC_CORECRG_B_M_APLL3SSCCTRL_apll3_ssc_divval_END (9)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int apll5_ssc_reset : 1;
        unsigned int apll5_ssc_disable : 1;
        unsigned int apll5_ssc_downspread : 1;
        unsigned int apll5_ssc_spread : 3;
        unsigned int apll5_ssc_divval : 4;
        unsigned int reserved : 22;
    } reg;
} SOC_CORECRG_B_M_APLL5SSCCTRL_UNION;
#endif
#define SOC_CORECRG_B_M_APLL5SSCCTRL_apll5_ssc_reset_START (0)
#define SOC_CORECRG_B_M_APLL5SSCCTRL_apll5_ssc_reset_END (0)
#define SOC_CORECRG_B_M_APLL5SSCCTRL_apll5_ssc_disable_START (1)
#define SOC_CORECRG_B_M_APLL5SSCCTRL_apll5_ssc_disable_END (1)
#define SOC_CORECRG_B_M_APLL5SSCCTRL_apll5_ssc_downspread_START (2)
#define SOC_CORECRG_B_M_APLL5SSCCTRL_apll5_ssc_downspread_END (2)
#define SOC_CORECRG_B_M_APLL5SSCCTRL_apll5_ssc_spread_START (3)
#define SOC_CORECRG_B_M_APLL5SSCCTRL_apll5_ssc_spread_END (5)
#define SOC_CORECRG_B_M_APLL5SSCCTRL_apll5_ssc_divval_START (6)
#define SOC_CORECRG_B_M_APLL5SSCCTRL_apll5_ssc_divval_END (9)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int gt_clk_cpu_big0 : 1;
        unsigned int gt_clk_cpu_middle0 : 1;
        unsigned int reserved_0 : 1;
        unsigned int reserved_1 : 13;
        unsigned int bitmasken : 16;
    } reg;
} SOC_CORECRG_B_M_FCM_CLKEN_UNION;
#endif
#define SOC_CORECRG_B_M_FCM_CLKEN_gt_clk_cpu_big0_START (0)
#define SOC_CORECRG_B_M_FCM_CLKEN_gt_clk_cpu_big0_END (0)
#define SOC_CORECRG_B_M_FCM_CLKEN_gt_clk_cpu_middle0_START (1)
#define SOC_CORECRG_B_M_FCM_CLKEN_gt_clk_cpu_middle0_END (1)
#define SOC_CORECRG_B_M_FCM_CLKEN_bitmasken_START (16)
#define SOC_CORECRG_B_M_FCM_CLKEN_bitmasken_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sc_jm_clk_en : 1;
        unsigned int reserved_0 : 1;
        unsigned int sc_jm_rst_n : 1;
        unsigned int sc_one2two_cyc : 1;
        unsigned int sc_timing_ctrl_en : 1;
        unsigned int sc_dtc_start : 1;
        unsigned int sc_fdtc_rev : 1;
        unsigned int sc_cdtc_rev : 1;
        unsigned int sc_max2min_cyc : 5;
        unsigned int reserved_1 : 3;
        unsigned int sc_dtc_margin : 10;
        unsigned int reserved_2 : 1;
        unsigned int sc_cdtc_gap : 5;
    } reg;
} SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_UNION;
#endif
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_jm_clk_en_START (0)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_jm_clk_en_END (0)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_jm_rst_n_START (2)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_jm_rst_n_END (2)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_one2two_cyc_START (3)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_one2two_cyc_END (3)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_timing_ctrl_en_START (4)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_timing_ctrl_en_END (4)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_dtc_start_START (5)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_dtc_start_END (5)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_fdtc_rev_START (6)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_fdtc_rev_END (6)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_cdtc_rev_START (7)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_cdtc_rev_END (7)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_max2min_cyc_START (8)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_max2min_cyc_END (12)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_dtc_margin_START (16)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_dtc_margin_END (25)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_cdtc_gap_START (27)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG0_sc_cdtc_gap_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sc_fdtc_code_init : 8;
        unsigned int sc_cdtc_code_init : 8;
        unsigned int sc_fdtc_cyc : 5;
        unsigned int reserved_0 : 3;
        unsigned int sc_cdtc_cyc : 5;
        unsigned int reserved_1 : 3;
    } reg;
} SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_UNION;
#endif
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_sc_fdtc_code_init_START (0)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_sc_fdtc_code_init_END (7)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_sc_cdtc_code_init_START (8)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_sc_cdtc_code_init_END (15)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_sc_fdtc_cyc_START (16)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_sc_fdtc_cyc_END (20)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_sc_cdtc_cyc_START (24)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG1_sc_cdtc_cyc_END (28)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sc_fdtc_step : 7;
        unsigned int reserved_0 : 1;
        unsigned int sc_cdtc_step : 6;
        unsigned int reserved_1 : 2;
        unsigned int sc_fdtc_length : 8;
        unsigned int sc_cdtc_length : 8;
    } reg;
} SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_UNION;
#endif
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_sc_fdtc_step_START (0)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_sc_fdtc_step_END (6)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_sc_cdtc_step_START (8)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_sc_cdtc_step_END (13)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_sc_fdtc_length_START (16)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_sc_fdtc_length_END (23)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_sc_cdtc_length_START (24)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_CFG2_sc_cdtc_length_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int jm_code_b : 8;
        unsigned int jm_code_a : 8;
        unsigned int jm_code_d : 8;
        unsigned int jm_code_c : 8;
    } reg;
} SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_UNION;
#endif
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_jm_code_b_START (0)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_jm_code_b_END (7)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_jm_code_a_START (8)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_jm_code_a_END (15)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_jm_code_d_START (16)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_jm_code_d_END (23)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_jm_code_c_START (24)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE0_jm_code_c_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int jm_dtc_mea_done : 1;
        unsigned int test_comp : 1;
        unsigned int jm_underflow : 1;
        unsigned int jm_overflow : 1;
        unsigned int test_dtc_curr_st : 3;
        unsigned int reserved : 1;
        unsigned int jm_code_e : 8;
        unsigned int jm_code_min : 8;
        unsigned int jm_code_max : 8;
    } reg;
} SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_UNION;
#endif
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_dtc_mea_done_START (0)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_dtc_mea_done_END (0)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_test_comp_START (1)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_test_comp_END (1)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_underflow_START (2)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_underflow_END (2)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_overflow_START (3)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_overflow_END (3)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_test_dtc_curr_st_START (4)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_test_dtc_curr_st_END (6)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_code_e_START (8)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_code_e_END (15)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_code_min_START (16)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_code_min_END (23)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_code_max_START (24)
#define SOC_CORECRG_B_M_JITTER_MONITOR_APLL3_STATE1_jm_code_max_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int phe_sel_en_fnpll_1v2 : 1;
        unsigned int dtc_test_fnpll_1v2 : 1;
        unsigned int reserved_0 : 2;
        unsigned int phe_sel_in_fnpll_1v2 : 2;
        unsigned int reserved_1 : 2;
        unsigned int dtc_m_cfg_fnpll_1v2 : 6;
        unsigned int reserved_2 : 2;
        unsigned int spread_en_fnpll_1v2 : 1;
        unsigned int downspread_fnpll_1v2 : 1;
        unsigned int sel_extwave_fnpll_1v2 : 1;
        unsigned int reserved_3 : 1;
        unsigned int divval_fnpll_1v2 : 4;
        unsigned int spread_fnpll_1v2 : 3;
        unsigned int reserved_4 : 5;
    } reg;
} SOC_CORECRG_B_M_FNPLL_CFG0_UNION;
#endif
#define SOC_CORECRG_B_M_FNPLL_CFG0_phe_sel_en_fnpll_1v2_START (0)
#define SOC_CORECRG_B_M_FNPLL_CFG0_phe_sel_en_fnpll_1v2_END (0)
#define SOC_CORECRG_B_M_FNPLL_CFG0_dtc_test_fnpll_1v2_START (1)
#define SOC_CORECRG_B_M_FNPLL_CFG0_dtc_test_fnpll_1v2_END (1)
#define SOC_CORECRG_B_M_FNPLL_CFG0_phe_sel_in_fnpll_1v2_START (4)
#define SOC_CORECRG_B_M_FNPLL_CFG0_phe_sel_in_fnpll_1v2_END (5)
#define SOC_CORECRG_B_M_FNPLL_CFG0_dtc_m_cfg_fnpll_1v2_START (8)
#define SOC_CORECRG_B_M_FNPLL_CFG0_dtc_m_cfg_fnpll_1v2_END (13)
#define SOC_CORECRG_B_M_FNPLL_CFG0_spread_en_fnpll_1v2_START (16)
#define SOC_CORECRG_B_M_FNPLL_CFG0_spread_en_fnpll_1v2_END (16)
#define SOC_CORECRG_B_M_FNPLL_CFG0_downspread_fnpll_1v2_START (17)
#define SOC_CORECRG_B_M_FNPLL_CFG0_downspread_fnpll_1v2_END (17)
#define SOC_CORECRG_B_M_FNPLL_CFG0_sel_extwave_fnpll_1v2_START (18)
#define SOC_CORECRG_B_M_FNPLL_CFG0_sel_extwave_fnpll_1v2_END (18)
#define SOC_CORECRG_B_M_FNPLL_CFG0_divval_fnpll_1v2_START (20)
#define SOC_CORECRG_B_M_FNPLL_CFG0_divval_fnpll_1v2_END (23)
#define SOC_CORECRG_B_M_FNPLL_CFG0_spread_fnpll_1v2_START (24)
#define SOC_CORECRG_B_M_FNPLL_CFG0_spread_fnpll_1v2_END (26)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ext_maxaddr_fnpll_1v2 : 8;
        unsigned int extwaveval_fnpll_1v2 : 8;
        unsigned int input_cfg_en_fnpll_1v2 : 1;
        unsigned int test_mode_fnpll_1v2 : 1;
        unsigned int lock_thr_fnpll_1v2 : 2;
        unsigned int refdiv_rst_n_fnpll_1v2 : 1;
        unsigned int fbdiv_rst_n_fnpll_1v2 : 1;
        unsigned int div2_pd_fnpll_1v2 : 1;
        unsigned int lock_en_fnpll_1v2 : 1;
        unsigned int fbdiv_delay_num_fnpll_1v2 : 7;
        unsigned int reserved : 1;
    } reg;
} SOC_CORECRG_B_M_FNPLL_CFG1_UNION;
#endif
#define SOC_CORECRG_B_M_FNPLL_CFG1_ext_maxaddr_fnpll_1v2_START (0)
#define SOC_CORECRG_B_M_FNPLL_CFG1_ext_maxaddr_fnpll_1v2_END (7)
#define SOC_CORECRG_B_M_FNPLL_CFG1_extwaveval_fnpll_1v2_START (8)
#define SOC_CORECRG_B_M_FNPLL_CFG1_extwaveval_fnpll_1v2_END (15)
#define SOC_CORECRG_B_M_FNPLL_CFG1_input_cfg_en_fnpll_1v2_START (16)
#define SOC_CORECRG_B_M_FNPLL_CFG1_input_cfg_en_fnpll_1v2_END (16)
#define SOC_CORECRG_B_M_FNPLL_CFG1_test_mode_fnpll_1v2_START (17)
#define SOC_CORECRG_B_M_FNPLL_CFG1_test_mode_fnpll_1v2_END (17)
#define SOC_CORECRG_B_M_FNPLL_CFG1_lock_thr_fnpll_1v2_START (18)
#define SOC_CORECRG_B_M_FNPLL_CFG1_lock_thr_fnpll_1v2_END (19)
#define SOC_CORECRG_B_M_FNPLL_CFG1_refdiv_rst_n_fnpll_1v2_START (20)
#define SOC_CORECRG_B_M_FNPLL_CFG1_refdiv_rst_n_fnpll_1v2_END (20)
#define SOC_CORECRG_B_M_FNPLL_CFG1_fbdiv_rst_n_fnpll_1v2_START (21)
#define SOC_CORECRG_B_M_FNPLL_CFG1_fbdiv_rst_n_fnpll_1v2_END (21)
#define SOC_CORECRG_B_M_FNPLL_CFG1_div2_pd_fnpll_1v2_START (22)
#define SOC_CORECRG_B_M_FNPLL_CFG1_div2_pd_fnpll_1v2_END (22)
#define SOC_CORECRG_B_M_FNPLL_CFG1_lock_en_fnpll_1v2_START (23)
#define SOC_CORECRG_B_M_FNPLL_CFG1_lock_en_fnpll_1v2_END (23)
#define SOC_CORECRG_B_M_FNPLL_CFG1_fbdiv_delay_num_fnpll_1v2_START (24)
#define SOC_CORECRG_B_M_FNPLL_CFG1_fbdiv_delay_num_fnpll_1v2_END (30)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int bbpd_calib_byp_fnpll_1v2 : 1;
        unsigned int dtc_ctrl_inv_fnpll_1v2 : 1;
        unsigned int dll_force_en_fnpll_1v2 : 1;
        unsigned int reserved_0 : 1;
        unsigned int phe_code_a_fnpll_1v2 : 2;
        unsigned int phe_code_b_fnpll_1v2 : 2;
        unsigned int ctinue_lock_num_fnpll_1v2 : 5;
        unsigned int reserved_1 : 3;
        unsigned int read_en_fnpll_1v2 : 1;
        unsigned int k_gain_cfg_en_fnpll_1v2 : 1;
        unsigned int reserved_2 : 2;
        unsigned int k_gain_av_thr_fnpll_1v2 : 3;
        unsigned int reserved_3 : 1;
        unsigned int k_gain_cfg_fnpll_1v2 : 6;
        unsigned int reserved_4 : 2;
    } reg;
} SOC_CORECRG_B_M_FNPLL_CFG2_UNION;
#endif
#define SOC_CORECRG_B_M_FNPLL_CFG2_bbpd_calib_byp_fnpll_1v2_START (0)
#define SOC_CORECRG_B_M_FNPLL_CFG2_bbpd_calib_byp_fnpll_1v2_END (0)
#define SOC_CORECRG_B_M_FNPLL_CFG2_dtc_ctrl_inv_fnpll_1v2_START (1)
#define SOC_CORECRG_B_M_FNPLL_CFG2_dtc_ctrl_inv_fnpll_1v2_END (1)
#define SOC_CORECRG_B_M_FNPLL_CFG2_dll_force_en_fnpll_1v2_START (2)
#define SOC_CORECRG_B_M_FNPLL_CFG2_dll_force_en_fnpll_1v2_END (2)
#define SOC_CORECRG_B_M_FNPLL_CFG2_phe_code_a_fnpll_1v2_START (4)
#define SOC_CORECRG_B_M_FNPLL_CFG2_phe_code_a_fnpll_1v2_END (5)
#define SOC_CORECRG_B_M_FNPLL_CFG2_phe_code_b_fnpll_1v2_START (6)
#define SOC_CORECRG_B_M_FNPLL_CFG2_phe_code_b_fnpll_1v2_END (7)
#define SOC_CORECRG_B_M_FNPLL_CFG2_ctinue_lock_num_fnpll_1v2_START (8)
#define SOC_CORECRG_B_M_FNPLL_CFG2_ctinue_lock_num_fnpll_1v2_END (12)
#define SOC_CORECRG_B_M_FNPLL_CFG2_read_en_fnpll_1v2_START (16)
#define SOC_CORECRG_B_M_FNPLL_CFG2_read_en_fnpll_1v2_END (16)
#define SOC_CORECRG_B_M_FNPLL_CFG2_k_gain_cfg_en_fnpll_1v2_START (17)
#define SOC_CORECRG_B_M_FNPLL_CFG2_k_gain_cfg_en_fnpll_1v2_END (17)
#define SOC_CORECRG_B_M_FNPLL_CFG2_k_gain_av_thr_fnpll_1v2_START (20)
#define SOC_CORECRG_B_M_FNPLL_CFG2_k_gain_av_thr_fnpll_1v2_END (22)
#define SOC_CORECRG_B_M_FNPLL_CFG2_k_gain_cfg_fnpll_1v2_START (24)
#define SOC_CORECRG_B_M_FNPLL_CFG2_k_gain_cfg_fnpll_1v2_END (29)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int dtc_o_cfg_fnpll_1v2 : 6;
        unsigned int reserved_0 : 2;
        unsigned int bbpd_lock_num_fnpll_1v2 : 8;
        unsigned int lpf_bw_fnpll_1v2 : 3;
        unsigned int reserved_1 : 1;
        unsigned int lpf_bw1_fnpll_1v2 : 3;
        unsigned int reserved_2 : 1;
        unsigned int calib_lpf_bw_fnpll_1v2 : 3;
        unsigned int reserved_3 : 1;
        unsigned int calib_lpf_bw1_fnpll_1v2 : 3;
        unsigned int reserved_4 : 1;
    } reg;
} SOC_CORECRG_B_M_FNPLL_CFG3_UNION;
#endif
#define SOC_CORECRG_B_M_FNPLL_CFG3_dtc_o_cfg_fnpll_1v2_START (0)
#define SOC_CORECRG_B_M_FNPLL_CFG3_dtc_o_cfg_fnpll_1v2_END (5)
#define SOC_CORECRG_B_M_FNPLL_CFG3_bbpd_lock_num_fnpll_1v2_START (8)
#define SOC_CORECRG_B_M_FNPLL_CFG3_bbpd_lock_num_fnpll_1v2_END (15)
#define SOC_CORECRG_B_M_FNPLL_CFG3_lpf_bw_fnpll_1v2_START (16)
#define SOC_CORECRG_B_M_FNPLL_CFG3_lpf_bw_fnpll_1v2_END (18)
#define SOC_CORECRG_B_M_FNPLL_CFG3_lpf_bw1_fnpll_1v2_START (20)
#define SOC_CORECRG_B_M_FNPLL_CFG3_lpf_bw1_fnpll_1v2_END (22)
#define SOC_CORECRG_B_M_FNPLL_CFG3_calib_lpf_bw_fnpll_1v2_START (24)
#define SOC_CORECRG_B_M_FNPLL_CFG3_calib_lpf_bw_fnpll_1v2_END (26)
#define SOC_CORECRG_B_M_FNPLL_CFG3_calib_lpf_bw1_fnpll_1v2_START (28)
#define SOC_CORECRG_B_M_FNPLL_CFG3_calib_lpf_bw1_fnpll_1v2_END (30)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int max_k_gain_fnpll_1v2 : 6;
        unsigned int reserved_0 : 2;
        unsigned int min_k_gain_fnpll_1v2 : 6;
        unsigned int reserved_1 : 2;
        unsigned int en_dac_test_fnpll_1v2 : 1;
        unsigned int updn_sel_fnpll_1v2 : 1;
        unsigned int icp_ctrl_fnpll_1v2 : 1;
        unsigned int reserved_2 : 1;
        unsigned int vdc_sel_fnpll_1v2 : 4;
        unsigned int reserved_3 : 4;
        unsigned int test_data_sel_fnpll_1v2 : 4;
    } reg;
} SOC_CORECRG_B_M_FNPLL_CFG4_UNION;
#endif
#define SOC_CORECRG_B_M_FNPLL_CFG4_max_k_gain_fnpll_1v2_START (0)
#define SOC_CORECRG_B_M_FNPLL_CFG4_max_k_gain_fnpll_1v2_END (5)
#define SOC_CORECRG_B_M_FNPLL_CFG4_min_k_gain_fnpll_1v2_START (8)
#define SOC_CORECRG_B_M_FNPLL_CFG4_min_k_gain_fnpll_1v2_END (13)
#define SOC_CORECRG_B_M_FNPLL_CFG4_en_dac_test_fnpll_1v2_START (16)
#define SOC_CORECRG_B_M_FNPLL_CFG4_en_dac_test_fnpll_1v2_END (16)
#define SOC_CORECRG_B_M_FNPLL_CFG4_updn_sel_fnpll_1v2_START (17)
#define SOC_CORECRG_B_M_FNPLL_CFG4_updn_sel_fnpll_1v2_END (17)
#define SOC_CORECRG_B_M_FNPLL_CFG4_icp_ctrl_fnpll_1v2_START (18)
#define SOC_CORECRG_B_M_FNPLL_CFG4_icp_ctrl_fnpll_1v2_END (18)
#define SOC_CORECRG_B_M_FNPLL_CFG4_vdc_sel_fnpll_1v2_START (20)
#define SOC_CORECRG_B_M_FNPLL_CFG4_vdc_sel_fnpll_1v2_END (23)
#define SOC_CORECRG_B_M_FNPLL_CFG4_test_data_sel_fnpll_1v2_START (28)
#define SOC_CORECRG_B_M_FNPLL_CFG4_test_data_sel_fnpll_1v2_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ref_lost_thr_fnpll_1v2 : 8;
        unsigned int reserved_0 : 8;
        unsigned int reserved_1 : 16;
    } reg;
} SOC_CORECRG_B_M_FNPLL_CFG5_UNION;
#endif
#define SOC_CORECRG_B_M_FNPLL_CFG5_ref_lost_thr_fnpll_1v2_START (0)
#define SOC_CORECRG_B_M_FNPLL_CFG5_ref_lost_thr_fnpll_1v2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0 : 16;
        unsigned int pll_lock_fnpll_1v2 : 1;
        unsigned int pll_lock_ate_fnpll_1v2 : 1;
        unsigned int bbpd_calib_done_fnpll_1v2 : 1;
        unsigned int bbpd_calib_fail_fnpll_1v2 : 1;
        unsigned int bbpd_calib_success_fnpll_1v2 : 1;
        unsigned int reserved_1 : 1;
        unsigned int clock_lost_fnpll_1v2 : 1;
        unsigned int reserved_2 : 1;
        unsigned int test_data_fnpll_1v2 : 8;
    } reg;
} SOC_CORECRG_B_M_FNPLL_STAT_UNION;
#endif
#define SOC_CORECRG_B_M_FNPLL_STAT_pll_lock_fnpll_1v2_START (16)
#define SOC_CORECRG_B_M_FNPLL_STAT_pll_lock_fnpll_1v2_END (16)
#define SOC_CORECRG_B_M_FNPLL_STAT_pll_lock_ate_fnpll_1v2_START (17)
#define SOC_CORECRG_B_M_FNPLL_STAT_pll_lock_ate_fnpll_1v2_END (17)
#define SOC_CORECRG_B_M_FNPLL_STAT_bbpd_calib_done_fnpll_1v2_START (18)
#define SOC_CORECRG_B_M_FNPLL_STAT_bbpd_calib_done_fnpll_1v2_END (18)
#define SOC_CORECRG_B_M_FNPLL_STAT_bbpd_calib_fail_fnpll_1v2_START (19)
#define SOC_CORECRG_B_M_FNPLL_STAT_bbpd_calib_fail_fnpll_1v2_END (19)
#define SOC_CORECRG_B_M_FNPLL_STAT_bbpd_calib_success_fnpll_1v2_START (20)
#define SOC_CORECRG_B_M_FNPLL_STAT_bbpd_calib_success_fnpll_1v2_END (20)
#define SOC_CORECRG_B_M_FNPLL_STAT_clock_lost_fnpll_1v2_START (22)
#define SOC_CORECRG_B_M_FNPLL_STAT_clock_lost_fnpll_1v2_END (22)
#define SOC_CORECRG_B_M_FNPLL_STAT_test_data_fnpll_1v2_START (24)
#define SOC_CORECRG_B_M_FNPLL_STAT_test_data_fnpll_1v2_END (31)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
