/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: add iosmart(weight) policy for blkcg
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/blktrace_api.h>
#include <linux/blk-cgroup.h>
#include <linux/sched/signal.h>
#include <linux/version.h>
#include "blk.h"

#define SHARE_SHIFT 14
#define MAX_SHARE   (1 << SHARE_SHIFT)

#define BW_SLICE_MIN    (8ULL << 20)    /* 8MB */
#define BW_SLICE_MAX    (256ULL << 20)  /* 256MB */
#define BW_SLICE_DEF    (32ULL << 20)   /* 32MB */

#define IOPS_SLICE_MIN  32
#define IOPS_SLICE_MAX  4096
#define IOPS_SLICE_DEF  256

#define IOSMART_IDLE_INTERVAL 20        /* ms */

#define IOSMART_MAX_WAKEUP    20

/* get_disk is removed in mainline v4.16-rc4 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
#define get_disk_local get_disk
#define put_disk_local put_disk
#else
#define get_disk_local get_disk_and_module
#define put_disk_local put_disk_and_module
#endif

/* spin_lock_* changed in mainline v5.0
 * blk_queue_bypass* removed in mainline v5.0
 * */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
#define get_queue_lock(q) (&(q)->queue_lock)
#define blk_queue_bypass(q) 0
#else
#define get_queue_lock(q) ((q)->queue_lock)
#endif

enum run_mode {
	MODE_WEIGHT_NONE = 0,
	MODE_WEIGHT_BANDWIDTH = 2,
	MODE_WEIGHT_IOPS = 3,
	MODE_WEIGHT_MAX = 4,
};

enum blk_iosmart_weight_onoff_mode {
	BLK_IOSMART_WEIGHT_OFF,
	BLK_IOSMART_WEIGHT_ON_FS,
};

struct iosmart_service_queue {
	struct iosmart_service_queue *parent_sq;

	unsigned int            children_weight;

	/* disk bandwidth share of the queue */
	unsigned int            share;
};

struct iosmart_data {
	struct iosmart_service_queue service_queue;
	struct request_queue *queue;

	struct list_head      active;
	struct list_head      expired;

	uint64_t              bw_slice;
	unsigned int          iops_slice;

	enum run_mode         mode;
};

struct iosmart_grp {
	/* must be the first member */
	struct blkg_policy_data pd;

	/* this group's service queue */
	struct iosmart_service_queue service_queue;

	/* iosmart_data this group belongs to */
	struct iosmart_data *sd;

	uint64_t bps;
	uint64_t bytes_disp;

	unsigned int iops;
	unsigned int io_disp;

	unsigned long intime;

	wait_queue_head_t       wait;

	struct list_head        node;
	bool                    expired;
};

static struct blkcg_policy blkcg_policy_iosmart;

static int blk_iosmart_weight_onoff;

static DEFINE_MUTEX(iosmart_mode_lock);

static char *run_mode_name[MODE_WEIGHT_MAX] = {
	[MODE_WEIGHT_NONE] = "none",
	[MODE_WEIGHT_BANDWIDTH] = "weight_bw",
	[MODE_WEIGHT_IOPS] = "weight_iops",
};

static bool sd_weight_based(struct iosmart_data *sd)
{
	return sd->mode >= MODE_WEIGHT_BANDWIDTH;
}

static inline struct iosmart_grp *pd_to_sg(struct blkg_policy_data *pd)
{
	return pd ? container_of(pd, struct iosmart_grp, pd) : NULL;
}

static inline struct iosmart_grp *blkg_to_sg(struct blkcg_gq *blkg)
{
	return pd_to_sg(blkg_to_pd(blkg, &blkcg_policy_iosmart));
}

static inline struct blkcg_gq *sg_to_blkg(struct iosmart_grp *sg)
{
	return pd_to_blkg(&sg->pd);
}

static inline struct iosmart_grp *sq_to_sg(
	struct iosmart_service_queue *sq)
{
	if (sq && sq->parent_sq)
		return container_of(sq, struct iosmart_grp, service_queue);

	return NULL;
}

static inline struct iosmart_data *sq_to_sd(
	struct iosmart_service_queue *sq)
{
	struct iosmart_grp *sg = sq_to_sg(sq);

	if (sg)
		return sg->sd;

