/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_BLKDEV_H
#define _LINUX_BLKDEV_H

#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/major.h>
#include <linux/genhd.h>
#include <linux/list.h>
#include <linux/llist.h>
#include <linux/minmax.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/pagemap.h>
#include <linux/backing-dev-defs.h>
#include <linux/wait.h>
#include <linux/mempool.h>
#include <linux/pfn.h>
#include <linux/bio.h>
#include <linux/stringify.h>
#include <linux/gfp.h>
#include <linux/bsg.h>
#include <linux/smp.h>
#include <linux/rcupdate.h>
#include <linux/percpu-refcount.h>
#include <linux/scatterlist.h>
#include <linux/blkzoned.h>
#include <linux/pm.h>
#include <linux/android_kabi.h>
#include <linux/android_vendor.h>

struct module;
struct scsi_ioctl_command;
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
struct Scsi_Host;
#endif

struct request_queue;
struct elevator_queue;
struct blk_trace;
struct request;
struct sg_io_hdr;
struct bsg_job;
struct blkcg_gq;
struct blk_flush_queue;
struct pr_ops;
struct rq_qos;
struct blk_queue_stats;
struct blk_stat_callback;
struct blk_keyslot_manager;

#define BLKDEV_MIN_RQ	4
#define BLKDEV_MAX_RQ	128	/* Default maximum */

/* Must be consistent with blk_mq_poll_stats_bkt() */
#define BLK_MQ_POLL_STATS_BKTS 16

/* Doing classic polling */
#define BLK_MQ_POLL_CLASSIC -1

/*
 * Maximum number of blkcg policies allowed to be registered concurrently.
 * Defined here to simplify include dependency.
 */
#define BLKCG_MAX_POLS		5

#if defined(CONFIG_HUAWEI_QOS_BLKIO) || defined(CONFIG_ROW_VIP_QUEUE)
#ifndef BLKIO_QOS_HIGH
#define BLKIO_QOS_HIGH	VALUE_QOS_HIGH
#endif
#ifndef BLKIO_QOS_DEFAULT
#define BLKIO_QOS_DEFAULT	VALUE_QOS_NORMAL
#endif
#ifndef HUAWEI_QOS_BLKIO_DEBUG
#define HUAWEI_QOS_BLKIO_DEBUG
#endif
#endif

typedef void (rq_end_io_fn)(struct request *, blk_status_t);

/*
 * request flags */
typedef __u32 __bitwise req_flags_t;

/* elevator knows about this request */
#define RQF_SORTED		((__force req_flags_t)(1 << 0))
/* drive already may have started this one */
#define RQF_STARTED		((__force req_flags_t)(1 << 1))
/* may not be passed by ioscheduler */
#define RQF_SOFTBARRIER		((__force req_flags_t)(1 << 3))
/* request for flush sequence */
#define RQF_FLUSH_SEQ		((__force req_flags_t)(1 << 4))
/* merge of different types, fail separately */
#define RQF_MIXED_MERGE		((__force req_flags_t)(1 << 5))
/* track inflight for MQ */
#define RQF_MQ_INFLIGHT		((__force req_flags_t)(1 << 6))
/* don't call prep for this one */
#define RQF_DONTPREP		((__force req_flags_t)(1 << 7))
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
/* set for "ide_preempt" requests and also for requests for which the SCSI
   "quiesce" state must be ignored. */
#define RQF_PREEMPT		((__force req_flags_t)(1 << 8))
#endif
/* vaguely specified driver internal error.  Ignored by the block layer */
#define RQF_FAILED		((__force req_flags_t)(1 << 10))
/* don't warn about errors */
#define RQF_QUIET		((__force req_flags_t)(1 << 11))
/* elevator private data attached */
#define RQF_ELVPRIV		((__force req_flags_t)(1 << 12))
/* account into disk and partition IO statistics */
#define RQF_IO_STAT		((__force req_flags_t)(1 << 13))
/* request came from our alloc pool */
#define RQF_ALLOCED		((__force req_flags_t)(1 << 14))
/* runtime pm request */
#define RQF_PM			((__force req_flags_t)(1 << 15))
/* on IO scheduler merge hash */
#define RQF_HASHED		((__force req_flags_t)(1 << 16))
/* track IO completion time */
#define RQF_STATS		((__force req_flags_t)(1 << 17))
/* Look at ->special_vec for the actual data payload instead of the
   bio chain. */
#define RQF_SPECIAL_PAYLOAD	((__force req_flags_t)(1 << 18))
/* The per-zone write lock is held for this request */
#define RQF_ZONE_WRITE_LOCKED	((__force req_flags_t)(1 << 19))
/* already slept for hybrid poll */
#define RQF_MQ_POLL_SLEPT	((__force req_flags_t)(1 << 20))
/* ->timeout has been called, don't expire again */
#define RQF_TIMED_OUT		((__force req_flags_t)(1 << 21))

/* flags that prevent us from merging requests: */
#define RQF_NOMERGE_FLAGS \
	(RQF_STARTED | RQF_SOFTBARRIER | RQF_FLUSH_SEQ | RQF_SPECIAL_PAYLOAD)

#ifdef CONFIG_MAS_BLK
enum customer_rq_flag_bits {
	__REQ_HEAD_OF_QUEUE = 0,
	__REQ_COMMAND_PRIO,
	__REQ_COMMAND_ORDER,
	__REQ_TURBO_ZONE,
};
int get_mq_all_tag_used(struct request_queue *q);
int get_mq_prio_tag_used(struct request_queue *q);
extern blk_qc_t __cfi_ufs_mq_make_request(struct bio *bio);

#define CUST_REQ_HEAD_OF_QUEUE (1ULL << __REQ_HEAD_OF_QUEUE)
#define CUST_REQ_COMMAND_PRIO (1ULL << __REQ_COMMAND_PRIO)
#define CUST_REQ_ORDER (1ULL << __REQ_COMMAND_ORDER)
#define CUST_REQ_TURBO_ZONE (1ULL << __REQ_TURBO_ZONE)

#define req_hoq(req)                                                           \
	((req)->mas_req.mas_featrue_flag & CUST_REQ_HEAD_OF_QUEUE)
#define req_cp(req) ((req)->mas_req.mas_featrue_flag & CUST_REQ_COMMAND_PRIO)
#define req_tz(req) ((req)->mas_req.mas_featrue_flag & CUST_REQ_TURBO_ZONE)

#define req_order(req) \
		((req)->mas_req.mas_featrue_flag & CUST_REQ_ORDER)

enum requeue_reason_enum {
	REQ_REQUEUE_IO_NO_REQUEUE = 0,
	REQ_REQUEUE_IO_HW_LIMIT,
	REQ_REQUEUE_IO_SW_LIMIT,
	REQ_REQUEUE_IO_CP_LIMIT,
	REQ_REQUEUE_IO_HW_PENDING,
};

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
enum blk_ft_rq_sim_mode {
	BLK_FT_RQ_SIM_NONE = 0, /* The value can't be changed! */
	BLK_FT_RQ_SIM_TIMEOUT_HANDLED,
	BLK_FT_RQ_SIM_RESET_TIMER,
	BLK_FT_RQ_SIM_EXPECT_REQUEUE,
};
#endif

/*
 * This struct defines all the variable in vendor block layer.
 */
struct blk_req_cust {
	/* Dispatch IO process task struct */
	struct task_struct *dispatch_task;
	/* Dispatch IO process PID */
	pid_t task_pid;
	/* Dispatch IO process TGID */
	pid_t task_tgid;
	/* Dispatch IO process name */
	char task_comm[TASK_COMM_LEN];

	/* mas feature flag */
	unsigned long long mas_featrue_flag;
	/* io comes from fs or not */
	unsigned char fs_io_flag;
	/* The reason for IO requeue */
	enum requeue_reason_enum requeue_reason;
	/* The CTX which make the request */
	struct blk_mq_ctx *mq_ctx_generate;
	/* Non-FS request endup call back function */
	rq_end_io_fn *uplayer_end_io;
#ifdef CONFIG_MAS_QOS_MQ
	unsigned char mas_rq_qos;
	unsigned int slot_cpu;
#endif
	ktime_t req_stage_ktime[REQ_PROC_STAGE_MAX];

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	/*
	 * Member for Vertical Opti
	 */
	unsigned char stream_type;
	unsigned char cp_tag;
	unsigned long data_ino;
	unsigned long data_idx;
	bool fsync_ind;
	bool slc_mode;
	bool fg_io;
	unsigned int protocol_nr_cnt;
#endif

	unsigned int make_req_nr;
	unsigned int protocol_nr;

	/*
	 * Below info for debug info
	 */

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	atomic_t req_used;
	enum blk_ft_rq_sim_mode simulate_mode;
#endif
};

#ifdef CONFIG_MAS_QOS_MQ
enum mas_mq_qos_val {
	MAS_MQ_QOS_0 = 0,
	MAS_MQ_QOS_1,
	MAS_MQ_QOS_2,
	MAS_MQ_QOS_3,
	MAS_MQ_QOS_4,
	MAS_MQ_QOS_5,
	MAS_MQ_QOS_6,
	MAS_MQ_QOS_7,
};
#endif /* CONFIG_MAS_QOS_MQ */
#endif /* CONFIG_MAS_BLK */

/*
 * Request state for blk-mq.
 */
enum mq_rq_state {
	MQ_RQ_IDLE		= 0,
	MQ_RQ_IN_FLIGHT		= 1,
	MQ_RQ_COMPLETE		= 2,
};

/*
 * Try to put the fields that are referenced together in the same cacheline.
 *
 * If you modify this structure, make sure to update blk_rq_init() and
 * especially blk_mq_rq_ctx_init() to take care of the added fields.
 */
struct request {
	struct request_queue *q;
	struct blk_mq_ctx *mq_ctx;
	struct blk_mq_hw_ctx *mq_hctx;

	unsigned int cmd_flags;		/* op and common flags */
	req_flags_t rq_flags;

	int tag;
	int internal_tag;

	/* the following two fields are internal, NEVER access directly */
	unsigned int __data_len;	/* total data len */
	sector_t __sector;		/* sector cursor */

	struct bio *bio;
	struct bio *biotail;

	struct list_head queuelist;

	/*
	 * The hash is used inside the scheduler, and killed once the
	 * request reaches the dispatch list. The ipi_list is only used
	 * to queue the request for softirq completion, which is long
	 * after the request has been unhashed (and even removed from
	 * the dispatch list).
	 */
	union {
		struct hlist_node hash;	/* merge hash */
		struct list_head ipi_list;
	};

	/*
	 * The rb_node is only used inside the io scheduler, requests
	 * are pruned when moved to the dispatch queue. So let the
	 * completion_data share space with the rb_node.
	 */
	union {
		struct rb_node rb_node;	/* sort/lookup */
		struct bio_vec special_vec;
		void *completion_data;
		int error_count; /* for legacy drivers, don't use */
	};

	/*
	 * Three pointers are available for the IO schedulers, if they need
	 * more they have to dynamically allocate it.  Flush requests are
	 * never put on the IO scheduler. So let the flush fields share
	 * space with the elevator data.
	 */
	union {
		struct {
			struct io_cq		*icq;
			void			*priv[2];
		} elv;

		struct {
			unsigned int		seq;
			struct list_head	list;
			rq_end_io_fn		*saved_end_io;
		} flush;
	};

	struct gendisk *rq_disk;
	struct hd_struct *part;
#ifdef CONFIG_BLK_RQ_ALLOC_TIME
	/* Time that the first bio started allocating this request. */
	u64 alloc_time_ns;
#endif
	/* Time that this request was allocated for this IO. */
	u64 start_time_ns;
	/* Time that I/O was submitted to the device. */
	u64 io_start_time_ns;

#ifdef CONFIG_MAS_BLK
#ifdef CONFIG_MMC_MQ_CQ_HCI
	struct list_head cmdq_list;
#endif
	unsigned long mas_cmd_flags;

	struct blk_req_cust mas_req;
	/*
	 * Below is for MAS IO Scheduler
	 */
	struct list_head async_list;
	/*
	 * Below is for MAS Block Debug purpose
	 */
	struct list_head cnt_list;

	struct blk_issue_stat issue_stat;
#endif /* CONFIG_MAS_BLK */

#ifdef CONFIG_BLK_WBT
	unsigned short wbt_flags;
#endif
	/*
	 * rq sectors used for blk stats. It has the same value
	 * with blk_rq_sectors(rq), except that it never be zeroed
	 * by completion.
	 */

	/*
	 * Below info for inline crypto
	 */
	void *ci_key;
	int ci_key_len;
	int ci_key_index;
	u8 *ci_metadata;

	unsigned short stats_sectors;

	/*
	 * Number of scatter-gather DMA addr+len pairs after
	 * physical address coalescing is performed.
	 */
	unsigned short nr_phys_segments;

#if defined(CONFIG_BLK_DEV_INTEGRITY)
	unsigned short nr_integrity_segments;
#endif

#ifdef CONFIG_BLK_INLINE_ENCRYPTION
	struct bio_crypt_ctx *crypt_ctx;
	struct blk_ksm_keyslot *crypt_keyslot;
#endif

	unsigned short write_hint;
	unsigned short ioprio;

	enum mq_rq_state state;
	refcount_t ref;

	unsigned int timeout;
	unsigned long deadline;

	union {
		struct __call_single_data csd;
		u64 fifo_time;
	};

	/*
	 * completion callback.
	 */
	rq_end_io_fn *end_io;
	void *end_io_data;

	ANDROID_KABI_RESERVE(1);
};

static inline bool blk_op_is_scsi(unsigned int op)
{
	return op == REQ_OP_SCSI_IN || op == REQ_OP_SCSI_OUT;
}

