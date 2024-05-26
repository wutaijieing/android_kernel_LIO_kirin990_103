#include <linux/dma-mapping.h>
#include <osl_malloc.h>
#include "ipf_balong.h"
#include "hi_ipf.h"
#include <bsp_ddr.h>
#include <mdrv_ipf_comm.h>
#include <osl_malloc.h>
#include <linux/dma-mapping.h>
#include <securec.h>

extern struct ipf_ctx g_ipf_ctx;

const unsigned int ipf32_save_table[][2] = {
    { HI_IPF32_CH0_CTRL_OFFSET, 0xffffffff },      { HI_IPF32_CH0_BDQ_SIZE_OFFSET, 0xffffffff },
    { HI_IPF32_CH0_BDQ_BADDR_OFFSET, 0xffffffff }, { HI_IPF32_CH1_ADQ_CTRL_OFFSET, 0xfffffff0 },
    { HI_IPF32_CH1_ADQ0_RPTR_OFFSET, 0xffffffff }, { HI_IPF32_CH1_ADQ1_RPTR_OFFSET, 0xffffffff },
    { HI_IPF32_CH1_ADQ0_WPTR_OFFSET, 0xffffffff }, { HI_IPF32_CH1_ADQ1_WPTR_OFFSET, 0xffffffff },
    { HI_IPF32_CH1_ADQ_CTRL_OFFSET, 0xffffffff },
};

/*
 * Check whether downstream channel is busy.
 * return 1: busy, 0: idle.
 */
static int ipf32_is_busy(void)
{
    HI_IPF_CH1_STATE_T state;

    state.u32 = ipf_readl(HI_IPF32_CH1_STATE_OFFSET);

    return state.bits.dl_busy;
}

static void _ipf32_reg_save(unsigned int *base)
{
    unsigned int i = 0;

    for (i = 0; i < sizeof(ipf32_save_table) / sizeof(ipf32_save_table[0]); i++) {
        base[i] = ipf_readl(ipf32_save_table[i][0]);
    }
}

static void _ipf32_reg_load(unsigned int *base)
{
    unsigned int i = 0;

    for (i = 0; i < sizeof(ipf32_save_table) / sizeof(ipf32_save_table[0]); i++) {
        ipf_writel((base[i] & ipf32_save_table[i][1]), ipf32_save_table[i][0]);
    }
}

void ipf_cd_en_set(void *bd_base, unsigned int index, unsigned short en)
{
    ipf_bd_s *bd = (ipf_bd_s *)bd_base;
    ipf_desc_attr_t *attrp = (ipf_desc_attr_t *)&bd[index].attribute;
    attrp->bits.cd_en = en;
    return;
}

int ipf_cd_en_get(void *bd_base, unsigned int index)
{
    ipf_bd_s *bd = (ipf_bd_s *)bd_base;
    ipf_desc_attr_t *attrp = (ipf_desc_attr_t *)&bd[index].attribute;
    return attrp->bits.cd_en;
}

void ipf_cd_clear(void *cd_base, unsigned int index)
{
    ipf_cd_s *cd = (ipf_cd_s *)cd_base;
    if (memset_s(&cd[index], sizeof(cd[index]), 0, sizeof(ipf_cd_s))) {
        bsp_err("memset_s failed\n");
    }
}

void ipf_bd_s2h(ipf_config_param_s *param, void *bd_base, unsigned int index)
{
    ipf_bd_s *bd = (ipf_bd_s *)bd_base;
    bd[index].attribute.u16 = 0;
    bd[index].attribute.bits.int_en = param->int_en;
    bd[index].attribute.bits.fc_head = param->fc_head;
    bd[index].attribute.bits.mode = param->mode;
    bd[index].in_ptr = ADDR_TO_U32(param->data);
    bd[index].pkt_len = param->len;
    bd[index].usr_field1 = param->usr_field1;
    bd[index].usr_field2 = param->usr_field2;
    bd[index].usr_field3 = param->usr_field3;
}

void ipf_bd_h2s(ipf_config_param_s *param, void *bd_base, unsigned int index)
{
    ipf_bd_s *bd = (ipf_bd_s *)bd_base;

    param->int_en = bd[index].attribute.bits.int_en;
    param->fc_head = bd[index].attribute.bits.fc_head;
    param->mode = bd[index].attribute.bits.mode;
    param->data = U32_TO_ADDR(bd[index].in_ptr);
    param->len = bd[index].pkt_len;
    param->usr_field1 = bd[index].usr_field1;
    param->usr_field2 = bd[index].usr_field2;
    param->usr_field3 = bd[index].usr_field3;
    g_ipf_ctx.last_bd = &bd[index];
}