	return container_of(sq, struct iosmart_data, service_queue);
}

static void sg_update_perf(struct iosmart_grp *sg)
{
	enum run_mode mode = sg->sd->mode;
	struct iosmart_service_queue *sq = &sg->service_queue;

	if (mode == MODE_WEIGHT_BANDWIDTH) {
		sg->bps = max_t(uint64_t,
			(sg->sd->bw_slice * sq->share) >> SHARE_SHIFT,
			1024);
		sg->iops = UINT_MAX;
	} else if (mode == MODE_WEIGHT_IOPS) {
		sg->bps = U64_MAX;
		sg->iops = max_t(unsigned int,
			(sg->sd->iops_slice * sq->share) >> SHARE_SHIFT,
			1);
	} else {
		sg->bps = U64_MAX;
		sg->iops = UINT_MAX;
	}
}

static void sg_update_share(struct iosmart_data *sd,
			    struct iosmart_grp *sg)
{
	struct cgroup_subsys_state *pos_css;
	struct blkcg_gq *blkg, *parent_blkg;
	struct iosmart_grp *child;
	struct iosmart_service_queue *sq;

	/*
	 * cgroup v1 uses the if path, although cgroup v2 is not
	 * supported, reserve else path.
	 */
	if (!sg || !sg->service_queue.parent_sq ||
	    !sg->service_queue.parent_sq->parent_sq)
		parent_blkg = sd->queue->root_blkg;
	else
		parent_blkg = sg_to_blkg(sq_to_sg(sg->service_queue.parent_sq));

	blkg_for_each_descendant_pre(blkg, pos_css, parent_blkg) {
		child = blkg_to_sg(blkg);
		sq = &child->service_queue;

		/* root blkg is not restricted */
		if (blkg == sd->queue->root_blkg)
			continue;

		if (!sq->parent_sq || !blkg->online)
			continue;

		sq->share = max_t(unsigned int,
			sq->parent_sq->share * blkg->weight /
				sq->parent_sq->children_weight,
			1);

		sg_update_perf(child);
	}
}

static void sd_change_mode(struct iosmart_data *sd, int mode)
{
	struct request_queue *q = sd->queue;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	blk_mq_freeze_queue(q);
#else
	if (q->mq_ops)
		blk_mq_freeze_queue(q);
	else
		blk_queue_bypass_start(q);
#endif
	spin_lock_irq(get_queue_lock(q));

	blk_iosmart_drain_queue(q);

	sd->mode = mode;

	rcu_read_lock();
	sg_update_share(sd, NULL);
	rcu_read_unlock();

	spin_unlock_irq(get_queue_lock(q));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	blk_mq_unfreeze_queue(q);
#else
	if (q->mq_ops)
		blk_mq_unfreeze_queue(q);
	else
		blk_queue_bypass_end(q);
#endif
}

static int iosmart_print_weight_offon(struct seq_file *sf, void *v)
{
	seq_puts(sf, "Weight Based iosmart: ");

	if (blk_iosmart_weight_onoff == BLK_IOSMART_WEIGHT_ON_FS)
		seq_puts(sf, "on-fs\n");
	else
		seq_puts(sf, "off\n");

	return 0;
}

static ssize_t iosmart_set_weight_offon(struct kernfs_open_file *of,
				char *buf, size_t nbytes, loff_t off)
{
	buf = strstrip(buf);
	if (!strcmp(buf, "on-fs"))
		blk_iosmart_weight_onoff = BLK_IOSMART_WEIGHT_ON_FS;
	else if (!strcmp(buf, "off"))
		blk_iosmart_weight_onoff = BLK_IOSMART_WEIGHT_OFF;
	else
		return -EINVAL;

	return nbytes;
}

static u64 sd_prfill_mode_device(struct seq_file *sf,
				 struct blkg_policy_data *pd, int off)
{
	struct iosmart_grp *sg = pd_to_sg(pd);
	const char *dname = blkg_dev_name(pd_to_blkg(pd));
	enum run_mode mode = sg->sd->mode;

	if (!dname)
		return 0;

	if (mode == MODE_WEIGHT_NONE)
		return 0;

	seq_printf(sf, "%s %s\n", dname, run_mode_name[mode]);
	return 0;
}

