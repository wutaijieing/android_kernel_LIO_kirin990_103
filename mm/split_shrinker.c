/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: split shrinker_list into hot list and cold list
 * Author: Gong Chen <gongchen4@huawei.com>
 * Create: 2021-05-28
 */
#include <linux/split_shrinker.h>

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/shrinker.h>
#include <linux/swap.h>

#define SHRINKER_MAX_NUM 50
#define SHRINK_ACTIVE_HOT 30

struct shrinker_active_record {
	u64 kswapd_time;
	u64 kswapd_freed;
	u64 total_freed;
	unsigned long (*scan_objects)(struct shrinker *shrinker,
		struct shrink_control *sc);
	unsigned int active;
};

static LIST_HEAD(shrinker_list_cold);
static LIST_HEAD(shrinker_list_hot);

static DECLARE_RWSEM(shrinker_rwsem_cold);
static DECLARE_RWSEM(shrinker_rwsem_hot);

static DECLARE_RWSEM(record_rwsem);
static struct shrinker_active_record record[SHRINKER_MAX_NUM];

static unsigned int init_active_record(struct shrinker *shrinker)
{
	int i;
	unsigned int active;

	active = 0;
	down_write(&record_rwsem);
	for (i = 0; i < SHRINKER_MAX_NUM; i++) {
		if (!record[i].scan_objects) {
			record[i].scan_objects = shrinker->scan_objects;
			record[i].active = 0;
			active = 0;
			break;
		}
		if (record[i].scan_objects == shrinker->scan_objects) {
			active = record[i].active;
			break;
		}
	}
	up_write(&record_rwsem);

	return active;
}

static unsigned int get_active_record(struct shrinker *shrinker)
{
	int i;
	unsigned int active;

	active = 0;
	down_read(&record_rwsem);
	for (i = 0; i < SHRINKER_MAX_NUM; i++) {
		if (record[i].scan_objects == shrinker->scan_objects) {
			active = record[i].active;
			break;
		}
		if (!record[i].scan_objects) {
			up_read(&record_rwsem);
			return init_active_record(shrinker);
		}
	}
	up_read(&record_rwsem);

	return active;
}

static void update_active_record(struct shrinker *shrinker)
{
	int i;

	down_write(&record_rwsem);
	for (i = 0; i < SHRINKER_MAX_NUM; i++) {
		if (record[i].scan_objects == shrinker->scan_objects) {
			record[i].active++;
			break;
		}
	}
	up_write(&record_rwsem);
}

void update_shrinker_info(const struct shrinker *shrinker,
	u64 time, unsigned long freed)
{
	int i;
	u64 kswapd_time;
	unsigned long kswapd_freed;

	if (!shrinker)
		return;

	if (current_is_kswapd()) {
		kswapd_time = time;
		kswapd_freed = freed;
	} else {
		kswapd_time = 0;
		kswapd_freed = 0;
	}

	down_write(&record_rwsem);
	for (i = 0; i < SHRINKER_MAX_NUM; i++) {
		if (record[i].scan_objects == shrinker->scan_objects) {
			record[i].kswapd_time += kswapd_time;
			record[i].kswapd_freed += kswapd_freed;
			record[i].total_freed += freed;
		}
	}
	up_write(&record_rwsem);
}

void register_split_shrinker(struct shrinker *shrinker)
{
	shrinker->active = get_active_record(shrinker);
	if (shrinker->active < SHRINK_ACTIVE_HOT) {
		down_write(&shrinker_rwsem_cold);
		list_add_tail(&shrinker->list, &shrinker_list_cold);
		up_write(&shrinker_rwsem_cold);
	} else {
		down_write(&shrinker_rwsem_hot);
		list_add_tail(&shrinker->list, &shrinker_list_hot);
		up_write(&shrinker_rwsem_hot);
	}
}

void unregister_split_shrinker(struct shrinker *shrinker)
{
	if (shrinker->active < SHRINK_ACTIVE_HOT) {
		down_write(&shrinker_rwsem_cold);
		list_del(&shrinker->list);
		up_write(&shrinker_rwsem_cold);
	} else {
		down_write(&shrinker_rwsem_hot);
		list_del(&shrinker->list);
		up_write(&shrinker_rwsem_hot);
	}
	update_active_record(shrinker);
}

unsigned long shrink_slab(gfp_t gfp_mask, int nid,
	struct mem_cgroup *memcg, int priority)
{
	unsigned long freed;

	freed = shrink_slab_legacy(gfp_mask, nid, memcg, priority,
		&shrinker_rwsem_cold, &shrinker_list_cold);

	freed += shrink_slab_legacy(gfp_mask, nid, memcg, priority,
		&shrinker_rwsem_hot, &shrinker_list_hot);

	return freed;
}


static int shrinker_info_proc_show(struct seq_file *m,
	struct rw_semaphore *p_shrinker_rwsem,
	struct list_head *p_shrinker_list)
{
	struct shrinker *shrinker = NULL;
	int num;

	num = 0;
	down_read(p_shrinker_rwsem);
	list_for_each_entry(shrinker, p_shrinker_list, list) {
		seq_printf(m, "%ps,active:%lu\n", shrinker->scan_objects,
			shrinker->active);
		num++;
	}
	up_read(p_shrinker_rwsem);

	seq_printf(m, "Total : %d shrinkers\n", num);
	return 0;
}

static int cold_shrinker_info_proc_show(struct seq_file *m, void *v)
{
	return shrinker_info_proc_show(m, &shrinker_rwsem_cold,
		&shrinker_list_cold);
}

static int cold_shrinker_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cold_shrinker_info_proc_show, NULL);
}

static const struct proc_ops cold_shrinker_info_fops = {
	.proc_open    = cold_shrinker_info_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int hot_shrinker_info_proc_show(struct seq_file *m, void *v)
{
	return shrinker_info_proc_show(m, &shrinker_rwsem_hot,
		&shrinker_list_hot);
}

static int hot_shrinker_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hot_shrinker_info_proc_show, NULL);
}

static const struct proc_ops hot_shrinker_info_fops = {
	.proc_open    = hot_shrinker_info_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int active_info_proc_show(struct seq_file *m, void *v)
{
	int i;

	seq_puts(m, "shrinker active kswapd_time kswapd_freed total_freed\n");
	down_read(&record_rwsem);
	for (i = 0; i < SHRINKER_MAX_NUM; i++) {
		if (record[i].scan_objects)
			seq_printf(m, "%ps, %lu, %llu, %llu, %llu\n",
				record[i].scan_objects,
				record[i].active,
				record[i].kswapd_time,
				record[i].kswapd_freed,
				record[i].total_freed);
	}
	up_read(&record_rwsem);

	return 0;
}

static int shrinker_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, active_info_proc_show, NULL);
}

static const struct proc_ops shrinker_info_fops = {
	.proc_open    = shrinker_info_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int __init split_shrinker_init(void)
{
	proc_create("cold_shrinker", 0440, NULL, &cold_shrinker_info_fops);
	proc_create("hot_shrinker", 0440, NULL, &hot_shrinker_info_fops);
	proc_create("shrinker_info", 0440, NULL, &shrinker_info_fops);

	return 0;
}

module_init(split_shrinker_init);
