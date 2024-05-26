/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: 哈希表
 * Create: 2019-11-11
 * Version: V1.5
 * Contact: 有任何问题或建议，请前往
 *          http://rnd-isource.huawei.com/bugs/hw-cstl
 */

#ifndef HIHASH_H
#define HIHASH_H

#include "oam_ext_if.h"

/* 基于链表，链地址法 */
#include "hilist.h"

/*
 * 用户定义，判断两个节点是否相同
 * 入参是 hi_node 指针！可使用 NODE_ENTRY 获取节点数据
 */
typedef bool (*hi_hash_equal_func)(const struct hi_node *a, const struct hi_node *b);

/*
 * 用户定义，hash key 生成函数，用于索引 hash 桶
 * 返回值不应超过 hash 桶大小限制！
 * 入参是 hi_node 指针！可使用 NODE_ENTRY 获取节点数据
 */
typedef size_t (*hi_hash_key_func)(const struct hi_node *node, size_t bkt_size);

/*
 * Thomas Wong 整数散列函数
 * 你可以按需使用，并实现你的 hi_hash_key_func
 * 注意：返回值或超过哈希桶大小，需要进一步处理！
 */
static inline size_t hi_tw_int_hash(unsigned int key)
{
    key = ~key + (key << 15);
    key = key ^ (key >> 12);
    key = key + (key << 2);
    key = key ^ (key >> 4);
    key = (key + (key << 3)) + (key << 11);
    key = key ^ (key >> 16);
    return (size_t)key;
}

/*
 * BKDR 字符串散列函数
 * 你可以按需使用，并实现你的 hi_hash_key_func
 * 注意：返回值或超过哈希桶大小，需要进一步处理！
 */
static inline size_t hi_bkdr_hash(const char *str)
{
    size_t hash = 0;
    register size_t ch = 0;
    while ((ch = (size_t)(*str++)) != '\0') {
        hash = (hash << 7) + (hash << 1) + hash + ch;
    }
    return hash;
}

struct hi_hash_table {
    size_t bkt_size;
    hi_hash_equal_func equal;
    hi_hash_key_func key;
    struct hi_list *bkts;
};

/* 成功返回 0；失败返回 -1 */
static inline int hi_hash_init(struct hi_hash_table *ht,
                               size_t bkt_size,
                               hi_hash_equal_func equal,
                               hi_hash_key_func key)
{
    size_t i;
    ht->bkt_size = bkt_size;
    ht->equal = equal;
    ht->key = key;
    ht->bkts = (struct hi_list*)oal_memalloc(sizeof(struct hi_list) * bkt_size);
    if (ht->bkts == NULL) {
        return -1;
    }

    for (i = 0; i < bkt_size; i++) {
        hi_list_init(&ht->bkts[i]);
    }
    return 0;
}

/* 注意：nodeDeinit 函数入参是 hi_node 指针! */
static inline void hi_hash_deinit(struct hi_hash_table *ht, hi_node_func node_deinit)
{
    size_t i;
    if (node_deinit != NULL) {
        for (i = 0; i < ht->bkt_size; i++) {
            hi_list_deinit(&ht->bkts[i], node_deinit);
        }
    }
    oal_free(ht->bkts);
}

static inline void hi_hash_add(struct hi_hash_table *ht, struct hi_node *node)
{
    size_t k = ht->key(node, ht->bkt_size);
    struct hi_list *list = &ht->bkts[k];
    hi_list_add_tail(list, node);
}

static inline void hi_hash_remove(struct hi_node *node)
{
    hi_list_remove(node);
}

static inline struct hi_node *hi_hash_find(const struct hi_hash_table *ht,
                                           const struct hi_node *cmp_node)
{
    size_t k = ht->key(cmp_node, ht->bkt_size);
    struct hi_list *list = &ht->bkts[k];
    struct hi_node *node = NULL;
    hi_list_for_each(node, list) {
        if (ht->equal(cmp_node, node)) {
            return node;
        }
    }
    return NULL;
}

/*
 * 遍历处理哈希节点
 * 注意: nodeProc 函数入参是 hi_node 指针！
 *       遍历时不要改变 key 值及链表结构！
 */
static inline void hi_hash_walk(struct hi_hash_table *ht,
                                hi_node_func_x node_proc,
                                void *arg)
{
    size_t i;
    struct hi_node *node = NULL;

    for (i = 0; i < ht->bkt_size; i++) {
        hi_list_for_each(node, &ht->bkts[i]) {
            node_proc(node, arg);
        }
    }
}

#endif /* HIHASH_H */