void ipf_rd_h2s(ipf_rd_desc_s *param, void *rd_base, unsigned int index)
{
    ipf_rd_s *rd = (ipf_rd_s *)rd_base;

    param->attribute = rd[index].attribute.u16;
    param->pkt_len = rd[index].pkt_len;
    param->result = rd[index].result;
    param->in_ptr = (modem_phy_addr)(uintptr_t)rd[index].in_ptr;
#ifdef CONFIG_PSAM
    param->out_ptr = U32_TO_ADDR(rd[index].usr_field3);
#else
    param->out_ptr = U32_TO_ADDR(rd[index].out_ptr);
#endif
    param->usr_field1 = rd[index].usr_field1;
    param->usr_field2 = rd[index].usr_field2;
    param->usr_field3 = rd[index].usr_field3;
    g_ipf_ctx.last_rd = &rd[index];
    g_ipf_ctx.last_cd = (modem_phy_addr)(uintptr_t)rd[index].in_ptr;
    rd[index].attribute.bits.dl_direct_set ? g_ipf_ctx.stax.direct_bd++ : g_ipf_ctx.stax.indirect_bd++;
}

#ifndef CONFIG_PSAM
int ipf32_ad_s2h(ipf_ad_type_e type, unsigned int n, ipf_ad_desc_s *param)
{
    unsigned int i;
    u32 wptr;
    unsigned int offset;
    unsigned int size;
    ipf_ad_s *ad;

    ad = (IPF_AD_0 == type) ? (ipf_ad_s *)g_ipf_ctx.dl_info.pstIpfADQ0 : (ipf_ad_s *)g_ipf_ctx.dl_info.pstIpfADQ1;

    offset = (IPF_AD_0 == type) ? HI_IPF32_CH1_ADQ0_WPTR_OFFSET : HI_IPF32_CH1_ADQ1_WPTR_OFFSET;

    size = (IPF_AD_0 == type) ? IPF_DLAD0_DESC_SIZE : IPF_DLAD1_DESC_SIZE;

    /* 读出写指针 */
    wptr = ipf_readl(offset);

    for (i = 0; i < n; i++) {
        if (0 == param->out_ptr1) {
            g_ipf_ctx.status->ad_out_ptr_null[type]++;
            return BSP_ERR_IPF_INVALID_PARA;
        }
        ad[wptr].out_ptr0 = ADDR_TO_U32(param[i].out_ptr0);
        ad[wptr].out_ptr1 = ADDR_TO_U32(param[i].out_ptr1);
        wptr = ((wptr + 1) < size) ? (wptr + 1) : 0;
    }
    g_ipf_ctx.status->cfg_ad_cnt[type] += n;
    /* 更新AD0写指针 */
    ipf_writel(wptr, offset);

    if (IPF_AD_0 == type) {
        g_ipf_ctx.last_ad0 = &ad[wptr];
    } else {
        g_ipf_ctx.last_ad1 = &ad[wptr];
    }

    return 0;
}
#endif

void ipf_ad_h2s(ipf_ad_desc_s *param, void *ad_base, unsigned int index)
{
    ipf_ad_s *ad = (ipf_ad_s *)ad_base;
    param->out_ptr0 = U32_TO_ADDR(ad[index].out_ptr0);
    param->out_ptr1 = U32_TO_ADDR(ad[index].out_ptr1);
}

unsigned int ipf_last_get(void *cd_base, unsigned int index)
{
    ipf_cd_s *cd = (ipf_cd_s *)cd_base;
    return cd[index].attribute;
}

void ipf32_config(void)
{
    struct ipf_share_mem_map *sm = bsp_ipf_get_sharemem();

    ipf_writel((unsigned int)g_ipf_ctx.ul_info.pstIpfPhyBDQ, HI_IPF32_CH0_BDQ_BADDR_OFFSET);

    ipf_writel((unsigned int)g_ipf_ctx.dl_info.pstIpfPhyRDQ, HI_IPF32_CH1_RDQ_BADDR_OFFSET);

#ifndef CONFIG_PSAM
    ipf_writel((unsigned int)g_ipf_ctx.dl_info.pstIpfPhyADQ0, HI_IPF32_CH1_ADQ0_BASE_OFFSET);

    ipf_writel((unsigned int)g_ipf_ctx.dl_info.pstIpfPhyADQ1, HI_IPF32_CH1_ADQ1_BASE_OFFSET);
#endif

    sm->ipf_acore_reg_size = sizeof(ipf32_save_table) / sizeof(ipf32_save_table[0]);
    return;
}