static inline bool blk_op_is_private(unsigned int op)
{
	return op == REQ_OP_DRV_IN || op == REQ_OP_DRV_OUT;
}

static inline bool blk_rq_is_scsi(struct request *rq)
{
	return blk_op_is_scsi(req_op(rq));
}

static inline bool blk_rq_is_private(struct request *rq)
{
	return blk_op_is_private(req_op(rq));
}

static inline bool blk_rq_is_passthrough(struct request *rq)
{
	return blk_rq_is_scsi(rq) || blk_rq_is_private(rq);
}

static inline bool bio_is_passthrough(struct bio *bio)
{
	unsigned op = bio_op(bio);

	return blk_op_is_scsi(op) || blk_op_is_private(op);
}

static inline unsigned short req_get_ioprio(struct request *req)
{
	return req->ioprio;
}

#include <linux/elevator.h>

struct blk_queue_ctx;

struct bio_vec;

enum blk_eh_timer_return {
	BLK_EH_DONE,		/* drivers has completed the command */
	BLK_EH_RESET_TIMER,	/* reset timer and try again */
#ifdef CONFIG_MAS_BLK
	BLK_EH_REQUEUE,
#endif
};

enum blk_queue_state {
	Queue_down,
	Queue_up,
};

#define BLK_TAG_ALLOC_FIFO 0 /* allocate starting from 0 */
#define BLK_TAG_ALLOC_RR 1 /* allocate starting from last allocated tag */

#define BLK_SCSI_MAX_CMDS	(256)
#define BLK_SCSI_CMD_PER_LONG	(BLK_SCSI_MAX_CMDS / (sizeof(long) * 8))

/*
 * Zoned block device models (zoned limit).
 *
 * Note: This needs to be ordered from the least to the most severe
 * restrictions for the inheritance in blk_stack_limits() to work.
 */
enum blk_zoned_model {
	BLK_ZONED_NONE = 0,	/* Regular block device */
	BLK_ZONED_HA,		/* Host-aware zoned block device */
	BLK_ZONED_HM,		/* Host-managed zoned block device */
};

struct queue_limits {
	unsigned long		bounce_pfn;
	unsigned long		seg_boundary_mask;
	unsigned long		virt_boundary_mask;

	unsigned int		max_hw_sectors;
	unsigned int		max_dev_sectors;
	unsigned int		chunk_sectors;
	unsigned int		max_sectors;
	unsigned int		max_segment_size;
	unsigned int		physical_block_size;
	unsigned int		logical_block_size;
	unsigned int		alignment_offset;
	unsigned int		io_min;
	unsigned int		io_opt;
	unsigned int		max_discard_sectors;
	unsigned int		max_hw_discard_sectors;
	unsigned int		max_write_same_sectors;
	unsigned int		max_write_zeroes_sectors;
	unsigned int		max_zone_append_sectors;
	unsigned int		discard_granularity;
	unsigned int		discard_alignment;

	unsigned short		max_segments;
	unsigned short		max_integrity_segments;
	unsigned short		max_discard_segments;

	unsigned char		misaligned;
	unsigned char		discard_misaligned;
	unsigned char		raid_partial_stripes_expensive;
	enum blk_zoned_model	zoned;

	ANDROID_KABI_RESERVE(1);
};

typedef int (*report_zones_cb)(struct blk_zone *zone, unsigned int idx,
			       void *data);

void blk_queue_set_zoned(struct gendisk *disk, enum blk_zoned_model model);

#ifdef CONFIG_BLK_DEV_ZONED

#define BLK_ALL_ZONES  ((unsigned int)-1)
int blkdev_report_zones(struct block_device *bdev, sector_t sector,
			unsigned int nr_zones, report_zones_cb cb, void *data);
unsigned int blkdev_nr_zones(struct gendisk *disk);
extern int blkdev_zone_mgmt(struct block_device *bdev, enum req_opf op,
			    sector_t sectors, sector_t nr_sectors,
			    gfp_t gfp_mask);
int blk_revalidate_disk_zones(struct gendisk *disk,
			      void (*update_driver_data)(struct gendisk *disk));

extern int blkdev_report_zones_ioctl(struct block_device *bdev, fmode_t mode,
				     unsigned int cmd, unsigned long arg);
extern int blkdev_zone_mgmt_ioctl(struct block_device *bdev, fmode_t mode,
				  unsigned int cmd, unsigned long arg);

#else /* CONFIG_BLK_DEV_ZONED */

static inline unsigned int blkdev_nr_zones(struct gendisk *disk)
{
	return 0;
}

static inline int blkdev_report_zones_ioctl(struct block_device *bdev,
					    fmode_t mode, unsigned int cmd,
					    unsigned long arg)
{
	return -ENOTTY;
}

static inline int blkdev_zone_mgmt_ioctl(struct block_device *bdev,
					 fmode_t mode, unsigned int cmd,
					 unsigned long arg)
{
	return -ENOTTY;
}

#endif /* CONFIG_BLK_DEV_ZONED */

#ifdef CONFIG_MAS_BLK
enum blk_lld_base {
	BLK_LLD_QUEUE_BASE = 0,
	BLK_LLD_QUEUE_TAG_BASE,
	BLK_LLD_TAGSET_BASE,
};

enum blk_freeze_obj_type {
	BLK_LLD = 0,
	BLK_QUEUE,
};

enum blk_dump_scene {
	BLK_DUMP_WARNING = 0,
	BLK_DUMP_PANIC,
};

enum blk_busyidle_callback_ret {
	/* Event Proc won't trigger IO */
	BUSYIDLE_NO_IO = 0,
	/* Event Proc will trigger new IO */
	BUSYIDLE_IO_TRIGGERED,
	/* Event Proc meets errors */
	BUSYIDLE_ERR,
};

enum blk_idle_notify_state {
	BLK_BUSY_NOTIFY = 0, /* IO Busy Event */
	BLK_IDLE_NOTIFY,     /* IO Idle Event */
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	BLK_FG_BUSY_NOTIFY,  /* FG IO Busy Event */
	BLK_FG_IDLE_NOTIFY,  /* FG IO Idle Event */
#endif
};

enum blk_io_state {
	BLK_IO_BUSY = 0,
	BLK_IO_IDLE,
};
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
enum blk_fg_io_state {
	BLK_FG_IO_BUSY = 0,
	BLK_FG_IO_IDLE,
};
#endif

enum blk_idle_dur_enum {
	BLK_IDLE_DUR_IDX_100MS,
	BLK_IDLE_DUR_IDX_500MS,
	BLK_IDLE_DUR_IDX_1000MS,
	BLK_IDLE_DUR_IDX_2000MS,
	BLK_IDLE_DUR_IDX_4000MS,
	BLK_IDLE_DUR_IDX_6000MS,
	BLK_IDLE_DUR_IDX_8000MS,
	BLK_IDLE_DUR_IDX_10000MS,
	BLK_IDLE_DUR_IDX_FOR_AGES,
	BLK_IDLE_DUR_IDX_DUR_NUM,
};

struct blk_dev_lld;

#define SUBSCRIBER_NAME_LEN 32
struct blk_busyidle_event_node {
	/* Identify a subscriber uniquely */
	char subscriber[SUBSCRIBER_NAME_LEN];
	/* should be provided by subscriber module */
	enum blk_busyidle_callback_ret (*busyidle_callback)(
		struct blk_busyidle_event_node *, enum blk_idle_notify_state);
	/* optional */
	void *param_data;

	/* busy idle private data, don't touch it */
	struct blk_dev_lld *lld;
};

struct blk_idle_state {
	/* Hardware idle support or not */
	bool idle_intr_support;
	/* Idle event process worker */
	struct delayed_work idle_notify_worker;
	/* delay avoid jitter */
	unsigned int idle_notify_delay_ms;

	/* list of subcribed events */
	struct list_head subscribed_list;
	struct blocking_notifier_head nh;
	atomic_t io_count; /* io count variable */
	struct mutex io_count_mutex;
	/* busy idle state */
	enum blk_io_state idle_state;

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	/*
	 * for busy idle statistic purpose
	 */
	ktime_t last_idle_ktime;
	ktime_t last_busy_ktime;
	ktime_t total_busy_ktime;
	ktime_t total_idle_ktime;
	unsigned long long total_idle_count;
	/* statistic for idle time */
	s64 blk_idle_dur[BLK_IDLE_DUR_IDX_DUR_NUM];
	/* max idle time */
	s64 max_idle_dur;
	struct blk_busyidle_event_node busy_idle_test_node;
	struct blk_busyidle_event_node busy_idle_test_nodes[5];
#endif

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	/* foreground io idle event process worker */
	struct delayed_work fg_io_idle_notify_worker;
	unsigned int fg_io_idle_notify_delay_ms;
	/* foreground io count variable */
	atomic_t fg_io_count;
	struct mutex fg_io_count_mutex;
	/* foreground io busy idle state */
	enum blk_fg_io_state fg_io_idle_state;
	atomic_t idle_trigger_runtime_change;
#endif

	/*
	 * Below info is just for busy idle debug purpose!
	 */
	/* The number of bios have been counted */
	u64 bio_count;
	/* The number of reqs have been counted */
	u64 req_count;
	/* The list for all the counted bios */
	struct list_head bio_list;
	/* The list for all the counted reqs */
	struct list_head req_list;
	spinlock_t counted_list_lock;
};

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
#define STREAM_TYPE_RPMB    0xf0
#define STREAM_TYPE_INVALID 0xff
#define BLK_ORDER_STREAM_NUM 4
#define DATA_MOVE_STREAM_NUM 2
#define MAX_RESCUE_SEG_CNT 240
#define DATA_MOVE_MAX_NUM 768
#define MAS_MQ_QUEUE_DEPTH 192
#define UNISTORE_MQ_QUEUE_DEPTH 32
#define UNISTORE_QUEUE_RESERVE_DEPTH 8

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
enum reset_debug {
	RESET_DEBUG_NO = 0,
	RESET_DEBUG_100E,
	RESET_DEBUG_DISORDER,
	RESET_DEBUG_CLOSE,
};
#endif

enum mas_stream_type {
	BLK_STREAM_META = 0,
	BLK_STREAM_COLD_NODE,
	BLK_STREAM_COLD_DATA,
	BLK_STREAM_HOT_NODE,
	BLK_STREAM_HOT_DATA,
	BLK_STREAM_HP,

	/* add new stream above */
	BLK_STREAM_MAX_STRAM,
};

enum bkops_fs_work_result {
	BKOPS_START_SUC = 0,
	BKOPS_DEV_NOT_SUPPORT,
	BKOPS_INPUT_ERR,
	BKOPS_FUNC_NOT_SUPPORT,
	BKOPS_STATE_NOT_IDLE,
	BKOPS_ALREADY_START,
	BKOPS_QUERY_ERR,
	BKOPS_NO_NEED_START,
	BKOPS_NEED_START,
	BKOPS_START_ERR,
};

enum pwron_type {
	DM_STRAM0_TYPE = 0,
	DM_STRAM1_TYPE,
	RECOVERY_TYPE,

	PWRON_MAX_TYPE,
};

typedef void (stor_dev_pwron_info_done_fn)(int, u8);

struct stor_dev_pwron_info_done_info {
	stor_dev_pwron_info_done_fn *done;
	u8 pwron_type;
};

#define STREAM_SECTION_NUM 5

struct stor_dev_pwron_info {
	unsigned int dev_stream_addr[BLK_STREAM_MAX_STRAM];
	unsigned int rescue_seg_cnt;
	unsigned int *rescue_seg;
	unsigned char pe_limit_status;
	unsigned int dm_stream_addr[DATA_MOVE_STREAM_NUM];
	unsigned char stream_lun_info[BLK_STREAM_MAX_STRAM];
	unsigned char dm_lun_info[DATA_MOVE_STREAM_NUM];
	unsigned char io_slc_mode_status;
	unsigned char dm_slc_mode_status;
	unsigned int section_info[BLK_ORDER_STREAM_NUM][STREAM_SECTION_NUM];
	struct stor_dev_pwron_info_done_info done_info;
};

struct stor_dev_stream_info {
	unsigned char stream_id;
	bool dm_stream;
	unsigned int stream_start_lba;
};

struct stor_dev_stream_oob_info {
	unsigned long data_ino;
	unsigned long data_idx;
};

struct stor_dev_verify_info {
	unsigned int next_to_be_verify_4k_lba;
	unsigned int next_available_write_4k_lba;
	unsigned char lun_info;
	unsigned char verify_done_status;
	unsigned char verify_fail_reason;
	unsigned int pu_size;
};

typedef void (stor_dev_data_move_done_fn)(struct stor_dev_verify_info, void *);

struct stor_dev_data_move_done_info {
	stor_dev_data_move_done_fn *done;
	void *private_data;
	sector_t start_addr;
	unsigned int mas_sec_size;
};

struct stor_dev_data_move_source_addr {
	unsigned int data_move_source_addr;
	unsigned char src_lun;
	unsigned int source_length;
};

struct stor_dev_data_move_source_inode {
	unsigned int data_move_source_inode;
	unsigned int data_move_source_offset;
};

struct stor_dev_data_move_info {
	unsigned int data_move_total_length;
	unsigned int dest_4k_lba;
	unsigned char dest_lun_info;
	unsigned char dest_stream_id;
	unsigned char dest_blk_mode;
	unsigned char force_flush_option;
	unsigned char repeat_option;
	unsigned char error_injection;
	unsigned int source_addr_num;
	struct stor_dev_data_move_source_addr *source_addr;
	unsigned int source_inode_num;
	struct stor_dev_data_move_source_inode *source_inode;
	struct stor_dev_verify_info verify_info;
	struct stor_dev_data_move_done_info done_info;
};

