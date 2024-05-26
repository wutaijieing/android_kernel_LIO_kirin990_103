

/* 1 头文件包含 */
#include "hmac_chba_mgmt.h"
#include "hmac_chba_function.h"
#include "hmac_chba_sync.h"
#include "hmac_chba_ps.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_chan_switch.h"
#include "hmac_chba_coex.h"
#include "hmac_mgmt_sta.h"
#include "securec.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_MGMT_C

#ifdef _PRE_WLAN_CHBA_MGMT
/* 2 全局变量定义 */
/* CHBA本设备硬件信息 */
hmac_chba_device_para_stru g_chba_device_info = {
    .device_type = DEVICE_TYPE_HANDSET,
    .remain_power = BATTERY_LEVEL_MIDDLE,
    .hardwire_cap = HW_CAP_SINGLE_CHIP,
    .drv_band_cap = BAND_CAP_5G_ONLY,
    .config_para = { MAC_CHBA_VERSION_2, MAC_CHBA_SLOT_DURATION, MAC_CHBA_SCHEDULE_PERIOD, MAC_CHBA_SYNC_SLOT_CNT,
        MAC_CHBA_INIT_LISTEN_PERIOD, MAC_CHBA_VAP_ALIVE_TIMEOUT, MAC_CHBA_LINK_ALIVE_TIMEOUT },
};

/* CHBA管理信息 */
hmac_chba_mgmt_info_stru g_chba_mgmt_info = {
    .chba_module_state = MAC_CHBA_MODULE_STATE_UNINIT,
    .alive_device_table = NULL,
};

/* 3 函数声明 */
uint8_t hmac_chba_one_dev_update_alive_dev_table(const uint8_t* mac_addr);

/* 4 函数实现 */
void hmac_chba_print_island_info(hmac_chba_island_list_stru *island_info)
{
    uint32_t j;
    struct hi_node *node = NULL;
    struct hi_node *tmp = NULL;
    hmac_chba_tmp_island_list_node_stru *chba_node = NULL;
    hmac_chba_per_island_tmp_info_stru *curr_island = NULL;
    hmac_chba_alive_device_table_stru *ht = hmac_chba_get_alive_device_table();

    oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_print_island_info::island cnt %d, aliveDeviceCnt %u",
        island_info->island_cnt, ht->node_cnt);
    hi_list_for_each_safe(node, tmp, &island_info->island_list) {
        chba_node = hi_node_entry(node, hmac_chba_tmp_island_list_node_stru, node);
        curr_island = &chba_node->data;
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_print_island_info::island id %d", curr_island->island_id);
        for (j = 0; j < curr_island->device_cnt; j++) {
            oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_print_island_info::device %d ", curr_island->device_list[j]);
        }
    }
}

void hmac_chba_print_self_conn_info(void)
{
    uint32_t j;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    oam_warning_log3(0, OAM_SF_CHBA,
        "hmac_chba_print_self_conn_info::SelfConnInfo. device_id %d, role %d, ownerValid %d",
        self_conn_info->self_device_id, self_conn_info->role, self_conn_info->island_owner_valid);
    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_print_self_conn_info::SelfConnInfo. deviceCnt %d",
        self_conn_info->island_device_cnt);
    oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_print_self_conn_info::SelfConnInfo. ownerAddr XX:XX:XX:%2X:%2X:%2X",
        self_conn_info->island_owner_addr[3], self_conn_info->island_owner_addr[4],
        self_conn_info->island_owner_addr[5]);
    for (j = 0; j < self_conn_info->island_device_cnt; j++) {
        oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_print_self_conn_info::device addr XX:XX:XX:%2X:%2X:%2X",
            self_conn_info->island_device_list[j].mac_addr[3], self_conn_info->island_device_list[j].mac_addr[4],
            self_conn_info->island_device_list[j].mac_addr[5]);
    }
}


size_t hmac_chba_bkdr_hash(const uint8_t *str)
{
    size_t hash = 0;
    size_t ch;
    uint32_t loop;

    if (str == NULL) {
        return 0;
    }

    for (loop = 0; loop < OAL_MAC_ADDR_LEN; loop++) {
        ch = (size_t)(*str);
        hash = (hash << 7) + (hash << 1) + hash + ch;
        str++;
    }

    return hash;
}


static bool hmac_chba_node_equal(const struct hi_node *a, const struct hi_node *b)
{
    hmac_chba_hash_node_stru *na = NULL;
    hmac_chba_hash_node_stru *nb = NULL;
    if (oal_any_null_ptr2(a, b)) {
        return 0;
    }

    na = hi_node_entry(a, hmac_chba_hash_node_stru, node);
    nb = hi_node_entry(b, hmac_chba_hash_node_stru, node);
    return oal_memcmp(na->mac_addr, nb->mac_addr, OAL_MAC_ADDR_LEN) == 0;
}


static size_t hmac_chba_node_key(const struct hi_node *node, size_t bkt_size)
{
    hmac_chba_hash_node_stru *n = NULL;
    size_t k;
    if ((node == NULL) || (bkt_size == 0)) {
        return 0;
    }

    n = hi_node_entry(node, hmac_chba_hash_node_stru, node);
    k = hmac_chba_bkdr_hash(n->mac_addr);
    return k % bkt_size;
}


hmac_chba_alive_device_table_stru* hmac_chba_hash_creat(void)
{
    int ret;
    hmac_chba_alive_device_table_stru *chba_hash =
        (hmac_chba_alive_device_table_stru *)oal_memalloc(sizeof(hmac_chba_alive_device_table_stru));
    if (chba_hash == NULL) {
        return NULL;
    }
    chba_hash->node_cnt = 0;
    ret = hi_hash_init(&chba_hash->raw, MAC_CHBA_MAX_DEVICE_NUM, hmac_chba_node_equal, hmac_chba_node_key);
    if (ret != 0) {
        oal_free(chba_hash);
        return NULL;
    }

    return chba_hash;
}


void hmac_chba_hash_clear(hmac_chba_alive_device_table_stru *ht)
{
    size_t i;
    struct hi_node *node = NULL;
    struct hi_node *tmp = NULL;
    hmac_chba_hash_node_stru *chba_node = NULL;
    if (ht == NULL) {
        return;
    }

    for (i = 0; i < ht->raw.bkt_size; i++) {
        hi_list_for_each_safe(node, tmp, &ht->raw.bkts[i]) {
            hi_hash_remove(node);
            chba_node = hi_node_entry(node, hmac_chba_hash_node_stru, node);
            oal_free(chba_node);
        }
    }

    ht->node_cnt = 0;
}


void hmac_chba_tmp_island_list_node_free(struct hi_node *node)
{
    hmac_chba_tmp_island_list_node_stru *n = NULL;
    if (node == NULL) {
        return;
    }
    n = hi_node_entry(node, hmac_chba_tmp_island_list_node_stru, node);
    oal_free(n);
}


void hmac_chba_island_list_node_free(struct hi_node *node)
{
    hmac_chba_island_list_node_stru *n = NULL;
    if (node == NULL) {
        return;
    }
    n = hi_node_entry(node, hmac_chba_island_list_node_stru, node);
    oal_free(n);
}


uint8_t hmac_chba_alloc_device_id(void)
{
    uint32_t i;
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    for (i = 0; i < MAC_CHBA_MAX_DEVICE_NUM; i++) {
        if (device_id_info[i].use_flag == FALSE) {
            return i;
        }
    }

    return UINT8_INVALID_VALUE;
}


void hmac_chba_clear_one_dev_from_network_topo(uint8_t device_id)
{
    uint32_t row;
    uint32_t *network_topo_row = NULL;
    if (device_id >= MAC_CHBA_MAX_DEVICE_NUM) {
        return;
    }

    /* 连接拓扑图对应device_id的行清0 */
    network_topo_row = hmac_chba_get_network_topo_row(device_id);
    memset_s(network_topo_row, sizeof(uint32_t) * MAC_CHBA_MAX_BITMAP_WORD_NUM,
        0, sizeof(uint32_t) * MAC_CHBA_MAX_BITMAP_WORD_NUM);

    /* 连接拓扑图对应device_id的列清0 */
    for (row = 0; row < MAC_CHBA_MAX_DEVICE_NUM; row++) {
        hmac_chba_set_network_topo_ele_false(row, device_id);
    }
}


void hmac_chba_update_device_id_info(uint8_t device_id, uint8_t *mac_addr,
    uint8_t chan_number, uint8_t bandwidth)
{
    int32_t ret;
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();
    if (mac_addr == NULL) {
        return;
    }

    device_id_info[device_id].use_flag = TRUE;
    ret = memcpy_s(device_id_info[device_id].mac_addr, OAL_MAC_ADDR_LEN, mac_addr, OAL_MAC_ADDR_LEN);
    if (ret != 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_update_device_id_info: memcpy_s fail");
        return;
    }
    device_id_info[device_id].chan_number = chan_number;
    device_id_info[device_id].bandwidth = bandwidth;
}


uint8_t hmac_chba_get_root(uint8_t *parent, uint8_t i)
{
    if (parent == NULL) {
        return i;
    }

    /* 已到达根节点，返回 */
    if (parent[i] == i) {
        return i;
    }

    /* 否则获取父节点的根节点 */
    return hmac_chba_get_root(parent, parent[i]);
}