static int sd_print_mode_device(struct seq_file *sf, void *v)
{
	int i;

	seq_puts(sf, "available ");
	for (i = MODE_WEIGHT_NONE; i < MODE_WEIGHT_MAX; i++) {
		if (i != MODE_WEIGHT_NONE &&
		    i != MODE_WEIGHT_BANDWIDTH &&
		    i != MODE_WEIGHT_IOPS)
			continue;

		seq_printf(sf, "%s(%d) ", run_mode_name[i], i);
	}
	seq_printf(sf, "\ndefault %s\n", run_mode_name[MODE_WEIGHT_NONE]);

	mutex_lock(&iosmart_mode_lock);
	blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
			  sd_prfill_mode_device, &blkcg_policy_iosmart,
			  0, false);
	mutex_unlock(&iosmart_mode_lock);

	return 0;
}

static ssize_t sd_set_mode_device(struct kernfs_open_file *of, char *buf,
				  size_t nbytes, loff_t off)
{
	struct blkcg *blkcg = css_to_blkcg(of_css(of));
	struct blkg_conf_ctx ctx;
	struct iosmart_grp *sg;
	struct gendisk *disk;
	int ret, mode;
	bool need_change = false;

	mutex_lock(&iosmart_mode_lock);
	ret = blkg_conf_prep(blkcg, &blkcg_policy_iosmart, buf, &ctx);
	if (ret)
		goto out;

	if (kstrtoint(ctx.body, 10, &mode) < 0) {
		ret = -EINVAL;
		goto out_finish;
	}

	if (mode != MODE_WEIGHT_NONE &&
	    mode != MODE_WEIGHT_BANDWIDTH &&
	    mode != MODE_WEIGHT_IOPS) {
		ret = -EINVAL;
		goto out_finish;
	}

	sg = blkg_to_sg(ctx.blkg);
	if (sg->sd->mode == mode)
		goto out_finish;

	disk = ctx.disk;
	if (!get_disk_local(disk)) {
		ret = -EBUSY;
		goto out_finish;
	}

	need_change = true;

out_finish:
	blkg_conf_finish(&ctx);

	if (need_change) {
		sd_change_mode(sg->sd, mode);
		put_disk_local(disk);
	}

out:
	mutex_unlock(&iosmart_mode_lock);
	return ret ?: nbytes;
}

static u64 sd_prfill_bw_slice_device(struct seq_file *sf,
				struct blkg_policy_data *pd, int off)
{
	struct blkcg_gq *blkg = pd_to_blkg(pd);
	const char *dname = blkg_dev_name(blkg);
	uint64_t bw_slice;

	if (!dname)
		return 0;

	bw_slice = pd_to_sg(pd)->sd->bw_slice;
	if (bw_slice == BW_SLICE_DEF)
		return 0;

	seq_printf(sf, "%s: bw_slice %llu\n", dname, bw_slice);
	return 0;
}

static int sd_print_bw_slice_device(struct seq_file *sf, void *v)
{
	seq_printf(sf, "default: %llu\n", BW_SLICE_DEF);

	blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
			  sd_prfill_bw_slice_device,
			  &blkcg_policy_iosmart, 0, false);

	return 0;
}

static ssize_t sd_set_bw_slice_device(struct kernfs_open_file *of,
				      char *buf, size_t nbytes,
				      loff_t off)
{
	struct blkcg *blkcg = css_to_blkcg(of_css(of));
	struct iosmart_grp *sg;
	struct blkg_conf_ctx ctx;
	uint64_t val;
	int ret;

	ret = blkg_conf_prep(blkcg, &blkcg_policy_iosmart, buf, &ctx);
	if (ret)
		return ret;

	if (kstrtou64(ctx.body, 10, &val) < 0) {
		ret = -EINVAL;
		goto out_finish;
	}

	if (val < BW_SLICE_MIN || val > BW_SLICE_MAX) {
		ret = -EINVAL;
		goto out_finish;
	}

	sg = blkg_to_sg(ctx.blkg);
	if (sg->sd->bw_slice == val)
		goto out_finish;

	sg->sd->bw_slice = val;

	sg_update_share(sg->sd, NULL);

out_finish:
	blkg_conf_finish(&ctx);
	return ret ?: nbytes;
}