struct stor_dev_sync_read_verify_info {
	unsigned char stream_id;
	unsigned int cp_verify_l4k;
	unsigned int cp_open_l4k;
	unsigned int cp_cache_l4k;
	unsigned char error_injection;
	struct stor_dev_verify_info verify_info;
};

struct stor_dev_bad_block_info {
	unsigned short tlc_total_block_num;
	unsigned char tlc_bad_block_num;
};

struct stor_dev_program_size {
	unsigned short tlc_program_size;
	unsigned short slc_program_size;
};

struct stor_dev_reset_ftl {
	unsigned char op_type; /* 0:reset ftl, 1:close section */
	unsigned char stream_type; /* 0: normal, 1:datamove */
	unsigned char stream_id;
};

enum {
	PARTITION_TYPE_META0, /* first 4K-mapping region */
	PARTITION_TYPE_META1, /* second 4K-mapping region */
	PARTITION_TYPE_USER0, /* first 2M-mapping region */
	PARTITION_TYPE_USER1, /* second 2M-mapping region */
	PARTITION_TYPE_MAX
};

struct stor_dev_mapping_partition {
	unsigned int partion_start[PARTITION_TYPE_MAX];
	unsigned int partion_size[PARTITION_TYPE_MAX];
};

struct stor_dev_section_info {
	unsigned int section_start_lba;
	int flash_mode;
	unsigned char stream_type;
};

typedef void (*blk_dev_bad_block_notify_fn)(
		struct stor_dev_bad_block_info, void *);
typedef int (*lld_dev_pwron_info_sync_fn)(struct request_queue *,
		struct stor_dev_pwron_info *, unsigned int rescue_seg_size);
typedef int (*lld_dev_stream_oob_info_fetch_fn)(
		struct request_queue *, struct stor_dev_stream_info,
		unsigned int, struct stor_dev_stream_oob_info *);
typedef int (*lld_dev_reset_ftl_fn)(struct request_queue *,
		struct stor_dev_reset_ftl *);
typedef int (*lld_dev_read_section_fn)(
		struct request_queue *, unsigned int *);
typedef int (*lld_dev_read_mapping_partition_fn)(
		struct request_queue *, struct stor_dev_mapping_partition *);
typedef int (*lld_dev_config_mapping_partition_fn)(
		struct request_queue *, struct stor_dev_mapping_partition *);
typedef int (*lld_dev_fs_sync_done_fn)(struct request_queue *);
typedef int (*lld_dev_data_move_fn)(struct request_queue *,
		struct stor_dev_data_move_info *);
typedef int (*lld_dev_slc_mode_configuration_fn)(
		struct request_queue *, int *);
typedef int (*lld_dev_sync_read_verify_fn)(struct request_queue *,
		struct stor_dev_sync_read_verify_info *);
typedef int (*lld_dev_get_bad_block_info_fn)(
		struct request_queue *, struct stor_dev_bad_block_info *);
typedef int (*lld_dev_get_program_size_fn)(
		struct request_queue *, struct stor_dev_program_size *);
typedef int (*lld_dev_read_op_size_fn)(struct request_queue *, int *);
typedef int (*lld_dev_read_lrb_in_use_fn)(
		struct request_queue *, unsigned long *);
typedef int (*lld_dev_bad_block_notify_register_fn)(
		struct request_queue *, void (*func)(struct Scsi_Host *host,
		struct stor_dev_bad_block_info *bad_block_info));
#ifdef CONFIG_MAS_DEBUG_FS
typedef int (*lld_dev_rescue_block_inject_data_fn)(
		struct request_queue *, unsigned int);
typedef int (*lld_dev_bad_block_error_inject_fn)(
		struct request_queue *, unsigned char, unsigned char);
typedef int (*lld_dev_slc_mode_error_inject_fn)(
		struct request_queue *, unsigned char);
#endif
#endif

typedef void (*lld_dump_status_fn)(
	struct request_queue *, enum blk_dump_scene);
typedef int (*lld_tz_query_fn)(struct request_queue *q, u32 type, u8 *buf,
			       u32 buf_len);
typedef int (*lld_tz_ctrl_fn)(
	struct request_queue *q, int desc_id, uint8_t index);
typedef int (*blk_direct_flush_fn)(struct request_queue *);

#ifdef CONFIG_HP_CORE
typedef int (*lld_query_health_fn)(struct request_queue *q,
	u8 *pre_eol_info, u8 *life_time_est_a, u8 *life_time_est_b);
#endif

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
struct mas_unistore_ops {
	lld_dev_pwron_info_sync_fn dev_pwron_info_sync;
	lld_dev_stream_oob_info_fetch_fn dev_stream_oob_info_fetch;
	lld_dev_reset_ftl_fn dev_reset_ftl;
	lld_dev_read_section_fn dev_read_section;
	lld_dev_config_mapping_partition_fn dev_config_mapping_partition;
	lld_dev_read_mapping_partition_fn dev_read_mapping_partition;
	lld_dev_fs_sync_done_fn dev_fs_sync_done;
	lld_dev_data_move_fn dev_data_move;
	lld_dev_slc_mode_configuration_fn dev_slc_mode_configuration;
	lld_dev_sync_read_verify_fn dev_sync_read_verify;
	lld_dev_get_bad_block_info_fn dev_get_bad_block_info;
	blk_dev_bad_block_notify_fn dev_bad_block_notfiy_fn;
	void *dev_bad_block_notfiy_param_data;
	lld_dev_get_program_size_fn dev_get_program_size;
	lld_dev_bad_block_notify_register_fn dev_bad_block_notify_register;
	lld_dev_read_op_size_fn dev_read_op_size;
	lld_dev_read_lrb_in_use_fn dev_read_lrb_in_use;
#ifdef CONFIG_MAS_DEBUG_FS
	lld_dev_rescue_block_inject_data_fn dev_rescue_block_inject_data;
	lld_dev_bad_block_error_inject_fn dev_bad_block_err_inject;
	lld_dev_slc_mode_error_inject_fn dev_slc_mode_err_inject;
#endif
};
#endif

struct blk_dev_lld {
	/* Magic Number for the struct */
	unsigned int init_magic;
	/* The object on queue, tag or tagset */
	enum blk_lld_base type;
	/* Private data */
	void *data;
	/* is hw idle enabled */
	atomic_t hw_idle_en;
#define MAS_BLK_LLD_IDLE_INTR_EN (1 << 3)
	/* LLD Feature flag bit */
	unsigned long features;
#define MAS_BLK_LLD_IDLE_INTR_CAP (1 << 3)
	unsigned long lld_cap;
	lld_dump_status_fn dump_fn;
	/* IO Latency warning threshold */
	unsigned int latency_warning_threshold_ms;
	/* Emergency Flush Operation */
	blk_direct_flush_fn flush_fn;
	/* query api of turbu zone */
	lld_tz_query_fn tz_query;
	lld_tz_ctrl_fn tz_ctrl;

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	struct mas_unistore_ops unistore_ops;

	unsigned char lock_map[NR_CPUS];

	unsigned int mas_sec_size;
	unsigned int mas_pu_size;
	atomic_t bad_block_atomic;
	struct work_struct bad_block_work;
	struct stor_dev_bad_block_info bad_block_info;
	unsigned char last_stream_type;
	bool	fsync_ind;
	spinlock_t fsync_ind_lock;
	struct mas_bkops *bkops;
	spinlock_t expected_lba_lock[BLK_ORDER_STREAM_NUM];
	sector_t	expected_lba[BLK_ORDER_STREAM_NUM];
	sector_t	expected_pu[BLK_ORDER_STREAM_NUM];
	sector_t	current_pu_size[BLK_ORDER_STREAM_NUM];
	ktime_t expected_refresh_time[BLK_ORDER_STREAM_NUM];
	sector_t old_section[BLK_ORDER_STREAM_NUM];
	struct list_head section_list[BLK_ORDER_STREAM_NUM];

	atomic_t reset_cnt;
	atomic_t recovery_flag;
	atomic_t recovery_pwron_flag;
	atomic_t recovery_pwron_inprocess_flag;
	struct rw_semaphore recovery_rwsem;
	struct mutex recovery_mutex;
	struct list_head buf_bio_list[BLK_ORDER_STREAM_NUM + 1];
	spinlock_t buf_bio_list_lock[BLK_ORDER_STREAM_NUM + 1];
	unsigned int buf_bio_size[BLK_ORDER_STREAM_NUM + 1];
	unsigned int buf_bio_num[BLK_ORDER_STREAM_NUM + 1];
	unsigned int buf_page_num[BLK_ORDER_STREAM_NUM + 1];
	atomic_t replaced_page_cnt[BLK_ORDER_STREAM_NUM + 1];
	sector_t max_recovery_size;

	unsigned int write_curr_cnt;
	unsigned int write_pre_cnt;
	atomic_t async_fail_cnt;
#endif

	/* For busy idle feature */
	struct blk_idle_state blk_idle;
	/* accumulated write len of the whole device */
	unsigned long write_len;
	/* accumulated discard len of the whole device */
	unsigned long discard_len;
	bool dev_order_en;
	unsigned int write_num;
	spinlock_t write_num_lock;
	unsigned int make_req_nr;
	spinlock_t make_req_nr_lock;
	struct list_head lld_list;
	/*
	 * MAS IO Scheduler private data
	 */
	bool sched_ds_lld_inited;
	void *sched_ds_lld;
	int sqr_v;
	int sqw_v;
};

struct blk_queue_ops;

/*
 * This struct defines all the variable in vendor block layer.
 */
struct blk_queue_cust {
	/* The disk struct of the request queue */
	struct gendisk *queue_disk;
	/* The request queue has the partition table or not */
	bool blk_part_tbl_exist;
	unsigned long usr_ctrl_n;

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	bool io_prio_sim;
	unsigned long tz_write_bytes;
#endif

	/*
	 * Flush Optimise
	 */
	struct delayed_work flush_work;
	atomic_t flush_work_trigger;
	atomic_t write_after_flush;
	struct list_head flush_queue_node;
	int flush_optimize;
	/*
	 * IO latency statistic function
	 */
	int io_latency_enable;
	unsigned int io_lat_warning_thresh;
	struct list_head dump_list;
	/*
	 * IO Latency for test purpose only
	 */
	unsigned long sr_l; /* Seq Read Latency */
	unsigned long sw_l; /* Seq Write Latency */
	unsigned long rr_l; /* Rand Read Latency */
	unsigned long rw_l; /* Rand Write Latency */
	struct timer_list limit_setting_protect_timer;
	/*
	 * MAS IO Scheduler private data
	 */
	void *cust_queuedata;
};
#endif /* CONFIG_MAS_BLK */

struct request_queue {
	struct request		*last_merge;
	struct elevator_queue	*elevator;

	struct percpu_ref	q_usage_counter;

	struct blk_queue_stats	*stats;
	struct rq_qos		*rq_qos;

	const struct blk_mq_ops	*mq_ops;

	/* sw queues */
	struct blk_mq_ctx __percpu	*queue_ctx;

	unsigned int		queue_depth;

	/* hw dispatch queues */
	struct blk_mq_hw_ctx	**queue_hw_ctx;
	unsigned int		nr_hw_queues;

	struct backing_dev_info	*backing_dev_info;

	/*
	 * The queue owner gets to use this for whatever they like.
	 * ll_rw_blk doesn't touch it.
	 */
	void			*queuedata;

	/*
	 * various queue flags, see QUEUE_* below
	 */
	unsigned long		queue_flags;
	/*
	 * Number of contexts that have called blk_set_pm_only(). If this
	 * counter is above zero then only RQF_PM requests are processed.
	 */
	atomic_t		pm_only;

	/*
	 * ida allocated id for this queue.  Used to index queues from
	 * ioctx.
	 */
	int			id;

	/*
	 * queue needs bounce pages for pages above this limit
	 */
	gfp_t			bounce_gfp;

	spinlock_t		queue_lock;

	/*
	 * queue kobject
	 */
	struct kobject kobj;

	/*
	 * mq queue kobject
	 */
	struct kobject *mq_kobj;

#ifdef  CONFIG_BLK_DEV_INTEGRITY
	struct blk_integrity integrity;
#endif	/* CONFIG_BLK_DEV_INTEGRITY */

#ifdef CONFIG_PM
	struct device		*dev;
	enum rpm_status		rpm_status;
	unsigned int		nr_pending;
#endif

	/*
	 * queue settings
	 */
	unsigned long		nr_requests;	/* Max # of requests */

	unsigned int		dma_pad_mask;
	unsigned int		dma_alignment;

#ifdef CONFIG_BLK_INLINE_ENCRYPTION
	/* Inline crypto capabilities */
	struct blk_keyslot_manager *ksm;
#endif

	unsigned int		rq_timeout;
	int			poll_nsec;

	struct blk_stat_callback	*poll_cb;
	struct blk_rq_stat	poll_stat[BLK_MQ_POLL_STATS_BKTS];

	struct timer_list	timeout;
	struct work_struct	timeout_work;

	atomic_t		nr_active_requests_shared_sbitmap;

	struct list_head	icq_list;
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
	int inline_crypt_support;
#endif
#ifdef CONFIG_BLK_CGROUP
	DECLARE_BITMAP		(blkcg_pols, BLKCG_MAX_POLS);
	struct blkcg_gq		*root_blkg;
	struct list_head	blkg_list;
#endif

	struct queue_limits	limits;

#ifdef CONFIG_HP_CORE
	/* query api of health */
	lld_query_health_fn health_query;
#endif

#ifdef CONFIG_MAS_BLK
	struct blk_queue_cust mas_queue;
	struct blk_queue_ops *mas_queue_ops;
	struct blk_dev_lld lld_func;
#endif

