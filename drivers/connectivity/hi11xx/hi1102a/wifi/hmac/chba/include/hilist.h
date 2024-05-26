/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: ˫������
 * Create: 2019-11-11
 * Version: V1.5
 * Contact: ���κ�������飬��ǰ��
 *          http://rnd-isource.huawei.com/bugs/hw-cstl
 */

#ifndef HILIST_H
#define HILIST_H

#include "stdbool.h"
#include "oam_ext_if.h"

/*
 * �ýṹ������Ƕ�뵽ҵ�����ݽṹ���У�����ʵ������
 * ����
 *     struct Entry {           // ���ҵ�����ݽṹ��
 *         ...
 *         struct hi_node node;  // Ƕ�����У�λ������
 *         ...
 *     };
 */
struct hi_node {
    struct hi_node *next, *prev;
};

/*
 * �ɳ�Ա��ַ��ȡ�ṹ���ַ
 * ����
 *     struct Entry entry;
 *     struct hi_node *n = &entry.node;
 *     struct Entry *p = hi_node_entry(n, struct Entry, node);
 *     ��ʱ p ָ�� entry
 */
#define hi_node_entry(node, type, member) \
    ((type*)((char*)(node) - offsetof(type, member)))

/*
 * �û����壬��� node �ڵ�Ĵ�����
 * ע��: ����� hi_node ָ�룡
 * �������Ҫʹ�� hi_node_entry ����ȡ�������������
 */
typedef void (*hi_node_func)(struct hi_node *node);

/*
 * �û����壬��� node �ڵ�Ĵ�����������ѡ����
 * ע��: ����� hi_node ָ�룡
 * �������Ҫʹ�� hi_node_entry ����ȡ�������������
 */
typedef void (*hi_node_func_x)(struct hi_node *node, void *arg);


/* ���ڱ��ڵ��˫������ */
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

/* node ���뵽 pos ǰ�� */
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
 * ������������ڵ�
 * ע��: nodeProc ��������� hi_node ָ�룡
 *       ����ʱ��Ҫ�ı�����ṹ��
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

/* ע�⣺HiNodeFunc ��������� hi_node ָ��! */
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

/* ��ȡͷ��㣬��� */
#define hi_list_head_entry(list, type, member) \
    (hi_list_empty(list) ? NULL : hi_node_entry((list)->base.next, type, member))

/* ��ȡβ��㣬��� */
#define hi_list_tail_entry(list, type, member) \
    (hi_list_empty(list) ? NULL : hi_node_entry((list)->base.prev, type, member))

/* ��ȡ��һ��㣬��� */
#define hi_list_next_entry(entry, list, type, member) \
    (hi_list_is_tail(list, &(entry)->member) ? \
        NULL : \
        hi_node_entry((entry)->member.next, type, member))

/* ��ȡ��һ��㣬��� */
#define hi_list_prev_entry(entry, list, type, member) \
    (hi_list_is_head(list, &(entry)->member) ? \
        NULL : \
        hi_node_entry((entry)->member.prev, type, member))

/* ���������������������������ʹ�� _SAFE �汾 */
#define hi_list_for_each_entry(entry, list, type, member) \
    for (entry = hi_node_entry((list)->base.next, type, member); \
         &(entry)->member != &(list)->base; \
         entry = hi_node_entry((entry)->member.next, type, member))

#define hi_list_for_each_entry_safe(entry, tmp, list, type, member) \
    for (entry = hi_node_entry((list)->base.next, type, member), \
         tmp = hi_node_entry((entry)->member.next, type, member); \
         &(entry)->member != &(list)->base; \
         entry = tmp, tmp = hi_node_entry((entry)->member.next, type, member))

/* ������������������������������ʹ�� _SAFE �汾 */
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