static u64 sd_prfill_iops_slice_device(struct seq_file *sf,
				       struct blkg_policy_data *pd,
				       int off)
{
	struct blkcg_gq *blkg = pd_to_blkg(pd);
	const char *dname = blkg_dev_name(blkg);
	uint64_t iops_slice;

	if (!dname)
		return 0;

	iops_slice = pd_to_sg(pd)->sd->iops_slice;
	if (iops_slice == IOPS_SLICE_DEF)
		return 0;

	seq_printf(sf, "%s: iops_slice %llu\n", dname, iops_slice);
	return 0;
}

static int sd_print_iops_slice_device(struct seq_file *sf, void *v)
{
	seq_printf(sf, "default: %d\n", IOPS_SLICE_DEF);

	blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
			  sd_prfill_iops_slice_device,
			  &blkcg_policy_iosmart, 0, false);

	return 0;
}

static ssize_t sd_set_iops_slice_device(struct kernfs_open_file *of,
					char *buf, size_t nbytes,
					loff_t off)
{
	struct blkcg *blkcg = css_to_blkcg(of_css(of));
	struct iosmart_grp *sg;
	struct blkg_conf_ctx ctx;
	unsigned int val;
	int ret;

	ret = blkg_conf_prep(blkcg, &blkcg_policy_iosmart, buf, &ctx);
	if (ret)
		return ret;

	if (kstrtouint(ctx.body, 10, &val) < 0) {
		ret = -EINVAL;
		goto out_finish;
	}

	if (val < IOPS_SLICE_MIN || val > IOPS_SLICE_MAX) {
		ret = -EINVAL;
		goto out_finish;
	}

	sg = blkg_to_sg(ctx.blkg);
	if (sg->sd->iops_slice == val)
		goto out_finish;

	sg->sd->iops_slice = val;

	sg_update_share(sg->sd, NULL);

out_finish:
	blkg_conf_finish(&ctx);
	return ret ?: nbytes;
}

static int iosmart_print_cgroup_type(struct seq_file *sf, void *v)
{
	struct blkcg *blkcg = css_to_blkcg(seq_css(sf));

	seq_printf(sf, "type: %d\n", blkcg->type);

	return 0;
}

static int iosmart_set_cgroup_type(struct cgroup_subsys_state *css,
			      struct cftype *cft, u64 val)
{
	struct blkcg *blkcg = css_to_blkcg(css);
	unsigned int type = (unsigned int)val;

	if (type >= BLK_THROTL_TYPE_NR)
		return -EINVAL;

	blkcg->type = type;
	return 0;
}

static u64 sg_prfill_weight_device(struct seq_file *sf,
				   struct blkg_policy_data *pd,
				   int off)
{
	struct blkcg_gq *blkg = pd_to_blkg(pd);
	const char *dname = blkg_dev_name(blkg);

	if (!dname)
		return 0;

	if (blkg->weight == blkg->blkcg->weight)
		return 0;

	seq_printf(sf, "%s: weight %u\n", dname, blkg->weight);
	return 0;
}

static int iosmart_print_weight(struct seq_file *sf, void *v)
{
	struct blkcg *blkcg = css_to_blkcg(seq_css(sf));

	/*
	 * blkcg weight default is 500, If do not set blkg weight,
	 * blkg weight will be 500.
	 */
	seq_printf(sf, "default: %u\n", blkcg->weight);
	blkcg_print_blkgs(sf, blkcg, sg_prfill_weight_device,
			  &blkcg_policy_iosmart, 0, false);

	return 0;
}

static ssize_t sg_set_weight_device(struct kernfs_open_file *of,
				    char *buf, size_t nbytes, loff_t off)
{
	struct blkcg *blkcg = css_to_blkcg(of_css(of));
	struct iosmart_grp *sg;
	struct iosmart_service_queue *parent_sq;
	struct blkg_conf_ctx ctx;
	unsigned int val;
	int ret;

	ret = blkg_conf_prep(blkcg, &blkcg_policy_iosmart, buf, &ctx);
	if (ret)
		return ret;

	if (kstrtouint(ctx.body, 10, &val) < 0) {
		ret = -EINVAL;
		goto out_finish;
	}

	if (val < BLKIO_WEIGHT_MIN || val > BLKIO_WEIGHT_MAX) {
		ret = -EINVAL;
		goto out_finish;
	}

	if (ctx.blkg->weight == val)
		goto out_finish;

	sg = blkg_to_sg(ctx.blkg);
	parent_sq = sg->service_queue.parent_sq;
	if (parent_sq) {
		parent_sq->children_weight -= ctx.blkg->weight;
		parent_sq->children_weight += val;
	}

	ctx.blkg->weight = val;

	sg_update_share(sg->sd, sg);

out_finish:
	blkg_conf_finish(&ctx);
	return ret ?: nbytes;
}

