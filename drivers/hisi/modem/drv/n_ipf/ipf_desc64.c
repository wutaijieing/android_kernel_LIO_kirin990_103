#include <linux/dma-mapping.h>
#include <osl_malloc.h>
#include "ipf_balong.h"
#include "hi_ipf.h"
#include <bsp_ddr.h>
#include <mdrv_ipf_comm.h>
#include <osl_malloc.h>
#include <linux/dma-mapping.h>
#include <bsp_ipc.h>
#include <linux/skbuff.h>
#include <bsp_slice.h>
#include <securec.h>
#include <../../adrv/adrv.h>
#include <linux/ip.h>
#include <linux/byteorder/generic.h>
#include <linux/in.h>
#include <linux/printk.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#include <linux/ipv6.h>
#include <linux/igmp.h>
#include <bsp_print.h>
#include <osl_types.h>
#ifdef CONFIG_HUAWEI_DUBAI
#include <chipset_common/dubai/dubai.h>
#endif

extern struct ipf_ctx g_ipf_ctx;

const unsigned int ipf64_save_table[][2] = {
    { HI_IPF64_CH0_CTRL_OFFSET, 0xffffffff },
    { HI_IPF64_CH0_BDQ_SIZE_OFFSET, 0xffffffff },
    { HI_IPF64_CH0_BDQ_BADDR_L_OFFSET, 0xffffffff },
    { HI_IPF64_CH0_BDQ_BADDR_H_OFFSET, 0xffffffff },
};

/*
 * Check whether downstream channel is busy.
 * return 1: busy, 0: idle.
 */
static int ipf64_is_busy(void)
{
    HI_IPF_CH0_STATE_T state0;
    HI_IPF_CH1_STATE_T state1;
    int ret = 0;

    state0.u32 = ipf_readl(HI_IPF64_CH0_STATE_OFFSET);
    state1.u32 = ipf_readl(HI_IPF64_CH1_STATE_OFFSET);

    if (state0.bits.ul_busy) {
        g_ipf_ctx.stax.ch0_busy++;
        ret = 1;
    }

    if (state1.bits.dl_busy) {
        g_ipf_ctx.stax.ch1_busy++;
        ret = 1;
    }

    return ret;
}

static void _ipf64_reg_save(unsigned int *base)
{
    unsigned int i = 0;

    for (i = 0; i < sizeof(ipf64_save_table) / sizeof(ipf64_save_table[0]); i++) {
        base[i] = ipf_readl(ipf64_save_table[i][0]);
    }
}

static void _ipf64_reg_load(unsigned int *base)
{
    unsigned int i = 0;

    for (i = 0; i < sizeof(ipf64_save_table) / sizeof(ipf64_save_table[0]); i++) {
        ipf_writel((base[i] & ipf64_save_table[i][1]), ipf64_save_table[i][0]);
    }
}

void ipf64_cd_en_set(void *bd_base, unsigned int index, unsigned short en)
{
    ipf64_bd_s *bd = (ipf64_bd_s *)bd_base;
    ipf_desc_attr_t *attrp = (ipf_desc_attr_t *)&bd[index].attribute;
    attrp->bits.cd_en = en;
    return;
}

int ipf64_cd_en_get(void *bd_base, unsigned int index)
{
    ipf64_bd_s *bd = (ipf64_bd_s *)bd_base;
    ipf_desc_attr_t *attrp = (ipf_desc_attr_t *)&bd[index].attribute;
    return attrp->bits.cd_en;
}

void ipf64_cd_clear(void *cd_base, unsigned int index)
{
    ipf64_cd_s *cd = (ipf64_cd_s *)cd_base;
    if (memset_s(&cd[index], sizeof(cd[index]), 0, sizeof(ipf64_cd_s))) {
        bsp_err("memset_s failed\n");
    }
}

void ipf64_bd_s2h(ipf_config_param_s *param, void *bd_base, unsigned int index)
{
    ipf64_bd_s *bd = (ipf64_bd_s *)bd_base;
    bd[index].attribute.u16 = 0;
    bd[index].attribute.bits.int_en = param->int_en;
    bd[index].attribute.bits.fc_head = param->fc_head;
    bd[index].attribute.bits.mode = param->mode;
    bd[index].input_ptr.addr = (uintptr_t)param->data;
    bd[index].pkt_len = param->len;
    bd[index].user_field1 = param->usr_field1;
    bd[index].user_field2 = param->usr_field2;
    bd[index].user_field3 = param->usr_field3;
}

