

/* 1 ͷ�ļ����� */
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
/* 2 ȫ�ֱ������� */
/* CHBA���豸Ӳ����Ϣ */
hmac_chba_device_para_stru g_chba_device_info = {
    .device_type = DEVICE_TYPE_HANDSET,
    .remain_power = BATTERY_LEVEL_MIDDLE,
    .hardwire_cap = HW_CAP_SINGLE_CHIP,
    .drv_band_cap = BAND_CAP_5G_ONLY,
    .config_para = { MAC_CHBA_VERSION_2, MAC_CHBA_SLOT_DURATION, MAC_CHBA_SCHEDULE_PERIOD, MAC_CHBA_SYNC_SLOT_CNT,
        MAC_CHBA_INIT_LISTEN_PERIOD, MAC_CHBA_VAP_ALIVE_TIMEOUT, MAC_CHBA_LINK_ALIVE_TIMEOUT },
};

/* CHBA������Ϣ */
hmac_chba_mgmt_info_stru g_chba_mgmt_info = {
    .chba_module_state = MAC_CHBA_MODULE_STATE_UNINIT,
    .alive_device_table = NULL,
};

/* 3 �������� */
uint8_t hmac_chba_one_dev_update_alive_dev_table(const uint8_t* mac_addr);

/* 4 ����ʵ�� */
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

    /* ��������ͼ��Ӧdevice_id������0 */
    network_topo_row = hmac_chba_get_network_topo_row(device_id);
    memset_s(network_topo_row, sizeof(uint32_t) * MAC_CHBA_MAX_BITMAP_WORD_NUM,
        0, sizeof(uint32_t) * MAC_CHBA_MAX_BITMAP_WORD_NUM);

    /* ��������ͼ��Ӧdevice_id������0 */
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

    /* �ѵ�����ڵ㣬���� */
    if (parent[i] == i) {
        return i;
    }

    /* �����ȡ���ڵ�ĸ��ڵ� */
    return hmac_chba_get_root(parent, parent[i]);
}


void hmac_chba_clear_island_info(hmac_chba_island_list_stru *island_info, hi_node_func node_deinit)
{
    if (island_info == NULL) {
        return;
    }

    /* �ͷŵ��ڵ� */
    if (hmac_chba_get_module_state() >= MAC_CHBA_MODULE_STATE_INIT) {
        hi_list_deinit(&island_info->island_list, node_deinit);
    }

    /* ��ʼ������Ϣ���� */
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

    /* ��д��id, �����豸���������豸�б� */
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

    /* ����������ͼ��ɾ�����豸������Ϣ */
    hmac_chba_clear_one_dev_from_network_topo(device_id);
    /* �ӻ�Ծ��Ϣ�б��в��Ҹýڵ� */
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
    /* ͬ��device id��DMAC����ɾ�� */
    hmac_chba_device_info_sync(CHBA_DEVICE_ID_DEL, device_id, cur_device_info->mac_addr);
    /* �����device id�ķ�����Ϣ */
    memset_s(cur_device_info, sizeof(hmac_chba_per_device_id_info_stru), 0, sizeof(hmac_chba_per_device_id_info_stru));
    /* �ӻ�Ծ�豸��ϣ����ɾ�����豸 */
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

    /* �����ڴ� */
    for (i = 0; i < MAC_CHBA_MAX_DEVICE_NUM; i++) {
        device_list[i] = (uint8_t *)oal_memalloc(MAC_CHBA_MAX_ISLAND_DEVICE_NUM * sizeof(uint8_t));
        if (device_list[i] == NULL) {
            oam_warning_log0(0, OAM_SF_CHBA, "{hmac_chba_generate_island_info::memalloc failed.}");
            hmac_chba_free_device_list(device_list, MAC_CHBA_MAX_DEVICE_NUM);
            return;
        }
        memset_s(device_list[i], MAC_CHBA_MAX_ISLAND_DEVICE_NUM, 0, MAC_CHBA_MAX_ISLAND_DEVICE_NUM);
    }

    /* ͳ��ÿ�����ڵ��µ��豸�б� */
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

    /* ���ɵ���Ϣ */
    for (i = 0; i < MAC_CHBA_MAX_DEVICE_NUM; i++) {
        /* ���������豸���㵺, ���ڵ���Ϊ��id */
        if (device_cnt[i] > 1) {
            hmac_chba_add_island_to_list(island_info, i, device_cnt[i], &device_list[i][0]);
        }
    }

    /* �ͷ��ڴ� */
    hmac_chba_free_device_list(device_list, MAC_CHBA_MAX_DEVICE_NUM);
}