void hmac_chba_clear_island_info(hmac_chba_island_list_stru *island_info, hi_node_func node_deinit)
{
    if (island_info == NULL) {
        return;
    }

    /* 释放岛节点 */
    if (hmac_chba_get_module_state() >= MAC_CHBA_MODULE_STATE_INIT) {
        hi_list_deinit(&island_info->island_list, node_deinit);
    }

    /* 初始化岛信息链表 */
    hi_list_init(&island_info->island_list);
    island_info->island_cnt = 0;
}


void hmac_chba_add_island_to_list(hmac_chba_island_list_stru *island_info, uint8_t island_id,
    uint8_t device_cnt, uint8_t *device_list)
{
    int32_t ret;
    hmac_chba_tmp_island_list_node_stru *chba_node = NULL;
    hmac_chba_per_island_tmp_info_stru *curr_island = NULL;

    if (oal_any_null_ptr2(island_info, device_list)) {
        return;
    }

    chba_node = (hmac_chba_tmp_island_list_node_stru *)oal_memalloc(sizeof(hmac_chba_tmp_island_list_node_stru));
    if (chba_node == NULL) {
        return;
    }

    /* 填写岛id, 岛内设备数，岛内设备列表 */
    curr_island = &chba_node->data;
    curr_island->island_id = island_id;
    curr_island->device_cnt = device_cnt;
    ret = memcpy_s(curr_island->device_list, MAC_CHBA_MAX_DEVICE_NUM, device_list, device_cnt);
    if (ret != 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_add_island_to_list: memcpy_s fail");
        oal_free(chba_node);
        return;
    }

    hi_list_add_tail(&island_info->island_list, &chba_node->node);
    island_info->island_cnt++;
}
OAL_STATIC void hmac_chba_free_device_list(uint8_t *device_list[], uint8_t list_num)
{
    uint8_t index;
    for (index = 0; index < list_num; index++) {
        if (device_list[index] == NULL) {
            continue;
        }
        oal_free(device_list[index]);
    }
}

static void hmac_chba_del_one_device_info(uint8_t device_id)
{
    hmac_chba_hash_node_stru cmp;
    struct hi_node *node = NULL;
    hmac_chba_hash_node_stru *chba_node = NULL;
    hmac_chba_alive_device_table_stru *ht = hmac_chba_get_alive_device_table();
    hmac_chba_per_device_id_info_stru *device_id_list = hmac_chba_get_device_id_info();
    hmac_chba_per_device_id_info_stru *cur_device_info = &device_id_list[device_id];

    /* 从连接拓扑图中删除该设备连接信息 */
    hmac_chba_clear_one_dev_from_network_topo(device_id);
    /* 从活跃信息列表中查找该节点 */
    oal_set_mac_addr(cmp.mac_addr, device_id_list[device_id].mac_addr);
    node = hi_hash_find(&ht->raw, &cmp.node);
    if (node == NULL) {
        return;
    }
    chba_node = hi_node_entry(node, hmac_chba_hash_node_stru, node);

    oam_warning_log4(0, OAM_SF_CHBA,
        "{hmac_chba_del_one_device_info: del chba device XX:XX:XX:%2X:%2X:%2X, device_id[%d]}",
        cur_device_info->mac_addr[MAC_ADDR_3], cur_device_info->mac_addr[MAC_ADDR_4],
        cur_device_info->mac_addr[MAC_ADDR_5], device_id);
    /* 同步device id到DMAC进行删除 */
    hmac_chba_device_info_sync(CHBA_DEVICE_ID_DEL, device_id, cur_device_info->mac_addr);
    /* 清除该device id的分配信息 */
    memset_s(cur_device_info, sizeof(hmac_chba_per_device_id_info_stru), 0, sizeof(hmac_chba_per_device_id_info_stru));
    /* 从活跃设备哈希表中删除该设备 */
    hi_hash_remove(node);
    ht->node_cnt--;
    oal_free(chba_node);
}


void hmac_chba_generate_island_info(hmac_chba_island_list_stru *island_info, uint8_t *parent, uint8_t len)
{
    uint32_t i;
    uint8_t root;
    uint8_t device_cnt[MAC_CHBA_MAX_DEVICE_NUM];
    uint8_t *device_list[MAC_CHBA_MAX_DEVICE_NUM];
    if (parent == NULL) {
        return;
    }

    /* 分配内存 */
    for (i = 0; i < MAC_CHBA_MAX_DEVICE_NUM; i++) {
        device_list[i] = (uint8_t *)oal_memalloc(MAC_CHBA_MAX_ISLAND_DEVICE_NUM * sizeof(uint8_t));
        if (device_list[i] == NULL) {
            oam_warning_log0(0, OAM_SF_CHBA, "{hmac_chba_generate_island_info::memalloc failed.}");
            hmac_chba_free_device_list(device_list, MAC_CHBA_MAX_DEVICE_NUM);
            return;
        }
        memset_s(device_list[i], MAC_CHBA_MAX_ISLAND_DEVICE_NUM, 0, MAC_CHBA_MAX_ISLAND_DEVICE_NUM);
    }

    /* 统计每个根节点下的设备列表 */
    memset_s(device_cnt, sizeof(device_cnt), 0, sizeof(device_cnt));
    for (i = 0; i < len; i++) {
        root = hmac_chba_get_root(parent, i);
        if (device_cnt[root] >= MAC_CHBA_MAX_ISLAND_DEVICE_NUM) {
            oam_warning_log0(0, OAM_SF_CHBA, "{hmac_chba_generate_island_info::device cnt exceed.}");
            hmac_chba_free_device_list(device_list, MAC_CHBA_MAX_DEVICE_NUM);
            return;
        }

        device_list[root][device_cnt[root]] = i;
        device_cnt[root]++;
    }

    /* 生成岛信息 */
    for (i = 0; i < MAC_CHBA_MAX_DEVICE_NUM; i++) {
        /* 至少两个设备才算岛, 根节点作为岛id */
        if (device_cnt[i] > 1) {
            hmac_chba_add_island_to_list(island_info, i, device_cnt[i], &device_list[i][0]);
        }
    }

    /* 释放内存 */
    hmac_chba_free_device_list(device_list, MAC_CHBA_MAX_DEVICE_NUM);
}


static void hmac_chba_del_other_island_device(uint8_t *parent, uint8_t chba_device_num)
{
    uint8_t device_idx;
    uint8_t self_island_id;
    uint8_t island_id;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    /* 获取自身岛id, 即自身device_id在parent数组中对应的根节点 */
    self_island_id = hmac_chba_get_root(parent, self_conn_info->self_device_id);
    for (device_idx = 0; device_idx < chba_device_num; device_idx++) {
        island_id = hmac_chba_get_root(parent, device_idx);
        /* 岛id与自身岛id不同，则删除该岛信息 */
        if (island_id != self_island_id) {
            hmac_chba_del_one_device_info(device_idx); /* 删除该device */
        }
    }
}

static void hmac_chba_generate_island_parent_info(uint8_t *parent, uint8_t chba_device_num)
{
    uint8_t device_idx;
    uint8_t topo_row, topo_col, root_row, root_col;

    for (device_idx = 0; device_idx < chba_device_num; device_idx++) {
        parent[device_idx] = device_idx;
    }

    /* 根据连接拓扑，使用并查集更新parent数组 */
    for (topo_row = 0; topo_row < chba_device_num; topo_row++) {
        for (topo_col = 0; topo_col < topo_row; topo_col++) {
            if (hmac_chba_get_network_topo_ele(topo_row, topo_col) == 0) {
                continue;
            }

            root_row = hmac_chba_get_root(parent, topo_row);
            root_col = hmac_chba_get_root(parent, topo_col);
            if (root_row <= root_col) {
                parent[root_col] = root_row;
            } else {
                parent[root_row] = root_col;
            }
        }
    }
}

void hmac_chba_update_island_info(hmac_vap_stru *hmac_vap, hmac_chba_island_list_stru *island_info)
{
    hmac_chba_vap_stru *chba_vap_info = NULL;
    uint8_t parent[MAC_CHBA_MAX_DEVICE_NUM] = {};

    chba_vap_info = hmac_chba_get_chba_vap_info(hmac_vap);
    if (chba_vap_info == NULL) {
        oam_error_log0(0, OAM_SF_CHBA, "{hmac_chba_update_island_info:fail to find chba vap!}");
        return;
    }
    /* 初始化岛信息链表 */
    hi_list_init(&island_info->island_list);
    island_info->island_cnt = 0;

    /* 根据topo信息生成并查集根节点数组 */
    hmac_chba_generate_island_parent_info(parent, MAC_CHBA_MAX_DEVICE_NUM);

    /* 若不支持一域多岛 */
    if (chba_vap_info->is_support_multiple_island == OAL_FALSE) {
        /* 从topo图与活跃信息列表中删除非自身岛设备 */
        hmac_chba_del_other_island_device(parent, MAC_CHBA_MAX_DEVICE_NUM);
        /* 根据新topo图重新生成parent数组 */
        hmac_chba_generate_island_parent_info(parent, MAC_CHBA_MAX_DEVICE_NUM);
    }
    /* 根据并查集结果生成岛信息 */
    hmac_chba_generate_island_info(island_info, parent, MAC_CHBA_MAX_DEVICE_NUM);
}


