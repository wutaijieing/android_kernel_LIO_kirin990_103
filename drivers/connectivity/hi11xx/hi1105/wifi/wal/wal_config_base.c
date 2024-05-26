

/* 1 头文件包含 */
#include "wal_config_base.h"
#include "wal_ext_if.h"
#include "wal_config.h"
#include "wal_main.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_CONFIG_BASE_C

OAL_STATIC uint32_t wal_config_process_pkt_msg_type(mac_vap_stru *mac_vap,
    wal_msg_stru *msg, wal_msg_stru *rsp_msg, uint8_t *rsp_len)
{
    uint32_t ret;
    /* 取配置消息的长度 */
    uint16_t msg_len = msg->st_msg_hdr.us_msg_len;
    uint8_t *msg_data = &msg->auc_msg_data[0];
    uint8_t *rsp_msg_data = &rsp_msg->auc_msg_data[0];
    switch (msg->st_msg_hdr.en_msg_type) {
        case WAL_MSG_TYPE_QUERY:
            ret = wal_config_process_query(mac_vap, msg_data, msg_len, rsp_msg_data, rsp_len);
            if (ret != OAL_SUCC) {
                oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_ANY,
                    "{wal_config_process_pkt_msg_type_handle::wal_config_process_query return error code %d!}\r\n",
                    ret);
                return ret;
            }
            break;

        case WAL_MSG_TYPE_WRITE:
            ret = wal_config_process_write(mac_vap, msg_data, msg_len, rsp_msg_data, rsp_len);
            if (ret != OAL_SUCC) {
                oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_ANY,
                    "{wal_config_process_pkt_msg_type_handle::wal_config_process_write return error code %d!}\r\n",
                    ret);
                return ret;
            }
            break;

        default:
            oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_ANY,
                "{wal_config_process_pkt_msg_type_handle::msg->st_msg_hdr.en_msg_type error, msg_type is %d!}\r\n",
                msg->st_msg_hdr.en_msg_type);

            return OAL_ERR_CODE_INVALID_CONFIG;
    }
    return OAL_SUCC;
}