static int sg_print_weight_device(struct seq_file *sf, void *v)
{
	blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
			  sg_prfill_weight_device,
			  &blkcg_policy_iosmart, 0, false);
	return 0;
}


static struct cftype iosmart_legacy_files[] = {
	{
		.name = "throttle.weight_based_offon",
		.flags = CFTYPE_ONLY_ON_ROOT,
		.seq_show = iosmart_print_weight_offon,
		.write = iosmart_set_weight_offon,
	},
	{
		.name = "throttle.mode_device",
		.flags = CFTYPE_ONLY_ON_ROOT,
		.seq_show = sd_print_mode_device,
		.write = sd_set_mode_device,
	},
	{
		.name = "throttle.bw_slice_device",
		.flags = CFTYPE_ONLY_ON_ROOT,
		.seq_show = sd_print_bw_slice_device,
		.write = sd_set_bw_slice_device,
	},
	{
		.name = "throttle.iops_slice_device",
		.flags = CFTYPE_ONLY_ON_ROOT,
		.seq_show = sd_print_iops_slice_device,
		.write = sd_set_iops_slice_device,
	},
	{
		.name = "throttle.type",
		.flags = CFTYPE_NOT_ON_ROOT,
		.seq_show = iosmart_print_cgroup_type,
		.write_u64 = iosmart_set_cgroup_type,
	},
	{
		.name = "throttle.weight",
		.flags = CFTYPE_NOT_ON_ROOT,
		.seq_show = iosmart_print_weight,
	},
	{
		.name = "throttle.weight_device",
		.flags = CFTYPE_NOT_ON_ROOT,
		.seq_show = sg_print_weight_device,
		.write = sg_set_weight_device,
	},
	{ }     /* terminate */
};

static bool sg_may_dispatch(struct iosmart_grp *sg)
{
	if (sg->sd->mode == MODE_WEIGHT_BANDWIDTH)
		return (sg->bytes_disp < sg->bps);

	if (sg->sd->mode == MODE_WEIGHT_IOPS)
		return (sg->io_disp < sg->iops);

	return true;
}

static bool sd_should_start_new_slice(struct iosmart_data *sd)
{
	struct iosmart_grp *sg;
	unsigned long idle_jiffies;

	list_for_each_entry(sg, &sd->expired, node) {
		if (sg_to_blkg(sg)->blkcg->type <= BLK_THROTL_FG)
			return true;
	}

	idle_jiffies = msecs_to_jiffies(IOSMART_IDLE_INTERVAL);
	list_for_each_entry(sg, &sd->active, node) {
		if (time_before(jiffies, sg->intime + idle_jiffies))
			return false;
	}

	return true;
}

static void sd_start_new_slice(struct iosmart_data *sd)
{
	struct iosmart_grp *sg;

	/*
	 * This is in q->queue_lock, to prevent too long execution time,
	 * let each sg wakeup to IOSMART_MAX_WAKEUP.
	 */
	list_for_each_entry(sg, &sd->expired, node) {
		sg->expired = false;
		if (sg->sd->mode == MODE_WEIGHT_BANDWIDTH) {
			sg->bytes_disp = 0;
			wake_up_nr(&sg->wait, IOSMART_MAX_WAKEUP);
		} else if (sg->sd->mode == MODE_WEIGHT_IOPS) {
			sg->io_disp = 0;

			if (sg->iops > IOSMART_MAX_WAKEUP)
				wake_up_nr(&sg->wait, IOSMART_MAX_WAKEUP);
			else
				wake_up_nr(&sg->wait, sg->iops);
		}
	}

	list_splice_tail_init(&sd->expired, &sd->active);
}