static void hmac_chba_del_other_island_device(uint8_t *parent, uint8_t chba_device_num)
{
    uint8_t device_idx;
    uint8_t self_island_id;
    uint8_t island_id;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    /* ��ȡ����id, ������device_id��parent�����ж�Ӧ�ĸ��ڵ� */
    self_island_id = hmac_chba_get_root(parent, self_conn_info->self_device_id);
    for (device_idx = 0; device_idx < chba_device_num; device_idx++) {
        island_id = hmac_chba_get_root(parent, device_idx);
        /* ��id������id��ͬ����ɾ���õ���Ϣ */
        if (island_id != self_island_id) {
            hmac_chba_del_one_device_info(device_idx); /* ɾ����device */
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

    /* �����������ˣ�ʹ�ò��鼯����parent���� */
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
    /* ��ʼ������Ϣ���� */
    hi_list_init(&island_info->island_list);
    island_info->island_cnt = 0;

    /* ����topo��Ϣ���ɲ��鼯���ڵ����� */
    hmac_chba_generate_island_parent_info(parent, MAC_CHBA_MAX_DEVICE_NUM);

    /* ����֧��һ��ൺ */
    if (chba_vap_info->is_support_multiple_island == OAL_FALSE) {
        /* ��topoͼ���Ծ��Ϣ�б���ɾ���������豸 */
        hmac_chba_del_other_island_device(parent, MAC_CHBA_MAX_DEVICE_NUM);
        /* ������topoͼ��������parent���� */
        hmac_chba_generate_island_parent_info(parent, MAC_CHBA_MAX_DEVICE_NUM);
    }
    /* ���ݲ��鼯������ɵ���Ϣ */
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

    /* ��д����mac��ַ */
    if (owner_valid == TRUE) {
        /* �豸�б��У��������ڵ�һ��λ�� */
        device_info = &self_conn_info->island_device_list[0];
        conn_flag = hmac_chba_get_network_topo_ele(self_conn_info->self_device_id, owner_device_id);
        hmac_chba_fill_one_device_info(device_info, &device_id_info[owner_device_id], conn_flag);
        curr_idx++;
    }

    /* �豸�б��У����뵺�����Լ��������豸 */
    for (i = 0; i < curr_island->device_cnt; i++) {
        device_id = curr_island->device_list[i];
        /* ���Լ��������豸 */
        if (hmac_chba_get_network_topo_ele(self_conn_info->self_device_id, device_id) == TRUE) {
            /* �����Ѿ��Ź��ˣ����� */
            if ((owner_valid == TRUE) && (device_id == owner_device_id)) {
                continue;
            }
            device_info = &self_conn_info->island_device_list[curr_idx];
            hmac_chba_fill_one_device_info(device_info, &device_id_info[device_id], TRUE);
            curr_idx++;
        }
    }

    /* �豸�б��У����뵺�����Լ����������豸 */
    for (i = 0; i < curr_island->device_cnt; i++) {
        device_id = curr_island->device_list[i];
        /* ���Լ����������豸 */
        if (hmac_chba_get_network_topo_ele(self_conn_info->self_device_id, device_id) == FALSE) {
            /* �����Ѿ��Ź��ˣ����� */
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

    /* ��д����mac��ַ */
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

    /* ��д���豸������Ϣ�еĵ��豸�б� */
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

    /* ���豸������Ϣ�е�����Ч */
    if (self_conn_info->island_owner_valid == TRUE) {
        owner_device_id = hmac_chba_one_dev_update_alive_dev_table(self_conn_info->island_owner_addr);
        /* ���ҵ����Ƿ��ڵ�ǰ����ĵ��豸�б��� */
        for (i = 0; i < device_cnt; i++) {
            if (owner_device_id == curr_island->device_list[i]) {
                break;
            }
        }

        /* ����������ڵ�ǰ����ĵ��豸�б��У���������Ϣ��Ϊ��Ч */
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

    /* ����Ϣ�а���������Ϣ */
    if (owner_info_flag == TRUE) {
        hmac_chba_fill_self_conn_info_with_owner(curr_island, self_conn_info);
    } else { /* ����Ϣ�в�����������Ϣ */
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
    /* ����netbuf�ڴ� */
    conn_info_netbuf = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF,
        sizeof(mac_chba_self_conn_info_stru), OAL_NETBUF_PRIORITY_MID);
    if (conn_info_netbuf == NULL) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_sync_self_conn_info::alloc conn_info_netbuf failed.}");
        return;
    }

    /***************************************************************************
        ���¼���DMAC��, ͬ��DMAC����
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

    /* �ӵ���Ϣ��ȡ���豸���������豸�б� */
    hi_list_for_each_safe(node, tmp, &tmp_island_info->island_list) {
        chba_node = hi_node_entry(node, hmac_chba_tmp_island_list_node_stru, node);
        curr_island = &chba_node->data;
        for (j = 0; j < curr_island->device_cnt; j++) {
            /* �ҵ����豸������ */
            if (self_device_id == curr_island->device_list[j]) {
                find_island_flag = TRUE;
                hmac_chba_fill_self_conn_info(curr_island, self_conn_info, owner_info_flag);
                break;
            }
        }
    }

    /* �Ҳ������豸����������usercntΪ0����ձ��豸����Ϣ */
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

    /* ��ϣ����ʧ�ܣ�ֱ�ӷ��� */
    if (ht == NULL) {
        return UINT8_INVALID_VALUE;
    }

    /* ������Ծ�豸���ҵ����Լ��ⲻ��Ծʱ�������豸���ӻ�Ծ�豸����ɾ�� */
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
            /* ����������ͼ��ɾ�����豸������Ϣ */
            hmac_chba_clear_one_dev_from_network_topo(device_id);
            /* ͬ��device id��DMAC */
            hmac_chba_device_info_sync(CHBA_DEVICE_ID_DEL, device_id, device_id_info[device_id].mac_addr);
            /* �����device id�ķ�����Ϣ */
            memset_s(&device_id_info[device_id], sizeof(hmac_chba_per_device_id_info_stru),
                0, sizeof(hmac_chba_per_device_id_info_stru));
            /* ��������Ϣͬ����DMAC */
            hmac_chba_topo_info_sync();
        }

        /* �ӻ�Ծ�豸��ϣ����ɾ�����豸 */
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

    /* ��ϣ����ʧ�ܣ�ֱ�ӷ��� */
    if (oal_any_null_ptr2(mac_addr, ht)) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_one_dev_update_alive_dev_table: null");
        return UINT8_INVALID_VALUE;
    }

    /* �����ַȫ0��ֱ�ӷ��� */
    if (oal_compare_mac_addr(mac_addr, all_zero_addr) == 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_one_dev_update_alive_dev_table: mac addr all zero");
        return UINT8_INVALID_VALUE;
    }

    /* ����mac��ַ�ڻ�Ծ�豸���в��� */
    oal_set_mac_addr(cmp.mac_addr, mac_addr);
    node = hi_hash_find(&ht->raw, &cmp.node);
    /* �ҵ����豸������lastAliveTime */
    if (node != NULL) {
        chba_node = hi_node_entry(node, hmac_chba_hash_node_stru, node);
        chba_node->last_alive_time = (uint32_t)oal_time_get_stamp_ms();
        device_id = chba_node->device_id;
        return device_id;
    }

    /* �Ҳ������豸�������豸�����ϣ�� */
    chba_node = (hmac_chba_hash_node_stru*)oal_memalloc(sizeof(hmac_chba_hash_node_stru));
    if (chba_node == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_one_dev_update_alive_dev_table: malloc fail");
        return UINT8_INVALID_VALUE;
    }

    /* ����device id */
    device_id = hmac_chba_alloc_device_id();
    /* device id����ʧ�� */
    if (device_id == UINT8_INVALID_VALUE) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_one_dev_update_alive_dev_table: alloc device id fail");
        /* �ӹ�ϣ����ɾ������Ծʱ������豸��ռ����device id */
        device_id = hmac_chba_del_longest_unactive_dev(ht);
        if (device_id == UINT8_INVALID_VALUE) {
            oal_free(chba_node);
            return UINT8_INVALID_VALUE;
        }
    }

    /* ��д�ڵ���Ϣ */
    oal_set_mac_addr(chba_node->mac_addr, mac_addr);

    /* ����device id��� */
    device_id_info[device_id].use_flag = TRUE;

    chba_node->device_id = device_id;
    chba_node->last_alive_time = (uint32_t)oal_time_get_stamp_ms();

    /* �����豸�����ϣ�� */
    hi_hash_add(&ht->raw, &chba_node->node);
    ht->node_cnt++;
    oam_warning_log3(0, OAM_SF_CHBA, "update_alive_dev_table: add new node,mac[0x%x:%x], deviceid[%d]",
        mac_addr[MAC_ADDR_4], mac_addr[MAC_ADDR_5], device_id);
    /* ͬ��device id��DMAC */
    hmac_chba_device_info_sync(CHBA_DEVICE_ID_ADD, device_id, mac_addr);

    return device_id;
}