	unsigned int		required_elevator_features;

#ifdef CONFIG_BLK_DEV_ZONED
	/*
	 * Zoned block device information for request dispatch control.
	 * nr_zones is the total number of zones of the device. This is always
	 * 0 for regular block devices. conv_zones_bitmap is a bitmap of nr_zones
	 * bits which indicates if a zone is conventional (bit set) or
	 * sequential (bit clear). seq_zones_wlock is a bitmap of nr_zones
	 * bits which indicates if a zone is write locked, that is, if a write
	 * request targeting the zone was dispatched. All three fields are
	 * initialized by the low level device driver (e.g. scsi/sd.c).
	 * Stacking drivers (device mappers) may or may not initialize
	 * these fields.
	 *
	 * Reads of this information must be protected with blk_queue_enter() /
	 * blk_queue_exit(). Modifying this information is only allowed while
	 * no requests are being processed. See also blk_mq_freeze_queue() and
	 * blk_mq_unfreeze_queue().
	 */
	unsigned int		nr_zones;
	unsigned long		*conv_zones_bitmap;
	unsigned long		*seq_zones_wlock;
	unsigned int		max_open_zones;
	unsigned int		max_active_zones;
#endif /* CONFIG_BLK_DEV_ZONED */

	/*
	 * sg stuff
	 */
	unsigned int		sg_timeout;
	unsigned int		sg_reserved_size;
	int			node;
	struct mutex		debugfs_mutex;
#ifdef CONFIG_BLK_DEV_IO_TRACE
	struct blk_trace __rcu	*blk_trace;
#endif
	/*
	 * for flush operations
	 */
	struct blk_flush_queue	*fq;

	struct list_head	requeue_list;
	spinlock_t		requeue_lock;
	struct delayed_work	requeue_work;

	struct mutex		sysfs_lock;
	struct mutex		sysfs_dir_lock;

	/*
	 * for reusing dead hctx instance in case of updating
	 * nr_hw_queues
	 */
	struct list_head	unused_hctx_list;
	spinlock_t		unused_hctx_lock;

	int			mq_freeze_depth;

#if defined(CONFIG_BLK_DEV_BSG)
	struct bsg_class_device bsg_dev;
#endif

#ifdef CONFIG_BLK_DEV_THROTTLING
	/* Throttle data */
	struct throtl_data *td;
	/* for turbo zone */
	struct block_device *tz_weighted_bdev;
#endif

#ifdef CONFIG_BLK_CGROUP_IOSMART
	struct iosmart_data *sd;
#endif

	struct rcu_head		rcu_head;
	wait_queue_head_t	mq_freeze_wq;
	/*
	 * Protect concurrent access to q_usage_counter by
	 * percpu_ref_kill() and percpu_ref_reinit().
	 */
	struct mutex		mq_freeze_lock;

	struct blk_mq_tag_set	*tag_set;
	struct list_head	tag_set_list;
	struct bio_set		bio_split;

	struct dentry		*debugfs_dir;

#ifdef CONFIG_BLK_DEBUG_FS
	struct dentry		*sched_debugfs_dir;
	struct dentry		*rqos_debugfs_dir;
#endif

	bool			mq_sysfs_init_done;

	size_t			cmd_size;

#define BLK_MAX_WRITE_HINTS	5
	u64			write_hints[BLK_MAX_WRITE_HINTS];

	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
	ANDROID_KABI_RESERVE(3);
	ANDROID_KABI_RESERVE(4);
	ANDROID_OEM_DATA(1);
};

/* Keep blk_queue_flag_name[] in sync with the definitions below */
#define QUEUE_FLAG_STOPPED	0	/* queue is stopped */
#define QUEUE_FLAG_DYING	1	/* queue being torn down */
#define QUEUE_FLAG_NOMERGES     3	/* disable merge attempts */
#define QUEUE_FLAG_SAME_COMP	4	/* complete on same CPU-group */
#define QUEUE_FLAG_FAIL_IO	5	/* fake timeout */
#define QUEUE_FLAG_NONROT	6	/* non-rotational device (SSD) */
#define QUEUE_FLAG_VIRT		QUEUE_FLAG_NONROT /* paravirt device */
#define QUEUE_FLAG_IO_STAT	7	/* do disk/partitions IO accounting */
#define QUEUE_FLAG_DISCARD	8	/* supports DISCARD */
#define QUEUE_FLAG_NOXMERGES	9	/* No extended merges */
#define QUEUE_FLAG_ADD_RANDOM	10	/* Contributes to random pool */
#define QUEUE_FLAG_SECERASE	11	/* supports secure erase */
#define QUEUE_FLAG_SAME_FORCE	12	/* force complete on same CPU */
#define QUEUE_FLAG_DEAD		13	/* queue tear-down finished */
#define QUEUE_FLAG_INIT_DONE	14	/* queue is initialized */
#define QUEUE_FLAG_STABLE_WRITES 15	/* don't modify blks until WB is done */
#define QUEUE_FLAG_POLL		16	/* IO polling enabled if set */
#define QUEUE_FLAG_WC		17	/* Write back caching */
#define QUEUE_FLAG_FUA		18	/* device supports FUA writes */
#define QUEUE_FLAG_DAX		19	/* device supports DAX */
#define QUEUE_FLAG_STATS	20	/* track IO start and completion times */
#define QUEUE_FLAG_POLL_STATS	21	/* collecting stats for hybrid polling */
#define QUEUE_FLAG_REGISTERED	22	/* queue has been registered to a disk */
#define QUEUE_FLAG_SCSI_PASSTHROUGH 23	/* queue supports SCSI commands */
#define QUEUE_FLAG_QUIESCED	24	/* queue has been quiesced */
#define QUEUE_FLAG_PCI_P2PDMA	25	/* device supports PCI p2p requests */
#define QUEUE_FLAG_ZONE_RESETALL 26	/* supports Zone Reset All */
#define QUEUE_FLAG_RQ_ALLOC_TIME 27	/* record rq->alloc_time_ns */
#define QUEUE_FLAG_HCTX_ACTIVE	28	/* at least one blk-mq hctx is active */
#define QUEUE_FLAG_NOWAIT       29	/* device supports NOWAIT */
#define QUEUE_FLAG_QOS		   30		 /* device supports qos on off */

#define QUEUE_FLAG_MQ_DEFAULT	((1 << QUEUE_FLAG_IO_STAT) |		\
				 (1 << QUEUE_FLAG_SAME_COMP) |		\
				 (1 << QUEUE_FLAG_NOWAIT))

void blk_queue_flag_set(unsigned int flag, struct request_queue *q);
void blk_queue_flag_clear(unsigned int flag, struct request_queue *q);
bool blk_queue_flag_test_and_set(unsigned int flag, struct request_queue *q);

#define blk_queue_stopped(q)	test_bit(QUEUE_FLAG_STOPPED, &(q)->queue_flags)
#define blk_queue_dying(q)	test_bit(QUEUE_FLAG_DYING, &(q)->queue_flags)
#define blk_queue_dead(q)	test_bit(QUEUE_FLAG_DEAD, &(q)->queue_flags)
#define blk_queue_init_done(q)	test_bit(QUEUE_FLAG_INIT_DONE, &(q)->queue_flags)
#define blk_queue_nomerges(q)	test_bit(QUEUE_FLAG_NOMERGES, &(q)->queue_flags)
#define blk_queue_noxmerges(q)	\
	test_bit(QUEUE_FLAG_NOXMERGES, &(q)->queue_flags)
#define blk_queue_nonrot(q)	test_bit(QUEUE_FLAG_NONROT, &(q)->queue_flags)
#define blk_queue_stable_writes(q) \
	test_bit(QUEUE_FLAG_STABLE_WRITES, &(q)->queue_flags)
#define blk_queue_io_stat(q)	test_bit(QUEUE_FLAG_IO_STAT, &(q)->queue_flags)
#define blk_queue_add_random(q)	test_bit(QUEUE_FLAG_ADD_RANDOM, &(q)->queue_flags)
#define blk_queue_discard(q)	test_bit(QUEUE_FLAG_DISCARD, &(q)->queue_flags)
#define blk_queue_zone_resetall(q)	\
	test_bit(QUEUE_FLAG_ZONE_RESETALL, &(q)->queue_flags)
#define blk_queue_secure_erase(q) \
	(test_bit(QUEUE_FLAG_SECERASE, &(q)->queue_flags))
#define blk_queue_dax(q)	test_bit(QUEUE_FLAG_DAX, &(q)->queue_flags)
#define blk_queue_qos_on(q)	test_bit(QUEUE_FLAG_QOS, &(q)->queue_flags)
#define blk_queue_scsi_passthrough(q)	\
	test_bit(QUEUE_FLAG_SCSI_PASSTHROUGH, &(q)->queue_flags)
#define blk_queue_pci_p2pdma(q)	\
	test_bit(QUEUE_FLAG_PCI_P2PDMA, &(q)->queue_flags)
#ifdef CONFIG_BLK_RQ_ALLOC_TIME
#define blk_queue_rq_alloc_time(q)	\
	test_bit(QUEUE_FLAG_RQ_ALLOC_TIME, &(q)->queue_flags)
#else
#define blk_queue_rq_alloc_time(q)	false
#endif

#define blk_noretry_request(rq) \
	((rq)->cmd_flags & (REQ_FAILFAST_DEV|REQ_FAILFAST_TRANSPORT| \
			     REQ_FAILFAST_DRIVER))
#define blk_queue_quiesced(q)	test_bit(QUEUE_FLAG_QUIESCED, &(q)->queue_flags)
#define blk_queue_pm_only(q)	atomic_read(&(q)->pm_only)
#define blk_queue_fua(q)	test_bit(QUEUE_FLAG_FUA, &(q)->queue_flags)
#define blk_queue_registered(q)	test_bit(QUEUE_FLAG_REGISTERED, &(q)->queue_flags)
#define blk_queue_nowait(q)	test_bit(QUEUE_FLAG_NOWAIT, &(q)->queue_flags)

extern void blk_set_pm_only(struct request_queue *q);
extern void blk_clear_pm_only(struct request_queue *q);

static inline bool blk_account_rq(struct request *rq)
{
	return (rq->rq_flags & RQF_STARTED) && !blk_rq_is_passthrough(rq);
}

#define list_entry_rq(ptr)	list_entry((ptr), struct request, queuelist)

#define rq_data_dir(rq)		(op_is_write(req_op(rq)) ? WRITE : READ)

#define rq_dma_dir(rq) \
	(op_is_write(req_op(rq)) ? DMA_TO_DEVICE : DMA_FROM_DEVICE)

#define dma_map_bvec(dev, bv, dir, attrs) \
	dma_map_page_attrs(dev, (bv)->bv_page, (bv)->bv_offset, (bv)->bv_len, \
	(dir), (attrs))

static inline bool queue_is_mq(struct request_queue *q)
{
	return q->mq_ops;
}

static inline enum blk_zoned_model
blk_queue_zoned_model(struct request_queue *q)
{
	if (IS_ENABLED(CONFIG_BLK_DEV_ZONED))
		return q->limits.zoned;
	return BLK_ZONED_NONE;
}

static inline bool blk_queue_is_zoned(struct request_queue *q)
{
	switch (blk_queue_zoned_model(q)) {
	case BLK_ZONED_HA:
	case BLK_ZONED_HM:
		return true;
	default:
		return false;
	}
}

static inline sector_t blk_queue_zone_sectors(struct request_queue *q)
{
	return blk_queue_is_zoned(q) ? q->limits.chunk_sectors : 0;
}

#ifdef CONFIG_BLK_DEV_ZONED
static inline unsigned int blk_queue_nr_zones(struct request_queue *q)
{
	return blk_queue_is_zoned(q) ? q->nr_zones : 0;
}

static inline unsigned int blk_queue_zone_no(struct request_queue *q,
					     sector_t sector)
{
	if (!blk_queue_is_zoned(q))
		return 0;
	return sector >> ilog2(q->limits.chunk_sectors);
}

static inline bool blk_queue_zone_is_seq(struct request_queue *q,
					 sector_t sector)
{
	if (!blk_queue_is_zoned(q))
		return false;
	if (!q->conv_zones_bitmap)
		return true;
	return !test_bit(blk_queue_zone_no(q, sector), q->conv_zones_bitmap);
}

static inline void blk_queue_max_open_zones(struct request_queue *q,
		unsigned int max_open_zones)
{
	q->max_open_zones = max_open_zones;
}

static inline unsigned int queue_max_open_zones(const struct request_queue *q)
{
	return q->max_open_zones;
}

static inline void blk_queue_max_active_zones(struct request_queue *q,
		unsigned int max_active_zones)
{
	q->max_active_zones = max_active_zones;
}

static inline unsigned int queue_max_active_zones(const struct request_queue *q)
{
	return q->max_active_zones;
}
#else /* CONFIG_BLK_DEV_ZONED */
static inline unsigned int blk_queue_nr_zones(struct request_queue *q)
{
	return 0;
}
static inline bool blk_queue_zone_is_seq(struct request_queue *q,
					 sector_t sector)
{
	return false;
}
static inline unsigned int blk_queue_zone_no(struct request_queue *q,
					     sector_t sector)
{
	return 0;
}
static inline unsigned int queue_max_open_zones(const struct request_queue *q)
{
	return 0;
}
static inline unsigned int queue_max_active_zones(const struct request_queue *q)
{
	return 0;
}
#endif /* CONFIG_BLK_DEV_ZONED */

static inline bool rq_is_sync(struct request *rq)
{
	return op_is_sync(rq->cmd_flags);
}

static inline bool rq_mergeable(struct request *rq)
{
	if (blk_rq_is_passthrough(rq))
		return false;

	if (req_op(rq) == REQ_OP_FLUSH)
		return false;

	if (req_op(rq) == REQ_OP_WRITE_ZEROES)
		return false;

	if (req_op(rq) == REQ_OP_ZONE_APPEND)
		return false;

	if (rq->cmd_flags & REQ_NOMERGE_FLAGS)
		return false;
	if (rq->rq_flags & RQF_NOMERGE_FLAGS)
		return false;

#ifdef CONFIG_MAS_BLK
	if (rq->rq_flags & RQF_DONTPREP)
		return false;
#endif

	return true;
}

