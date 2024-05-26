


// 1 头文件包含
#include "hwal_net.h"
#include "wal_main.h"
#include "securec.h"

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#include "lwip/tcpip.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HWAL_NET_C

// 2 全局变量定义
netif_flow_ctrl_enum g_netif_flow_ctrl = NETIF_FLOW_CTRL_OFF; // [HL] platform\inc\oal\liteos\arch\oal_net.h

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
hwal_txdata_thread_stru   g_wal_txdata_thread;
#endif

// 3 函数实现

void netif_set_flow_ctrl_status(oal_lwip_netif *netif, netif_flow_ctrl_enum_uint8 status)
{
    if (netif == NULL) {
        oam_error_log0(0, 0, "netif parameter NULL.");
        return;
    }

    if (status == NETIF_FLOW_CTRL_ON) {
        g_netif_flow_ctrl = NETIF_FLOW_CTRL_ON;
    } else if (status == NETIF_FLOW_CTRL_OFF) {
        g_netif_flow_ctrl = NETIF_FLOW_CTRL_OFF;
    } else {
        oam_error_log0(0, 0, "netif_set_flow_ctrl_status::status invalid!");
    }
}


int32_t hwal_netif_rx(oal_net_device_stru *net_dev, oal_netbuf_stru *netbuf)
{
    uint16_t ether_type;
    oal_ether_header_stru *eth_hdr = NULL;
    oal_hisi_eapol_stru *hisi_eapol = NULL;

    if ((netbuf == NULL) || (net_dev == NULL)) {
        oam_error_log0(0, 0, "netif_rx_ni parameter NULL.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取帧类型 */
    eth_hdr = (oal_ether_header_stru *)(netbuf->data);
    ether_type = eth_hdr->us_ether_type;

    /* 根据不同帧类型，进行相应处理 */
    if ((OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == ether_type) ||
        (OAL_HOST2NET_SHORT(ETHER_TYPE_ARP) == ether_type) ||
        (OAL_HOST2NET_SHORT(ETHER_TYPE_RARP) == ether_type)) {
        hwal_lwip_receive(net_dev->lwip_netif, netbuf);

#ifdef _PRE_LWIP_ZERO_COPY
        if (OAL_SUCC != hwal_skb_struct_free(netbuf)) {
            oam_error_log0(0, 0, "[hwal_lwip_receive] skb_struct free fail");
        }
#else
        oal_netbuf_free(netbuf);
#endif
    } else if (OAL_HOST2NET_SHORT(ETHER_TYPE_PAE) == ether_type) {
        hisi_eapol = &net_dev->hisi_eapol;

        /* 关中断将报文入队 */
        oal_netbuf_list_tail(&hisi_eapol->st_eapol_skb_head, netbuf);

        /* 入队后抛事件让WPA获取 */
        if ((hisi_eapol->en_register == TRUE) && (hisi_eapol->notify_callback != NULL)) {
            hisi_eapol->notify_callback(net_dev->name, hisi_eapol->context);
        } else {
            oam_error_log0(0, 0, "eapol process is not register.");
        }
    } else {
        oal_netbuf_free(netbuf);
    }

    return OAL_SUCC;
}


oal_netbuf_stru *hwal_lwip_skb_alloc(oal_net_device_stru *net_dev, uint16_t lwip_buflen)
{
    oal_netbuf_stru *buf = NULL;
    uint32_t total_size;

    if (net_dev == NULL) {
        oam_error_log0(0, 0, "hwal_lwip_skb_alloc net_dev NULL.");
        return NULL;
    }

    /* 申请大小为HCC允许最大长度加上头空间及尾空间 */
    total_size = lwip_buflen + (uint16_t)net_dev->needed_headroom + (uint16_t)net_dev->needed_tailroom;
    /* magic num to be modified by tangshuang */
    buf = oal_netbuf_alloc(total_size, net_dev->needed_headroom, 32);

    return buf;
}


oal_netbuf_stru *hwal_skb_struct_alloc(void)
{
    oal_netbuf_stru *skb = (oal_netbuf_stru *)oal_memalloc(SKB_DATA_ALIGN(sizeof(oal_netbuf_stru)));
    if (OAL_UNLIKELY(skb == NULL)) {
        return NULL;
    }

    memset_s(skb, sizeof(oal_netbuf_stru), 0, sizeof(oal_netbuf_stru));
    oal_atomic_set(&skb->users, 1);

    return skb;
}


uint32_t hwal_skb_struct_free(oal_netbuf_stru *sk_buf)
{
    if (sk_buf == NULL) {
        return OAL_FAIL;
    }

    oal_free(sk_buf);

    return OAL_SUCC;
}


uint32_t hwal_pbuf_convert_2_skb(oal_lwip_buf *lwip_buf, oal_netbuf_stru *sk_buf)
{
    oal_net_device_stru *netdev = oal_dev_get_by_name("wlan0");
    uint32_t reserve_tail_room;

    if (netdev == NULL) {
        oam_error_log0(0, 0, "[hwal_pbuf_convert_2_skb] netdev is NULL!");
        return OAL_FAIL;
    }

    reserve_tail_room = OAL_NETDEVICE_TAILROOM(netdev);
    /*
     * 1.skb/pbuf指针转换
     * 2.sk/pbuf长度转换
     * 3.pbuf->ref自增，确保数据由HCC成功发送后再释放空间
     */
    if ((lwip_buf == NULL) || (sk_buf == NULL)) {
        oam_error_log2(0, 0, "[hwal_pbuf_convert_2_skb] lwip_buf[%x],sk_buf[%x]!", lwip_buf, sk_buf);
        return OAL_FAIL;
    }

    /*
     *                              pbuf's memory distribution
     * |-----PBUF-----|--------RESERVE--------|----------PAYLOAD---------|---TAILROOM---|
     *
     *                       converted sk_buff's ptr according to pbuf
     * p_mem_head     head                    data                       tail           end
     */
    sk_buf->p_mem_head = (uint8_t *)lwip_buf;
    sk_buf->head = sk_buf->p_mem_head + OAL_NLMSG_ALIGN(sizeof(oal_lwip_buf));
    sk_buf->data = (uint8_t *)lwip_buf->payload;
    skb_reset_tail_pointer(sk_buf);
    sk_buf->tail += lwip_buf->len;
    /* 内存申请时，已添加四字节对齐，此处不用担心内存越界 */
    sk_buf->end = OAL_NLMSG_ALIGN((uint32_t)sk_buf->tail) + OAL_NLMSG_ALIGN(reserve_tail_room);

    sk_buf->len = lwip_buf->len;

    lwip_buf->ref++;

    return OAL_SUCC;
}


oal_lwip_buf *hwal_skb_convert_2_pbuf(oal_netbuf_stru *sk_buf)
{
    /*
    1.skb/pbuf指针转换
    2.sk/pbuf长度转换 len, tot_len
 */
    oal_lwip_buf *lwip_buf = NULL;

    if ((sk_buf == NULL) || (sk_buf->p_mem_head == NULL)) {
        oam_error_log0(0, 0, "[hwal_skb_convert_2_pbuf] sk_buf or p_mem_head = NULL!");
        return NULL;
    }

    lwip_buf = (oal_lwip_buf *)sk_buf->p_mem_head;
    lwip_buf->payload = sk_buf->data;
    lwip_buf->tot_len = (uint16_t)sk_buf->len; // 32bit --->16bit
    lwip_buf->len = (uint16_t)sk_buf->len; // 32bit --->16bit

    return lwip_buf;
}

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)

uint8_t hwal_get_txdata_thread_enable(void)
{
    return g_wal_txdata_thread.txthread_enable;
}

OAL_STATIC int32_t hwal_txdata(void *data)
{
    oal_netbuf_stru *netbuf = NULL;
    oal_net_device_stru *net_dev = NULL;

    while (OAL_LIKELY(!oal_down_interruptible(&g_wal_txdata_thread.txdata_sema))) {
        if (oal_kthread_should_stop()) {
            break;
        }

        netbuf = oal_netbuf_delist(&g_wal_txdata_thread.txdata_netbuf_head);
        if (netbuf != NULL) {
            net_dev = (oal_net_device_stru *)netbuf->dev;
            net_dev->netdev_ops->ndo_start_xmit(netbuf, net_dev);
        }
    }

    return OAL_SUCC;
}

void hwal_txdata_sched(void)
{
    oal_up(&g_wal_txdata_thread.txdata_sema);
}

int32_t hwal_txdata_netbuf_enqueue(oal_netbuf_stru *netbuf)
{
    if (oal_netbuf_list_len(&g_wal_txdata_thread.txdata_netbuf_head) > 1000) {
        oal_netbuf_free(netbuf);
        g_wal_txdata_thread.pkt_loss_cnt++;
        oal_msleep(1);
        return OAL_FAIL;
    }
    oal_netbuf_list_tail(&g_wal_txdata_thread.txdata_netbuf_head, netbuf);
    return OAL_SUCC;
}

uint32_t hwal_txdata_thread_init(void)
{
    oal_kthread_param_stru thread_param = {0};

    OAL_WAIT_QUEUE_INIT_HEAD(&g_wal_txdata_thread.txdata_wq);

    oal_netbuf_list_head_init(&g_wal_txdata_thread.txdata_netbuf_head);

    g_wal_txdata_thread.txthread_enable = OAL_TRUE;
    g_wal_txdata_thread.pkt_loss_cnt = 0;

    oal_sema_init(&g_wal_txdata_thread.txdata_sema, 0);
    memset_s(&thread_param, sizeof(oal_kthread_param_stru), 0, sizeof(oal_kthread_param_stru));
    thread_param.l_cpuid = NOT_BIND_CPU;
    thread_param.l_policy = OAL_SCHED_FIFO;

    thread_param.l_prio = HWAL_TXDATA_THERAD_PRIORITY;
    thread_param.ul_stacksize = 0x2000;
    g_wal_txdata_thread.txdata_thread = oal_kthread_create("hwal_txdata", hwal_txdata, NULL, &thread_param);
    if (g_wal_txdata_thread.txdata_thread == NULL) {
        oam_error_log0(0, 0, "hwal_tx thread create failed!\n");
    }

    return OAL_SUCC;
}
#endif


void hwal_lwip_receive(oal_lwip_netif *netif, oal_netbuf_stru *drv_buf)
{
    oal_lwip_buf *lwip_buf = NULL;
    if ((netif == NULL) || (drv_buf == NULL)) {
        oam_error_log0(0, 0, "hwal_lwip_receive netif is null!");
        return;
    }

#ifdef _PRE_LWIP_ZERO_COPY
    lwip_buf = hwal_skb_convert_2_pbuf(drv_buf);
    if (lwip_buf != drv_buf->p_mem_head) {
        oam_error_log0(0, 0, "[hwal_lwip_receive] skb_convert_2_pbuf, addr not match!");
        return;
    }
#else
    /* 申请LWIP协议栈处理内存 */
    // 32bit->16bit
    lwip_buf = pbuf_alloc(PBUF_RAW, (uint16_t)(OAL_NETBUF_LEN(drv_buf) + ETH_PAD_SIZE), PBUF_RAM);
    if (lwip_buf == NULL) {
        oam_error_log1(0, 0, "hwal_lwip_receive pbuf_alloc failed! len = %d", OAL_NETBUF_LEN(drv_buf));
        return;
    }

    /* 将payload地址往后偏移2字节 */
#if ETH_PAD_SIZE
    pbuf_header(lwip_buf, -ETH_PAD_SIZE);
#endif

    /* 将内存复制到LWIP协议栈处理内存 */
    if (memcpy_s(lwip_buf->payload, OAL_NETBUF_LEN(drv_buf), OAL_NETBUF_DATA(drv_buf),
                 OAL_NETBUF_LEN(drv_buf)) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hwal_lwip_receive::memcpy_s failed!");
        return;
    }
#endif
    /* 将payload地址前移2字节 */
#if ETH_PAD_SIZE
    pbuf_header(lwip_buf, ETH_PAD_SIZE);
#endif

    /* 上报协议栈 */
    driverif_input(netif, lwip_buf);
}

OAL_STATIC OAL_INLINE oal_netbuf_stru *hwal_lwip_converted_skb(oal_net_device_stru *net_dev, oal_lwip_buf *lwip_buf)
{
    oal_lwip_buf *buf_tmp = NULL;
    uint32_t lwip_buf_index = 0;
    uint32_t drv_buf_offset = 0;
    oal_netbuf_stru *converted_skb = NULL;

#if ETH_PAD_SIZE
    /* origin liteos */
    /* SDK_B050 */
    /* magic num to be modified by tangshuang */
    converted_skb = hwal_lwip_skb_alloc(net_dev, lwip_buf->tot_len + 32 - (ETHER_HDR_LEN - SNAP_LLC_FRAME_LEN));
    if (converted_skb == NULL) {
        oam_error_log0(0, 0, "hwal_lwip_skb_alloc NULL.");
        return NULL;
    }
    skb_reserve(converted_skb, 32 - (ETHER_HDR_LEN - SNAP_LLC_FRAME_LEN));
#else
    converted_skb = hwal_lwip_skb_alloc(net_dev, lwip_buf->tot_len);
    if (converted_skb == NULL) {
        oam_error_log0(0, 0, "hwal_lwip_skb_alloc NULL.");
        return NULL;
    }
#endif

    for (buf_tmp = lwip_buf; buf_tmp != NULL; buf_tmp = buf_tmp->next) {
        lwip_buf_index++;
        if (oal_netbuf_tailroom(converted_skb) < buf_tmp->len) {
            oam_error_log3(0, 0, "oal_netbuf_tailroom.tail = %d, len = %d, idx = %d",
                oal_netbuf_tailroom(converted_skb), buf_tmp->len, lwip_buf_index);
            oal_netbuf_free(converted_skb);
            return NULL;
        }

        oal_netbuf_put(converted_skb, buf_tmp->len);
        if (memcpy_s(OAL_NETBUF_DATA(converted_skb) + drv_buf_offset,
            OAL_NETBUF_LEN(converted_skb) - drv_buf_offset, buf_tmp->payload, buf_tmp->len) != EOK) {
            oam_warning_log0(0, OAM_SF_ANY, "hwal_lwip_converted_skb::memcpy_s failed!");
        }
        /* 将偏移量更新，处理pbuf链表下一个buf */
        drv_buf_offset += buf_tmp->len;
    }
    return converted_skb;
}
OAL_STATIC OAL_INLINE oal_netbuf_stru *hwal_lwip_alloc_skb(oal_net_device_stru *net_dev, oal_lwip_buf *lwip_buf)
{
    oal_netbuf_stru *converted_skb = NULL;
#ifdef _PRE_LWIP_ZERO_COPY
    /* HCC发送成功之前,Lwip重传包不做再次下放,直接返回,避免对内存重复操作 */
    if (lwip_buf->ref >= 2) {
        return NULL;
    }

    converted_skb = hwal_skb_struct_alloc();
    if (converted_skb == NULL) {
        oam_error_log0(0, 0, "[hwal_lwip_send] skb_buff alloc fail!");
        return NULL;
    }

    if (hwal_pbuf_convert_2_skb(lwip_buf, converted_skb) != OAL_SUCC) {
        oam_error_log0(0, 0, "[hwal_lwip_send] pbuf convert 2 skb fail!");
        oal_netbuf_free(converted_skb);
        return NULL;
    }
#else
    converted_skb = hwal_lwip_converted_skb(net_dev, lwip_buf);
    if (converted_skb == NULL) {
        oam_error_log0(0, 0, "[hwal_lwip_send] skb_buff alloc fail!");
        return NULL;
    }
#endif
    return converted_skb;
}

void hwal_lwip_send(oal_lwip_netif *netif, oal_lwip_buf *lwip_buf)
{
    oal_netbuf_stru *skb = NULL;
    mac_vap_stru *vap = NULL;
    oal_net_device_stru *net_dev = NULL;

    if ((lwip_buf == NULL) || (netif == NULL)) {
        oam_error_log0(0, 0, "hwal_lwip_send parameter NULL.");
        return;
    }

    net_dev = (oal_net_device_stru *)netif->state;
    if ((net_dev == NULL) || (net_dev->netdev_ops == NULL) || (net_dev->netdev_ops->ndo_start_xmit == NULL)) {
        oam_error_log0(0, 0, "netif->state parameter NULL.");
        return;
    }

    /* 获取VAP结构体 */
    vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(net_dev);
    /* 如果VAP结构体不存在，则丢弃报文, hwal层进行一次判空处理，防止hostapd最初启动时的错误打印 */
    if (OAL_UNLIKELY(vap == NULL)) {
        oam_warning_log0(0, OAM_SF_TX, "{hwal_lwip_send::vap = NULL!}");
        return;
    }

    /* Flow_ctl */
    if ((oal_bool_enum)NETIF_IS_NOT_READY() == TRUE) { // pclint:bool value不能直接比较
        /* release thread, reduce packet loss rate */
        oal_msleep(1);

        if (mac_get_data_type_from_8023(lwip_buf->payload, MAC_NETBUFF_PAYLOAD_ETH,
                                        lwip_buf->tot_len) == MAC_DATA_DHCP) {
            oam_warning_log0(0, OAM_SF_TX, "[hwal_lwip_wifi_drv_send] dhcp drop to driver!");
        }
        return;
    }

    skb = hwal_lwip_alloc_skb(net_dev, lwip_buf);
    if (skb == NULL) {
        oam_error_log0(0, 0, "[hwal_lwip_send] skb_buff alloc fail!");
        return;
    }

    skb->queue_mapping = net_dev->netdev_ops->ndo_select_queue(net_dev, skb);
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    if (wal_get_txdata_thread_enable() == TRUE) {
        skb->dev = net_dev;
        if (hwal_txdata_netbuf_enqueue(skb) == OAL_SUCC) {
            hwal_txdata_sched();
        }
    } else {
#endif
        net_dev->netdev_ops->ndo_start_xmit(skb, net_dev);
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    }
#endif
    return;
}

/* 协议栈打开宏PF_PKT_SUPPORT，增加了该接口 */
void hwal_netif_drv_config(struct netif *netif, uint32_t config_flags, uint8_t setBit)
{
    return;
}


int32_t hwal_lwip_init(oal_net_device_stru *net_dev)
{
    oal_ip4_addr_t st_gw;
    oal_ip4_addr_t st_ipaddr;
    oal_ip4_addr_t st_netmask;
    oal_lwip_netif *netif = NULL;

    if (net_dev == NULL) {
        oam_error_log0(0, 0, "hwal_lwip_init parameter NULL.");
        return OAL_FAIL;
    }

    if (oal_strcmp(net_dev->name, "wlan0") != 0) {
        return OAL_SUCC;
    }
    /* 初始化skb list */
    oal_netbuf_head_init(&net_dev->hisi_eapol.st_eapol_skb_head);

    netif = oal_memalloc(sizeof(oal_lwip_netif));
    if (netif == NULL) {
        oam_error_log0(0, 0, "hwal_lwip_init failed mem alloc NULL.");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    memset_s(netif, sizeof(oal_lwip_netif), 0, sizeof(oal_lwip_netif));

    OAL_IP4_ADDR(&st_gw, 0, 0, 0, 0);
    OAL_IP4_ADDR(&st_ipaddr, 0, 0, 0, 0);
    OAL_IP4_ADDR(&st_netmask, 0, 0, 0, 0);

    /* 网络设备注册 */
    netif->state = (void *)net_dev;
    netif->drv_send = hwal_lwip_send;
    netif->link_layer_type = WIFI_DRIVER_IF;
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    /* 协议栈打开宏PF_PKT_SUPPORT，增加了该接口 */
    netif->drv_config = hwal_netif_drv_config;

    if (netifapi_netif_add(netif, &st_ipaddr, &st_netmask, &st_gw) != 0) {
        oal_free(netif);

        oam_error_log0(0, 0, "hwal_lwip_init failed netif_add NULL.");
        return OAL_FAIL;
    }

    if (memcpy_s(netif->hwaddr, ETHARP_HWADDR_LEN, net_dev->dev_addr, ETHER_ADDR_LEN) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hwal_lwip_init::memcpy_s failed!");
        oal_free(netif);
        return OAL_FAIL;
    }
    net_dev->lwip_netif = netif;

    return OAL_SUCC;
}


void hwal_lwip_deinit(oal_net_device_stru *net_dev)
{
    if (net_dev == NULL) {
        return;
    }

    if (net_dev->lwip_netif != NULL) {
        oal_free(net_dev->lwip_netif);
    }

    net_dev->lwip_netif = NULL;

    return;
}


int32_t hwal_lwip_notify(oal_lwip_netif *netif, uint32_t notify_type)
{
    oal_net_device_stru *net_dev = NULL;
    oal_net_notify_stru notify_stru = {0};
    int32_t ret = OAL_FAIL;

    if (netif == NULL) {
        oam_error_log0(0, 0, "hwal_lwip_notify: netif NULL.");
        return -OAL_EFAIL;
    }

    net_dev = (oal_net_device_stru *)netif->state;
    if (net_dev == NULL) {
        oam_error_log0(0, 0, "hwal_lwip_notify: net_dev NULL.");
        return -OAL_EFAIL;
    }

    notify_stru.ul_ip_addr = *(uint32_t *)(&netif->ip_addr);
    notify_stru.ul_notify_type = notify_type;

    if ((net_dev->netdev_ops != NULL) &&
        (net_dev->netdev_ops->ndo_netif_notify != NULL)) {
        ret = net_dev->netdev_ops->ndo_netif_notify(net_dev, &notify_stru);

        oam_warning_log4(0, 0,
            "{hwal_lwip_notify::net_dev[0x%08X],ndo_netif_notify[0x%08X],ul_notify_type[0x%08X],ret[%d].}\n",
            net_dev, net_dev->netdev_ops->ndo_netif_notify, ul_notify_type, ret);
    }

    return ret;
}


void hwal_tcpip_init(void)
{
    tcpip_init(NULL, NULL);
}


int32_t hwal_dhcp_start(oal_net_device_stru *netdev)
{
    int32_t ret;
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_dhcp_start netdev is null.");
        return -HISI_EFAIL;
    }
    oal_netif_set_up(netdev->lwip_netif);
    ret = oal_dhcp_start(netdev->lwip_netif);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, 0, "dhcp_start fail %d.", ret);
    }
    return ret;
}


int32_t hwal_dhcp_stop(oal_net_device_stru *netdev)
{
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_dhcp_stop netdev is null.");
        return -HISI_EFAIL;
    }
    oal_dhcp_stop(netdev->lwip_netif);
    oal_netif_set_down(netdev->lwip_netif);
    return HISI_SUCC;
}


int32_t hwal_dhcp_succ_check(oal_net_device_stru *netdev)
{
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_dhcp_succ_check netdev is null.");
        return -HISI_EFAIL;
    }

    return oal_dhcp_succ_check(netdev->lwip_netif);
}


oal_lwip_netif *hwal_lwip_get_netif(const int8_t *pc_name)
{
    oal_net_device_stru *net_dev;

    net_dev = oal_dev_get_by_name(pc_name);
    if (net_dev == NULL) {
        return NULL;
    }

    return net_dev->lwip_netif;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