static inline void iosmart_update_quota(struct iosmart_grp *sg,
					unsigned int size,
					enum run_mode mode)
{
	sg->intime = jiffies;
	if (mode == MODE_WEIGHT_BANDWIDTH)
		sg->bytes_disp += size;
	else
		sg->io_disp++;
}

static struct blkcg_gq *task_blkcg_gq(struct task_struct *tsk,
				      struct block_device *bdev)
{
	struct request_queue *q = NULL;
	struct blkcg *blkcg = NULL;
	struct blkcg_gq *blkg = NULL;

	if (!bdev)
		return NULL;

	q = bdev_get_queue(bdev);

	WARN_ON_ONCE(!rcu_read_lock_held());
	blkcg = task_blkcg(tsk);

	blkg = blkg_lookup(blkcg, q);
	return blkg;
}

struct blkcg_gq *task_blkg_get(struct task_struct *tsk,
			       struct block_device *bdev)
{
	struct blkcg_gq *blkg = NULL;

	rcu_read_lock();
	blkg = task_blkcg_gq(tsk, bdev);
	if (blkg)
		blkg_get(blkg);
	else
		blkg = NULL;
	rcu_read_unlock();

	return blkg;
}

void task_blkg_put(struct blkcg_gq *blkg)
{
	if (blkg)
		blkg_put(blkg);
}

void task_blkg_inc_writer(struct blkcg_gq *blkg)
{
	if (blkg)
		atomic_inc(&blkg->writers);
}

void task_blkg_dec_writer(struct blkcg_gq *blkg)
{
	if (blkg)
		atomic_dec(&blkg->writers);
}

unsigned int blkcg_weight(struct blkcg_gq *blkg)
{
	unsigned int weight;

	if (blkg && blkg->blkcg != &blkcg_root &&
	    blk_iosmart_weight_onoff) {
		weight = blkg->weight;
		weight = max_t(unsigned int,
			       weight / atomic_read(&blkg->writers),
			       BLKIO_WEIGHT_MIN);
	} else {
		weight = BLKIO_WEIGHT_DEFAULT;
	}

	return weight;
}