void hmac_chba_fill_one_device_info(mac_chba_per_device_info_stru *device_info,
    hmac_chba_per_device_id_info_stru *device_id_info, uint8_t conn_flag)
{
    int32_t ret;
    if (oal_any_null_ptr2(device_info, device_id_info)) {
        return;
    }

    ret = memcpy_s(device_info->mac_addr, OAL_MAC_ADDR_LEN, device_id_info->mac_addr, OAL_MAC_ADDR_LEN);
    if (ret != 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_fill_one_device_info: memcpy_s fail");
        return;
    }

    device_info->ps_bitmap = device_id_info->ps_bitmap;

    device_info->is_conn_flag = conn_flag;
}


void hmac_chba_fill_self_island_device_list(hmac_chba_per_island_tmp_info_stru *curr_island,
    mac_chba_self_conn_info_stru *self_conn_info, hmac_chba_per_device_id_info_stru *device_id_info,
    uint8_t owner_valid, uint8_t owner_device_id)
{
    uint8_t device_id, conn_flag;
    uint32_t i;
    uint32_t curr_idx = 0;
    mac_chba_per_device_info_stru *device_info = NULL;

    if (oal_any_null_ptr3(curr_island, self_conn_info, device_id_info)) {
        return;
    }

    /* 填写岛主mac地址 */
    if (owner_valid == TRUE) {
        /* 设备列表中，岛主放在第一个位置 */
        device_info = &self_conn_info->island_device_list[0];
        conn_flag = hmac_chba_get_network_topo_ele(self_conn_info->self_device_id, owner_device_id);
        hmac_chba_fill_one_device_info(device_info, &device_id_info[owner_device_id], conn_flag);
        curr_idx++;
    }

    /* 设备列表中，加入岛内与自己相连的设备 */
    for (i = 0; i < curr_island->device_cnt; i++) {
        device_id = curr_island->device_list[i];
        /* 与自己相连的设备 */
        if (hmac_chba_get_network_topo_ele(self_conn_info->self_device_id, device_id) == TRUE) {
            /* 岛主已经放过了，跳过 */
            if ((owner_valid == TRUE) && (device_id == owner_device_id)) {
                continue;
            }
            device_info = &self_conn_info->island_device_list[curr_idx];
            hmac_chba_fill_one_device_info(device_info, &device_id_info[device_id], TRUE);
            curr_idx++;
        }
    }

    /* 设备列表中，加入岛内与自己不相连的设备 */
    for (i = 0; i < curr_island->device_cnt; i++) {
        device_id = curr_island->device_list[i];
        /* 与自己不相连的设备 */
        if (hmac_chba_get_network_topo_ele(self_conn_info->self_device_id, device_id) == FALSE) {
            /* 岛主已经放过了，跳过 */
            if ((owner_valid == TRUE) && (device_id == owner_device_id)) {
                continue;
            }
            device_info = &self_conn_info->island_device_list[curr_idx];
            if (self_conn_info->self_device_id != device_id) {
                oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_fill_self_island_device_list:: curr_idx %d, device id %d",
                    curr_idx, device_id);
                oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_fill_self_island_device_list: mac addr %2X:%2X:%2X",
                    device_id_info[device_id].mac_addr[3], device_id_info[device_id].mac_addr[4],
                    device_id_info[device_id].mac_addr[5]);
            }
            hmac_chba_fill_one_device_info(device_info, &device_id_info[device_id], FALSE);
            curr_idx++;
        }
    }
}


static void hmac_chba_fill_self_conn_info_with_owner(hmac_chba_per_island_tmp_info_stru *curr_island,
    mac_chba_self_conn_info_stru *self_conn_info)
{
    int32_t ret;
    uint8_t owner_device_id = UINT8_INVALID_VALUE;
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    if (oal_any_null_ptr2(curr_island, self_conn_info)) {
        return;
    }

    /* 填写岛主mac地址 */
    self_conn_info->island_owner_valid = curr_island->owner_valid;
    if (self_conn_info->island_owner_valid == TRUE) {
        owner_device_id = curr_island->owner_device_id;
        ret = memcpy_s(self_conn_info->island_owner_addr, OAL_MAC_ADDR_LEN, device_id_info[owner_device_id].mac_addr,
            OAL_MAC_ADDR_LEN);
        if (ret != 0) {
            oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_fill_self_conn_info_with_owner::memcpy_s fail");
            return;
        }
    }

    /* 填写本设备连接信息中的岛设备列表 */
    self_conn_info->island_device_cnt = curr_island->device_cnt;
    hmac_chba_fill_self_island_device_list(curr_island, self_conn_info, device_id_info,
        self_conn_info->island_owner_valid, owner_device_id);
}


void hmac_chba_fill_self_conn_info_without_owner(hmac_chba_per_island_tmp_info_stru *curr_island,
    mac_chba_self_conn_info_stru *self_conn_info)
{
    uint32_t i;
    uint8_t device_cnt;
    uint8_t owner_device_id = UINT8_INVALID_VALUE;
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    if (oal_any_null_ptr2(curr_island, self_conn_info)) {
        return;
    }

    device_cnt = curr_island->device_cnt;

    /* 本设备连接信息中岛主有效 */
    if (self_conn_info->island_owner_valid == TRUE) {
        owner_device_id = hmac_chba_one_dev_update_alive_dev_table(self_conn_info->island_owner_addr);
        /* 查找岛主是否还在当前输入的岛设备列表中 */
        for (i = 0; i < device_cnt; i++) {
            if (owner_device_id == curr_island->device_list[i]) {
                break;
            }
        }

        /* 如果岛主不在当前输入的岛设备列表中，将岛主信息置为无效 */
        if (i == device_cnt) {
            self_conn_info->island_owner_valid = FALSE;
        }
    }

    self_conn_info->island_device_cnt = device_cnt;
    hmac_chba_fill_self_island_device_list(curr_island, self_conn_info, device_id_info,
        self_conn_info->island_owner_valid, owner_device_id);
}


void hmac_chba_fill_self_conn_info(hmac_chba_per_island_tmp_info_stru *curr_island,
    mac_chba_self_conn_info_stru *self_conn_info, uint8_t owner_info_flag)
{
    if (oal_any_null_ptr2(curr_island, self_conn_info)) {
        return;
    }

    /* 岛信息中包含岛主信息 */
    if (owner_info_flag == TRUE) {
        hmac_chba_fill_self_conn_info_with_owner(curr_island, self_conn_info);
    } else { /* 岛信息中不包含岛主信息 */
        hmac_chba_fill_self_conn_info_without_owner(curr_island, self_conn_info);
    }
}
static void hmac_sync_self_conn_info(mac_chba_self_conn_info_stru *self_conn_info)
{
    uint32_t ret;
    oal_netbuf_stru *conn_info_netbuf = NULL;
    frw_event_mem_stru *event_mem = NULL;
    frw_event_stru *h2d_self_conn_event = NULL;
    dmac_tx_event_stru *self_conn_tx_event = NULL;
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    mac_vap_stru *mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(chba_vap_info->mac_vap_id);

    if (mac_vap == NULL) {
        oam_error_log0(0, OAM_SF_CFG, "{hmac_sync_self_conn_info::mac_vap null.}");
        return;
    }
    /* 申请netbuf内存 */
    conn_info_netbuf = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF,
        sizeof(mac_chba_self_conn_info_stru), OAL_NETBUF_PRIORITY_MID);
    if (conn_info_netbuf == NULL) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_sync_self_conn_info::alloc conn_info_netbuf failed.}");
        return;
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    event_mem = frw_event_alloc_m(sizeof(dmac_tx_event_stru));
    if (event_mem == NULL) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_sync_self_conn_info::pst_event_mem null.}");
        oal_netbuf_free(conn_info_netbuf);
        return;
    }
    h2d_self_conn_event = (frw_event_stru *)event_mem->puc_data;
    frw_event_hdr_init(&(h2d_self_conn_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX, DMAC_WLAN_CTX_EVENT_SUB_TYPE_CHBA_SELF_CONN_INFO_SYNC,
                       sizeof(dmac_tx_event_stru), FRW_EVENT_PIPELINE_STAGE_1,
                       mac_vap->uc_chip_id, mac_vap->uc_device_id, mac_vap->uc_vap_id);
    memset_s(oal_netbuf_cb(conn_info_netbuf), OAL_TX_CB_LEN, 0, OAL_TX_CB_LEN);
    if (memcpy_s(oal_netbuf_data(conn_info_netbuf), sizeof(mac_chba_self_conn_info_stru), self_conn_info,
        sizeof(mac_chba_self_conn_info_stru)) != EOK) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_sync_self_conn_info::memcpy fail!}");
        oal_netbuf_free(conn_info_netbuf);
        frw_event_free_m(event_mem);
        return;
    }
    self_conn_tx_event = (dmac_tx_event_stru *)(h2d_self_conn_event->auc_event_data);
    self_conn_tx_event->pst_netbuf = conn_info_netbuf;
    self_conn_tx_event->us_frame_len = sizeof(mac_chba_self_conn_info_stru);

    ret = frw_event_dispatch_event(event_mem);
    if (ret != OAL_SUCC) {
        oam_error_log1(mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{hmac_sync_self_conn_info::frw_event_dispatch_event failed[%d].}", ret);
    }
    oal_netbuf_free(conn_info_netbuf);
    frw_event_free_m(event_mem);
}

