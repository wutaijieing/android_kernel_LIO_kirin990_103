/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: 双向链表
 * Create: 2019-11-11
 * Version: V1.5
 * Contact: 有任何问题或建议，请前往
 *          http://rnd-isource.huawei.com/bugs/hw-cstl
 */

#ifndef HILIST_H
#define HILIST_H

#include "stdbool.h"
#include "oam_ext_if.h"

/*
 * 该结构体用于嵌入到业务数据结构体中，用于实现链表
 * 例：
 *     struct Entry {           // 你的业务数据结构体
 *         ...
 *         struct hi_node node;  // 嵌入其中，位置任意
 *         ...
 *     };
 */
struct hi_node {
    struct hi_node *next, *prev;
};

/*
 * 由成员地址获取结构体地址
 * 例：
 *     struct Entry entry;
 *     struct hi_node *n = &entry.node;
 *     struct Entry *p = hi_node_entry(n, struct Entry, node);
 *     此时 p 指向 entry
 */
#define hi_node_entry(node, type, member) \
    ((type*)((char*)(node) - offsetof(type, member)))

/*
 * 用户定义，针对 node 节点的处理函数
 * 注意: 入参是 hi_node 指针！
 * 你可能需要使用 hi_node_entry 来获取并处理你的数据
 */
typedef void (*hi_node_func)(struct hi_node *node);

/*
 * 用户定义，针对 node 节点的处理函数，带可选参数
 * 注意: 入参是 hi_node 指针！
 * 你可能需要使用 hi_node_entry 来获取并处理你的数据
 */
typedef void (*hi_node_func_x)(struct hi_node *node, void *arg);


/* 带哨兵节点的双向链表 */
struct hi_list {
    struct hi_node base;
};

static inline void hi_list_init(struct hi_list *list)
{
    list->base.next = &list->base;
    list->base.prev = &list->base;
}

static inline bool hi_list_empty(const struct hi_list *list)
{
    return list->base.next == &list->base;
}

static inline bool hi_list_is_head(const struct hi_list *list, const struct hi_node *node)
{
    return list->base.next == node;
}

static inline bool hi_list_is_tail(const struct hi_list *list, const struct hi_node *node)
{
    return list->base.prev == node;
}

/* node 插入到 pos 前面 */
static inline void hi_list_insert(struct hi_node *pos, struct hi_node *node)
{
    node->prev = pos->prev;
    node->next = pos;
    node->prev->next = node;
    node->next->prev = node;
}

static inline void hi_list_add_tail(struct hi_list *list, struct hi_node *node)
{
    hi_list_insert(&list->base, node);
}

static inline void hi_list_add_head(struct hi_list *list, struct hi_node *node)
{
    hi_list_insert(list->base.next, node);
}

static inline void hi_list_remove(struct hi_node *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

static inline void hi_list_remove_tail(struct hi_list *list)
{
    hi_list_remove(list->base.prev);
}

static inline void hi_list_remove_head(struct hi_list *list)
{
    hi_list_remove(list->base.next);
}

static inline void hi_list_replace(struct hi_node *old, struct hi_node *node)
{
    node->next = old->next;
    node->next->prev = node;
    node->prev = old->prev;
    node->prev->next = node;
}

#define hi_list_for_each(node, list) \
    for ((node) = (list)->base.next; (node) != &(list)->base; (node) = (node)->next)

#define hi_list_for_each_safe(node, tmp, list) \
    for ((node) = (list)->base.next, (tmp) = (node)->next; \
         (node) != &(list)->base; (node) = (tmp), (tmp) = (node)->next)

/*
 * 遍历处理链表节点
 * 注意: nodeProc 函数入参是 hi_node 指针！
 *       遍历时不要改变链表结构！
 */
static inline void hi_list_walk(struct hi_list *list,
                              hi_node_func_x node_proc,
                              void *arg)
{
    struct hi_node *node = NULL;
    hi_list_for_each(node, list) {
        node_proc(node, arg);
    }
}

/* 注意：HiNodeFunc 函数入参是 hi_node 指针! */
static inline void hi_list_deinit(struct hi_list *list, hi_node_func node_deinit)
{
    struct hi_node *node = NULL;
    struct hi_node *tmp = NULL;

    if (node_deinit == NULL) {
        return;
    }

    hi_list_for_each_safe(node, tmp, list) {
        node_deinit(node);
    }
}

/* 获取头结点，或空 */
#define hi_list_head_entry(list, type, member) \
    (hi_list_empty(list) ? NULL : hi_node_entry((list)->base.next, type, member))

/* 获取尾结点，或空 */
#define hi_list_tail_entry(list, type, member) \
    (hi_list_empty(list) ? NULL : hi_node_entry((list)->base.prev, type, member))

/* 获取下一结点，或空 */
#define hi_list_next_entry(entry, list, type, member) \
    (hi_list_is_tail(list, &(entry)->member) ? \
        NULL : \
        hi_node_entry((entry)->member.next, type, member))

/* 获取上一结点，或空 */
#define hi_list_prev_entry(entry, list, type, member) \
    (hi_list_is_head(list, &(entry)->member) ? \
        NULL : \
        hi_node_entry((entry)->member.prev, type, member))

/* 遍历链表；过程中如需操作链表，请使用 _SAFE 版本 */
#define hi_list_for_each_entry(entry, list, type, member) \
    for (entry = hi_node_entry((list)->base.next, type, member); \
         &(entry)->member != &(list)->base; \
         entry = hi_node_entry((entry)->member.next, type, member))

#define hi_list_for_each_entry_safe(entry, tmp, list, type, member) \
    for (entry = hi_node_entry((list)->base.next, type, member), \
         tmp = hi_node_entry((entry)->member.next, type, member); \
         &(entry)->member != &(list)->base; \
         entry = tmp, tmp = hi_node_entry((entry)->member.next, type, member))

/* 倒序遍历链表；过程中如需操作链表，请使用 _SAFE 版本 */
#define hi_list_for_each_entry_reverse(entry, list, type, member) \
    for (entry = hi_node_entry((list)->base.prev, type, member); \
         &(entry)->member != &(list)->base; \
         entry = hi_node_entry((entry)->member.prev, type, member))

#define hi_list_for_each_entry_reverse_safe(entry, tmp, list, type, member) \
    for (entry = hi_node_entry((list)->base.prev, type, member), \
         tmp = hi_node_entry((entry)->member.prev, type, member); \
         &(entry)->member != &(list)->base; \
         entry = tmp, tmp = hi_node_entry((entry)->member.prev, type, member))

#endif /* HILIST_H */