bool blk_throtl_get_quota(struct block_device *bdev, unsigned int size,
			   unsigned long jiffies_time_out, bool wait)
{
	bool ret = true;
	struct request_queue *q;
	struct iosmart_data *sd;
	struct iosmart_grp *sg = NULL;
	struct blkcg_gq *blkg = NULL;
	struct blkcg *new_blkcg, *ori_blkcg = NULL;
	unsigned long start_jiffies, idle_jiffies;
	unsigned long wait_jiffies;

	if (current->flags & (PF_MEMALLOC | PF_SWAPWRITE |
			      PF_KSWAPD | PF_MUTEX_GC))
		return true;

	/*
	 * bdev has been got at mount(), and will be freed at kill_sb() after
	 * all files closed and all dirty pages flushed. So it's safe to
	 * using bdev without lock here.
	 */
	if (unlikely(!bdev) || unlikely(!bdev->bd_disk))
		return true;

	q = bdev_get_queue(bdev);
	if (unlikely(!q))
		return true;

	if (q->tz_weighted_bdev)
		q = bdev_get_queue(q->tz_weighted_bdev);

	sd = q->sd;

	start_jiffies = jiffies;

get_new_group:
	/*
	 * A iosamrt_grp pointer retrieved under rcu can be used to access
	 * basic fields.
	 */
	rcu_read_lock();

	if (blk_iosmart_weight_onoff != BLK_IOSMART_WEIGHT_ON_FS)
		goto out_unlock_rcu;

	if (!sd_weight_based(sd))
		goto out_unlock_rcu;

	new_blkcg = task_blkcg(current);
	if (new_blkcg == &blkcg_root)
		goto out_unlock_rcu;

	if (ori_blkcg != new_blkcg) {
		ori_blkcg = new_blkcg;

		blkg = blkg_lookup(ori_blkcg, q);
		if (unlikely(!blkg)) {
			spin_lock_irq(get_queue_lock(q));
			blkg = blkg_lookup_create(ori_blkcg, q);
			if (IS_ERR(blkg))
				blkg = NULL;
			spin_unlock_irq(get_queue_lock(q));
		}

		sg = blkg_to_sg(blkg);
	}

	if (unlikely(!sg))
		goto out_unlock_rcu;

	spin_lock_irq(get_queue_lock(q));

	if (unlikely(blk_queue_bypass(q) || blk_queue_dying(q)))
		goto out_unlock;

	if (unlikely(!blkg->online))
		goto out_unlock;

	if (unlikely(!sd_weight_based(sd)))
		goto out_unlock;

retry:
	if (sg_may_dispatch(sg)) {
		iosmart_update_quota(sg, size, sd->mode);
		if (!list_empty(&sg->node))
			goto out_unlock;
		blkg_get(blkg);
		list_add_tail(&sg->node, &sd->active);
		goto out_unlock;
	} else if (!wait) {
		ret = false;
		goto out_unlock;
	}

	if (!sg->expired) {
		sg->expired = true;
		if (unlikely(list_empty(&sg->node))) {
			/*
			 * sg not in sd->active list,
			 * this should never happen.
			 */
			WARN_ON(1);
			blkg_get(blkg);
		}
		list_move_tail(&sg->node, &sd->expired);

		if (list_empty(&sd->active))
			goto start_new_slice;
	}

	/*
	 * If jiffies_time_out expires, just return to continue
	 * to read/write.
	 */
	wait_jiffies = jiffies_time_out - (jiffies - start_jiffies);
	if (jiffies - start_jiffies >= jiffies_time_out)
		goto out_unlock;

	if (sd_should_start_new_slice(sd))
		goto start_new_slice;

	blkg_get(blkg);
	spin_unlock_irq(get_queue_lock(q));
	rcu_read_unlock();

	/* Waiting for starting new slice, max waiting time is 20ms */
	idle_jiffies = msecs_to_jiffies(IOSMART_IDLE_INTERVAL);
	if (wait_jiffies > idle_jiffies)
		wait_jiffies = idle_jiffies;

	wait_event_interruptible_timeout(sg->wait, !sg->expired, wait_jiffies);

	blkg_put(blkg);

	/* If this task is interrupted, just return */
	if (signal_pending_state(TASK_INTERRUPTIBLE, current))
		return true;

	/* this task may switch to other group, get tg again */
	goto get_new_group;

start_new_slice:
	sd_start_new_slice(sd);
	goto retry;

out_unlock:
	spin_unlock_irq(get_queue_lock(q));

out_unlock_rcu:
	rcu_read_unlock();
	return ret;
}

static struct blkg_policy_data *iosmart_grp_alloc(gfp_t flags, int node)
{
	struct iosmart_grp *sg;

	sg = kzalloc_node(sizeof(*sg), flags, node);
	if (!sg)
		return NULL;

	sg->bps = U64_MAX;
	sg->iops = UINT_MAX;

	init_waitqueue_head(&sg->wait);

	INIT_LIST_HEAD(&sg->node);
	sg->expired = false;