void hmac_chba_update_self_conn_info(hmac_vap_stru *hmac_vap, hmac_chba_island_list_stru *tmp_island_info,
    uint8_t owner_info_flag)
{
    uint8_t self_device_id;
    uint32_t j;
    uint8_t find_island_flag = FALSE;
    struct hi_node *node = NULL;
    struct hi_node *tmp = NULL;
    hmac_chba_tmp_island_list_node_stru *chba_node = NULL;
    hmac_chba_per_island_tmp_info_stru *curr_island = NULL;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    if (hmac_vap == NULL) {
        return;
    }

    self_device_id = self_conn_info->self_device_id;
    self_conn_info->island_device_cnt = 0;

    /* 从岛信息获取本设备所属岛的设备列表 */
    hi_list_for_each_safe(node, tmp, &tmp_island_info->island_list) {
        chba_node = hi_node_entry(node, hmac_chba_tmp_island_list_node_stru, node);
        curr_island = &chba_node->data;
        for (j = 0; j < curr_island->device_cnt; j++) {
            /* 找到本设备所属岛 */
            if (self_device_id == curr_island->device_list[j]) {
                find_island_flag = TRUE;
                hmac_chba_fill_self_conn_info(curr_island, self_conn_info, owner_info_flag);
                break;
            }
        }
    }

    /* 找不到本设备所属岛或者usercnt为0，清空本设备岛信息 */
    if ((find_island_flag == FALSE) || (hmac_vap->st_vap_base_info.us_user_nums == 0)) {
        self_conn_info->island_owner_valid = FALSE;
        self_conn_info->island_device_cnt = 0;
        memset_s(self_conn_info->island_owner_addr, OAL_MAC_ADDR_LEN, 0, OAL_MAC_ADDR_LEN);
        memset_s(self_conn_info->island_device_list,
            sizeof(mac_chba_per_device_info_stru) * MAC_CHBA_MAX_ISLAND_DEVICE_NUM,
            0, sizeof(mac_chba_per_device_info_stru) * MAC_CHBA_MAX_ISLAND_DEVICE_NUM);
    }
    hmac_chba_coex_island_info_report(self_conn_info);
    hmac_sync_self_conn_info(self_conn_info);
}


uint8_t hmac_chba_del_longest_unactive_dev(hmac_chba_alive_device_table_stru *ht)
{
    uint8_t device_id = 0;
    uint32_t i, curr_time, time_offset;
    uint32_t max_time_offset = 0;
    struct hi_node *node = NULL;
    struct hi_node *tmp = NULL;
    hmac_chba_hash_node_stru *chba_node = NULL;
    hmac_chba_hash_node_stru *chba_node_max = NULL;
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    /* 哈希表创建失败，直接返回 */
    if (ht == NULL) {
        return UINT8_INVALID_VALUE;
    }

    /* 遍历活跃设备表，找到除自己外不活跃时间最大的设备，从活跃设备表中删除 */
    for (i = 0; i < (uint32_t)ht->raw.bkt_size; i++) {
        hi_list_for_each_safe(node, tmp, &ht->raw.bkts[i]) {
            chba_node = hi_node_entry(node, hmac_chba_hash_node_stru, node);
            curr_time = (uint32_t)oal_time_get_stamp_ms();
            time_offset = (uint32_t)oal_time_get_runtime(chba_node->last_alive_time, curr_time);
            if ((time_offset >= max_time_offset) && (chba_node->device_id != self_conn_info->self_device_id)) {
                max_time_offset = time_offset;
                chba_node_max = chba_node;
            }
        }
    }

    if (chba_node_max != NULL) {
        device_id = chba_node_max->device_id;
        if (device_id < MAC_CHBA_MAX_DEVICE_NUM) {
            /* 从连接拓扑图中删除该设备连接信息 */
            hmac_chba_clear_one_dev_from_network_topo(device_id);
            /* 同步device id到DMAC */
            hmac_chba_device_info_sync(CHBA_DEVICE_ID_DEL, device_id, device_id_info[device_id].mac_addr);
            /* 清除该device id的分配信息 */
            memset_s(&device_id_info[device_id], sizeof(hmac_chba_per_device_id_info_stru),
                0, sizeof(hmac_chba_per_device_id_info_stru));
            /* 将拓扑信息同步到DMAC */
            hmac_chba_topo_info_sync();
        }

        /* 从活跃设备哈希表中删除该设备 */
        hi_hash_remove(&chba_node_max->node);
        ht->node_cnt--;
        oal_free(chba_node_max);
    }

    return device_id;
}


mac_chba_per_device_info_stru *hmac_chba_find_island_device_info(const uint8_t* mac_addr)
{
    uint8_t idx;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    mac_chba_per_device_info_stru *device_list = self_conn_info->island_device_list;

    for (idx = 0; idx < self_conn_info->island_device_cnt; idx++) {
        if (oal_compare_mac_addr(mac_addr, device_list[idx].mac_addr) == 0) {
            return &device_list[idx];
        }
    }
    return NULL;
}

uint8_t hmac_chba_one_dev_update_alive_dev_table(const uint8_t* mac_addr)
{
    uint8_t device_id;
    hmac_chba_hash_node_stru cmp;
    hmac_chba_hash_node_stru *chba_node = NULL;
    struct hi_node *node = NULL;
    hmac_chba_alive_device_table_stru *ht = hmac_chba_get_alive_device_table();
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();
    uint8_t all_zero_addr[OAL_MAC_ADDR_LEN] = { 0, 0, 0, 0, 0, 0};

    /* 哈希表创建失败，直接返回 */
    if (oal_any_null_ptr2(mac_addr, ht)) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_one_dev_update_alive_dev_table: null");
        return UINT8_INVALID_VALUE;
    }

    /* 输入地址全0，直接返回 */
    if (oal_compare_mac_addr(mac_addr, all_zero_addr) == 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_one_dev_update_alive_dev_table: mac addr all zero");
        return UINT8_INVALID_VALUE;
    }

    /* 根据mac地址在活跃设备表中查找 */
    oal_set_mac_addr(cmp.mac_addr, mac_addr);
    node = hi_hash_find(&ht->raw, &cmp.node);
    /* 找到该设备，更新lastAliveTime */
    if (node != NULL) {
        chba_node = hi_node_entry(node, hmac_chba_hash_node_stru, node);
        chba_node->last_alive_time = (uint32_t)oal_time_get_stamp_ms();
        device_id = chba_node->device_id;
        return device_id;
    }

    /* 找不到该设备，将该设备加入哈希表 */
    chba_node = (hmac_chba_hash_node_stru*)oal_memalloc(sizeof(hmac_chba_hash_node_stru));
    if (chba_node == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_one_dev_update_alive_dev_table: malloc fail");
        return UINT8_INVALID_VALUE;
    }

    /* 分配device id */
    device_id = hmac_chba_alloc_device_id();
    /* device id分配失败 */
    if (device_id == UINT8_INVALID_VALUE) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_one_dev_update_alive_dev_table: alloc device id fail");
        /* 从哈希表中删除不活跃时间最长的设备，占用其device id */
        device_id = hmac_chba_del_longest_unactive_dev(ht);
        if (device_id == UINT8_INVALID_VALUE) {
            oal_free(chba_node);
            return UINT8_INVALID_VALUE;
        }
    }

    /* 填写节点信息 */
    oal_set_mac_addr(chba_node->mac_addr, mac_addr);

    /* 更新device id标记 */
    device_id_info[device_id].use_flag = TRUE;

    chba_node->device_id = device_id;
    chba_node->last_alive_time = (uint32_t)oal_time_get_stamp_ms();

    /* 将该设备加入哈希表 */
    hi_hash_add(&ht->raw, &chba_node->node);
    ht->node_cnt++;
    oam_warning_log3(0, OAM_SF_CHBA, "update_alive_dev_table: add new node,mac[0x%x:%x], deviceid[%d]",
        mac_addr[MAC_ADDR_4], mac_addr[MAC_ADDR_5], device_id);
    /* 同步device id到DMAC */
    hmac_chba_device_info_sync(CHBA_DEVICE_ID_ADD, device_id, mac_addr);

    return device_id;
}


void hmac_chba_conn_add_update_network_topo(mac_chba_conn_info_rpt conn_info_rpt,
    uint8_t device_id, uint8_t peer_device_id)
{
    /* 更新连接拓扑图 */
    hmac_chba_set_network_topo_ele_true(device_id, peer_device_id);
    hmac_chba_set_network_topo_ele_true(peer_device_id, device_id);

    /* 更新设备ID信息 */
    hmac_chba_update_device_id_info(device_id, conn_info_rpt.self_mac_addr, conn_info_rpt.work_channel.uc_chan_number,
        conn_info_rpt.work_channel.en_bandwidth);
    hmac_chba_update_device_id_info(peer_device_id, conn_info_rpt.peer_mac_addr,
        conn_info_rpt.work_channel.uc_chan_number, conn_info_rpt.work_channel.en_bandwidth);
}