static inline bool blk_write_same_mergeable(struct bio *a, struct bio *b)
{
	if (bio_page(a) == bio_page(b) &&
	    bio_offset(a) == bio_offset(b))
		return true;

	return false;
}

static inline unsigned int blk_queue_depth(struct request_queue *q)
{
	if (q->queue_depth)
		return q->queue_depth;

	return q->nr_requests;
}

extern unsigned long blk_max_low_pfn, blk_max_pfn;

/*
 * standard bounce addresses:
 *
 * BLK_BOUNCE_HIGH	: bounce all highmem pages
 * BLK_BOUNCE_ANY	: don't bounce anything
 * BLK_BOUNCE_ISA	: bounce pages above ISA DMA boundary
 */

#if BITS_PER_LONG == 32
#define BLK_BOUNCE_HIGH		((u64)blk_max_low_pfn << PAGE_SHIFT)
#else
#define BLK_BOUNCE_HIGH		-1ULL
#endif
#define BLK_BOUNCE_ANY		(-1ULL)
#define BLK_BOUNCE_ISA		(DMA_BIT_MASK(24))

/*
 * default timeout for SG_IO if none specified
 */
#define BLK_DEFAULT_SG_TIMEOUT	(60 * HZ)
#define BLK_MIN_SG_TIMEOUT	(7 * HZ)

struct rq_map_data {
	struct page **pages;
	int page_order;
	int nr_entries;
	unsigned long offset;
	int null_mapped;
	int from_user;
};

struct req_iterator {
	struct bvec_iter iter;
	struct bio *bio;
};

/* This should not be used directly - use rq_for_each_segment */
#define for_each_bio(_bio)		\
	for (; _bio; _bio = _bio->bi_next)
#define __rq_for_each_bio(_bio, rq)	\
	if ((rq->bio))			\
		for (_bio = (rq)->bio; _bio; _bio = _bio->bi_next)

#define rq_for_each_segment(bvl, _rq, _iter)			\
	__rq_for_each_bio(_iter.bio, _rq)			\
		bio_for_each_segment(bvl, _iter.bio, _iter.iter)

#define rq_for_each_bvec(bvl, _rq, _iter)			\
	__rq_for_each_bio(_iter.bio, _rq)			\
		bio_for_each_bvec(bvl, _iter.bio, _iter.iter)

#define rq_iter_last(bvec, _iter)				\
		(_iter.bio->bi_next == NULL &&			\
		 bio_iter_last(bvec, _iter.iter))

#ifndef ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE
# error	"You should define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE for your platform"
#endif
#if ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE
extern void rq_flush_dcache_pages(struct request *rq);
#else
static inline void rq_flush_dcache_pages(struct request *rq)
{
}
#endif

extern int blkdev_get(struct block_device *bdev, fmode_t mode, void *holder);
extern int blk_register_queue(struct gendisk *disk);
extern void blk_unregister_queue(struct gendisk *disk);
blk_qc_t submit_bio_noacct(struct bio *bio);
extern void blk_rq_init(struct request_queue *q, struct request *rq);
extern void blk_put_request(struct request *);
extern struct request *blk_get_request(struct request_queue *, unsigned int op,
				       blk_mq_req_flags_t flags);
extern int blk_lld_busy(struct request_queue *q);
extern int blk_rq_prep_clone(struct request *rq, struct request *rq_src,
			     struct bio_set *bs, gfp_t gfp_mask,
			     int (*bio_ctr)(struct bio *, struct bio *, void *),
			     void *data);
extern void blk_rq_unprep_clone(struct request *rq);
extern blk_status_t blk_insert_cloned_request(struct request_queue *q,
				     struct request *rq);
extern int blk_rq_append_bio(struct request *rq, struct bio **bio);
extern void blk_queue_split(struct bio **);
extern int scsi_verify_blk_ioctl(struct block_device *, unsigned int);
extern int scsi_cmd_blk_ioctl(struct block_device *, fmode_t,
			      unsigned int, void __user *);
extern int scsi_cmd_ioctl(struct request_queue *, struct gendisk *, fmode_t,
			  unsigned int, void __user *);
extern int sg_scsi_ioctl(struct request_queue *, struct gendisk *, fmode_t,
			 struct scsi_ioctl_command __user *);
extern int get_sg_io_hdr(struct sg_io_hdr *hdr, const void __user *argp);
extern int put_sg_io_hdr(const struct sg_io_hdr *hdr, void __user *argp);

extern int blk_queue_enter(struct request_queue *q, blk_mq_req_flags_t flags);
extern void blk_queue_exit(struct request_queue *q);
extern void blk_sync_queue(struct request_queue *q);
extern int blk_rq_map_user(struct request_queue *, struct request *,
			   struct rq_map_data *, void __user *, unsigned long,
			   gfp_t);
extern int blk_rq_unmap_user(struct bio *);
extern int blk_rq_map_kern(struct request_queue *, struct request *, void *, unsigned int, gfp_t);
extern int blk_rq_map_user_iov(struct request_queue *, struct request *,
			       struct rq_map_data *, const struct iov_iter *,
			       gfp_t);
extern void blk_execute_rq(struct request_queue *, struct gendisk *,
			  struct request *, int);
extern void blk_execute_rq_nowait(struct request_queue *, struct gendisk *,
				  struct request *, int, rq_end_io_fn *);

/* Helper to convert REQ_OP_XXX to its string format XXX */
extern const char *blk_op_str(unsigned int op);

int blk_status_to_errno(blk_status_t status);
blk_status_t errno_to_blk_status(int errno);

int blk_poll(struct request_queue *q, blk_qc_t cookie, bool spin);

static inline struct request_queue *bdev_get_queue(struct block_device *bdev)
{
	return bdev->bd_disk->queue;	/* this is never NULL */
}

/*
 * The basic unit of block I/O is a sector. It is used in a number of contexts
 * in Linux (blk, bio, genhd). The size of one sector is 512 = 2**9
 * bytes. Variables of type sector_t represent an offset or size that is a
 * multiple of 512 bytes. Hence these two constants.
 */
#ifndef SECTOR_SHIFT
#define SECTOR_SHIFT 9
#endif
#ifndef SECTOR_SIZE
#define SECTOR_SIZE (1 << SECTOR_SHIFT)
#endif

/*
 * blk_rq_pos()			: the current sector
 * blk_rq_bytes()		: bytes left in the entire request
 * blk_rq_cur_bytes()		: bytes left in the current segment
 * blk_rq_err_bytes()		: bytes left till the next error boundary
 * blk_rq_sectors()		: sectors left in the entire request
 * blk_rq_cur_sectors()		: sectors left in the current segment
 * blk_rq_stats_sectors()	: sectors of the entire request used for stats
 */
static inline sector_t blk_rq_pos(const struct request *rq)
{
	return rq->__sector;
}

static inline unsigned int blk_rq_bytes(const struct request *rq)
{
	return rq->__data_len;
}

static inline int blk_rq_cur_bytes(const struct request *rq)
{
	return rq->bio ? bio_cur_bytes(rq->bio) : 0;
}

extern unsigned int blk_rq_err_bytes(const struct request *rq);

static inline unsigned int blk_rq_sectors(const struct request *rq)
{
	return blk_rq_bytes(rq) >> SECTOR_SHIFT;
}

static inline unsigned int blk_rq_cur_sectors(const struct request *rq)
{
	return blk_rq_cur_bytes(rq) >> SECTOR_SHIFT;
}

static inline unsigned int blk_rq_stats_sectors(const struct request *rq)
{
	return rq->stats_sectors;
}

#ifdef CONFIG_BLK_DEV_ZONED

/* Helper to convert BLK_ZONE_ZONE_XXX to its string format XXX */
const char *blk_zone_cond_str(enum blk_zone_cond zone_cond);

static inline unsigned int blk_rq_zone_no(struct request *rq)
{
	return blk_queue_zone_no(rq->q, blk_rq_pos(rq));
}

static inline unsigned int blk_rq_zone_is_seq(struct request *rq)
{
	return blk_queue_zone_is_seq(rq->q, blk_rq_pos(rq));
}
#endif /* CONFIG_BLK_DEV_ZONED */

/*
 * Some commands like WRITE SAME have a payload or data transfer size which
 * is different from the size of the request.  Any driver that supports such
 * commands using the RQF_SPECIAL_PAYLOAD flag needs to use this helper to
 * calculate the data transfer size.
 */
static inline unsigned int blk_rq_payload_bytes(struct request *rq)
{
	if (rq->rq_flags & RQF_SPECIAL_PAYLOAD)
		return rq->special_vec.bv_len;
	return blk_rq_bytes(rq);
}

/*
 * Return the first full biovec in the request.  The caller needs to check that
 * there are any bvecs before calling this helper.
 */
static inline struct bio_vec req_bvec(struct request *rq)
{
	if (rq->rq_flags & RQF_SPECIAL_PAYLOAD)
		return rq->special_vec;
	return mp_bvec_iter_bvec(rq->bio->bi_io_vec, rq->bio->bi_iter);
}

static inline unsigned int blk_queue_get_max_sectors(struct request_queue *q,
						     int op)
{
	if (unlikely(op == REQ_OP_DISCARD || op == REQ_OP_SECURE_ERASE))
		return min(q->limits.max_discard_sectors,
			   UINT_MAX >> SECTOR_SHIFT);

	if (unlikely(op == REQ_OP_WRITE_SAME))
		return q->limits.max_write_same_sectors;

	if (unlikely(op == REQ_OP_WRITE_ZEROES))
		return q->limits.max_write_zeroes_sectors;

	return q->limits.max_sectors;
}

/*
 * Return maximum size of a request at given offset. Only valid for
 * file system requests.
 */
static inline unsigned int blk_max_size_offset(struct request_queue *q,
					       sector_t offset,
					       unsigned int chunk_sectors)
{
	if (!chunk_sectors) {
		if (q->limits.chunk_sectors)
			chunk_sectors = q->limits.chunk_sectors;
		else
			return q->limits.max_sectors;
	}

	if (likely(is_power_of_2(chunk_sectors)))
		chunk_sectors -= offset & (chunk_sectors - 1);
	else
		chunk_sectors -= sector_div(offset, chunk_sectors);

	return min(q->limits.max_sectors, chunk_sectors);
}

static inline unsigned int blk_rq_get_max_sectors(struct request *rq,
						  sector_t offset)
{
	struct request_queue *q = rq->q;

	if (blk_rq_is_passthrough(rq))
		return q->limits.max_hw_sectors;

	if (!q->limits.chunk_sectors ||
	    req_op(rq) == REQ_OP_DISCARD ||
	    req_op(rq) == REQ_OP_SECURE_ERASE)
		return blk_queue_get_max_sectors(q, req_op(rq));

	return min(blk_max_size_offset(q, offset, 0),
			blk_queue_get_max_sectors(q, req_op(rq)));
}

static inline unsigned int blk_rq_count_bios(struct request *rq)
{
	unsigned int nr_bios = 0;
	struct bio *bio;

	__rq_for_each_bio(bio, rq)
		nr_bios++;

	return nr_bios;
}

void blk_steal_bios(struct bio_list *list, struct request *rq);

/*
 * Request completion related functions.
 *
 * blk_update_request() completes given number of bytes and updates
 * the request without completing it.
 */
extern bool blk_update_request(struct request *rq, blk_status_t error,
			       unsigned int nr_bytes);

extern void blk_abort_request(struct request *);

/*
 * Access functions for manipulating queue properties
 */
extern void blk_cleanup_queue(struct request_queue *);
extern void blk_queue_bounce_limit(struct request_queue *, u64);
extern void blk_queue_max_hw_sectors(struct request_queue *, unsigned int);
extern void blk_queue_chunk_sectors(struct request_queue *, unsigned int);
extern void blk_queue_max_segments(struct request_queue *, unsigned short);
extern void blk_queue_max_discard_segments(struct request_queue *,
		unsigned short);
extern void blk_queue_max_segment_size(struct request_queue *, unsigned int);
extern void blk_queue_max_discard_sectors(struct request_queue *q,
		unsigned int max_discard_sectors);
extern void blk_queue_max_write_same_sectors(struct request_queue *q,
		unsigned int max_write_same_sectors);
extern void blk_queue_max_write_zeroes_sectors(struct request_queue *q,
		unsigned int max_write_same_sectors);
extern void blk_queue_logical_block_size(struct request_queue *, unsigned int);
extern void blk_queue_max_zone_append_sectors(struct request_queue *q,
		unsigned int max_zone_append_sectors);
extern void blk_queue_physical_block_size(struct request_queue *, unsigned int);
extern void blk_queue_alignment_offset(struct request_queue *q,
				       unsigned int alignment);
void blk_queue_update_readahead(struct request_queue *q);
extern void blk_limits_io_min(struct queue_limits *limits, unsigned int min);
extern void blk_queue_io_min(struct request_queue *q, unsigned int min);
extern void blk_limits_io_opt(struct queue_limits *limits, unsigned int opt);
extern void blk_queue_io_opt(struct request_queue *q, unsigned int opt);
extern void blk_set_queue_depth(struct request_queue *q, unsigned int depth);
extern void blk_set_default_limits(struct queue_limits *lim);
extern void blk_set_stacking_limits(struct queue_limits *lim);
extern int blk_stack_limits(struct queue_limits *t, struct queue_limits *b,
			    sector_t offset);
extern void disk_stack_limits(struct gendisk *disk, struct block_device *bdev,
			      sector_t offset);