void ipf64_bd_h2s(ipf_config_param_s *param, void *bd_base, unsigned int index)
{
    ipf64_bd_s *bd = (ipf64_bd_s *)bd_base;

    param->int_en = bd[index].attribute.bits.int_en;
    param->fc_head = bd[index].attribute.bits.fc_head;
    param->mode = bd[index].attribute.bits.mode;
    param->data = (modem_phy_addr)(uintptr_t)bd[index].input_ptr.addr;
    param->len = bd[index].pkt_len;
    param->usr_field1 = bd[index].user_field1;
    param->usr_field2 = bd[index].user_field2;
    param->usr_field3 = bd[index].user_field3;

    g_ipf_ctx.last_bd = &bd[index];
}

void ipf64_rd_h2s(ipf_rd_desc_s *param, void *rd_base, unsigned int index)
{
    static ipf64_rd_s tmp;
    ipf64_rd_s *rd = (ipf64_rd_s *)rd_base;

    if (memcpy_s(&tmp, sizeof(tmp), &rd[index], sizeof(ipf64_rd_s))) {
        bsp_err("memcpy_s failed\n");
    }
    param->attribute = tmp.attribute.u16;
    param->fc_head = tmp.attribute.bits.fc_head;
    param->pkt_len = tmp.pkt_len;
    param->result = tmp.result;
    param->in_ptr = (modem_phy_addr)(uintptr_t)tmp.input_ptr.addr;
#ifdef CONFIG_PSAM
    param->out_ptr = (modem_phy_addr)(uintptr_t)tmp.virt.ptr;
#else
    param->out_ptr = (modem_phy_addr)(uintptr_t)tmp.output_ptr;
#endif
    param->usr_field1 = tmp.user_field1;
    param->usr_field2 = tmp.user_field2;
    param->usr_field3 = tmp.user_field3;

    g_ipf_ctx.last_rd = &rd[index];
    g_ipf_ctx.last_cd = (modem_phy_addr)(uintptr_t)tmp.input_ptr.addr;

    tmp.attribute.bits.dl_direct_set ? g_ipf_ctx.stax.direct_bd++ : g_ipf_ctx.stax.indirect_bd++;
}