uint32_t hmac_chba_del_non_alive_device(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    uint8_t device_id;
    hmac_chba_hash_node_stru cmp;
    struct hi_node *node = NULL;
    hmac_chba_hash_node_stru *chba_node = NULL;
    chba_device_id_stru *device_rpt = NULL;
    hmac_chba_alive_device_table_stru *ht = hmac_chba_get_alive_device_table();
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    device_rpt = (chba_device_id_stru *)param;

    /* 根据mac地址在活跃设备表中查找 */
    oal_set_mac_addr(cmp.mac_addr, device_rpt->mac_addr);
    node = hi_hash_find(&ht->raw, &cmp.node);
    /* 找不到该设备，将该设备加入哈希表 */
    if (node == NULL) {
        oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_del_non_alive_device: can't find device XX:XX:XX:%2X:%2X:%2X",
            device_rpt->mac_addr[MAC_ADDR_3], device_rpt->mac_addr[MAC_ADDR_4], device_rpt->mac_addr[MAC_ADDR_5]);
        return OAL_SUCC;
    }

    chba_node = hi_node_entry(node, hmac_chba_hash_node_stru, node);
    device_id = chba_node->device_id;
    /* 异常维测打印，不应该出现同一个MAC地址的device id不一致 */
    if (device_id != device_rpt->device_id) {
        oam_error_log2(0, OAM_SF_CHBA, "hmac_chba_del_non_alive_device: device id not match %d %d",
            device_id, device_rpt->device_id);
    }

    if ((device_id < MAC_CHBA_MAX_DEVICE_NUM) && (device_id != self_conn_info->self_device_id)) {
        /* 删除设备信息 */
        hmac_chba_del_one_device_info(device_id);
        /* 将拓扑信息同步到DMAC */
        hmac_chba_topo_info_sync();
    }
    return OAL_SUCC;
}


uint8_t *hmac_chba_get_own_mac_addr(void)
{
    uint8_t device_id;
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    device_id = self_conn_info->self_device_id;

    if (device_id_info[device_id].use_flag == FALSE) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_get_own_mac_addr::CHBA VAP is not start, no mac addr.");
        return NULL;
    }

    return device_id_info[device_id].mac_addr;
}


void hmac_chba_set_role(uint8_t role)
{
    hmac_chba_role_report_stru role_report = {0};
    uint8_t his_role;
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    hmac_chba_domain_merge_info *domain_merge = hmac_chba_get_domain_merge_info();
    his_role = hmac_chba_get_role();
    g_chba_mgmt_info.self_conn_info.role = role;
    domain_merge->his_role = his_role;
    oal_set_mac_addr(domain_merge->his_bssid, sync_info->domain_bssid);

    /* 上报上层HAL */
    role_report.report_type = HMAC_CHBA_ROLE_REPORT;
    role_report.role = role;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_chba_report((uint8_t *)(&role_report), sizeof(role_report));
#endif
    oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_set_role::New role :%d, last role %d", role, his_role);
}


uint8_t hmac_chba_update_dev_mgmt_info_by_frame(uint8_t *payload, int32_t len,
    uint8_t *peer_addr, int8_t *rssi, uint8_t *linkcnt, uint8_t sa_is_my_user)
{
    uint8_t *data = NULL;
    uint16_t offset;
    uint8_t chan_number, bandwidth, device_id, conn_device_id;
    uint32_t i;

    /* 解析Link Info Attribute */
    /* *********************** Link Info Attribute ************************************ */
    /* ------------------------------------------------------------------------------  */
    /* | CHBA ATTR ID | LEN |Channel Number |Bandwidth | rssi | LinkCnt | MacAddr   | */
    /* ------------------------------------------------------------------------------  */
    /* |        1      |  1  |       1       |     1    |  1   |     1   | LinkCnt*6 | */
    /* ------------------------------------------------------------------------------  */
    data = hmac_find_chba_attr(MAC_CHBA_ATTR_LINK_INFO, payload, len);
    if (data == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_update_dev_mgmt_info_by_frame::can't find link info attr.");
        return UINT8_INVALID_VALUE;
    }

    if (MAC_CHBA_ATTR_HDR_LEN + data[MAC_CHBA_ATTR_ID_LEN] < MAC_CHBA_LINK_INFO_LINK_CNT_POS + 1 ||
        MAC_CHBA_ATTR_HDR_LEN + data[MAC_CHBA_ATTR_ID_LEN] <
        MAC_CHBA_LINK_INFO_LINK_CNT_POS + 1 + data[MAC_CHBA_LINK_INFO_LINK_CNT_POS] * WLAN_MAC_ADDR_LEN) {
        oam_error_log1(0, OAM_SF_CHBA, "hmac_chba_update_dev_mgmt_info_by_frame::link info attr len[%d] invalid.",
            data[MAC_CHBA_ATTR_ID_LEN]);
        return UINT8_INVALID_VALUE;
    }
    offset = MAC_CHBA_ATTR_HDR_LEN;
    chan_number = data[offset];
    offset += HMAC_CHBA_BYTE_LEN_1;
    bandwidth = data[offset];
    offset += HMAC_CHBA_BYTE_LEN_1;
    *rssi = (int8_t)data[offset];
    offset += HMAC_CHBA_BYTE_LEN_1;
    *linkcnt = data[offset];
    offset += HMAC_CHBA_BYTE_LEN_1;
    if (*linkcnt > MAC_CHBA_MAX_LINK_NUM) {
        return UINT8_INVALID_VALUE;
    }

    /* 将帧发送方加入到活跃设备表 */
    device_id = hmac_chba_one_dev_update_alive_dev_table(peer_addr);
    /* 无效的device id直接返回 */
    if (device_id >= MAC_CHBA_MAX_DEVICE_NUM) {
        return UINT8_INVALID_VALUE;
    }

    /* 更新帧发送方的设备ID信息 */
    hmac_chba_update_device_id_info(device_id, peer_addr, chan_number, bandwidth);

    /* 从连接拓扑图中先清空该device_id连接信息 */
    hmac_chba_clear_one_dev_from_network_topo(device_id);
    /* 将与帧发送方连接的每个设备加入到活跃设备表 */
    for (i = 0; i < *linkcnt; i++) {
        conn_device_id = hmac_chba_one_dev_update_alive_dev_table(&data[offset]);
        /* 无效的device id不处理 */
        if (conn_device_id >= MAC_CHBA_MAX_DEVICE_NUM) {
            continue;
        }
        /* 如果conn_device_id为自己，但是sa非自己关联用户，说明已经去关联，不再更新网络拓扑 */
        if ((conn_device_id == hmac_chba_get_own_device_id()) && (sa_is_my_user == OAL_FALSE)) {
            continue;
        }
        /* 更新连接拓扑图 */
        hmac_chba_set_network_topo_ele_true(device_id, conn_device_id);
        hmac_chba_set_network_topo_ele_true(conn_device_id, device_id);

        /* 更新设备ID信息 */
        hmac_chba_update_device_id_info(conn_device_id, &data[offset], chan_number, bandwidth);

        offset += OAL_MAC_ADDR_LEN;
    }

    return device_id;
}


void hmac_chba_beacon_update_device_mgmt_info(uint8_t *sa_addr, uint8_t *payload,
    uint16_t payload_len, uint8_t is_my_user)
{
    uint32_t power_save_bitmap;
    int8_t rssi = 0;
    uint8_t linkcnt = 0;
    uint8_t device_id;
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    /************************ Beacon Payload ******************************************/
    /* ------------------------------------------------------------------------------ */
    /* | Timestamp | Beacon Interval | Capability | CHBA Vendor Element | ...  */
    /* ------------------------------------------------------------------------------ */
    /* |    8      |      2          |     2      |      variable        | ...  */
    /* ------------------------------------------------------------------------------ */
    /************************ CHBA Vendor Element ************************************/
    /* ------------------------------------------------------------------------------ */
    /* | EID | LEN | OUI |OUI Type | CHBA ATTRIBUTS | ...                            */
    /* ------------------------------------------------------------------------------ */
    /* |  1  |  1  |  3  |    1    |     variable    | ...                            */
    /* ------------------------------------------------------------------------------ */
    /* 根据该帧payload更新设备管理信息 */
    device_id = hmac_chba_update_dev_mgmt_info_by_frame(payload, payload_len, sa_addr, &rssi, &linkcnt, is_my_user);

    /* 解析低功耗属性 */
    power_save_bitmap = hmac_chba_decap_power_save_attr(payload, payload_len);

    /* 保存低功耗bitmap */
    if (device_id < MAC_CHBA_MAX_DEVICE_NUM) {
        device_id_info[device_id].ps_bitmap = power_save_bitmap;
    }
}


void hmac_chba_clear_device_mgmt_info(void)
{
    hmac_chba_alive_device_table_stru *alive_dev_table = hmac_chba_get_alive_device_table();
    hmac_chba_island_list_stru *island_info = hmac_chba_get_island_list();
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();
    uint32_t *network_topo_addr = hmac_chba_get_network_topo_addr();
    hmac_chba_mgmt_info_stru *mgmt_info = hmac_chba_get_mgmt_info();

    /* 哈希表创建失败，直接返回 */
    if (alive_dev_table == NULL) {
        return;
    }

    /* 清空活跃设备表 */
    hmac_chba_hash_clear(alive_dev_table);

    /* 清空岛信息 */
    hmac_chba_clear_island_info(island_info, hmac_chba_island_list_node_free);

    /* 清空设备管理信息 */
    memset_s(network_topo_addr, sizeof(uint32_t) * (MAC_CHBA_MAX_DEVICE_NUM * MAC_CHBA_MAX_BITMAP_WORD_NUM),
        0, sizeof(uint32_t) * (MAC_CHBA_MAX_DEVICE_NUM * MAC_CHBA_MAX_BITMAP_WORD_NUM));
    memset_s(device_id_info, sizeof(hmac_chba_per_device_id_info_stru) * MAC_CHBA_MAX_DEVICE_NUM,
        0, sizeof(hmac_chba_per_device_id_info_stru) * MAC_CHBA_MAX_DEVICE_NUM);
    memset_s(self_conn_info, sizeof(mac_chba_self_conn_info_stru), 0, sizeof(mac_chba_self_conn_info_stru));
    /* 初始化设备角色 */
    self_conn_info->role = CHBA_OTHER_DEVICE;

    mgmt_info->island_owner_attr_len = 0;
    memset_s(mgmt_info->island_owner_attr_buf, MAC_CHBA_MGMT_MID_PAYLOAD_BYTES, 0, MAC_CHBA_MGMT_MID_PAYLOAD_BYTES);

    /* 将拓扑信息同步到DMAC */
    hmac_chba_topo_info_sync();
    /* 同步device id到DMAC */
    hmac_chba_device_info_sync(CHBA_DEVICE_ID_CLEAR, UINT8_INVALID_VALUE, NULL);
}