	return &sg->pd;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
static struct blkg_policy_data *iosmart_pd_alloc(gfp_t gfp, struct request_queue *q,
						struct blkcg *blkcg)
{
	return iosmart_grp_alloc(gfp, q->node);
}
#else
static struct blkg_policy_data *iosmart_pd_alloc(gfp_t gfp, int node)
{
	return iosmart_grp_alloc(gfp, node);
}
#endif

static void iosmart_pd_init(struct blkg_policy_data *pd)
{
	struct iosmart_grp *sg = pd_to_sg(pd);
	struct blkcg_gq *blkg = pd_to_blkg(pd);
	struct iosmart_data *sd = blkg->q->sd;
	struct iosmart_service_queue *sq = &sg->service_queue;

	sq->parent_sq = &sd->service_queue;

	/* although cgroup v2 is not supported, reserve this code */
	if (cgroup_subsys_on_dfl(io_cgrp_subsys) && blkg->parent)
		sq->parent_sq = &blkg_to_sg(blkg->parent)->service_queue;

	sg->sd = sd;
}

static void iosmart_pd_online(struct blkg_policy_data *pd)
{
	struct iosmart_grp *sg = pd_to_sg(pd);
	struct blkcg_gq *blkg = pd_to_blkg(pd);
	struct iosmart_service_queue *parent_sq;

	/* root blkg is not restricted */
	if (blkg->blkcg == &blkcg_root)
		return;

	parent_sq = sg->service_queue.parent_sq;
	if (parent_sq)
		parent_sq->children_weight += blkg->weight;

	sg_update_share(sg->sd, sg);
}

static void iosmart_pd_offline(struct blkg_policy_data *pd)
{
	struct iosmart_grp *sg = pd_to_sg(pd);
	struct blkcg_gq *blkg = pd_to_blkg(pd);
	struct iosmart_service_queue *parent_sq;

	if (!blkg->online)
		return;

	sg->bps = U64_MAX;
	sg->iops = UINT_MAX;

	if (!list_empty(&sg->node)) {
		/*
		 * There is no need to wake up sg->wait, If access this,
		 * caller must be blkcg_css_offline. task's blkcg will be
		 * changed, ios in sg->wait will belong to new blkcg.
		 */
		list_del_init(&sg->node);
		blkg_put(sg_to_blkg(sg));
	}

	/* root blkg is not restricted */
	if (blkg->blkcg == &blkcg_root)
		return;

	parent_sq = sg->service_queue.parent_sq;
	if (parent_sq)
		parent_sq->children_weight -= blkg->weight;

	rcu_read_lock();
	sg_update_share(sg->sd, sg);
	rcu_read_unlock();
}

static void iosmart_pd_free(struct blkg_policy_data *pd)
{
	struct iosmart_grp *sg = pd_to_sg(pd);

	kfree(sg);
}

static struct blkcg_policy blkcg_policy_iosmart = {
	.legacy_cftypes = iosmart_legacy_files,

	.pd_alloc_fn    = iosmart_pd_alloc,
	.pd_init_fn     = iosmart_pd_init,
	.pd_online_fn   = iosmart_pd_online,
	.pd_offline_fn  = iosmart_pd_offline,
	.pd_free_fn     = iosmart_pd_free,
};

static void sg_drain_node(struct iosmart_grp *sg)
{
	WARN_ON(list_empty(&sg->node));

	list_del_init(&sg->node);
	sg->bytes_disp = 0;
	sg->io_disp = 0;

	/*
	 * Set these to max to ensure that all waiting io will be
	 * processed. If caller is sd_change_mode, will update these
	 * to the corresponding values in sg_update_share.
	 */
	sg->bps = U64_MAX;
	sg->iops = UINT_MAX;

	sg->expired = false;
	wake_up_all(&sg->wait);

	blkg_put(sg_to_blkg(sg));
}

void blk_iosmart_drain_queue(struct request_queue *q)
{
	struct iosmart_data *sd = q->sd;
	struct iosmart_grp *curr, *next;

	list_for_each_entry_safe(curr, next, &sd->active, node)
		sg_drain_node(curr);

	list_for_each_entry_safe(curr, next, &sd->expired, node)
		sg_drain_node(curr);
}

int blk_iosmart_init_queue(struct request_queue *q)
{
	int ret;
	struct iosmart_data *sd;

	sd = kzalloc_node(sizeof(*sd), GFP_KERNEL, q->node);
	if (!sd)
		return -ENOMEM;

	q->sd = sd;
	sd->service_queue.share = MAX_SHARE;
	sd->service_queue.parent_sq = NULL;
	sd->queue = q;

	INIT_LIST_HEAD(&sd->active);
	INIT_LIST_HEAD(&sd->expired);

	sd->bw_slice = BW_SLICE_DEF;
	sd->iops_slice = IOPS_SLICE_DEF;
	sd->mode = MODE_WEIGHT_NONE;

	ret = blkcg_activate_policy(q, &blkcg_policy_iosmart);
	if (ret) {
		kfree(sd);
		q->sd = NULL;
	}

	return ret;
}

void blk_iosmart_exit_queue(struct request_queue *q)
{
	WARN_ON(!q->sd);
	blkcg_deactivate_policy(q, &blkcg_policy_iosmart);
	kfree(q->sd);
}

static int __init iosmart_init(void)
{
	blk_iosmart_weight_onoff = BLK_IOSMART_WEIGHT_OFF;
	return blkcg_policy_register(&blkcg_policy_iosmart);
}

module_init(iosmart_init);
