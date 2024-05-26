

/* 头文件包含 */
#include "oam_main.h"
#include "oam_statistics.h"
#include "securec.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAM_STATISTICS_C

/* 函数实现 */
void oam_stats_report_irq_info_to_sdt(uint8_t *puc_irq_info_addr, uint16_t us_irq_info_len)
{
    uint32_t ul_tick;
    uint16_t us_skb_len; /* skb总长度 */
    oal_netbuf_stru *pst_netbuf = NULL;
    oam_ota_stru *pst_ota_data = NULL;

    if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
        return;
    }

    if (puc_irq_info_addr == NULL) {
        oal_io_print("oam_stats_report_irq_info_to_sdt::puc_irq_info_addr is null!\n");
        return;
    }

    /* 为上报描述符申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_irq_info_len + sizeof(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_irq_info_len = WLAN_SDT_NETBUF_MAX_PAYLOAD - sizeof(oam_ota_hdr_stru);
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == NULL) {
        return;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (uint32_t)oal_time_get_stamp_ms();

    /* 填写ota消息头结构体 */
    pst_ota_data->st_ota_hdr.ul_tick = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_IRQ;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_irq_info_len;
    pst_ota_data->st_ota_hdr.auc_resv[0] = OAM_OTA_TYPE_1103_HOST;

    /* 复制数据,填写ota数据 */
    if (memcpy_s((void *)pst_ota_data->auc_ota_data,
                 (uint32_t)pst_ota_data->st_ota_hdr.us_ota_data_len,
                 (const void *)puc_irq_info_addr,
                 (uint32_t)us_irq_info_len) != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        oal_io_print("oam_stats_report_irq_info_to_sdt::memcpy_s failed.\n");
        return;
    }
    /* 下发至sdt接收队列，若队列满则串口输出 */
    oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);
}

/*
 * 函 数 名  : oam_stats_report_timer_info_to_sdt
 * 功能描述  : 将软件定时器的信息上报SDT
 * 输入参数  : puc_timer_addr:定时器结构的地址
 *             uc_timer_len  :定时器结构的长度
 */
uint32_t oam_stats_report_timer_info_to_sdt(uint8_t *puc_timer_addr, uint8_t uc_timer_len)
{
    uint32_t ul_ret = OAL_SUCC;

    if (puc_timer_addr != NULL) {
        ul_ret = oam_ota_report(puc_timer_addr, uc_timer_len, 0, 0, OAM_OTA_TYPE_TIMER);
        return ul_ret;
    } else {
        oal_io_print("oam_stats_report_timer_info_to_sdt::puc_timer_addr is NULL");
        return OAL_ERR_CODE_PTR_NULL;
    }
}

/*
 * 函 数 名  : oam_stats_report_mempool_info_to_sdt
 * 功能描述  : 将内存池的某一个子池内存块的使用情况上报sdt
 * 输入参数  : uc_pool_id            :内存池id
 *             us_pool_total_cnt     :本内存池一共多少内存块
 *             us_pool_used_cnt      :本内存池已用内存块
 *             uc_subpool_id         :子池id
 *             us_subpool_total_cnt  :本子池内存块总数
 *             us_subpool_free_cnt   :本子池可用内存块个数
 */