void hmac_chba_find_max_pnf_rssi_node(hmac_chba_per_island_tmp_info_stru *tmp_island, uint8_t exclude_device_id,
    int8_t *max_pnf_rssi, uint8_t *max_rssi_device_id)
{
    uint8_t idx, device_id;
    int8_t pnf_rssi;
    uint32_t ps_bitmap;
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    if (oal_any_null_ptr3(tmp_island, max_pnf_rssi, max_rssi_device_id)) {
        *max_rssi_device_id = UINT8_INVALID_VALUE;
        oam_warning_log0(0, 0, "hmac_chba_find_max_pnf_rssi_node: NULL pointer");
        return;
    }

    *max_pnf_rssi = MIN_INT8;
    *max_rssi_device_id = UINT8_INVALID_VALUE;

    for (idx = 0; idx < tmp_island->device_cnt; idx++) {
        device_id = tmp_island->device_list[idx];
        /* 跳过要排除的设备 */
        if (device_id == exclude_device_id) {
            continue;
        }

        ps_bitmap = device_id_info[device_id].ps_bitmap;
        /* 跳过保活状态的设备 */
        if (ps_bitmap == CHBA_PS_KEEP_ALIVE_BITMAP) {
            continue;
        }

        pnf_rssi = device_id_info[device_id].pnf_rssi;
        if (pnf_rssi > *max_pnf_rssi) {
            *max_pnf_rssi = pnf_rssi;
            *max_rssi_device_id = device_id;
        }
    }
}


void hmac_chba_update_new_island_owner(hmac_chba_per_island_tmp_info_stru *curr_tmp_island,
    uint8_t master_device_id)
{
    uint8_t max_rssi_device_id;
    int8_t max_pnf_rssi;
    if (curr_tmp_island == NULL) {
        return;
    }

    curr_tmp_island->owner_valid = TRUE;
    curr_tmp_island->change_flag = TRUE;
    curr_tmp_island->pnf_loss_cnt = 0;

    /* 如果当前岛是否为master所在岛(此时本设备就是master)，将master选为岛主 */
    if (hmac_chba_is_in_device_list(master_device_id, curr_tmp_island->device_cnt, curr_tmp_island->device_list)) {
        curr_tmp_island->owner_device_id = master_device_id;
        return;
    }

    oam_error_log1(0, OAM_SF_CHBA,
        "{hmac_chba_update_new_island_owner:find other island with %d devices}", curr_tmp_island->device_cnt);
    /* 找到岛内非保活态pnf_rssi最大的节点，选为岛主 */
    hmac_chba_find_max_pnf_rssi_node(curr_tmp_island, UINT8_INVALID_VALUE, &max_pnf_rssi, &max_rssi_device_id);

    /* 找不到满足条件的，则选第一个 */
    if (max_rssi_device_id == UINT8_INVALID_VALUE) {
        curr_tmp_island->owner_device_id = curr_tmp_island->device_list[0];
        return;
    }

    curr_tmp_island->owner_device_id = max_rssi_device_id;
}


void hmac_chba_update_exist_island_owner(hmac_chba_per_island_tmp_info_stru *curr_tmp_island,
    hmac_chba_per_island_info_stru *curr_island, uint8_t master_device_id)
{
    uint8_t max_rssi_device_id, island_owner_device_id, island_owner_change;
    int8_t max_pnf_rssi, island_owner_rssi;
    uint32_t island_owner_ps_bitmap;
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    if (oal_any_null_ptr2(curr_tmp_island, curr_island)) {
        return;
    }

    /* 判断设备列表是否变化，设置change_flag */
    if ((curr_tmp_island->device_cnt == curr_island->device_cnt) &&
        (oal_memcmp(curr_tmp_island->device_list, curr_island->device_list, curr_tmp_island->device_cnt) == 0)) {
        curr_tmp_island->change_flag = FALSE;
    } else {
        curr_tmp_island->change_flag = TRUE;
    }

    island_owner_device_id = curr_island->owner_device_id;
    curr_tmp_island->owner_valid = TRUE;

    /* 如果当前岛为master所在岛，将master选为岛主 */
    if (hmac_chba_is_in_device_list(master_device_id, curr_tmp_island->device_cnt, curr_tmp_island->device_list)) {
        curr_tmp_island->owner_device_id = master_device_id;
        curr_tmp_island->pnf_loss_cnt = 0;
        return;
    }

    /* 找到岛内除原岛主外，非保活态pnf_rssi最大的节点 */
    hmac_chba_find_max_pnf_rssi_node(curr_tmp_island, island_owner_device_id, &max_pnf_rssi, &max_rssi_device_id);

    /* 找不到，则维持原岛主 */
    if (max_rssi_device_id == UINT8_INVALID_VALUE) {
        curr_tmp_island->owner_device_id = island_owner_device_id;
        curr_tmp_island->pnf_loss_cnt = curr_island->pnf_loss_cnt;
        return;
    }

    /* 连续丢pnf次数没有超过门限 */
    if (curr_island->pnf_loss_cnt < MAC_CHBA_ISLAND_OWNER_PNF_LOSS_TH) {
        island_owner_rssi = device_id_info[island_owner_device_id].pnf_rssi;
        island_owner_ps_bitmap = device_id_info[island_owner_device_id].ps_bitmap;
        /* 岛主变更条件 */
        island_owner_change = (max_pnf_rssi > island_owner_rssi + MAC_CHBA_ISLAND_OWNER_CHANGE_TH) ||
            (island_owner_ps_bitmap == CHBA_PS_KEEP_ALIVE_BITMAP);
        /* 如果满足岛主变更条件，将除原岛主外rssi最大的非保活设备选为岛主 */
        if (island_owner_change == TRUE) {
            curr_tmp_island->owner_device_id = max_rssi_device_id;
            curr_tmp_island->pnf_loss_cnt = 0;
            oam_warning_log2(0, 0, "CHBA: hmac_chba_update_exist_island_owner: change owner from %d to %d",
                island_owner_device_id, curr_tmp_island->owner_device_id);
        } else { /* 否则维持原岛主 */
            curr_tmp_island->owner_device_id = island_owner_device_id;
            curr_tmp_island->pnf_loss_cnt = curr_island->pnf_loss_cnt;
        }
    } else { /* 连续丢pnf次数超过门限，将除原岛主外rssi最大的非保活设备选为岛主 */
        curr_tmp_island->owner_device_id = max_rssi_device_id;
        curr_tmp_island->pnf_loss_cnt = 0;
    }
}


void hmac_chba_encap_island_owner_attr(hmac_chba_island_list_stru *tmp_island_info)
{
    uint16_t index;
    uint8_t island_owner_cnt = 0;
    uint8_t *attr_buf = NULL;
    struct hi_node *chba_node = NULL;
    struct hi_node *tmp = NULL;
    hmac_chba_per_island_tmp_info_stru *tmp_island = NULL;
    hmac_chba_tmp_island_list_node_stru *tmp_island_node = NULL;
    hmac_chba_mgmt_info_stru *chba_mgmt_info = hmac_chba_get_mgmt_info();
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    /* Attribute ID */
    attr_buf = &chba_mgmt_info->island_owner_attr_buf[0];
    attr_buf[0] = MAC_CHBA_ATTR_ISLAND_OWNER;

    index = MAC_CHBA_ATTR_HDR_LEN;
    /* 1字节island owner cnt */
    index++;

    hi_list_for_each_safe(chba_node, tmp, &tmp_island_info->island_list) {
        tmp_island_node = hi_node_entry(chba_node, hmac_chba_tmp_island_list_node_stru, node);
        tmp_island = &tmp_island_node->data;
        oal_set_mac_addr(&attr_buf[index], device_id_info[tmp_island->owner_device_id].mac_addr);
        index += OAL_MAC_ADDR_LEN;
        island_owner_cnt++;
    }

    /* 设置Attr Length字段 */
    attr_buf[MAC_CHBA_ATTR_ID_LEN] = index - MAC_CHBA_ATTR_HDR_LEN;
    /* 设置island owner cnt字段 */
    attr_buf[MAC_CHBA_ATTR_HDR_LEN] = island_owner_cnt;

    /* 保存attr_buf的长度 */
    chba_mgmt_info->island_owner_attr_len = index;
}


