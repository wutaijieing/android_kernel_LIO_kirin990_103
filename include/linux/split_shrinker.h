/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: split shrinker_list into hot list and cold list
 * Author: Gong Chen <gongchen4@huawei.com>
 * Create: 2021-05-28
 */
#ifndef _SPLIT_SHRINKER_H
#define _SPLIT_SHRINKER_H
#include <linux/list.h>
#include <linux/rwsem.h>
#include <linux/shrinker.h>

void register_split_shrinker(struct shrinker *shrinker);
void unregister_split_shrinker(struct shrinker *shrinker);

void update_shrinker_info(const struct shrinker *shrinker,
	u64 time, unsigned long freed);

#endif