extern void blk_queue_update_dma_pad(struct request_queue *, unsigned int);
extern void blk_queue_segment_boundary(struct request_queue *, unsigned long);
extern void blk_queue_virt_boundary(struct request_queue *, unsigned long);
extern void blk_queue_dma_alignment(struct request_queue *, int);
extern void blk_queue_update_dma_alignment(struct request_queue *, int);
extern void blk_queue_rq_timeout(struct request_queue *, unsigned int);
extern void blk_queue_write_cache(struct request_queue *q, bool enabled, bool fua);
extern void blk_queue_required_elevator_features(struct request_queue *q,
						 unsigned int features);
extern bool blk_queue_can_use_dma_map_merging(struct request_queue *q,
					      struct device *dev);

/*
 * Number of physical segments as sent to the device.
 *
 * Normally this is the number of discontiguous data segments sent by the
 * submitter.  But for data-less command like discard we might have no
 * actual data segments submitted, but the driver might have to add it's
 * own special payload.  In that case we still return 1 here so that this
 * special payload will be mapped.
 */
static inline unsigned short blk_rq_nr_phys_segments(struct request *rq)
{
	if (rq->rq_flags & RQF_SPECIAL_PAYLOAD)
		return 1;
	return rq->nr_phys_segments;
}

/*
 * Number of discard segments (or ranges) the driver needs to fill in.
 * Each discard bio merged into a request is counted as one segment.
 */
static inline unsigned short blk_rq_nr_discard_segments(struct request *rq)
{
	return max_t(unsigned short, rq->nr_phys_segments, 1);
}

int __blk_rq_map_sg(struct request_queue *q, struct request *rq,
		struct scatterlist *sglist, struct scatterlist **last_sg);
static inline int blk_rq_map_sg(struct request_queue *q, struct request *rq,
		struct scatterlist *sglist)
{
	struct scatterlist *last_sg = NULL;

	return __blk_rq_map_sg(q, rq, sglist, &last_sg);
}
extern void blk_dump_rq_flags(struct request *, char *);

bool __must_check blk_get_queue(struct request_queue *);
struct request_queue *blk_alloc_queue(int node_id);
extern void blk_put_queue(struct request_queue *);
extern void blk_set_queue_dying(struct request_queue *);

#ifdef CONFIG_BLOCK
/*
 * blk_plug permits building a queue of related requests by holding the I/O
 * fragments for a short period. This allows merging of sequential requests
 * into single larger request. As the requests are moved from a per-task list to
 * the device's request_queue in a batch, this results in improved scalability
 * as the lock contention for request_queue lock is reduced.
 *
 * It is ok not to disable preemption when adding the request to the plug list
 * or when attempting a merge, because blk_schedule_flush_list() will only flush
 * the plug list when the task sleeps by itself. For details, please see
 * schedule() where blk_schedule_flush_plug() is called.
 */
struct blk_plug {
	struct list_head mq_list; /* blk-mq requests */
	struct list_head cb_list; /* md requires an unplug callback */
	unsigned short rq_count;
	bool multiple_queues;
	bool nowait;
#ifdef CONFIG_MAS_BLK
	struct list_head mas_blk_list;
	void (*flush_plug_list_fn)(struct blk_plug *, bool);
#endif
};
#define BLK_MAX_REQUEST_COUNT 16
#define BLK_PLUG_FLUSH_SIZE (128 * 1024)

struct blk_plug_cb;
typedef void (*blk_plug_cb_fn)(struct blk_plug_cb *, bool);
struct blk_plug_cb {
	struct list_head list;
	blk_plug_cb_fn callback;
	void *data;
};
extern struct blk_plug_cb *blk_check_plugged(blk_plug_cb_fn unplug,
					     void *data, int size);
extern void blk_start_plug(struct blk_plug *);
extern void blk_finish_plug(struct blk_plug *);
extern void blk_flush_plug_list(struct blk_plug *, bool);

static inline void blk_flush_plug(struct task_struct *tsk)
{
	struct blk_plug *plug = tsk->plug;

	if (plug)
		blk_flush_plug_list(plug, false);
}

static inline void blk_schedule_flush_plug(struct task_struct *tsk)
{
	struct blk_plug *plug = tsk->plug;

	if (plug)
		blk_flush_plug_list(plug, true);
}

static inline bool blk_needs_flush_plug(struct task_struct *tsk)
{
	struct blk_plug *plug = tsk->plug;

	return plug &&
		 (!list_empty(&plug->mq_list) ||
#ifdef CONFIG_MAS_BLK
		!list_empty(&plug->mas_blk_list) ||
#endif
		 !list_empty(&plug->cb_list));
}

int blkdev_issue_flush(struct block_device *, gfp_t);
long nr_blockdev_pages(void);

#if defined(CONFIG_MAS_BLK) && defined(CONFIG_MAS_UNISTORE_PRESERVE)
struct request *mas_blk_get_request_reset(
	struct request_queue *q, unsigned int op, gfp_t gfp_mask);
void mas_blk_queue_split_for_wop_write(
	struct request_queue *, struct bio **);
void mas_blk_fsync_barrier(struct block_device *bdev);
void mas_blk_dump_unistore(struct request_queue *q, unsigned char *prefix);
int mas_blk_data_move(struct block_device *bi_bdev,
	struct stor_dev_data_move_info *data_move_info);
void mas_blk_mq_tagset_data_move_register(
	struct blk_mq_tag_set *tag_set, lld_dev_data_move_fn func);
int mas_blk_slc_mode_configuration(
	struct block_device *bi_bdev, int *status);
void mas_blk_mq_tagset_slc_mode_configuration_register(
	struct blk_mq_tag_set *tag_set, lld_dev_slc_mode_configuration_fn func);
int mas_blk_sync_read_verify(struct block_device *bi_bdev,
	struct stor_dev_sync_read_verify_info *verify_info);
void mas_blk_mq_tagset_sync_read_verify_register(
	struct blk_mq_tag_set *tag_set, lld_dev_sync_read_verify_fn func);
int mas_blk_get_bad_block_info(struct block_device *bi_bdev,
	struct stor_dev_bad_block_info *bad_block_info);
void mas_blk_mq_tagset_get_bad_block_info_register(
	struct blk_mq_tag_set *tag_set, lld_dev_get_bad_block_info_fn func);
int mas_blk_device_pwron_info_sync(struct block_device *bi_bdev,
	struct stor_dev_pwron_info *stor_info, unsigned int rescue_seg_size);
void mas_blk_mq_tagset_pwron_info_sync_register(
	struct blk_mq_tag_set *tag_set, lld_dev_pwron_info_sync_fn func);
int mas_blk_stream_oob_info_fetch(struct block_device *bi_bdev,
	struct stor_dev_stream_info stream_info, unsigned int oob_entry_cnt,
	struct stor_dev_stream_oob_info *oob_info);
void mas_blk_mq_tagset_stream_oob_info_register(
	struct blk_mq_tag_set *tag_set, lld_dev_stream_oob_info_fetch_fn func);
int mas_blk_device_read_section(
	struct block_device *bi_bdev, unsigned int *section_size);
void mas_blk_mq_tagset_read_section_register(
	struct blk_mq_tag_set *tag_set, lld_dev_read_section_fn func);
void mas_blk_mq_tagset_read_lrb_in_use_register(
	struct blk_mq_tag_set *tag_set, lld_dev_read_lrb_in_use_fn func);
void mas_blk_mq_tagset_read_op_size_register(
	struct blk_mq_tag_set *tag_set, lld_dev_read_op_size_fn func);
int mas_blk_device_config_mapping_partition(
	struct block_device *bi_bdev,
	struct stor_dev_mapping_partition *mapping_info);
void mas_blk_mq_tagset_config_mapping_partition_register(
	struct blk_mq_tag_set *tag_set, lld_dev_config_mapping_partition_fn func);
int mas_blk_device_read_mapping_partition(
	struct block_device *bi_bdev,
	struct stor_dev_mapping_partition *mapping_info);
int mas_blk_device_read_op_size(struct block_device *bi_bdev,
	int *op_size);
void mas_blk_mq_tagset_read_mapping_partition_register(
	struct blk_mq_tag_set *tag_set, lld_dev_read_mapping_partition_fn func);
int mas_blk_fs_sync_done(struct block_device *bi_bdev);
void mas_blk_mq_tagset_fs_sync_done_register(
	struct blk_mq_tag_set *tag_set, lld_dev_fs_sync_done_fn func);
int mas_blk_get_program_size(struct block_device *bi_bdev,
	struct stor_dev_program_size *program_size);
void mas_blk_mq_tagset_get_program_size_register(
	struct blk_mq_tag_set *tag_set, lld_dev_get_program_size_fn func);
int mas_blk_device_close_section(struct block_device *bi_bdev,
	struct stor_dev_reset_ftl *reset_ftl_info);
void mas_blk_mq_tagset_reset_ftl_register(
	struct blk_mq_tag_set *tag_set, lld_dev_reset_ftl_fn func);
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
int mas_blk_bad_block_error_inject(struct block_device *bi_bdev,
	unsigned char bad_slc_cnt, unsigned char bad_tlc_cnt);
int mas_blk_rescue_block_inject_data(
	struct block_device *bi_bdev, sector_t sect);
void mas_blk_mq_tagset_rescue_block_inject_data_register(
	struct blk_mq_tag_set *tag_set, lld_dev_rescue_block_inject_data_fn func);
void mas_blk_mq_tagset_bad_block_error_inject_register(
	struct blk_mq_tag_set *tag_set, lld_dev_bad_block_error_inject_fn func);
void mas_blk_mq_tagset_slc_mode_error_inject_register(
	struct blk_mq_tag_set *tag_set, lld_dev_slc_mode_error_inject_fn func);
int mas_blk_unistore_debug_en(void);
int mas_blk_recovery_debug_on(void);
void mas_blk_recovery_debug_off(void);
unsigned int mas_blk_get_stub_disorder_lba(struct request *req);
#endif
void mas_blk_bad_block_notify_register(struct block_device *bi_bdev,
	blk_dev_bad_block_notify_fn func, void* param_data);
void mas_blk_mq_tagset_bad_block_notify_register(
	struct blk_mq_tag_set *tag_set, lld_dev_bad_block_notify_register_fn func);
int mas_bkops_work_query(struct block_device *bdev);
int mas_bkops_work_start(struct block_device *bdev);
void mas_bkops_work_stop(struct block_device *bdev);
void mas_blk_req_get_order_nr_unistore(struct request *req,
	unsigned char new_stream_type, unsigned char *order,
	unsigned char *pre_order_cnt, bool extern_protect);
void mas_blk_mq_tagset_set_bkops(
	struct request_queue *q, struct mas_bkops *ufs_bkops);
void mas_blk_queue_unistore_enable(
	struct request_queue *q, bool enable);
void mas_blk_set_up_unistore_env(struct request_queue *q,
	unsigned int mas_sec_size, unsigned int mas_pu_size, bool enable);
bool blk_queue_query_unistore_enable(struct request_queue *q);
int mas_blk_mq_update_unistore_tags(struct blk_mq_tag_set *set);
void mas_blk_insert_section_list(struct block_device *bdev,
	unsigned int start_blkaddr, int stream_type, int flash_mode);
unsigned int mas_blk_get_sec_size(struct request_queue *q);
unsigned int mas_blk_get_pu_size(struct request_queue *q);
int mas_blk_update_buf_bio_page(struct block_device *bdev,
	struct page *page, struct page *cached_page);
void mas_blk_set_recovery_flag(struct request_queue *q);
int mas_blk_get_recovery_pages(bool page_anon);
void mas_blk_recovery_pages_add(struct page *page);
void mas_blk_recovery_pages_sub(struct page *page);
void mas_blk_clear_section_list(struct block_device *bdev,
	unsigned char stream_type);
void mas_blk_queue_cpplus_enable(struct request_queue *q, bool enable);
bool blk_queue_query_cpplus_enable(struct request_queue *q);
#else
static inline bool blk_queue_query_unistore_enable(struct request_queue *q)
{
	return false;
}
#endif

#ifdef CONFIG_HP_CORE
void blk_dev_health_query_register(
	struct request_queue *q, lld_query_health_fn func);
int blk_dev_health_query(struct block_device *bi_bdev,
	u8 *pre_eol_info, u8 *life_time_est_a, u8 *life_time_est_b);
#endif

#ifdef CONFIG_MAS_BLK
void blk_queue_dump_register(
	struct request_queue *q, lld_dump_status_fn func);
void blk_mq_tagset_dump_register(
	struct blk_mq_tag_set *tag_set, lld_dump_status_fn func);
void blk_mq_tagset_tz_query_register(
	struct blk_mq_tag_set *tag_set, lld_tz_query_fn func);
int blk_lld_tz_query(
	const struct block_device *bi_bdev, u32 type, u8 *buf, u32 buf_len);
void blk_mq_tagset_tz_ctrl_register(
	struct blk_mq_tag_set *tag_set, lld_tz_ctrl_fn func);
int blk_lld_tz_ctrl(
	const struct block_device *bi_bdev, int desc_id, uint8_t index);
void blk_mq_tagset_latency_warning_set(
	struct blk_mq_tag_set *tag_set, unsigned int warning_threshold_ms);
void blk_queue_latency_warning_set(
	struct request_queue *q, unsigned int warning_threshold_ms);
int blk_busyidle_event_subscribe(const struct block_device *bi_bdev,
	const struct blk_busyidle_event_node *event_node);
int blk_queue_busyidle_event_subscribe(
	const struct request_queue *q,
	const struct blk_busyidle_event_node *event_node);
int blk_lld_busyidle_event_subscribe(
	const struct blk_dev_lld *lld,
	struct blk_busyidle_event_node *event_node);