void hmac_chba_add_node_to_history_island_list(hmac_chba_per_island_tmp_info_stru *tmp_island,
    hmac_chba_island_list_stru *island_info)
{
    int32_t ret;
    hmac_chba_island_list_node_stru *chba_node = NULL;
    hmac_chba_per_island_info_stru *node_info = NULL;

    chba_node = (hmac_chba_island_list_node_stru *)oal_memalloc(sizeof(hmac_chba_island_list_node_stru));
    if (chba_node == NULL) {
        oam_warning_log0(0, 0, "CHBA: hmac_chba_add_node_to_history_island_list: memalloc fail");
        return;
    }

    /* 填写岛id, 岛内设备数，岛内设备列表 */
    node_info = &chba_node->data;
    node_info->owner_device_id = tmp_island->owner_device_id;
    node_info->pnf_loss_cnt = tmp_island->pnf_loss_cnt;
    node_info->device_cnt = tmp_island->device_cnt;
    ret = memcpy_s(node_info->device_list, MAC_CHBA_MAX_DEVICE_NUM, tmp_island->device_list, tmp_island->device_cnt);
    if (ret != 0) {
        oam_warning_log0(0, 0, "CHBA: hmac_chba_add_node_to_history_island_list: memcpy_s fail");
        oal_free(chba_node);
        return;
    }

    hi_list_add_tail(&island_info->island_list, &chba_node->node);
    island_info->island_cnt++;
}


void hmac_chba_update_history_island_list(hmac_chba_island_list_stru *tmp_island_info,
    hmac_chba_island_list_stru *island_info)
{
    struct hi_node *chba_node = NULL;
    struct hi_node *tmp = NULL;
    hmac_chba_per_island_tmp_info_stru *tmp_island = NULL;
    hmac_chba_tmp_island_list_node_stru *tmp_island_node = NULL;

    /* 清空历史岛信息链表 */
    hmac_chba_clear_island_info(island_info, hmac_chba_island_list_node_free);

    hi_list_for_each_safe(chba_node, tmp, &tmp_island_info->island_list) {
        tmp_island_node = hi_node_entry(chba_node, hmac_chba_tmp_island_list_node_stru, node);
        tmp_island = &tmp_island_node->data;

        /* 添加岛信息节点到历史岛信息链表 */
        hmac_chba_add_node_to_history_island_list(tmp_island, island_info);
    }
}


void hmac_chba_island_owner_selection_proc(hmac_chba_island_list_stru *tmp_island_info)
{
    uint8_t master_device_id, is_in_island, find_flag;
    struct hi_node *chba_node = NULL;
    struct hi_node *chba_node2 = NULL;
    struct hi_node *tmp = NULL;
    struct hi_node *tmp2 = NULL;
    hmac_chba_tmp_island_list_node_stru *tmp_island_node = NULL;
    hmac_chba_per_island_tmp_info_stru *tmp_island = NULL;
    hmac_chba_island_list_node_stru *curr_island_node = NULL;
    hmac_chba_per_island_info_stru *curr_island = NULL;
    hmac_chba_island_list_stru *island_info = hmac_chba_get_island_list();
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    /* 此时自己就是master */
    master_device_id = self_conn_info->self_device_id;

    hi_list_for_each_safe(chba_node, tmp, &tmp_island_info->island_list) {
        find_flag = FALSE;
        tmp_island_node = hi_node_entry(chba_node, hmac_chba_tmp_island_list_node_stru, node);
        tmp_island = &tmp_island_node->data;
        hi_list_for_each_safe(chba_node2, tmp2, &island_info->island_list) {
            curr_island_node = hi_node_entry(chba_node2, hmac_chba_island_list_node_stru, node);
            curr_island = &curr_island_node->data;
            is_in_island = hmac_chba_is_in_device_list(curr_island->owner_device_id,
                tmp_island->device_cnt, tmp_island->device_list);
            /* 历史岛链表中的某个岛主在当前岛的设备列表中 */
            if (is_in_island == TRUE) {
                find_flag = TRUE;
                break;
            }
        }

        if (find_flag == TRUE) { /* 该岛是一个已经存在的岛 */
            hmac_chba_update_exist_island_owner(tmp_island, curr_island, master_device_id);
        } else { /* 该岛是一个新的岛 */
            hmac_chba_update_new_island_owner(tmp_island, master_device_id);
            oam_warning_log1(0, 0, "CHBA: hmac_chba_island_owner_selection_proc: new island, owner id %d",
                tmp_island->owner_device_id);
        }
    }

    /* 封装岛主属性保存 */
    hmac_chba_encap_island_owner_attr(tmp_island_info);

    /* 把当前岛信息链表刷新到历史岛信息链表中 */
    hmac_chba_update_history_island_list(tmp_island_info, island_info);
}


uint8_t hmac_chba_rx_beacon_island_owner_attr_handler(uint8_t *sa_addr, uint8_t *payload,
    uint16_t payload_len)
{
    uint32_t island_idx;
    uint8_t island_cnt;
    uint8_t *attr_pos = NULL;
    uint16_t index;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    if (oal_memcmp(sa_addr, hmac_chba_get_master_mac_addr(), OAL_MAC_ADDR_LEN) != 0 &&
        oal_memcmp(sa_addr, self_conn_info->island_owner_addr, OAL_MAC_ADDR_LEN) != 0) {
        oam_warning_log0(0, 0, "hmac_chba_rx_beacon_island_owner_attr_handler: not master or IO's beacon.");
        return UINT8_INVALID_VALUE;
    }

    attr_pos = hmac_find_chba_attr(MAC_CHBA_ATTR_ISLAND_OWNER, payload, payload_len);
    if (attr_pos == NULL) {
        return UINT8_INVALID_VALUE;
    }
    if (MAC_CHBA_ATTR_HDR_LEN + attr_pos[MAC_CHBA_ATTR_ID_LEN] < MAC_CHBA_ISLAND_OWNER_CNT_POS + 1 ||
        MAC_CHBA_ATTR_HDR_LEN + attr_pos[MAC_CHBA_ATTR_ID_LEN] <
        MAC_CHBA_ISLAND_OWNER_CNT_POS + 1 + attr_pos[MAC_CHBA_ISLAND_OWNER_CNT_POS] * WLAN_MAC_ADDR_LEN) {
        oam_error_log1(0, 0, "hmac_chba_rx_beacon_island_owner_attr_handler: island owner attr len[%d] invalid.",
            attr_pos[MAC_CHBA_ATTR_ID_LEN]);
        return UINT8_INVALID_VALUE;
    }

    index = MAC_CHBA_ATTR_HDR_LEN;
    /* 1字节island owner cnt */
    island_cnt = attr_pos[index];
    if (island_cnt > MAC_CHBA_MAX_ISLAND_NUM) {
        return UINT8_INVALID_VALUE;
    }
    index++;
    for (island_idx = 0; island_idx < island_cnt; island_idx++) {
        /* 如果找到岛主地址在本设备所属岛的设备列表中，将其作为本设备的岛主 */
        if (hmac_chba_island_device_check(&attr_pos[index]) == TRUE) {
            /* 岛主变化时维测打印 */
            if (oal_compare_mac_addr(self_conn_info->island_owner_addr, &attr_pos[index]) != 0) {
                oam_warning_log3(0, 0, "rx_island_owner_attr::old owner XX:XX:XX:%2X:%2X:%2X",
                    self_conn_info->island_owner_addr[3], self_conn_info->island_owner_addr[4],
                    self_conn_info->island_owner_addr[5]);
                oam_warning_log3(0, 0, "rx_island_owner_attr::new owner XX:XX:XX:%2X:%2X:%2X",
                    attr_pos[index + 3], attr_pos[index + 4], attr_pos[index + 5]);
            }
            oal_set_mac_addr(self_conn_info->island_owner_addr, &attr_pos[index]);
            self_conn_info->island_owner_valid = TRUE;
            /* 将岛内设备信息更新到岛内同步信息结构体中 */
            hmac_chba_update_island_sync_info();
            return island_idx;
        }
        index += OAL_MAC_ADDR_LEN;
    }

    return UINT8_INVALID_VALUE;
}


void hmac_chba_module_init(uint8_t device_type)
{
    mac_chba_set_init_para init_para;
    hmac_chba_alive_device_table_stru *alive_dev_table = hmac_chba_get_alive_device_table();
    hmac_chba_device_para_stru *device_info = hmac_chba_get_device_info();
    hmac_chba_island_list_stru *island_info = hmac_chba_get_island_list();
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();
    uint32_t *network_topo_addr = hmac_chba_get_network_topo_addr();

    /* 创建活跃设备哈希表 */
    if (alive_dev_table == NULL) {
        alive_dev_table = hmac_chba_hash_creat();
        /* 创建失败返回 */
        if (alive_dev_table == NULL) {
            oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_module_init::creat hash fail");
            return;
        }
        /* 一定要给指针全局变量赋值回来 */
        g_chba_mgmt_info.alive_device_table = alive_dev_table;
    } else { /* 如果非空，清空哈希表 */
        hmac_chba_hash_clear(alive_dev_table);
    }

    /* 清空岛信息 */
    hmac_chba_clear_island_info(island_info, hmac_chba_island_list_node_free);

    /* 初始化设备管理信息 */
    memset_s(network_topo_addr, sizeof(uint32_t) * (MAC_CHBA_MAX_DEVICE_NUM * MAC_CHBA_MAX_BITMAP_WORD_NUM),
        0, sizeof(uint32_t) * (MAC_CHBA_MAX_DEVICE_NUM * MAC_CHBA_MAX_BITMAP_WORD_NUM));
    memset_s(device_id_info, sizeof(hmac_chba_per_device_id_info_stru) * MAC_CHBA_MAX_DEVICE_NUM,
        0, sizeof(hmac_chba_per_device_id_info_stru) * MAC_CHBA_MAX_DEVICE_NUM);
    memset_s(self_conn_info, sizeof(mac_chba_self_conn_info_stru), 0, sizeof(mac_chba_self_conn_info_stru));
    /* 初始化设备角色 */
    self_conn_info->role = CHBA_OTHER_DEVICE;

    /* 读取系统参数，初始化设备类型 */
    device_info->device_type = device_type;
    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_module_init::device type %d", device_info->device_type);

    /* 设置状态为已初始化 */
    hmac_chba_set_module_state(MAC_CHBA_MODULE_STATE_INIT);

    /* 同步模块初始化 */
    hmac_chba_sync_module_init(device_info);

    /* 下发CHBA初始化参数给driver */
    init_para.config_para = device_info->config_para;
    /* 封装主设备选举属性 */
    hmac_chba_set_master_election_attr(init_para.master_election_attr);
    /* 初始化CHBA 信息 */
    hmac_config_chba_base_params(&init_para);
    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_module_init:: device type %d", device_type);
}