uint32_t oam_stats_report_mempool_info_to_sdt(uint8_t uc_pool_id,
                                              uint16_t us_pool_total_cnt,
                                              uint16_t us_pool_used_cnt,
                                              uint8_t uc_subpool_id,
                                              uint16_t us_subpool_total_cnt,
                                              uint16_t us_subpool_free_cnt)
{
    oam_stats_mempool_stru st_device_mempool_info;
    oal_netbuf_stru *pst_netbuf = NULL;
    oam_ota_stru *pst_ota_data = NULL;
    uint16_t us_skb_len; /* skb总长度 */
    uint32_t ul_tick;
    uint32_t ul_ret;
    uint16_t us_stru_len;

    if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写要上报给sdt的内存池信息结构体 */
    st_device_mempool_info.uc_mem_pool_id = uc_pool_id;
    st_device_mempool_info.uc_subpool_id = uc_subpool_id;
    st_device_mempool_info.auc_resv[0] = 0;
    st_device_mempool_info.auc_resv[1] = 0;
    st_device_mempool_info.us_mem_pool_total_cnt = us_pool_total_cnt;
    st_device_mempool_info.us_mem_pool_used_cnt = us_pool_used_cnt;
    st_device_mempool_info.us_subpool_total_cnt = us_subpool_total_cnt;
    st_device_mempool_info.us_subpool_free_cnt = us_subpool_free_cnt;

    us_stru_len = sizeof(oam_stats_mempool_stru);
    /* 为ota消息上报SDT申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_stru_len + sizeof(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_stru_len = us_skb_len - sizeof(oam_ota_hdr_stru);
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (uint32_t)oal_time_get_stamp_ms();

    /* 填写ota消息结构体 */
    pst_ota_data->st_ota_hdr.ul_tick = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_MEMPOOL;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_stru_len;

    /* 复制数据,填写ota数据 */
    if (memcpy_s((void *)pst_ota_data->auc_ota_data,
                 (uint32_t)pst_ota_data->st_ota_hdr.us_ota_data_len,
                 (const void *)&st_device_mempool_info,
                 (uint32_t)us_stru_len) != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        oal_io_print("oam_stats_report_mempool_info_to_sdt::memcpy_s failed.\n");
        return OAL_FAIL;
    }
    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}

/*
 * 函 数 名  : oam_stats_report_memblock_info_to_sdt
 * 功能描述  : 将标准内存块的信息上报SDT
 * 输入参数  : puc_origin_data:内存块的起始地址
 *             uc_user_cnt    :该内存块引用计数
 *             uc_pool_id     :所属的内存池id
 *             uc_subpool_id  :所属的子池id
 *             us_len         :该内存块长度
 *             ul_file_id     :申请该内存块的文件id
 *             ul_alloc_line_num :申请该内存块的行号
 */