int blk_busyidle_event_unsubscribe(
	const struct blk_busyidle_event_node *event_node);
int blk_queue_busyidle_event_unsubscribe(
	const struct blk_busyidle_event_node *event_node);
void blk_queue_busyidle_enable(
	const struct request_queue *q, int enable);
void blk_mq_tagset_busyidle_enable(
	struct blk_mq_tag_set *tag_set, int enable);
void blk_mq_tagset_hw_idle_notify_enable(
	struct blk_mq_tag_set *tag_set, int enable);
void blk_queue_direct_flush_register(
	struct request_queue *q, blk_direct_flush_fn func);
void blk_mq_tagset_direct_flush_register(
	struct blk_mq_tag_set *tag_set, blk_direct_flush_fn func);
void blk_queue_flush_reduce_config(
	struct request_queue *q, bool flush_reduce_enable);
void blk_mq_tagset_flush_reduce_config(
	struct blk_mq_tag_set *tag_set, bool flush_reduce_enable);
void blk_flush_set_async(struct bio *bio);
int blk_flush_async_support(const struct block_device *bi_bdev);
void blk_mq_tagset_ufs_mq_iosched_enable(
	struct blk_mq_tag_set *tag_set, int enable);
void blk_mq_tagset_mmc_mq_iosched_enable(
	struct blk_mq_tag_set *tag_set, int enable);
bool ufs_order_panic_wait_datasync_handle(struct blk_dev_lld *blk_lld);
void ufs_order_panic_datasync_handle(struct blk_dev_lld *blk_lld);
void blk_power_off_flush(int emergency);
void mas_blk_panic_flush(void);
void blk_write_throttle(struct request_queue *queue, int level);
void blk_generic_freeze(
	const void *freeze_obj, enum blk_freeze_obj_type type, bool freeze);
unsigned char req_get_streamid(struct request *req);
void blk_lld_idle_notify(const struct blk_dev_lld *lld);
bool blk_dev_write_order_preserved(struct block_device *bdev);
unsigned int blk_req_get_order_nr(struct request *req, bool extern_protect);
void blk_queue_order_enable(struct request_queue *q, bool enable);
bool blk_queue_query_order_enable(struct request_queue *q);
void blk_order_nr_reset(struct blk_mq_tag_set *tag_set);
int blk_mq_get_io_in_list_count(struct block_device *bdev);
#ifdef CONFIG_MAS_MQ_USING_CP
void blk_queue_cp_enable(struct request_queue *q, bool enable);
#endif
#ifdef CONFIG_MAS_QOS_MQ
void blk_mq_tagset_ufs_qos_mq_iosched_enable(
	struct blk_mq_tag_set *tag_set, int enable);
static __always_inline unsigned int qos_ufs_mq_get_send_cpu(struct request *rq)
{
	return rq->mas_req.slot_cpu;
}
#endif /* CONFIG_MAS_QOS_MQ */

void blk_dio_ck(struct gendisk *target_disk,
	ktime_t dio_start, int dio_op, int dio_page_count);
void blk_mq_tagset_vl_setup(
	struct blk_mq_tag_set *tag_set, u64 device_capacity);
void blk_mq_tagset_vl_setup_ufs_3_1(
	struct blk_mq_tag_set *tag_set, u64 device_capacity);


#ifdef CONFIG_MMC_MQ_CQ_HCI
extern void mmc_mq_requeue_invalidate_reqs(struct request_queue *q);
#endif

static inline char *mas_blk_prefix_str(enum blk_dump_scene s)
{
	return (s == BLK_DUMP_PANIC) ? "dump" : "io_latency";
}

#endif /* CONFIG_MAS_BLK */
#else /* CONFIG_BLOCK */
struct blk_plug {
};

static inline void blk_start_plug(struct blk_plug *plug)
{
}

static inline void blk_finish_plug(struct blk_plug *plug)
{
}

static inline void blk_flush_plug(struct task_struct *task)
{
}

static inline void blk_schedule_flush_plug(struct task_struct *task)
{
}


static inline bool blk_needs_flush_plug(struct task_struct *tsk)
{
	return false;
}

static inline int blkdev_issue_flush(struct block_device *bdev, gfp_t gfp_mask)
{
	return 0;
}

static inline long nr_blockdev_pages(void)
{
	return 0;
}
#endif /* CONFIG_BLOCK */

extern void blk_io_schedule(void);

extern int blkdev_issue_write_same(struct block_device *bdev, sector_t sector,
		sector_t nr_sects, gfp_t gfp_mask, struct page *page);

#define BLKDEV_DISCARD_SECURE	(1 << 0)	/* issue a secure erase */

extern int blkdev_issue_discard(struct block_device *bdev, sector_t sector,
		sector_t nr_sects, gfp_t gfp_mask, unsigned long flags);
extern int __blkdev_issue_discard(struct block_device *bdev, sector_t sector,
		sector_t nr_sects, gfp_t gfp_mask, int flags,
		struct bio **biop);

#define BLKDEV_ZERO_NOUNMAP	(1 << 0)  /* do not free blocks */
#define BLKDEV_ZERO_NOFALLBACK	(1 << 1)  /* don't write explicit zeroes */

extern int __blkdev_issue_zeroout(struct block_device *bdev, sector_t sector,
		sector_t nr_sects, gfp_t gfp_mask, struct bio **biop,
		unsigned flags);
extern int blkdev_issue_zeroout(struct block_device *bdev, sector_t sector,
		sector_t nr_sects, gfp_t gfp_mask, unsigned flags);

static inline int sb_issue_discard(struct super_block *sb, sector_t block,
		sector_t nr_blocks, gfp_t gfp_mask, unsigned long flags)
{
	return blkdev_issue_discard(sb->s_bdev,
				    block << (sb->s_blocksize_bits -
					      SECTOR_SHIFT),
				    nr_blocks << (sb->s_blocksize_bits -
						  SECTOR_SHIFT),
				    gfp_mask, flags);
}
static inline int sb_issue_zeroout(struct super_block *sb, sector_t block,
		sector_t nr_blocks, gfp_t gfp_mask)
{
	return blkdev_issue_zeroout(sb->s_bdev,
				    block << (sb->s_blocksize_bits -
					      SECTOR_SHIFT),
				    nr_blocks << (sb->s_blocksize_bits -
						  SECTOR_SHIFT),
				    gfp_mask, 0);
}

extern int blk_verify_command(unsigned char *cmd, fmode_t mode);

static inline bool bdev_is_partition(struct block_device *bdev)
{
	return bdev->bd_partno;
}

enum blk_default_limits {
	BLK_MAX_SEGMENTS	= 128,
	BLK_SAFE_MAX_SECTORS	= 255,
	BLK_DEF_MAX_SECTORS	= 2560,
	BLK_MAX_SEGMENT_SIZE	= 65536,
	BLK_SEG_BOUNDARY_MASK	= 0xFFFFFFFFUL,
};

static inline unsigned long queue_segment_boundary(const struct request_queue *q)
{
	return q->limits.seg_boundary_mask;
}

static inline unsigned long queue_virt_boundary(const struct request_queue *q)
{
	return q->limits.virt_boundary_mask;
}

static inline unsigned int queue_max_sectors(const struct request_queue *q)
{
	return q->limits.max_sectors;
}

static inline unsigned int queue_max_hw_sectors(const struct request_queue *q)
{
	return q->limits.max_hw_sectors;
}

static inline unsigned short queue_max_segments(const struct request_queue *q)
{
	return q->limits.max_segments;
}

static inline unsigned short queue_max_discard_segments(const struct request_queue *q)
{
	return q->limits.max_discard_segments;
}

static inline unsigned int queue_max_segment_size(const struct request_queue *q)
{
	return q->limits.max_segment_size;
}

static inline unsigned int queue_max_zone_append_sectors(const struct request_queue *q)
{

	const struct queue_limits *l = &q->limits;

	return min(l->max_zone_append_sectors, l->max_sectors);
}

static inline unsigned queue_logical_block_size(const struct request_queue *q)
{
	int retval = 512;

	if (q && q->limits.logical_block_size)
		retval = q->limits.logical_block_size;

	return retval;
}

static inline unsigned int bdev_logical_block_size(struct block_device *bdev)
{
	return queue_logical_block_size(bdev_get_queue(bdev));
}

static inline unsigned int queue_physical_block_size(const struct request_queue *q)
{
	return q->limits.physical_block_size;
}

static inline unsigned int bdev_physical_block_size(struct block_device *bdev)
{
	return queue_physical_block_size(bdev_get_queue(bdev));
}

static inline unsigned int queue_io_min(const struct request_queue *q)
{
	return q->limits.io_min;
}

static inline int bdev_io_min(struct block_device *bdev)
{
	return queue_io_min(bdev_get_queue(bdev));
}

static inline unsigned int queue_io_opt(const struct request_queue *q)
{
	return q->limits.io_opt;
}

static inline int bdev_io_opt(struct block_device *bdev)
{
	return queue_io_opt(bdev_get_queue(bdev));
}

static inline int queue_alignment_offset(const struct request_queue *q)
{
	if (q->limits.misaligned)
		return -1;

	return q->limits.alignment_offset;
}

static inline int queue_limit_alignment_offset(struct queue_limits *lim, sector_t sector)
{
	unsigned int granularity = max(lim->physical_block_size, lim->io_min);
	unsigned int alignment = sector_div(sector, granularity >> SECTOR_SHIFT)
		<< SECTOR_SHIFT;

	return (granularity + lim->alignment_offset - alignment) % granularity;
}

static inline int bdev_alignment_offset(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);

	if (q->limits.misaligned)
		return -1;
	if (bdev_is_partition(bdev))
		return queue_limit_alignment_offset(&q->limits,
				bdev->bd_part->start_sect);
	return q->limits.alignment_offset;
}

static inline int queue_discard_alignment(const struct request_queue *q)
{
	if (q->limits.discard_misaligned)
		return -1;

	return q->limits.discard_alignment;
}

static inline int queue_limit_discard_alignment(struct queue_limits *lim, sector_t sector)
{
	unsigned int alignment, granularity, offset;

	if (!lim->max_discard_sectors)
		return 0;

	/* Why are these in bytes, not sectors? */
	alignment = lim->discard_alignment >> SECTOR_SHIFT;
	granularity = lim->discard_granularity >> SECTOR_SHIFT;
	if (!granularity)
		return 0;

	/* Offset of the partition start in 'granularity' sectors */
	offset = sector_div(sector, granularity);

	/* And why do we do this modulus *again* in blkdev_issue_discard()? */
	offset = (granularity + alignment - offset) % granularity;

	/* Turn it back into bytes, gaah */
	return offset << SECTOR_SHIFT;
}

static inline int bdev_discard_alignment(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);

	if (bdev_is_partition(bdev))
		return queue_limit_discard_alignment(&q->limits,
				bdev->bd_part->start_sect);
	return q->limits.discard_alignment;
}

static inline unsigned int bdev_write_same(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);

	if (q)
		return q->limits.max_write_same_sectors;

	return 0;
}

static inline unsigned int bdev_write_zeroes_sectors(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);

	if (q)
		return q->limits.max_write_zeroes_sectors;

	return 0;
}

static inline enum blk_zoned_model bdev_zoned_model(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);

	if (q)
		return blk_queue_zoned_model(q);

	return BLK_ZONED_NONE;
}

static inline bool bdev_is_zoned(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);

	if (q)
		return blk_queue_is_zoned(q);

	return false;
}

static inline sector_t bdev_zone_sectors(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);

	if (q)
		return blk_queue_zone_sectors(q);
	return 0;
}

static inline unsigned int bdev_max_open_zones(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);

	if (q)
		return queue_max_open_zones(q);
	return 0;
}

static inline unsigned int bdev_max_active_zones(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);

	if (q)
		return queue_max_active_zones(q);
	return 0;
}

static inline int queue_dma_alignment(const struct request_queue *q)
{
	return q ? q->dma_alignment : 511;
}

static inline int blk_rq_aligned(struct request_queue *q, unsigned long addr,
				 unsigned int len)
{
	unsigned int alignment = queue_dma_alignment(q) | q->dma_pad_mask;
	return !(addr & alignment) && !(len & alignment);
}

/* assumes size > 256 */
static inline unsigned int blksize_bits(unsigned int size)
{
	unsigned int bits = 8;
	do {
		bits++;
		size >>= 1;
	} while (size > 256);
	return bits;
}

static inline unsigned int block_size(struct block_device *bdev)
{
	return 1 << bdev->bd_inode->i_blkbits;
}

int kblockd_schedule_work(struct work_struct *work);
int kblockd_mod_delayed_work_on(int cpu, struct delayed_work *dwork, unsigned long delay);
#ifdef CONFIG_MAS_BLK
int kblockd_schedule_delayed_work(struct delayed_work *dwork, unsigned long delay);
#endif

#define MODULE_ALIAS_BLOCKDEV(major,minor) \
	MODULE_ALIAS("block-major-" __stringify(major) "-" __stringify(minor))
#define MODULE_ALIAS_BLOCKDEV_MAJOR(major) \
	MODULE_ALIAS("block-major-" __stringify(major) "-*")

#if defined(CONFIG_BLK_DEV_INTEGRITY)

enum blk_integrity_flags {
	BLK_INTEGRITY_VERIFY		= 1 << 0,
	BLK_INTEGRITY_GENERATE		= 1 << 1,
	BLK_INTEGRITY_DEVICE_CAPABLE	= 1 << 2,
	BLK_INTEGRITY_IP_CHECKSUM	= 1 << 3,
};

struct blk_integrity_iter {
	void			*prot_buf;
	void			*data_buf;
	sector_t		seed;
	unsigned int		data_size;
	unsigned short		interval;
	const char		*disk_name;
};