unsigned int ipf32_get_ulbd_num(void)
{
    HI_IPF_CH0_DQ_DEPTH_T dq_depth;

    dq_depth.u32 = ipf_readl(HI_IPF32_CH0_DQ_DEPTH_OFFSET);
    return (unsigned int)(IPF_ULBD_DESC_SIZE - (dq_depth.bits.ul_bdq_depth));
}

unsigned int ipf32_get_ulrd_num(void)
{
    HI_IPF_CH0_DQ_DEPTH_T dq_depth;

    dq_depth.u32 = ipf_readl(HI_IPF32_CH0_DQ_DEPTH_OFFSET);
    return dq_depth.bits.ul_rdq_depth;
}

void ipf32_config_bd(unsigned int u32Num, ipf_config_ulparam_s *pstUlParam)
{
    HI_IPF_CH0_BDQ_WPTR_T bdq_wptr;
    unsigned int u32BD;
    unsigned int i;

    bdq_wptr.u32 = ipf_readl(HI_IPF32_CH0_BDQ_WPTR_OFFSET);
    u32BD = bdq_wptr.bits.ul_bdq_wptr;
    for (i = 0; i < u32Num; i++) {
        g_ipf_ctx.desc->bd_s2h(&pstUlParam[i], g_ipf_ctx.ul_info.pstIpfBDQ, u32BD);
        g_ipf_ctx.desc->cd_en_set(g_ipf_ctx.ul_info.pstIpfBDQ, u32BD, ipf_disable);

        u32BD = ((u32BD + 1) < IPF_ULBD_DESC_SIZE) ? (u32BD + 1) : 0;
    }

    g_ipf_ctx.status->cfg_bd_cnt += u32Num;

    ipf_writel(u32BD, HI_IPF32_CH0_BDQ_WPTR_OFFSET);
}

void ipf32_get_dlrd(unsigned int *pu32Num, ipf_rd_desc_s *pstRd)
{
    unsigned int u32RdqRptr;
    unsigned int u32RdqDepth;
    unsigned int u32Num;
    unsigned int i;
    unsigned int u32CdqRptr;
    HI_IPF_CH1_DQ_DEPTH_T dq_depth;

    dq_depth.u32 = ipf_readl(HI_IPF32_CH1_DQ_DEPTH_OFFSET);
    u32RdqDepth = dq_depth.bits.dl_rdq_depth;

    g_ipf_ctx.status->get_rd_times++;

    u32Num = (u32RdqDepth < *pu32Num) ? u32RdqDepth : *pu32Num;
    if (0 == u32Num) {
        *pu32Num = 0;
        return;
    }

    u32RdqRptr = ipf_readl(HI_IPF32_CH1_RDQ_RPTR_OFFSET);
    for (i = 0; i < u32Num; i++) {
        g_ipf_ctx.desc->rd_h2s(&pstRd[i], g_ipf_ctx.dl_info.pstIpfRDQ, u32RdqRptr);
        if (ipf_enable == g_ipf_ctx.desc->cd_en_get(g_ipf_ctx.dl_info.pstIpfRDQ, u32RdqRptr)) {
            u32CdqRptr = ((unsigned long)SHD_DDR_P2V((void *)(uintptr_t)MDDR_FAMA(pstRd[i].in_ptr)) -
                          (uintptr_t)g_ipf_ctx.dl_info.pstIpfCDQ) /
                         (unsigned long)sizeof(ipf_cd_s);  //lint !e712

            while (g_ipf_ctx.desc->cd_last_get(g_ipf_ctx.dl_info.pstIpfCDQ, u32CdqRptr) != 1) {
                u32CdqRptr = ((u32CdqRptr + 1) < IPF_DLCD_DESC_SIZE) ? (u32CdqRptr + 1) : 0;
            }
            u32CdqRptr = ((u32CdqRptr + 1) < IPF_DLCD_DESC_SIZE) ? (u32CdqRptr + 1) : 0;
            *(g_ipf_ctx.dl_info.u32IpfCdRptr) = u32CdqRptr;
        }
        ipf_pm_print_packet((void *)(uintptr_t)MDDR_FAMA(pstRd[i].out_ptr),
                            (unsigned int)pstRd[i].pkt_len);  //lint !e747

        u32RdqRptr = ((u32RdqRptr + 1) < IPF_DLRD_DESC_SIZE) ? (u32RdqRptr + 1) : 0;
        pstRd[i].pkt_len > (g_ipf_ctx.status->ad_thred) ? g_ipf_ctx.status->get_rd_cnt[IPF_AD_1]++
                                                            : g_ipf_ctx.status->get_rd_cnt[IPF_AD_0]++;

        g_ipf_ctx.status->rd_len_update += pstRd[i].pkt_len;
    }

    ipf_writel(u32RdqRptr, HI_IPF32_CH1_RDQ_RPTR_OFFSET);
    *pu32Num = u32Num;
}