OAL_STATIC uint32_t wal_config_process_pkt_rsp_msg(wal_msg_stru *msg, uint8_t *rsp_msg,
    uintptr_t request_address, uint8_t rsp_len)
{
    wal_msg_stru *wal_rsp_msg = (wal_msg_stru *)rsp_msg;
    /* response 长度要包含头长 */
    uint8_t rsp_toal_len = rsp_len + sizeof(wal_msg_hdr_stru);
    if (oal_unlikely(rsp_toal_len > HMAC_RSP_MSG_MAX_LEN)) {
        oam_error_log1(0, OAM_SF_ANY,
                       "{wal_config_process_pkt_rsp_msg_handle::invaild response len %u!}\r\n",
                       rsp_toal_len);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    /* 填充返回消息头 */
    wal_rsp_msg->st_msg_hdr.en_msg_type = WAL_MSG_TYPE_RESPONSE;
    wal_rsp_msg->st_msg_hdr.uc_msg_sn = msg->st_msg_hdr.uc_msg_sn;
    wal_rsp_msg->st_msg_hdr.us_msg_len = rsp_len;

    if (request_address) {
        /* need response */
        uint8_t *rsp_msg_tmp = oal_memalloc(rsp_toal_len);
        if (rsp_msg_tmp == NULL) {
            /* no mem */
            oam_error_log1(0, OAM_SF_ANY,
                           "{wal_config_process_pkt_rsp_msg_handle::wal_config_process_pkt msg alloc %u failed!",
                           rsp_toal_len);
            wal_set_msg_response_by_addr(request_address, NULL, OAL_ERR_CODE_PTR_NULL, rsp_toal_len);
        } else {
            if (memcpy_s((void *)rsp_msg_tmp, rsp_toal_len, (void *)rsp_msg, rsp_toal_len) != EOK) {
                oam_error_log0(0, OAM_SF_ANY, "{wal_config_process_pkt_rsp_msg::memcpy msg_tmp fail!");
            }
            if (OAL_SUCC != wal_set_msg_response_by_addr(request_address, (void *)rsp_msg_tmp,
                OAL_SUCC, rsp_toal_len)) {
                oam_error_log0(0, OAM_SF_ANY,
                    "{wal_config_process_pkt_rsp_msg_handle::wal_config_process_pkt did't found the request msg!");
                oal_free(rsp_msg_tmp);
            }
        }
    }
    return OAL_SUCC;
}


uint32_t wal_config_process_pkt(frw_event_mem_stru *event_mem)
{
    frw_event_stru *event = NULL;
    wal_msg_stru *msg = NULL;
    wal_msg_stru *rsp_msg = NULL;
    frw_event_hdr_stru *event_hdr = NULL;
    mac_vap_stru *mac_vap = NULL;
    uint8_t uc_rsp_len = 0;
    uint32_t ret;
    uintptr_t request_address;
    uint8_t ac_rsp_msg[HMAC_RSP_MSG_MAX_LEN] = { 0 };

    if (oal_unlikely(event_mem == NULL)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_config_process_pkt::event_mem null ptr error!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    event = frw_get_event_stru(event_mem);
    event_hdr = &(event->st_event_hdr);
    request_address = ((wal_msg_rep_hdr *)event->auc_event_data)->request_address;
    msg = (wal_msg_stru *)(frw_get_event_payload(event_mem) + sizeof(wal_msg_rep_hdr));

    mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(event_hdr->uc_vap_id);
    if (oal_unlikely(mac_vap == NULL)) {
        oam_warning_log0(event_hdr->uc_vap_id, OAM_SF_ANY,
                         "{wal_config_process_pkt::hmac_get_vap_by_id return err code!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 取返回消息 */
    rsp_msg = (wal_msg_stru *)ac_rsp_msg;

    oam_info_log0(event_hdr->uc_vap_id, OAM_SF_ANY, "{wal_config_process_pkt::a config event occur!}\r\n");

    ret = wal_config_process_pkt_msg_type(mac_vap, msg, rsp_msg, &uc_rsp_len);
    if (ret != OAL_SUCC) {
        return ret;
    }
    ret = wal_config_process_pkt_rsp_msg(msg, ac_rsp_msg, request_address, uc_rsp_len);
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* 唤醒WAL等待的进程 */
    wal_cfg_msg_task_sched();

    return OAL_SUCC;
}

OAL_STATIC int32_t wal_process_recv_config_cmd_event(mac_vap_stru *mac_vap, uint8_t *buf,
    wal_msg_request_stru *st_msg_request, uint16_t len)
{
    uint32_t ret;
    uint16_t need_response = OAL_FALSE;
    wal_msg_rep_hdr *rep_hdr = NULL;
    wal_msg_stru *msg = NULL;
    frw_event_stru *event = NULL;
    frw_event_mem_stru *event_mem = NULL;

    /* 申请内存 */
    event_mem = frw_event_alloc_m(len + sizeof(wal_msg_rep_hdr));
    if (oal_unlikely(event_mem == NULL)) {
        oam_error_log1(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_config_cmd::request %d mem failed}\r\n", len);
        return OAL_ERR_CODE_PTR_NULL;
    }

    event = frw_get_event_stru(event_mem);

    /* 填写事件头 */
    frw_event_hdr_init(&(event->st_event_hdr), FRW_EVENT_TYPE_HOST_CRX, WAL_HOST_CRX_SUBTYPE_CFG, len,
        FRW_EVENT_PIPELINE_STAGE_0, mac_vap->uc_chip_id, mac_vap->uc_device_id, mac_vap->uc_vap_id);

    /* 填写事件payload */
    if (memcpy_s(frw_get_event_payload(event_mem) + sizeof(wal_msg_rep_hdr), len, buf + OAL_IF_NAME_SIZE, len) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "wal_recv_config_cmd1::memcpy fail!");
        return OAL_FAIL;
    }
    msg = (wal_msg_stru *)(buf + OAL_IF_NAME_SIZE);
    rep_hdr = (wal_msg_rep_hdr *)event->auc_event_data;

    WAL_RECV_CMD_NEED_RESP(msg, need_response);

    if (need_response == OAL_TRUE) {
        rep_hdr->request_address = (uintptr_t)st_msg_request;
        wal_msg_request_add_queue(st_msg_request);
    } else {
        rep_hdr->request_address = 0;
    }

    ret = wal_config_process_pkt(event_mem);

    if (need_response == OAL_TRUE) {
        wal_msg_request_remove_queue(st_msg_request);
    }

    /* 释放内存 */
    frw_event_free_m(event_mem);
    return (int32_t)ret;
}

OAL_STATIC int32_t wal_report_data2sdt(mac_vap_stru *mac_vap, wal_msg_request_stru *st_msg_request)
{
    oal_netbuf_stru *netbuf = NULL;
    uint16_t msg_len; /* 传给sdt的skb数据区不包括头尾空间的长度 */
    wal_msg_stru *rsp_msg = NULL;

    if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (st_msg_request->pst_resp_mem == NULL) {
        oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_config_cmd::get response ptr failed!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    rsp_msg = (wal_msg_stru *)st_msg_request->pst_resp_mem;

    msg_len = rsp_msg->st_msg_hdr.us_msg_len + 1; /* +1是sdt工具的需要 */

    msg_len = (msg_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) ? WLAN_SDT_NETBUF_MAX_PAYLOAD : msg_len;

    netbuf = oam_alloc_data2sdt(msg_len);
    if (netbuf == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_netbuf_data(netbuf)[0] = 'M'; /* sdt需要 */
    if (memcpy_s(oal_netbuf_data(netbuf) + 1, msg_len - 1, (uint8_t *)rsp_msg->auc_msg_data, msg_len - 1) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "wal_recv_config_cmd::memcpy fail!");
    }

    oam_report_data2sdt(netbuf, OAM_DATA_TYPE_CFG, OAM_PRIMID_TYPE_DEV_ACK);
    return OAL_SUCC;
}


int32_t wal_recv_config_cmd(uint8_t *buf, uint16_t len)
{
    int8_t vap_name[OAL_IF_NAME_SIZE];
    oal_net_device_stru *net_dev = NULL;
    mac_vap_stru *mac_vap = NULL;
    int32_t ret;
    wal_msg_stru *msg = NULL;
    uint16_t msg_size = len;
    uint16_t need_response = OAL_FALSE;

    DECLARE_WAL_MSG_REQ_STRU(st_msg_request);
    WAL_MSG_REQ_STRU_INIT(st_msg_request);
    if (msg_size < OAL_IF_NAME_SIZE) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_recv_config_cmd::msg_size[%d] overrun!}", msg_size);
        return OAL_FAIL;
    }
    if (memcpy_s(vap_name, OAL_IF_NAME_SIZE, buf, OAL_IF_NAME_SIZE) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "wal_recv_config_cmd::memcpy fail!");
        return OAL_FAIL;
    }
    vap_name[OAL_IF_NAME_SIZE - 1] = '\0'; /* 防止字符串异常 */

    /* 根据dev_name找到dev */
    net_dev = wal_config_get_netdev(vap_name, OAL_STRLEN(vap_name));
    if (net_dev == NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_recv_config_cmd::wal_config_get_netdev return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_dev_put(net_dev); /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */

    mac_vap = oal_net_dev_priv(net_dev); /* 获取mac vap */
    if (oal_unlikely(mac_vap == NULL)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_recv_config_cmd::OAL_NET_DEV_PRIV(net_dev) is null ptr.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    msg_size -= OAL_IF_NAME_SIZE;

    /* 接收到配置cmd 进行处理 */
    ret = wal_process_recv_config_cmd_event(mac_vap, buf, &st_msg_request, msg_size);
    if (ret != OAL_SUCC) {
        WAL_MSG_REQ_RESP_MEM_FREE(st_msg_request);
        return ret;
    }

    msg = (wal_msg_stru *)(buf + OAL_IF_NAME_SIZE);
    WAL_RECV_CMD_NEED_RESP(msg, need_response);

    /* 如果是查询消息类型，结果上报 */
    if (need_response != OAL_TRUE) {
        return OAL_SUCC;
    }
    /* 上报数据给sdt */
    ret = wal_report_data2sdt(mac_vap, &st_msg_request);
    WAL_MSG_REQ_RESP_MEM_FREE(st_msg_request);
    return ret;
}

int32_t wal_recv_memory_cmd(uint8_t *puc_buf, uint16_t us_len)
{
    oal_netbuf_stru *pst_netbuf = NULL;
    wal_sdt_mem_frame_stru *pst_mem_frame;
    uintptr_t mem_addr;           /* 读取内存地址 */
    uint16_t us_mem_len;          /* 需要读取的长度 */
    uint8_t uc_offload_core_mode; /* offload下，表示哪一个核 */
    int32_t l_ret = EOK;

    pst_mem_frame = (wal_sdt_mem_frame_stru *)puc_buf;
    mem_addr = pst_mem_frame->addr;
    us_mem_len = pst_mem_frame->us_len;
    uc_offload_core_mode = pst_mem_frame->en_offload_core_mode;

    if (uc_offload_core_mode == WAL_OFFLOAD_CORE_MODE_DMAC) {
        /* 如果是offload情形，并且要读取的内存是wifi芯片侧，需要抛事件，后续开发 */
        return OAL_SUCC;
    }

    if (mem_addr == 0) { /* 读写地址不合理 */
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (us_mem_len > WAL_SDT_MEM_MAX_LEN) {
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    if (pst_mem_frame->uc_mode == MAC_SDT_MODE_READ) {
        if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_netbuf = oam_alloc_data2sdt(us_mem_len);
        if (pst_netbuf == NULL) {
            return OAL_ERR_CODE_PTR_NULL;
        }
        l_ret += memcpy_s(oal_netbuf_data(pst_netbuf), us_mem_len, (void *)mem_addr, us_mem_len);
        oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_MEM_RW, OAM_PRIMID_TYPE_DEV_ACK);
    } else if (pst_mem_frame->uc_mode == MAC_SDT_MODE_WRITE) {
        l_ret += memcpy_s((void *)mem_addr, us_mem_len, pst_mem_frame->auc_data, us_mem_len);
    }

    if (l_ret != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "wal_recv_memory_cmd::memcpy fail!");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

int32_t wal_parse_global_var_cmd(wal_sdt_global_var_stru *pst_global_frame, unsigned long global_var_addr)
{
    oal_netbuf_stru *pst_netbuf = NULL;
    uint16_t us_skb_len;
    int32_t l_ret = EOK;

    if (pst_global_frame->uc_mode == MAC_SDT_MODE_WRITE) {
        l_ret += memcpy_s((void *)(uintptr_t)global_var_addr, pst_global_frame->us_len,
                          (void *)(pst_global_frame->auc_global_value), pst_global_frame->us_len);
    } else if (pst_global_frame->uc_mode == MAC_SDT_MODE_READ) {
        if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        us_skb_len = pst_global_frame->us_len;

        us_skb_len = (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) ? WLAN_SDT_NETBUF_MAX_PAYLOAD : us_skb_len;

        pst_netbuf = oam_alloc_data2sdt(us_skb_len);
        if (pst_netbuf == NULL) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        l_ret += memcpy_s(oal_netbuf_data(pst_netbuf), us_skb_len,
                          (void *)(uintptr_t)global_var_addr, us_skb_len);
        oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_MEM_RW, OAM_PRIMID_TYPE_DEV_ACK);
    }

    if (l_ret != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "wal_parse_global_var_cmd::memcpy fail!");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

int32_t wal_recv_global_var_cmd(uint8_t *puc_buf, uint16_t us_len)
{
    wal_sdt_global_var_stru *pst_global_frame = NULL;
    unsigned long global_var_addr;

    if (puc_buf == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_global_frame = (wal_sdt_global_var_stru *)puc_buf;

    if (pst_global_frame->en_offload_core_mode == WAL_OFFLOAD_CORE_MODE_DMAC) {
        /* offload情形，并且要读取的全局变量在wifi芯片侧，需要抛事件，后续开发 */
        return OAL_SUCC;
    }

    global_var_addr = oal_kallsyms_lookup_name(pst_global_frame->auc_global_value_name);
    if (global_var_addr == 0) { /* not found */
        oam_warning_log0(0, OAM_SF_ANY,
                         "{wal_recv_global_var_cmd::kernel lookup global var address returns 0!}\r\n");
        return OAL_FAIL;
    }

    return wal_parse_global_var_cmd(pst_global_frame, global_var_addr);
}

static int32_t wal_report_reg_data(mac_vap_stru *mac_vap, uint8_t *buf, hmac_vap_cfg_priv_stru *cfg_priv)
{
    int32_t ret;
    oal_netbuf_stru *net_buf = NULL;
    wal_sdt_reg_frame_stru *reg_frame = NULL;

    /* 获取读写模式: read 需要回填数据给SDT ; write 不需要 */
    reg_frame = (wal_sdt_reg_frame_stru *)buf;

    /* 需要读取数据时，反馈数据至SDT */
    if (reg_frame->uc_mode == MAC_SDT_MODE_READ || reg_frame->uc_mode == MAC_SDT_MODE_READ16) {
        wal_wake_lock();
        /*lint -e730*/ /* info, boolean argument to function */
#ifndef _PRE_WINDOWS_SUPPORT
        ret = oal_wait_event_interruptible_timeout_m(cfg_priv->st_wait_queue_for_sdt_reg,
            cfg_priv->en_wait_ack_for_sdt_reg, (2 * OAL_TIME_HZ)); /* 2 超时时间 */
#else
        oal_wait_event_interruptible_timeout_m(&cfg_priv->st_wait_queue_for_sdt_reg,
            cfg_priv->en_wait_ack_for_sdt_reg, (2 * OAL_TIME_HZ), ret); /* 2 超时时间 */
#endif
        /*lint +e730*/
        if (ret == 0) {
            /* 超时 */
            oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd:: wait queue timeout!}\r\n");
            wal_wake_unlock();
            return -OAL_EINVAL;
        } else if (ret < 0) {
            /* 异常 */
            oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd:: wait queue error!}\r\n");
            wal_wake_unlock();
            return -OAL_EINVAL;
        }
        wal_wake_unlock();
        /*lint +e774*/
        /* 读取返回的寄存器值 */
        reg_frame->reg_val = *((uint32_t *)(cfg_priv->ac_rsp_msg));

        if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func != NULL)) {
            net_buf = oam_alloc_data2sdt((uint16_t)sizeof(wal_sdt_reg_frame_stru));
            if (net_buf == NULL) {
                return OAL_ERR_CODE_PTR_NULL;
            }

            if (memcpy_s(oal_netbuf_data(net_buf), (uint16_t)sizeof(wal_sdt_reg_frame_stru), (uint8_t *)reg_frame,
                         (uint16_t)sizeof(wal_sdt_reg_frame_stru)) != EOK) {
                oam_error_log0(0, OAM_SF_ANY, "wal_recv_reg_cmd::memcpy fail!");
                return OAL_FAIL;
            }
            oam_report_data2sdt(net_buf, OAM_DATA_TYPE_REG_RW, OAM_PRIMID_TYPE_DEV_ACK);
        }
    }
    return OAL_SUCC;
}


int32_t wal_recv_reg_cmd(uint8_t *buf, uint16_t len)
{
    int8_t vap_name[OAL_IF_NAME_SIZE];
    oal_net_device_stru *net_dev = NULL;
    mac_vap_stru *mac_vap = NULL;
    hmac_vap_cfg_priv_stru *cfg_priv = NULL;
    uint32_t ret;

    if (len < sizeof(wal_sdt_reg_frame_stru)) {
        return -OAL_EINVAL;
    }
    if (memcpy_s(vap_name, OAL_IF_NAME_SIZE, buf, OAL_IF_NAME_SIZE) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "wal_recv_reg_cmd::memcpy fail!");
        return OAL_FAIL;
    }
    vap_name[OAL_IF_NAME_SIZE - 1] = '\0'; /* 防止字符串异常 */

    /* 根据dev_name找到dev */
    net_dev = wal_config_get_netdev(vap_name, OAL_STRLEN(vap_name));
    if (net_dev == NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_recv_reg_cmd::wal_config_get_netdev return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_dev_put(net_dev); /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */

    mac_vap = oal_net_dev_priv(net_dev); /* 获取mac vap */

    ret = hmac_vap_get_priv_cfg(mac_vap, &cfg_priv); /* 取配置私有结构体 */
    if (ret != OAL_SUCC) {
        oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd:: return null_ptr_err!}\r\n");
        return ret;
    }

    /* 等待事件发生的条件为收到信号 */
    cfg_priv->en_wait_ack_for_sdt_reg = OAL_FALSE;

    /* 抛事件到dmac */
    ret = hmac_sdt_recv_reg_cmd(mac_vap, buf, len);
    if (ret != OAL_SUCC) {
        oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd:: return error code!}\r\n");
        return ret;
    }

    return wal_report_reg_data(mac_vap, buf, cfg_priv);
}
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)