typedef blk_status_t (integrity_processing_fn) (struct blk_integrity_iter *);
typedef void (integrity_prepare_fn) (struct request *);
typedef void (integrity_complete_fn) (struct request *, unsigned int);

struct blk_integrity_profile {
	integrity_processing_fn		*generate_fn;
	integrity_processing_fn		*verify_fn;
	integrity_prepare_fn		*prepare_fn;
	integrity_complete_fn		*complete_fn;
	const char			*name;
};

extern void blk_integrity_register(struct gendisk *, struct blk_integrity *);
extern void blk_integrity_unregister(struct gendisk *);
extern int blk_integrity_compare(struct gendisk *, struct gendisk *);
extern int blk_rq_map_integrity_sg(struct request_queue *, struct bio *,
				   struct scatterlist *);
extern int blk_rq_count_integrity_sg(struct request_queue *, struct bio *);

static inline struct blk_integrity *blk_get_integrity(struct gendisk *disk)
{
	struct blk_integrity *bi = &disk->queue->integrity;

	if (!bi->profile)
		return NULL;

	return bi;
}

static inline
struct blk_integrity *bdev_get_integrity(struct block_device *bdev)
{
	return blk_get_integrity(bdev->bd_disk);
}

static inline bool
blk_integrity_queue_supports_integrity(struct request_queue *q)
{
	return q->integrity.profile;
}

static inline bool blk_integrity_rq(struct request *rq)
{
	return rq->cmd_flags & REQ_INTEGRITY;
}

static inline void blk_queue_max_integrity_segments(struct request_queue *q,
						    unsigned int segs)
{
	q->limits.max_integrity_segments = segs;
}

static inline unsigned short
queue_max_integrity_segments(const struct request_queue *q)
{
	return q->limits.max_integrity_segments;
}

/**
 * bio_integrity_intervals - Return number of integrity intervals for a bio
 * @bi:		blk_integrity profile for device
 * @sectors:	Size of the bio in 512-byte sectors
 *
 * Description: The block layer calculates everything in 512 byte
 * sectors but integrity metadata is done in terms of the data integrity
 * interval size of the storage device.  Convert the block layer sectors
 * to the appropriate number of integrity intervals.
 */
static inline unsigned int bio_integrity_intervals(struct blk_integrity *bi,
						   unsigned int sectors)
{
	return sectors >> (bi->interval_exp - 9);
}

static inline unsigned int bio_integrity_bytes(struct blk_integrity *bi,
					       unsigned int sectors)
{
	return bio_integrity_intervals(bi, sectors) * bi->tuple_size;
}

/*
 * Return the first bvec that contains integrity data.  Only drivers that are
 * limited to a single integrity segment should use this helper.
 */
static inline struct bio_vec *rq_integrity_vec(struct request *rq)
{
	if (WARN_ON_ONCE(queue_max_integrity_segments(rq->q) > 1))
		return NULL;
	return rq->bio->bi_integrity->bip_vec;
}

#else /* CONFIG_BLK_DEV_INTEGRITY */

struct bio;
struct block_device;
struct gendisk;
struct blk_integrity;

static inline int blk_integrity_rq(struct request *rq)
{
	return 0;
}
static inline int blk_rq_count_integrity_sg(struct request_queue *q,
					    struct bio *b)
{
	return 0;
}
static inline int blk_rq_map_integrity_sg(struct request_queue *q,
					  struct bio *b,
					  struct scatterlist *s)
{
	return 0;
}
static inline struct blk_integrity *bdev_get_integrity(struct block_device *b)
{
	return NULL;
}
static inline struct blk_integrity *blk_get_integrity(struct gendisk *disk)
{
	return NULL;
}
static inline bool
blk_integrity_queue_supports_integrity(struct request_queue *q)
{
	return false;
}
static inline int blk_integrity_compare(struct gendisk *a, struct gendisk *b)
{
	return 0;
}
static inline void blk_integrity_register(struct gendisk *d,
					 struct blk_integrity *b)
{
}
static inline void blk_integrity_unregister(struct gendisk *d)
{
}
static inline void blk_queue_max_integrity_segments(struct request_queue *q,
						    unsigned int segs)
{
}
static inline unsigned short queue_max_integrity_segments(const struct request_queue *q)
{
	return 0;
}

static inline unsigned int bio_integrity_intervals(struct blk_integrity *bi,
						   unsigned int sectors)
{
	return 0;
}

static inline unsigned int bio_integrity_bytes(struct blk_integrity *bi,
					       unsigned int sectors)
{
	return 0;
}

static inline struct bio_vec *rq_integrity_vec(struct request *rq)
{
	return NULL;
}

#endif /* CONFIG_BLK_DEV_INTEGRITY */

#ifdef CONFIG_BLK_INLINE_ENCRYPTION

bool blk_ksm_register(struct blk_keyslot_manager *ksm, struct request_queue *q);

void blk_ksm_unregister(struct request_queue *q);

#else /* CONFIG_BLK_INLINE_ENCRYPTION */

static inline bool blk_ksm_register(struct blk_keyslot_manager *ksm,
				    struct request_queue *q)
{
	return true;
}

static inline void blk_ksm_unregister(struct request_queue *q) { }

#endif /* CONFIG_BLK_INLINE_ENCRYPTION */


struct block_device_operations {
	blk_qc_t (*submit_bio) (struct bio *bio);
	int (*open) (struct block_device *, fmode_t);
	void (*release) (struct gendisk *, fmode_t);
	int (*rw_page)(struct block_device *, sector_t, struct page *, unsigned int);
	int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
	int (*compat_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
	unsigned int (*check_events) (struct gendisk *disk,
				      unsigned int clearing);
	void (*unlock_native_capacity) (struct gendisk *);
	int (*revalidate_disk) (struct gendisk *);
	int (*getgeo)(struct block_device *, struct hd_geometry *);
	/* this callback is with swap_lock and sometimes page table lock held */
	void (*swap_slot_free_notify) (struct block_device *, unsigned long);
	int (*report_zones)(struct gendisk *, sector_t sector,
			unsigned int nr_zones, report_zones_cb cb, void *data);
	char *(*devnode)(struct gendisk *disk, umode_t *mode);
	struct module *owner;
	const struct pr_ops *pr_ops;

	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
	ANDROID_OEM_DATA(1);
};

#ifdef CONFIG_COMPAT
extern int blkdev_compat_ptr_ioctl(struct block_device *, fmode_t,
				      unsigned int, unsigned long);
#else
#define blkdev_compat_ptr_ioctl NULL
#endif

extern int __blkdev_driver_ioctl(struct block_device *, fmode_t, unsigned int,
				 unsigned long);
extern int bdev_read_page(struct block_device *, sector_t, struct page *);
extern int bdev_write_page(struct block_device *, sector_t, struct page *,
						struct writeback_control *);
extern void print_bdev_access_info(void);

#ifdef CONFIG_BLK_DEV_ZONED
bool blk_req_needs_zone_write_lock(struct request *rq);
bool blk_req_zone_write_trylock(struct request *rq);
void __blk_req_zone_write_lock(struct request *rq);
void __blk_req_zone_write_unlock(struct request *rq);

static inline void blk_req_zone_write_lock(struct request *rq)
{
	if (blk_req_needs_zone_write_lock(rq))
		__blk_req_zone_write_lock(rq);
}

static inline void blk_req_zone_write_unlock(struct request *rq)
{
	if (rq->rq_flags & RQF_ZONE_WRITE_LOCKED)
		__blk_req_zone_write_unlock(rq);
}

static inline bool blk_req_zone_is_write_locked(struct request *rq)
{
	return rq->q->seq_zones_wlock &&
		test_bit(blk_rq_zone_no(rq), rq->q->seq_zones_wlock);
}

static inline bool blk_req_can_dispatch_to_zone(struct request *rq)
{
	if (!blk_req_needs_zone_write_lock(rq))
		return true;
	return !blk_req_zone_is_write_locked(rq);
}
#else
static inline bool blk_req_needs_zone_write_lock(struct request *rq)
{
	return false;
}

static inline void blk_req_zone_write_lock(struct request *rq)
{
}

static inline void blk_req_zone_write_unlock(struct request *rq)
{
}
static inline bool blk_req_zone_is_write_locked(struct request *rq)
{
	return false;
}

static inline bool blk_req_can_dispatch_to_zone(struct request *rq)
{
	return true;
}
#endif /* CONFIG_BLK_DEV_ZONED */

static inline void blk_wake_io_task(struct task_struct *waiter)
{
	/*
	 * If we're polling, the task itself is doing the completions. For
	 * that case, we don't need to signal a wakeup, it's enough to just
	 * mark us as RUNNING.
	 */
	if (waiter == current)
		__set_current_state(TASK_RUNNING);
	else
		wake_up_process(waiter);
}

unsigned long disk_start_io_acct(struct gendisk *disk, unsigned int sectors,
		unsigned int op);
void disk_end_io_acct(struct gendisk *disk, unsigned int op,
		unsigned long start_time);

unsigned long part_start_io_acct(struct gendisk *disk, struct hd_struct **part,
				 struct bio *bio);
void part_end_io_acct(struct hd_struct *part, struct bio *bio,
		      unsigned long start_time);

/**
 * bio_start_io_acct - start I/O accounting for bio based drivers
 * @bio:	bio to start account for
 *
 * Returns the start time that should be passed back to bio_end_io_acct().
 */
static inline unsigned long bio_start_io_acct(struct bio *bio)
{
	return disk_start_io_acct(bio->bi_disk, bio_sectors(bio), bio_op(bio));
}

/**
 * bio_end_io_acct - end I/O accounting for bio based drivers
 * @bio:	bio to end account for
 * @start:	start time returned by bio_start_io_acct()
 */
static inline void bio_end_io_acct(struct bio *bio, unsigned long start_time)
{
	return disk_end_io_acct(bio->bi_disk, bio_op(bio), start_time);
}

int bdev_read_only(struct block_device *bdev);
int set_blocksize(struct block_device *bdev, int size);

const char *bdevname(struct block_device *bdev, char *buffer);
struct block_device *lookup_bdev(const char *);

void blkdev_show(struct seq_file *seqf, off_t offset);

#define BDEVNAME_SIZE	32	/* Largest string for a blockdev identifier */
#define BDEVT_SIZE	10	/* Largest string for MAJ:MIN for blkdev */
#ifdef CONFIG_BLOCK
#define BLKDEV_MAJOR_MAX	512
#else
#define BLKDEV_MAJOR_MAX	0
#endif

struct block_device *blkdev_get_by_path(const char *path, fmode_t mode,
		void *holder);
struct block_device *blkdev_get_by_dev(dev_t dev, fmode_t mode, void *holder);
int bd_prepare_to_claim(struct block_device *bdev, struct block_device *whole,
		void *holder);
void bd_abort_claiming(struct block_device *bdev, struct block_device *whole,
		void *holder);
void blkdev_put(struct block_device *bdev, fmode_t mode);

struct block_device *I_BDEV(struct inode *inode);
struct block_device *bdget_part(struct hd_struct *part);
struct block_device *bdgrab(struct block_device *bdev);
void bdput(struct block_device *);

#ifdef CONFIG_BLOCK
void invalidate_bdev(struct block_device *bdev);
int truncate_bdev_range(struct block_device *bdev, fmode_t mode, loff_t lstart,
			loff_t lend);
int sync_blockdev(struct block_device *bdev);
#else
static inline void invalidate_bdev(struct block_device *bdev)
{
}
static inline int truncate_bdev_range(struct block_device *bdev, fmode_t mode,
				      loff_t lstart, loff_t lend)
{
	return 0;
}
static inline int sync_blockdev(struct block_device *bdev)
{
	return 0;
}
#endif
int fsync_bdev(struct block_device *bdev);

int freeze_bdev(struct block_device *bdev);
int thaw_bdev(struct block_device *bdev);

#ifdef CONFIG_MAS_BLK
/*
 * X-axis for IO latency histogram support.
 */
static const u_int64_t latency_x_axis_us[] = {
	100,
	200,
	300,
	400,
	500,
	600,
	700,
	800,
	900,
	1000,
	1200,
	1400,
	1600,
	1800,
	2000,
	2500,
	3000,
	4000,
	5000,
	6000,
	7000,
	9000,
	10000
};

#define BLK_IO_LAT_HIST_DISABLE         0
#define BLK_IO_LAT_HIST_ENABLE          1
#define BLK_IO_LAT_HIST_ZERO            2
#define ASYNC_LIMIT_STAGE_ONE 0
#define ASYNC_LIMIT_STAGE_TWO 1

struct io_latency_state {
	u_int64_t	latency_y_axis[ARRAY_SIZE(latency_x_axis_us) + 1];
	u_int64_t	latency_elems;
	u_int64_t	latency_sum;
};
static inline void
blk_update_latency_hist(struct io_latency_state *s, u_int64_t delta_us)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(latency_x_axis_us); i++)
		if (delta_us < (u_int64_t)latency_x_axis_us[i])
			break;
	s->latency_y_axis[i]++;
	s->latency_elems++;
	s->latency_sum += delta_us;
}

ssize_t blk_latency_hist_show(char *name, struct io_latency_state *s,
		char *buf, int buf_size);

#endif
#endif /* _LINUX_BLKDEV_H */
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
void blk_queue_set_inline_crypto_flag(
	struct request_queue *q, bool enable);
void mas_blk_inline_crypto_init_request_from_bio(
	struct request *req, const struct bio *bio);
bool mas_blk_inline_crypto_bio_merge_allow(
	const struct request *rq, const struct bio *bio);
void mas_blk_inline_crypto_bio_split_pre(
	struct bio *bio, struct bio *split);
void mas_blk_inline_crypto_bio_split_post(struct bio *bio);
void mas_blk_inline_crypto_req_init(struct request *rq);
#endif