#ifndef CONFIG_PSAM
int ipf64_ad_s2h(ipf_ad_type_e type, unsigned int n, ipf_ad_desc_s *param)
{
    unsigned int i;
    unsigned int wptr;
    unsigned int offset;
    unsigned int size;
    ipf64_ad_s *ad;

    ad = (IPF_AD_0 == type) ? (ipf64_ad_s *)g_ipf_ctx.dl_info.pstIpfADQ0 : (ipf64_ad_s *)g_ipf_ctx.dl_info.pstIpfADQ1;

    offset = (IPF_AD_0 == type) ? HI_IPF64_CH1_ADQ0_WPTR_OFFSET : HI_IPF64_CH1_ADQ1_WPTR_OFFSET;

    size = (IPF_AD_0 == type) ? IPF_DLAD0_DESC_SIZE : IPF_DLAD1_DESC_SIZE;

    /* 读出写指针 */
    wptr = ipf_readl(offset);
    for (i = 0; i < n; i++) {
        if (0 == param->out_ptr1) {
            g_ipf_ctx.status->ad_out_ptr_null[type]++;
            return BSP_ERR_IPF_INVALID_PARA;
        }
        ad[wptr].output_ptr0.addr = (uintptr_t)(param[i].out_ptr0);
        ad[wptr].output_ptr1.addr = (uintptr_t)(param[i].out_ptr1);
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

void ipf64_ad_h2s(ipf_ad_desc_s *param, void *ad_base, unsigned int index)
{
    ipf64_ad_s *ad = (ipf64_ad_s *)ad_base;
    param->out_ptr0 = (modem_phy_addr)(uintptr_t)ad[index].output_ptr0.bits.ptr;
    param->out_ptr1 = (modem_phy_addr)(uintptr_t)ad[index].output_ptr1.bits.ptr;
}

unsigned int ipf64_last_get(void *cd_base, unsigned int index)
{
    ipf64_cd_s *cd = (ipf64_cd_s *)cd_base;
    return cd[index].cd_last;
}

void ipf64_config(void)
{
    struct ipf_share_mem_map *sm = bsp_ipf_get_sharemem();

    phy_addr_write(g_ipf_ctx.ul_info.pstIpfPhyBDQ, (unsigned char *)g_ipf_ctx.regs + HI_IPF64_CH0_BDQ_BADDR_H_OFFSET,
                   (unsigned char *)g_ipf_ctx.regs + HI_IPF64_CH0_BDQ_BADDR_L_OFFSET);

    phy_addr_write(g_ipf_ctx.dl_info.pstIpfPhyRDQ, (unsigned char *)g_ipf_ctx.regs + HI_IPF64_CH1_RDQ_BADDR_H_OFFSET,
                   (unsigned char *)g_ipf_ctx.regs + HI_IPF64_CH1_RDQ_BADDR_L_OFFSET);

#ifndef CONFIG_PSAM
    phy_addr_write(g_ipf_ctx.dl_info.pstIpfPhyADQ0, (unsigned char *)g_ipf_ctx.regs + HI_IPF64_CH1_ADQ0_BASE_H_OFFSET,
                   (unsigned char *)g_ipf_ctx.regs + HI_IPF64_CH1_ADQ0_BASE_L_OFFSET);

    phy_addr_write(g_ipf_ctx.dl_info.pstIpfPhyADQ1, (unsigned char *)g_ipf_ctx.regs + HI_IPF64_CH1_ADQ1_BASE_H_OFFSET,
                   (unsigned char *)g_ipf_ctx.regs + HI_IPF64_CH1_ADQ1_BASE_L_OFFSET);

#endif
    sm->ipf_acore_reg_size = sizeof(ipf64_save_table) / sizeof(ipf64_save_table[0]);
    return;
}

unsigned int ipf64_get_ulbd_num(void)
{
    HI_IPF_CH0_BDQ_DEPTH_T dq_depth;

    /* 计算空闲BD数量 */
    dq_depth.u32 = ipf_readl(HI_IPF64_CH0_BDQ_DEPTH_OFFSET);
    return (unsigned int)(IPF_ULBD_DESC_SIZE - dq_depth.bits.ul_bdq_depth);
}

unsigned int ipf64_get_ulrd_num(void)
{
    HI_IPF_CH0_RDQ_DEPTH_T dq_depth;

    dq_depth.u32 = ipf_readl(HI_IPF64_CH0_RDQ_DEPTH_OFFSET);
    return dq_depth.bits.ul_rdq_depth;
}

void ipf64_config_bd(unsigned int u32Num, ipf_config_ulparam_s *pstUlParam)
{
    HI_IPF_CH0_BDQ_WPTR_T bdq_wptr;
    unsigned int u32BD;
    unsigned int i;

    /* è¯»å‡ºBDå†™æŒ‡é’ˆ,å°†u32BdqWpträ½œä¸ºä¸´æ—¶å†™æŒ‡é’ˆä½¿ç”¨ */
    bdq_wptr.u32 = ipf_readl(HI_IPF64_CH0_BDQ_WPTR_OFFSET);
    u32BD = bdq_wptr.bits.ul_bdq_wptr;
    for (i = 0; i < u32Num; i++) {
        g_ipf_ctx.desc->bd_s2h(&pstUlParam[i], g_ipf_ctx.ul_info.pstIpfBDQ, u32BD);
        g_ipf_ctx.desc->cd_en_set(g_ipf_ctx.ul_info.pstIpfBDQ, u32BD, ipf_disable);

        u32BD = ((u32BD + 1) < IPF_ULBD_DESC_SIZE) ? (u32BD + 1) : 0;
    }

    g_ipf_ctx.status->cfg_bd_cnt += u32Num;

    /* æ›´æ–°BDå†™æŒ‡é’ˆ */
    ipf_writel(u32BD, HI_IPF64_CH0_BDQ_WPTR_OFFSET);
}

void __attribute__((weak)) ipf_get_waking_pkt(void *data, unsigned int len)
{
    return;
};

void ipf_print_pkt_info(unsigned char *data)
{
    struct iphdr *iph = (struct iphdr *)data;
    struct udphdr *udph = NULL;
    struct tcphdr *tcph = NULL;
    struct icmphdr *icmph = NULL;
    struct ipv6hdr *ip6h = NULL;

    pr_err("ipf dl wakeup, ip version=%d\n", iph->version);

    if (iph->version == 4) {
        pr_err("src ip:%d.%d.%d.x, dst ip:%d.%d.%d.x\n", iph->saddr & 0xff, (iph->saddr >> 8) & 0xff,
               (iph->saddr >> 16) & 0xff, iph->daddr & 0xff, (iph->daddr >> 8) & 0xff, (iph->daddr >> 16) & 0xff);
        if (iph->protocol == IPPROTO_UDP) {
            udph = (struct udphdr *)(data + sizeof(struct iphdr));
            pr_err("UDP packet, src port:%d, dst port:%d.\n", ntohs(udph->source), ntohs(udph->dest));
#ifdef CONFIG_HUAWEI_DUBAI
            dubai_log_packet_wakeup_stats("DUBAI_TAG_MODEM_PACKET_WAKEUP_UDP_V4", "port", ntohs(udph->dest));
#endif
        } else if (iph->protocol == IPPROTO_TCP) {
            tcph = (struct tcphdr *)(data + sizeof(struct iphdr));
            pr_err("TCP packet, src port:%d, dst port:%d\n", ntohs(tcph->source), ntohs(tcph->dest));
#ifdef CONFIG_HUAWEI_DUBAI
            dubai_log_packet_wakeup_stats("DUBAI_TAG_MODEM_PACKET_WAKEUP_TCP_V4", "port", ntohs(tcph->dest));
#endif
        } else if (iph->protocol == IPPROTO_ICMP) {
            icmph = (struct icmphdr *)(data + sizeof(struct iphdr));
            pr_err("ICMP packet, type(%d):%s, code:%d.\n", icmph->type,
                   ((icmph->type == 0) ? "ping reply" : ((icmph->type == 8) ? "ping request" : "other icmp pkt")),
                   icmph->code);
#ifdef CONFIG_HUAWEI_DUBAI
            dubai_log_packet_wakeup_stats("DUBAI_TAG_MODEM_PACKET_WAKEUP", "protocol", (int)iph->protocol);
#endif
        } else if (iph->protocol == IPPROTO_IGMP) {
            pr_err("ICMP packet\n");
#ifdef CONFIG_HUAWEI_DUBAI
            dubai_log_packet_wakeup_stats("DUBAI_TAG_MODEM_PACKET_WAKEUP", "protocol", (int)iph->protocol);
#endif
        } else {
            pr_err("Other IPV4 packet\n");
#ifdef CONFIG_HUAWEI_DUBAI
            dubai_log_packet_wakeup_stats("DUBAI_TAG_MODEM_PACKET_WAKEUP", "protocol", (int)iph->protocol);
#endif
        }
    } else if (iph->version == 6) {
        ip6h = (struct ipv6hdr *)data;
        pr_err("version: %d, payload length: %d, nh->nexthdr: %d. \n", ip6h->version, ntohs(ip6h->payload_len),
               ip6h->nexthdr);
        pr_err("ipv6 src addr:%04x:%x:%xx:x:x:x:x:x  \n", ntohs(ip6h->saddr.in6_u.u6_addr16[7]),
               ntohs(ip6h->saddr.in6_u.u6_addr16[6]), (ip6h->saddr.in6_u.u6_addr8[11]));
        pr_err("ipv6 dst addr:%04x:%x:%xx:x:x:x:x:x \n", ntohs(ip6h->saddr.in6_u.u6_addr16[7]),
               ntohs(ip6h->saddr.in6_u.u6_addr16[6]), (ip6h->saddr.in6_u.u6_addr8[11]));
#ifdef CONFIG_HUAWEI_DUBAI
        dubai_log_packet_wakeup_stats("DUBAI_TAG_MODEM_PACKET_WAKEUP", "protocol", IPPROTO_IPV6);
#endif
    }
}

void ipf_waking_pkt_pick(void *buf, size_t len)
{
    struct sk_buff *skb = NULL;

    if (g_ipf_ctx.status->resume_with_intr) {
        if (!virt_addr_valid((uintptr_t)buf)) {  //lint !e648
            skb = phys_to_virt((uintptr_t)buf);
            if (!virt_addr_valid(skb)) {  //lint !e648
                return;
            }
        } else {
            return;
        }

        if (!virt_addr_valid(skb->data)) { /*lint !e644 !e648*/
            return;
        }

        dma_unmap_single(g_ipf_ctx.dev, (dma_addr_t)virt_to_phys(skb->data), len, DMA_FROM_DEVICE);

        g_ipf_ctx.stax.pkt_dbg_in = bsp_get_slice_value();
        ipf_get_waking_pkt(skb->data, skb->len);
        ipf_print_pkt_info(skb->data);
        g_ipf_ctx.stax.pkt_dbg_out = bsp_get_slice_value();

        g_ipf_ctx.stax.wakeup_irq++;
        g_ipf_ctx.status->resume_with_intr = 0;
    }

    return;
}

void ipf64_get_dlrd(unsigned int *pu32Num, ipf_rd_desc_s *pstRd)
{
    unsigned int u32RdqRptr;
    unsigned int u32RdqDepth;
    unsigned int u32Num;
    unsigned int i;
    unsigned int u32CdqRptr;
    HI_IPF_CH1_RDQ_DEPTH_T dq_depth;

    /* 读取RD深度 */
    dq_depth.u32 = ipf_readl(HI_IPF64_CH1_RDQ_DEPTH_OFFSET);
    u32RdqDepth = dq_depth.bits.dl_rdq_depth;

    g_ipf_ctx.status->get_rd_times++;

    u32Num = (u32RdqDepth < *pu32Num) ? u32RdqDepth : *pu32Num;
    if (0 == u32Num) {
        *pu32Num = 0;
        return;
    }

    u32RdqRptr = ipf_readl(HI_IPF64_CH1_RDQ_RPTR_OFFSET);
    for (i = 0; i < u32Num; i++) {
        g_ipf_ctx.desc->rd_h2s(&pstRd[i], g_ipf_ctx.dl_info.pstIpfRDQ, u32RdqRptr);
        if (ipf_enable == g_ipf_ctx.desc->cd_en_get(g_ipf_ctx.dl_info.pstIpfRDQ, u32RdqRptr)) {
            /* 更新CD读指针 */
            u32CdqRptr = ((unsigned long)(uintptr_t)SHD_DDR_P2V(pstRd[i].in_ptr) -
                          (unsigned long)(uintptr_t)g_ipf_ctx.dl_info.pstIpfCDQ) /
                         sizeof(ipf64_cd_s);  //lint !e712

            while (g_ipf_ctx.desc->cd_last_get(g_ipf_ctx.dl_info.pstIpfCDQ, u32CdqRptr) != 1) {
                u32CdqRptr = ((u32CdqRptr + 1) < IPF_DLCD_DESC_SIZE) ? (u32CdqRptr + 1) : 0;
            }
            u32CdqRptr = ((u32CdqRptr + 1) < IPF_DLCD_DESC_SIZE) ? (u32CdqRptr + 1) : 0;
            *(g_ipf_ctx.dl_info.u32IpfCdRptr) = u32CdqRptr;
        }
        ipf_waking_pkt_pick((void *)pstRd[i].out_ptr, (size_t)pstRd[i].pkt_len);
        /* 更新RD读指针 */
        u32RdqRptr = ((u32RdqRptr + 1) < IPF_DLRD_DESC_SIZE) ? (u32RdqRptr + 1) : 0;
        pstRd[i].pkt_len > (g_ipf_ctx.status->ad_thred) ? g_ipf_ctx.status->get_rd_cnt[IPF_AD_1]++
                                                            : g_ipf_ctx.status->get_rd_cnt[IPF_AD_0]++;

        g_ipf_ctx.status->rd_len_update += pstRd[i].pkt_len;
    }

    ipf_writel(u32RdqRptr, HI_IPF64_CH1_RDQ_RPTR_OFFSET);
    *pu32Num = u32Num;
}

unsigned int ipf64_get_dlrd_num(void)
{
    HI_IPF_CH1_RDQ_DEPTH_T dq_depth;

    /* 读取RD深度 */
    dq_depth.u32 = ipf_readl(HI_IPF64_CH1_RDQ_DEPTH_OFFSET);
    g_ipf_ctx.status->get_rd_num_times++;
    return dq_depth.bits.dl_rdq_depth;
}

void ipf64_dump_callback(void)
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
    if (memcpy_s(p, size, g_ipf_ctx.last_rd, sizeof(ipf64_rd_s))) {
        bsp_err("memcpy_s failed\n");
    }
    p += sizeof(ipf64_rd_s);
    size -= sizeof(ipf64_rd_s);

    if (g_ipf_ctx.last_bd == NULL)
        return;
    if (memcpy_s(p, size, g_ipf_ctx.last_bd, sizeof(ipf64_bd_s))) {
        bsp_err("memcpy_s failed\n");
    }
    p += sizeof(ipf64_bd_s);
    size -= sizeof(ipf64_bd_s);

    if (g_ipf_ctx.last_ad0 == NULL)
        return;
    if (memcpy_s(p, size, g_ipf_ctx.last_ad0, sizeof(ipf64_ad_s))) {
        bsp_err("memcpy_s failed\n");
    }
    p += sizeof(ipf64_ad_s);
    size -= sizeof(ipf64_ad_s);

    if (g_ipf_ctx.last_ad1 == NULL)
        return;
    if (memcpy_s(p, size, g_ipf_ctx.last_ad1, sizeof(ipf64_ad_s))) {
        bsp_err("memcpy_s failed\n");
    }

    if (memcpy_s(&g_ipf_ctx.share_mem->drd, sizeof(g_ipf_ctx.share_mem->drd), g_ipf_ctx.dl_info.pstIpfRDQ,
                 sizeof(ipf_drd_u))) {
        bsp_err("memcpy_s failed\n");
    }
}

void ipf64_acpu_wake_ccpu(void)
{
    unsigned int reg;
    unsigned long flags;
    struct ipf_share_mem_map *sm = bsp_ipf_get_sharemem();

    spin_lock_irqsave(&g_ipf_ctx.filter_spinlock, flags);
    if (MDRV_ERROR == bsp_ipc_spin_lock_timeout(IPC_SEM_IPF_PWCTRL, IPF_SHM_LOCK_TIMEOUT)) {
        bsp_err("ipc spin lock timeout.\n");
    }

    if (IPF_PWR_DOWN == sm->init.status.ccore) {
        UPDATE1(reg, HI_IPF_TIMER_LOAD, timer_load, IPF_FAST_TIMEOUT);

        UPDATE3(reg, HI_IPF_TRANS_CNT_CTRL, timer_en, 1, timer_clear, 1, timer_prescaler, IPF_FAST_TIMEOUT);
        g_ipf_ctx.stax.timer_start++;
        g_ipf_ctx.stax.wake_time = bsp_get_slice_value();
    }

    bsp_ipc_spin_unlock(IPC_SEM_IPF_PWCTRL);
    spin_unlock_irqrestore(&g_ipf_ctx.filter_spinlock, flags);
}

struct ipf_desc_handler_s ipf_desc64_handler = {
    .name = "ipf64_desc",
    .dma_mask = 0xffffffffffffffffULL,
    .config = ipf64_config,
    .cd_en_set = ipf64_cd_en_set,
    .cd_en_get = ipf64_cd_en_get,
    .cd_clear = ipf64_cd_clear,
    .bd_s2h = ipf64_bd_s2h,
    .bd_h2s = ipf64_bd_h2s,
    .rd_h2s = ipf64_rd_h2s,
#ifndef CONFIG_PSAM
    .ad_s2h = ipf64_ad_s2h,
#endif
    .ad_h2s = ipf64_ad_h2s,
    .cd_last_get = ipf64_last_get,
    .get_bd_num = ipf64_get_ulbd_num,
    .get_ulrd_num = ipf64_get_ulrd_num,
    .config_bd = ipf64_config_bd,
    .get_rd = ipf64_get_dlrd,
    .get_dlrd_num = ipf64_get_dlrd_num,
    .is_busy = ipf64_is_busy,
    .save_reg = _ipf64_reg_save,
    .restore_reg = _ipf64_reg_load,
    .acpu_wake_ccpu = ipf64_acpu_wake_ccpu,
    .dump = ipf64_dump_callback,
};

EXPORT_SYMBOL(ipf_desc64_handler);