int32_t wal_recv_sample_cmd(uint8_t *puc_buf, uint16_t us_len)
{
    int8_t ac_vap_name[OAL_IF_NAME_SIZE];
    oal_net_device_stru *pst_net_dev;
    mac_vap_stru *pst_mac_vap;

    if (us_len < OAL_IF_NAME_SIZE) {
        return OAL_FAIL;
    }
    if (memcpy_s(ac_vap_name, OAL_IF_NAME_SIZE, puc_buf, OAL_IF_NAME_SIZE) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "wal_recv_sample_cmd::memcpy fail!");
        return OAL_FAIL;
    }
    ac_vap_name[OAL_IF_NAME_SIZE - 1] = '\0'; /* 防止字符串异常 */

    /* 根据dev_name找到dev */
    pst_net_dev = wal_config_get_netdev(ac_vap_name, OAL_STRLEN(ac_vap_name));
    if (pst_net_dev == NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_recv_sample_cmd::wal_config_get_netdev return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_dev_put(pst_net_dev); /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
    pst_mac_vap = oal_net_dev_priv(pst_net_dev); /* 获取mac vap */

    hmac_sdt_recv_sample_cmd(pst_mac_vap, puc_buf, us_len);
    return OAL_SUCC;
}


uint32_t wal_sample_report2sdt(frw_event_mem_stru *pst_event_mem)
{
    uint16_t us_payload_len;
    oal_netbuf_stru *pst_net_buf;
    frw_event_stru *pst_event;

    if (pst_event_mem == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_sample_report2sdt::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    us_payload_len = pst_event->st_event_hdr.us_length - FRW_EVENT_HDR_LEN;

    pst_net_buf = oam_alloc_data2sdt(us_payload_len);
    if (pst_net_buf == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (memcpy_s(oal_netbuf_data(pst_net_buf), us_payload_len, pst_event->auc_event_data, us_payload_len) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "wal_sample_report2sdt::memcpy fail!");
        oal_mem_sdt_netbuf_free(pst_net_buf, OAL_TRUE);
        return OAL_FAIL;
    }

    oam_report_data2sdt(pst_net_buf, OAM_DATA_TYPE_SAMPLE, OAM_PRIMID_TYPE_DEV_ACK);
    return OAL_SUCC;
}
#endif

uint32_t wal_dpd_report2sdt(frw_event_mem_stru *pst_event_mem)
{
    uint16_t us_payload_len;
    oal_netbuf_stru *pst_net_buf = NULL;
    frw_event_stru *pst_event = NULL;

    if (pst_event_mem == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_sample_report2sdt::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    us_payload_len = pst_event->st_event_hdr.us_length - FRW_EVENT_HDR_LEN;

    pst_net_buf = oam_alloc_data2sdt(us_payload_len);
    if (pst_net_buf == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (memcpy_s(oal_netbuf_data(pst_net_buf), us_payload_len, pst_event->auc_event_data, us_payload_len) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "wal_dpd_report2sdt::memcpy fail!");
        oal_mem_sdt_netbuf_free(pst_net_buf, OAL_TRUE);
        return OAL_FAIL;
    }

    oam_report_data2sdt(pst_net_buf, OAM_DATA_TYPE_DPD, OAM_PRIMID_TYPE_DEV_ACK);
    return OAL_SUCC;
}

void wal_drv_cfg_func_hook_init(void)
{
    g_st_wal_drv_func_hook.p_wal_recv_cfg_data_func = wal_recv_config_cmd;
    g_st_wal_drv_func_hook.p_wal_recv_mem_data_func = wal_recv_memory_cmd;
    g_st_wal_drv_func_hook.p_wal_recv_reg_data_func = wal_recv_reg_cmd;
    g_st_wal_drv_func_hook.p_wal_recv_global_var_func = wal_recv_global_var_cmd;
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
    g_st_wal_drv_func_hook.p_wal_recv_sample_data_func = wal_recv_sample_cmd;
#endif
}
oal_bool_enum_uint8 wal_should_down_protocol_bw(mac_device_stru *mac_dev, mac_vap_stru *mac_vap)
{
    if ((mac_dev != NULL && mac_dev->st_p2p_info.p2p_scenes == MAC_P2P_SCENES_LOW_LATENCY)
#ifdef _PRE_WLAN_CHBA_MGMT
        || (mac_vap_is_find_chba_vap(mac_dev) == OAL_TRUE) /* 存在chba vap时，优先支持降协议 */
#endif
    ) {
        return OAL_TRUE;
    }
    return OAL_FALSE;
}