unsigned int ipf32_get_dlrd_num(void)
{
    HI_IPF_CH1_DQ_DEPTH_T dq_depth;

    /* 读取RD深度 */
    dq_depth.u32 = ipf_readl(HI_IPF32_CH1_DQ_DEPTH_OFFSET);
    g_ipf_ctx.status->get_rd_num_times++;
    return dq_depth.bits.dl_rdq_depth;
}

void ipf32_dump_callback(void)
{
    unsigned char *p = g_ipf_ctx.dump_area;
    unsigned int size = IPF_DUMP_SIZE;

    if (size < (sizeof(ipf64_rd_s) + sizeof(ipf64_bd_s) + sizeof(ipf64_ad_s) + sizeof(ipf64_ad_s))) {
        return;
    }
    if (p == NULL)
        return;
    if (g_ipf_ctx.last_rd == NULL)
        return;
    if (memcpy_s(p, size, g_ipf_ctx.last_rd, sizeof(ipf_rd_s))) {
        bsp_err("memcpy_s failed\n");
    }
    p += sizeof(ipf_rd_s);
    size -= sizeof(ipf_rd_s);

    if (g_ipf_ctx.last_bd == NULL)
        return;
    if (memcpy_s(p, size, g_ipf_ctx.last_bd, sizeof(ipf_bd_s))) {
        bsp_err("memcpy_s failed\n");
    }
    p += sizeof(ipf_bd_s);
    size -= sizeof(ipf_bd_s);

    if (g_ipf_ctx.last_ad0 == NULL)
        return;
    if (memcpy_s(p, size, g_ipf_ctx.last_ad0, sizeof(ipf_ad_s))) {
        bsp_err("memcpy_s failed\n");
    }
    p += sizeof(ipf_ad_s);
    size -= sizeof(ipf_ad_s);

    if (g_ipf_ctx.last_ad1 == NULL)
        return;
    if (memcpy_s(p, size, g_ipf_ctx.last_ad1, sizeof(ipf_ad_s))) {
        bsp_err("memcpy_s failed\n");
    }
}

void ipf_acpu_wake_ccpu(void)
{
    unsigned int reg;
    UPDATE1(reg, HI_IPF_INT_MASK0, dl_rdq_downoverflow_mask0, 1);
    ipf_writel(0, HI_IPF32_CH1_RDQ_RPTR_OFFSET);
}

struct ipf_desc_handler_s ipf_desc_handler = {
    .name = "ipf_desc",
    .dma_mask = 0xffffffffULL,
    .config = ipf32_config,
    .cd_en_set = ipf_cd_en_set,
    .cd_en_get = ipf_cd_en_get,
    .cd_clear = ipf_cd_clear,
    .bd_s2h = ipf_bd_s2h,
    .bd_h2s = ipf_bd_h2s,
    .rd_h2s = ipf_rd_h2s,
#ifndef CONFIG_PSAM
    .ad_s2h = ipf32_ad_s2h,
#endif
    .ad_h2s = ipf_ad_h2s,
    .cd_last_get = ipf_last_get,
    .get_bd_num = ipf32_get_ulbd_num,
    .get_ulrd_num = ipf32_get_ulrd_num,
    .config_bd = ipf32_config_bd,
    .get_rd = ipf32_get_dlrd,
    .get_dlrd_num = ipf32_get_dlrd_num,
    .is_busy = ipf32_is_busy,
    .save_reg = _ipf32_reg_save,
    .restore_reg = _ipf32_reg_load,
    .acpu_wake_ccpu = ipf_acpu_wake_ccpu,
    .dump = ipf32_dump_callback,
};

EXPORT_SYMBOL(ipf_desc_handler);