void hmac_chba_conn_add_update_network_topo(mac_chba_conn_info_rpt conn_info_rpt,
    uint8_t device_id, uint8_t peer_device_id)
{
    /* ������������ͼ */
    hmac_chba_set_network_topo_ele_true(device_id, peer_device_id);
    hmac_chba_set_network_topo_ele_true(peer_device_id, device_id);

    /* �����豸ID��Ϣ */
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

    /* ����mac��ַ�ڻ�Ծ�豸���в��� */
    oal_set_mac_addr(cmp.mac_addr, device_rpt->mac_addr);
    node = hi_hash_find(&ht->raw, &cmp.node);
    /* �Ҳ������豸�������豸�����ϣ�� */
    if (node == NULL) {
        oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_del_non_alive_device: can't find device XX:XX:XX:%2X:%2X:%2X",
            device_rpt->mac_addr[MAC_ADDR_3], device_rpt->mac_addr[MAC_ADDR_4], device_rpt->mac_addr[MAC_ADDR_5]);
        return OAL_SUCC;
    }

    chba_node = hi_node_entry(node, hmac_chba_hash_node_stru, node);
    device_id = chba_node->device_id;
    /* �쳣ά���ӡ����Ӧ�ó���ͬһ��MAC��ַ��device id��һ�� */
    if (device_id != device_rpt->device_id) {
        oam_error_log2(0, OAM_SF_CHBA, "hmac_chba_del_non_alive_device: device id not match %d %d",
            device_id, device_rpt->device_id);
    }

    if ((device_id < MAC_CHBA_MAX_DEVICE_NUM) && (device_id != self_conn_info->self_device_id)) {
        /* ɾ���豸��Ϣ */
        hmac_chba_del_one_device_info(device_id);
        /* ��������Ϣͬ����DMAC */
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

    /* �ϱ��ϲ�HAL */
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

    /* ����Link Info Attribute */
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

    /* ��֡���ͷ����뵽��Ծ�豸�� */
    device_id = hmac_chba_one_dev_update_alive_dev_table(peer_addr);
    /* ��Ч��device idֱ�ӷ��� */
    if (device_id >= MAC_CHBA_MAX_DEVICE_NUM) {
        return UINT8_INVALID_VALUE;
    }

    /* ����֡���ͷ����豸ID��Ϣ */
    hmac_chba_update_device_id_info(device_id, peer_addr, chan_number, bandwidth);

    /* ����������ͼ������ո�device_id������Ϣ */
    hmac_chba_clear_one_dev_from_network_topo(device_id);
    /* ����֡���ͷ����ӵ�ÿ���豸���뵽��Ծ�豸�� */
    for (i = 0; i < *linkcnt; i++) {
        conn_device_id = hmac_chba_one_dev_update_alive_dev_table(&data[offset]);
        /* ��Ч��device id������ */
        if (conn_device_id >= MAC_CHBA_MAX_DEVICE_NUM) {
            continue;
        }
        /* ���conn_device_idΪ�Լ�������sa���Լ������û���˵���Ѿ�ȥ���������ٸ����������� */
        if ((conn_device_id == hmac_chba_get_own_device_id()) && (sa_is_my_user == OAL_FALSE)) {
            continue;
        }
        /* ������������ͼ */
        hmac_chba_set_network_topo_ele_true(device_id, conn_device_id);
        hmac_chba_set_network_topo_ele_true(conn_device_id, device_id);

        /* �����豸ID��Ϣ */
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
    /* ���ݸ�֡payload�����豸������Ϣ */
    device_id = hmac_chba_update_dev_mgmt_info_by_frame(payload, payload_len, sa_addr, &rssi, &linkcnt, is_my_user);

    /* �����͹������� */
    power_save_bitmap = hmac_chba_decap_power_save_attr(payload, payload_len);

    /* ����͹���bitmap */
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

    /* ��ϣ����ʧ�ܣ�ֱ�ӷ��� */
    if (alive_dev_table == NULL) {
        return;
    }

    /* ��ջ�Ծ�豸�� */
    hmac_chba_hash_clear(alive_dev_table);

    /* ��յ���Ϣ */
    hmac_chba_clear_island_info(island_info, hmac_chba_island_list_node_free);

    /* ����豸������Ϣ */
    memset_s(network_topo_addr, sizeof(uint32_t) * (MAC_CHBA_MAX_DEVICE_NUM * MAC_CHBA_MAX_BITMAP_WORD_NUM),
        0, sizeof(uint32_t) * (MAC_CHBA_MAX_DEVICE_NUM * MAC_CHBA_MAX_BITMAP_WORD_NUM));
    memset_s(device_id_info, sizeof(hmac_chba_per_device_id_info_stru) * MAC_CHBA_MAX_DEVICE_NUM,
        0, sizeof(hmac_chba_per_device_id_info_stru) * MAC_CHBA_MAX_DEVICE_NUM);
    memset_s(self_conn_info, sizeof(mac_chba_self_conn_info_stru), 0, sizeof(mac_chba_self_conn_info_stru));
    /* ��ʼ���豸��ɫ */
    self_conn_info->role = CHBA_OTHER_DEVICE;

    mgmt_info->island_owner_attr_len = 0;
    memset_s(mgmt_info->island_owner_attr_buf, MAC_CHBA_MGMT_MID_PAYLOAD_BYTES, 0, MAC_CHBA_MGMT_MID_PAYLOAD_BYTES);

    /* ��������Ϣͬ����DMAC */
    hmac_chba_topo_info_sync();
    /* ͬ��device id��DMAC */
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
        /* ����Ҫ�ų����豸 */
        if (device_id == exclude_device_id) {
            continue;
        }

        ps_bitmap = device_id_info[device_id].ps_bitmap;
        /* ��������״̬���豸 */
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

    /* �����ǰ���Ƿ�Ϊmaster���ڵ�(��ʱ���豸����master)����masterѡΪ���� */
    if (hmac_chba_is_in_device_list(master_device_id, curr_tmp_island->device_cnt, curr_tmp_island->device_list)) {
        curr_tmp_island->owner_device_id = master_device_id;
        return;
    }

    oam_error_log1(0, OAM_SF_CHBA,
        "{hmac_chba_update_new_island_owner:find other island with %d devices}", curr_tmp_island->device_cnt);
    /* �ҵ����ڷǱ���̬pnf_rssi���Ľڵ㣬ѡΪ���� */
    hmac_chba_find_max_pnf_rssi_node(curr_tmp_island, UINT8_INVALID_VALUE, &max_pnf_rssi, &max_rssi_device_id);

    /* �Ҳ������������ģ���ѡ��һ�� */
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

    /* �ж��豸�б��Ƿ�仯������change_flag */
    if ((curr_tmp_island->device_cnt == curr_island->device_cnt) &&
        (oal_memcmp(curr_tmp_island->device_list, curr_island->device_list, curr_tmp_island->device_cnt) == 0)) {
        curr_tmp_island->change_flag = FALSE;
    } else {
        curr_tmp_island->change_flag = TRUE;
    }

    island_owner_device_id = curr_island->owner_device_id;
    curr_tmp_island->owner_valid = TRUE;

    /* �����ǰ��Ϊmaster���ڵ�����masterѡΪ���� */
    if (hmac_chba_is_in_device_list(master_device_id, curr_tmp_island->device_cnt, curr_tmp_island->device_list)) {
        curr_tmp_island->owner_device_id = master_device_id;
        curr_tmp_island->pnf_loss_cnt = 0;
        return;
    }

    /* �ҵ����ڳ�ԭ�����⣬�Ǳ���̬pnf_rssi���Ľڵ� */
    hmac_chba_find_max_pnf_rssi_node(curr_tmp_island, island_owner_device_id, &max_pnf_rssi, &max_rssi_device_id);

    /* �Ҳ�������ά��ԭ���� */
    if (max_rssi_device_id == UINT8_INVALID_VALUE) {
        curr_tmp_island->owner_device_id = island_owner_device_id;
        curr_tmp_island->pnf_loss_cnt = curr_island->pnf_loss_cnt;
        return;
    }

    /* ������pnf����û�г������� */
    if (curr_island->pnf_loss_cnt < MAC_CHBA_ISLAND_OWNER_PNF_LOSS_TH) {
        island_owner_rssi = device_id_info[island_owner_device_id].pnf_rssi;
        island_owner_ps_bitmap = device_id_info[island_owner_device_id].ps_bitmap;
        /* ����������� */
        island_owner_change = (max_pnf_rssi > island_owner_rssi + MAC_CHBA_ISLAND_OWNER_CHANGE_TH) ||
            (island_owner_ps_bitmap == CHBA_PS_KEEP_ALIVE_BITMAP);
        /* ������㵺���������������ԭ������rssi���ķǱ����豸ѡΪ���� */
        if (island_owner_change == TRUE) {
            curr_tmp_island->owner_device_id = max_rssi_device_id;
            curr_tmp_island->pnf_loss_cnt = 0;
            oam_warning_log2(0, 0, "CHBA: hmac_chba_update_exist_island_owner: change owner from %d to %d",
                island_owner_device_id, curr_tmp_island->owner_device_id);
        } else { /* ����ά��ԭ���� */
            curr_tmp_island->owner_device_id = island_owner_device_id;
            curr_tmp_island->pnf_loss_cnt = curr_island->pnf_loss_cnt;
        }
    } else { /* ������pnf�����������ޣ�����ԭ������rssi���ķǱ����豸ѡΪ���� */
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
    /* 1�ֽ�island owner cnt */
    index++;

    hi_list_for_each_safe(chba_node, tmp, &tmp_island_info->island_list) {
        tmp_island_node = hi_node_entry(chba_node, hmac_chba_tmp_island_list_node_stru, node);
        tmp_island = &tmp_island_node->data;
        oal_set_mac_addr(&attr_buf[index], device_id_info[tmp_island->owner_device_id].mac_addr);
        index += OAL_MAC_ADDR_LEN;
        island_owner_cnt++;
    }

    /* ����Attr Length�ֶ� */
    attr_buf[MAC_CHBA_ATTR_ID_LEN] = index - MAC_CHBA_ATTR_HDR_LEN;
    /* ����island owner cnt�ֶ� */
    attr_buf[MAC_CHBA_ATTR_HDR_LEN] = island_owner_cnt;

    /* ����attr_buf�ĳ��� */
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

    /* ��д��id, �����豸���������豸�б� */
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

    /* �����ʷ����Ϣ���� */
    hmac_chba_clear_island_info(island_info, hmac_chba_island_list_node_free);

    hi_list_for_each_safe(chba_node, tmp, &tmp_island_info->island_list) {
        tmp_island_node = hi_node_entry(chba_node, hmac_chba_tmp_island_list_node_stru, node);
        tmp_island = &tmp_island_node->data;

        /* ��ӵ���Ϣ�ڵ㵽��ʷ����Ϣ���� */
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

    /* ��ʱ�Լ�����master */
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
            /* ��ʷ�������е�ĳ�������ڵ�ǰ�����豸�б��� */
            if (is_in_island == TRUE) {
                find_flag = TRUE;
                break;
            }
        }

        if (find_flag == TRUE) { /* �õ���һ���Ѿ����ڵĵ� */
            hmac_chba_update_exist_island_owner(tmp_island, curr_island, master_device_id);
        } else { /* �õ���һ���µĵ� */
            hmac_chba_update_new_island_owner(tmp_island, master_device_id);
            oam_warning_log1(0, 0, "CHBA: hmac_chba_island_owner_selection_proc: new island, owner id %d",
                tmp_island->owner_device_id);
        }
    }

    /* ��װ�������Ա��� */
    hmac_chba_encap_island_owner_attr(tmp_island_info);

    /* �ѵ�ǰ����Ϣ����ˢ�µ���ʷ����Ϣ������ */
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
    /* 1�ֽ�island owner cnt */
    island_cnt = attr_pos[index];
    if (island_cnt > MAC_CHBA_MAX_ISLAND_NUM) {
        return UINT8_INVALID_VALUE;
    }
    index++;
    for (island_idx = 0; island_idx < island_cnt; island_idx++) {
        /* ����ҵ�������ַ�ڱ��豸���������豸�б��У�������Ϊ���豸�ĵ��� */
        if (hmac_chba_island_device_check(&attr_pos[index]) == TRUE) {
            /* �����仯ʱά���ӡ */
            if (oal_compare_mac_addr(self_conn_info->island_owner_addr, &attr_pos[index]) != 0) {
                oam_warning_log3(0, 0, "rx_island_owner_attr::old owner XX:XX:XX:%2X:%2X:%2X",
                    self_conn_info->island_owner_addr[3], self_conn_info->island_owner_addr[4],
                    self_conn_info->island_owner_addr[5]);
                oam_warning_log3(0, 0, "rx_island_owner_attr::new owner XX:XX:XX:%2X:%2X:%2X",
                    attr_pos[index + 3], attr_pos[index + 4], attr_pos[index + 5]);
            }
            oal_set_mac_addr(self_conn_info->island_owner_addr, &attr_pos[index]);
            self_conn_info->island_owner_valid = TRUE;
            /* �������豸��Ϣ���µ�����ͬ����Ϣ�ṹ���� */
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

    /* ������Ծ�豸��ϣ�� */
    if (alive_dev_table == NULL) {
        alive_dev_table = hmac_chba_hash_creat();
        /* ����ʧ�ܷ��� */
        if (alive_dev_table == NULL) {
            oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_module_init::creat hash fail");
            return;
        }
        /* һ��Ҫ��ָ��ȫ�ֱ�����ֵ���� */
        g_chba_mgmt_info.alive_device_table = alive_dev_table;
    } else { /* ����ǿգ���չ�ϣ�� */
        hmac_chba_hash_clear(alive_dev_table);
    }

    /* ��յ���Ϣ */
    hmac_chba_clear_island_info(island_info, hmac_chba_island_list_node_free);

    /* ��ʼ���豸������Ϣ */
    memset_s(network_topo_addr, sizeof(uint32_t) * (MAC_CHBA_MAX_DEVICE_NUM * MAC_CHBA_MAX_BITMAP_WORD_NUM),
        0, sizeof(uint32_t) * (MAC_CHBA_MAX_DEVICE_NUM * MAC_CHBA_MAX_BITMAP_WORD_NUM));
    memset_s(device_id_info, sizeof(hmac_chba_per_device_id_info_stru) * MAC_CHBA_MAX_DEVICE_NUM,
        0, sizeof(hmac_chba_per_device_id_info_stru) * MAC_CHBA_MAX_DEVICE_NUM);
    memset_s(self_conn_info, sizeof(mac_chba_self_conn_info_stru), 0, sizeof(mac_chba_self_conn_info_stru));
    /* ��ʼ���豸��ɫ */
    self_conn_info->role = CHBA_OTHER_DEVICE;

    /* ��ȡϵͳ��������ʼ���豸���� */
    device_info->device_type = device_type;
    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_module_init::device type %d", device_info->device_type);

    /* ����״̬Ϊ�ѳ�ʼ�� */
    hmac_chba_set_module_state(MAC_CHBA_MODULE_STATE_INIT);

    /* ͬ��ģ���ʼ�� */
    hmac_chba_sync_module_init(device_info);

    /* �·�CHBA��ʼ��������driver */
    init_para.config_para = device_info->config_para;
    /* ��װ���豸ѡ������ */
    hmac_chba_set_master_election_attr(init_para.master_election_attr);
    /* ��ʼ��CHBA ��Ϣ */
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

    /* �������µ�Beacon��RNF��Driver */
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
}



static void hmac_chba_init_device_mgmt_info(mac_chba_vap_start_rpt *info_report)
{
    int32_t ret;
    uint8_t device_id;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    /* ����豸������Ϣ */
    hmac_chba_clear_device_mgmt_info();

    /* ���豸�����Ծ�豸������device id */
    device_id = hmac_chba_one_dev_update_alive_dev_table(info_report->mac_addr);
    if (device_id >= MAC_CHBA_MAX_DEVICE_NUM) {
        oam_error_log4(0, OAM_SF_CHBA, "hmac_chba_init_device_mgmt_info: addr XX:XX:XX:%2X:%2X:%2X, wrong device_id %d",
            info_report->mac_addr[3], info_report->mac_addr[4], info_report->mac_addr[5], device_id);
        return;
    }
    /* ���±��豸������Ϣ */
    self_conn_info->self_device_id = device_id;
    /* �����豸ID��Ϣ */
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

    /* δ��ʼ�������� */
    if (hmac_chba_get_module_state() == MAC_CHBA_MODULE_STATE_UNINIT) {
        return;
    }

    /* ��ʼ���豸������Ϣ */
    hmac_chba_init_device_mgmt_info(info_report);

    /* vap�������ͬ������ */
    hmac_chba_vap_start_sync_proc(info_report);

    /* ����Hid2dModuleState״̬ΪUP */
    hmac_chba_set_module_state(MAC_CHBA_MODULE_STATE_UP);
}

void hmac_chba_vap_stop_proc(uint8_t *notify_msg)
{
    /* δ��ʼ�������� */
    if (hmac_chba_get_module_state() == MAC_CHBA_MODULE_STATE_UNINIT) {
        return;
    }

    /* ����Hid2dModuleState״̬ΪINIT */
    hmac_chba_set_module_state(MAC_CHBA_MODULE_STATE_INIT);

    /* ����豸������Ϣ */
    hmac_chba_clear_device_mgmt_info();

    /* ͬ��ģ��ȥ��ʼ�� */
    hmac_chba_sync_module_deinit();
}


void hmac_chba_add_conn_proc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user)
{
    uint8_t device_id, peer_device_id;
    hmac_chba_island_list_stru tmp_island_info = { 0 };
    mac_chba_conn_info_rpt conn_info_context;

    /* δ��ʼ�������� */
    if (hmac_chba_get_module_state() == MAC_CHBA_MODULE_STATE_UNINIT) {
        return;
    }
    /* ��װ��������ؼ����� */
    oal_set_mac_addr(conn_info_context.self_mac_addr, mac_mib_get_StationID(&hmac_vap->st_vap_base_info));
    oal_set_mac_addr(conn_info_context.peer_mac_addr, hmac_user->st_user_base_info.auc_user_mac_addr);
    conn_info_context.work_channel = hmac_vap->hmac_chba_vap_info->work_channel;

    /* ����˫���ֱ���»�Ծ�豸����ȡdevice id */
    device_id = hmac_chba_one_dev_update_alive_dev_table(conn_info_context.self_mac_addr);
    peer_device_id = hmac_chba_one_dev_update_alive_dev_table(hmac_user->st_user_base_info.auc_user_mac_addr);
    if ((device_id >= MAC_CHBA_MAX_DEVICE_NUM) || (peer_device_id >= MAC_CHBA_MAX_DEVICE_NUM)) {
        return;
    }
    hmac_chba_user_ps_info_init(hmac_user); /* ������ɺ��ʼ��user info */

    /* �����ϱ��Ľ�����Ϣ��������������ͼ���豸ID��Ϣ */
    hmac_chba_conn_add_update_network_topo(conn_info_context, device_id, peer_device_id);
    oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_add_conn_proc::device_id %d, peerDiviceId %d",
        device_id, peer_device_id);

    /* ������������ͼ�������ɵ���Ϣ */
    hmac_chba_update_island_info(hmac_vap, &tmp_island_info);
    hmac_chba_print_island_info(&tmp_island_info);

    /* ���±��豸������Ϣ */
    hmac_chba_update_self_conn_info(hmac_vap, &tmp_island_info, FALSE);
    hmac_chba_print_self_conn_info();
    /* �����ʱ����Ϣ���ͷ��ڴ� */
    hmac_chba_clear_island_info(&tmp_island_info, hmac_chba_tmp_island_list_node_free);

    /* �������豸��Ϣ���µ�����ͬ����Ϣ�ṹ���� */
    hmac_chba_update_island_sync_info();

    /* ��������Ϣͬ����DMAC */
    hmac_chba_topo_info_sync();

    /* �������µ�Beacon��PNF��Driver */
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
}


void hmac_chba_update_topo_info_proc(hmac_vap_stru *hmac_vap)
{
    hmac_chba_island_list_stru tmp_island_info = { 0 };
    uint8_t owner_info_flag = FALSE;

    /* ������������ͼ�������ɵ���Ϣ */
    hmac_chba_update_island_info(hmac_vap, &tmp_island_info);
    hmac_chba_print_island_info(&tmp_island_info);
    /* �����master�����е���ѡ������ */
    if (hmac_chba_get_role() == CHBA_MASTER) {
        hmac_chba_island_owner_selection_proc(&tmp_island_info);
        owner_info_flag = TRUE;
    }

    /* ���±��豸������Ϣ */
    hmac_chba_update_self_conn_info(hmac_vap, &tmp_island_info, owner_info_flag);
    hmac_chba_print_self_conn_info();

    /* �����ʱ����Ϣ���ͷ��ڴ� */
    hmac_chba_clear_island_info(&tmp_island_info, hmac_chba_tmp_island_list_node_free);

    /* ��������Ϣͬ����DMAC */
    hmac_chba_topo_info_sync();

    /* �������µ�Beacon��RNF��Driver */
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());

    /* �������豸��Ϣ���µ�����ͬ����Ϣ�ṹ���� */
    hmac_chba_update_island_sync_info();
}

/*
 * ��������  : ��������ӿ�, ���Ϊdevice type��
 */
uint32_t hmac_chba_module_init_cmd(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    uint8_t device_type;
    device_type = (uint8_t)params[0];
    hmac_chba_module_init(device_type);
    return OAL_SUCC;
}
/* ��������  : ��������ӿ�, ���Ϊ����. */
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

    /* ɾ��˫���ֱ����mac��ַ�ӹ�ϣ���ȡdevice id */
    device_id = hmac_chba_one_dev_update_alive_dev_table(own_mac_addr);
    peer_device_id = hmac_chba_one_dev_update_alive_dev_table(peer_mac_addr);
    /* ��Ч��device idֱ�ӷ��� */
    if ((device_id >= MAC_CHBA_MAX_DEVICE_NUM) || (peer_device_id >= MAC_CHBA_MAX_DEVICE_NUM)) {
        return;
    }

    /* ������������ͼ */
    hmac_chba_set_network_topo_ele_false(device_id, peer_device_id);
    hmac_chba_set_network_topo_ele_false(peer_device_id, device_id);
}
#endif