uint32_t oam_stats_report_memblock_info_to_sdt(uint8_t *puc_origin_data,
                                               uint8_t uc_user_cnt,
                                               uint8_t uc_pool_id,
                                               uint8_t uc_subpool_id,
                                               uint16_t us_len,
                                               uint32_t ul_file_id,
                                               uint32_t ul_alloc_line_num)
{
    oam_memblock_info_stru st_memblock_info;
    uint16_t us_memblock_info_len;
    oal_netbuf_stru *pst_netbuf = NULL;
    oam_ota_stru *pst_ota_data = NULL;
    uint16_t us_skb_len; /* skb总长度 */
    uint32_t ul_tick;
    uint32_t ul_ret;

    if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (puc_origin_data == NULL) {
        oal_io_print("oam_stats_report_memblock_info_to_sdt:puc_origin_data is null!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    us_memblock_info_len = sizeof(oam_memblock_info_stru);

    /* 填写要上报给sdt的内存块信息结构体 */
    st_memblock_info.uc_pool_id = uc_pool_id;
    st_memblock_info.uc_subpool_id = uc_subpool_id;
    st_memblock_info.uc_user_cnt = uc_user_cnt;
    st_memblock_info.auc_resv[0] = 0;
    st_memblock_info.ul_alloc_line_num = ul_alloc_line_num;
    st_memblock_info.ul_file_id = ul_file_id;

    /* 为ota消息上报SDT申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_memblock_info_len + us_len + sizeof(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        if ((us_memblock_info_len + sizeof(oam_ota_hdr_stru)) < us_skb_len) {
            us_len = us_skb_len - us_memblock_info_len - (uint16_t)sizeof(oam_ota_hdr_stru);
        } else {
            us_memblock_info_len = us_skb_len - sizeof(oam_ota_hdr_stru);
            us_len = 0;
        }
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (uint32_t)oal_time_get_stamp_ms();

    /* 填写ota消息结构体 */
    pst_ota_data->st_ota_hdr.ul_tick = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_MEMBLOCK;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = (uint8_t)us_memblock_info_len;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_memblock_info_len + us_len;

    /* 填写ota数据部分,首先复制内存块的信息结构体 */
    if (memcpy_s((void *)pst_ota_data->auc_ota_data,
                 (uint32_t)pst_ota_data->st_ota_hdr.us_ota_data_len,
                 (const void *)&st_memblock_info,
                 (uint32_t)us_memblock_info_len) != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        oal_io_print("oam_stats_report_memblock_info_to_sdt:: memcpy_s failed\r\n");
        return OAL_FAIL;
    }

    /* 复制内存块的具体内容 */ /*lint -e416*/
    if (memcpy_s((void *)(pst_ota_data->auc_ota_data + us_memblock_info_len),
                 (uint32_t)(pst_ota_data->st_ota_hdr.us_ota_data_len - us_memblock_info_len),
                 (const void *)puc_origin_data,
                 (uint32_t)us_len) != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        oal_io_print("oam_stats_report_memblock_info_to_sdt:: memcpy_s failed\r\n");
        return OAL_FAIL;
    }

    /*lint +e416*/
    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}

/*
 * 函 数 名  : oam_stats_report_event_queue_info_to_sdt
 * 功能描述  : 将事件队列中每个事件的事件头信息上报SDT
 * 输入参数  : puc_event_queue_addr:事件队列信息地址
 *             uc_event_queue_info_len:事件队列信息长度
 */
uint32_t oam_stats_report_event_queue_info_to_sdt(uint8_t *puc_event_queue_addr,
                                                  uint16_t us_event_queue_info_len)
{
    uint32_t ul_tick;
    uint16_t us_skb_len; /* skb总长度 */
    oal_netbuf_stru *pst_netbuf = NULL;
    oam_ota_stru *pst_ota_data = NULL;
    uint32_t ul_ret;

    if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (puc_event_queue_addr == NULL) {
        oal_io_print("oam_stats_report_event_queue_info_to_sdt::puc_event_queue_addr is null!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 为上报描述符申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_event_queue_info_len + sizeof(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_event_queue_info_len = WLAN_SDT_NETBUF_MAX_PAYLOAD - sizeof(oam_ota_hdr_stru);
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (uint32_t)oal_time_get_stamp_ms();

    /* 填写ota消息头结构体 */
    pst_ota_data->st_ota_hdr.ul_tick = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_EVENT_QUEUE;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_event_queue_info_len;

    /* 复制数据,填写ota数据 */
    if (memcpy_s((void *)pst_ota_data->auc_ota_data,
                 (uint32_t)pst_ota_data->st_ota_hdr.us_ota_data_len,
                 (const void *)puc_event_queue_addr,
                 (uint32_t)us_event_queue_info_len) != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        oal_io_print("oam_stats_report_event_queue_info_to_sdt:: memcpy_s failed\r\n");
        return OAL_FAIL;
    }

    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}

uint32_t oam_statistics_init(void)
{
#if ((_PRE_OS_VERSION_WIN32_RAW != _PRE_OS_VERSION))
    oal_mempool_info_to_sdt_register(oam_stats_report_mempool_info_to_sdt, oam_stats_report_memblock_info_to_sdt);
#endif
    return OAL_SUCC;
}

oal_module_symbol(oam_stats_report_irq_info_to_sdt);
oal_module_symbol(oam_stats_report_timer_info_to_sdt);
oal_module_symbol(oam_stats_report_mempool_info_to_sdt);
oal_module_symbol(oam_stats_report_memblock_info_to_sdt);
oal_module_symbol(oam_stats_report_event_queue_info_to_sdt);