void hmac_chba_battery_update_proc(uint8_t percent)
{
    hmac_chba_device_para_stru *device_info = hmac_chba_get_device_info();
    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_battery_update_proc::battery level is %d.", percent);

    if (percent >= REMAIN_POWER_PERCENT_70) {
        device_info->remain_power = BATTERY_LEVEL_HIGH;
    } else if (percent >= REMAIN_POWER_PERCENT_50) {
        device_info->remain_power = BATTERY_LEVEL_MIDDLE;
    } else if (percent >= REMAIN_POWER_PERCENT_30) {
        device_info->remain_power = BATTERY_LEVEL_LOW;
    } else {
        device_info->remain_power = BATTERY_LEVEL_VERY_LOW;
    }

    /* 更新最新的Beacon和RNF到Driver */
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
}



static void hmac_chba_init_device_mgmt_info(mac_chba_vap_start_rpt *info_report)
{
    int32_t ret;
    uint8_t device_id;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    /* 清空设备管理信息 */
    hmac_chba_clear_device_mgmt_info();

    /* 本设备加入活跃设备表，分配device id */
    device_id = hmac_chba_one_dev_update_alive_dev_table(info_report->mac_addr);
    if (device_id >= MAC_CHBA_MAX_DEVICE_NUM) {
        oam_error_log4(0, OAM_SF_CHBA, "hmac_chba_init_device_mgmt_info: addr XX:XX:XX:%2X:%2X:%2X, wrong device_id %d",
            info_report->mac_addr[3], info_report->mac_addr[4], info_report->mac_addr[5], device_id);
        return;
    }
    /* 更新本设备连接信息 */
    self_conn_info->self_device_id = device_id;
    /* 更新设备ID信息 */
    device_id_info[device_id].use_flag = TRUE;
    device_id_info[device_id].chan_number = info_report->work_channel.uc_chan_number;
    device_id_info[device_id].bandwidth = info_report->work_channel.en_bandwidth;
    ret = memcpy_s(device_id_info[device_id].mac_addr, sizeof(uint8_t) * OAL_MAC_ADDR_LEN,
        info_report->mac_addr, sizeof(uint8_t) * OAL_MAC_ADDR_LEN);
    if (ret != 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_init_device_mgmt_info: memcpy_s fail");
        return;
    }
}


void hmac_chba_vap_start_proc(mac_chba_vap_start_rpt *info_report)
{
    if (info_report == NULL) {
        return;
    }

    /* 未初始化不处理 */
    if (hmac_chba_get_module_state() == MAC_CHBA_MODULE_STATE_UNINIT) {
        return;
    }

    /* 初始化设备管理信息 */
    hmac_chba_init_device_mgmt_info(info_report);

    /* vap启动后的同步处理 */
    hmac_chba_vap_start_sync_proc(info_report);

    /* 设置Hid2dModuleState状态为UP */
    hmac_chba_set_module_state(MAC_CHBA_MODULE_STATE_UP);
}

void hmac_chba_vap_stop_proc(uint8_t *notify_msg)
{
    /* 未初始化不处理 */
    if (hmac_chba_get_module_state() == MAC_CHBA_MODULE_STATE_UNINIT) {
        return;
    }

    /* 设置Hid2dModuleState状态为INIT */
    hmac_chba_set_module_state(MAC_CHBA_MODULE_STATE_INIT);

    /* 清空设备管理信息 */
    hmac_chba_clear_device_mgmt_info();

    /* 同步模块去初始化 */
    hmac_chba_sync_module_deinit();
}


void hmac_chba_add_conn_proc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user)
{
    uint8_t device_id, peer_device_id;
    hmac_chba_island_list_stru tmp_island_info = { 0 };
    mac_chba_conn_info_rpt conn_info_context;

    /* 未初始化不处理 */
    if (hmac_chba_get_module_state() == MAC_CHBA_MODULE_STATE_UNINIT) {
        return;
    }
    /* 封装下文所需关键变量 */
    oal_set_mac_addr(conn_info_context.self_mac_addr, mac_mib_get_StationID(&hmac_vap->st_vap_base_info));
    oal_set_mac_addr(conn_info_context.peer_mac_addr, hmac_user->st_user_base_info.auc_user_mac_addr);
    conn_info_context.work_channel = hmac_vap->hmac_chba_vap_info->work_channel;

    /* 建链双方分别更新活跃设备表，获取device id */
    device_id = hmac_chba_one_dev_update_alive_dev_table(conn_info_context.self_mac_addr);
    peer_device_id = hmac_chba_one_dev_update_alive_dev_table(hmac_user->st_user_base_info.auc_user_mac_addr);
    if ((device_id >= MAC_CHBA_MAX_DEVICE_NUM) || (peer_device_id >= MAC_CHBA_MAX_DEVICE_NUM)) {
        return;
    }
    hmac_chba_user_ps_info_init(hmac_user); /* 建链完成后初始化user info */

    /* 根据上报的建链信息，更新连接拓扑图和设备ID信息 */
    hmac_chba_conn_add_update_network_topo(conn_info_context, device_id, peer_device_id);
    oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_add_conn_proc::device_id %d, peerDiviceId %d",
        device_id, peer_device_id);

    /* 根据连接拓扑图更新生成岛信息 */
    hmac_chba_update_island_info(hmac_vap, &tmp_island_info);
    hmac_chba_print_island_info(&tmp_island_info);

    /* 更新本设备连接信息 */
    hmac_chba_update_self_conn_info(hmac_vap, &tmp_island_info, FALSE);
    hmac_chba_print_self_conn_info();
    /* 清空临时岛信息，释放内存 */
    hmac_chba_clear_island_info(&tmp_island_info, hmac_chba_tmp_island_list_node_free);

    /* 将岛内设备信息更新到岛内同步信息结构体中 */
    hmac_chba_update_island_sync_info();

    /* 将拓扑信息同步到DMAC */
    hmac_chba_topo_info_sync();

    /* 更新最新的Beacon和PNF到Driver */
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
}


void hmac_chba_update_topo_info_proc(hmac_vap_stru *hmac_vap)
{
    hmac_chba_island_list_stru tmp_island_info = { 0 };
    uint8_t owner_info_flag = FALSE;

    /* 根据连接拓扑图更新生成岛信息 */
    hmac_chba_update_island_info(hmac_vap, &tmp_island_info);
    hmac_chba_print_island_info(&tmp_island_info);
    /* 如果是master，进行岛主选择流程 */
    if (hmac_chba_get_role() == CHBA_MASTER) {
        hmac_chba_island_owner_selection_proc(&tmp_island_info);
        owner_info_flag = TRUE;
    }

    /* 更新本设备连接信息 */
    hmac_chba_update_self_conn_info(hmac_vap, &tmp_island_info, owner_info_flag);
    hmac_chba_print_self_conn_info();

    /* 清空临时岛信息，释放内存 */
    hmac_chba_clear_island_info(&tmp_island_info, hmac_chba_tmp_island_list_node_free);

    /* 将拓扑信息同步到DMAC */
    hmac_chba_topo_info_sync();

    /* 更新最新的Beacon和RNF到Driver */
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());

    /* 将岛内设备信息更新到岛内同步信息结构体中 */
    hmac_chba_update_island_sync_info();
}

/*
 * 功能描述  : 调试命令接口, 入参为device type。
 */
uint32_t hmac_chba_module_init_cmd(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    uint8_t device_type;
    device_type = (uint8_t)params[0];
    hmac_chba_module_init(device_type);
    return OAL_SUCC;
}
/* 功能描述  : 调试命令接口, 入参为电量. */
uint32_t hmac_config_chba_set_battery(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    uint8_t percent;
    percent = (uint8_t)params[0];
    hmac_chba_battery_update_proc(percent);
    return OAL_SUCC;
}

void hmac_chba_update_network_topo(uint8_t *own_mac_addr, uint8_t *peer_mac_addr)
{
    uint8_t device_id;
    uint8_t peer_device_id;

    /* 删链双方分别根据mac地址从哈希表获取device id */
    device_id = hmac_chba_one_dev_update_alive_dev_table(own_mac_addr);
    peer_device_id = hmac_chba_one_dev_update_alive_dev_table(peer_mac_addr);
    /* 无效的device id直接返回 */
    if ((device_id >= MAC_CHBA_MAX_DEVICE_NUM) || (peer_device_id >= MAC_CHBA_MAX_DEVICE_NUM)) {
        return;
    }

    /* 更新连接拓扑图 */
    hmac_chba_set_network_topo_ele_false(device_id, peer_device_id);
    hmac_chba_set_network_topo_ele_false(peer_device_id, device_id);
}
#endif
