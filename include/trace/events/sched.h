/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM sched

#if !defined(_TRACE_SCHED_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SCHED_H

#include <linux/sched/numa_balancing.h>
#include <linux/tracepoint.h>
#include <linux/binfmts.h>
#include <securec.h>

#ifdef CONFIG_FRAME_RTG
#include <linux/sched/frame.h>
#include <linux/sched/frame_info.h>
#endif
#ifdef CONFIG_HW_QOS_THREAD
#include <chipset_common/hwqos/hwqos_common.h>
#endif

/*
 * Tracepoint for calling kthread_stop, performed to end a kthread:
 */
TRACE_EVENT(sched_kthread_stop,

	TP_PROTO(struct task_struct *t),

	TP_ARGS(t),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, t->comm, TASK_COMM_LEN);
		__entry->pid	= t->pid;
	),

	TP_printk("comm=%s pid=%d", __entry->comm, __entry->pid)
);

/*
 * Tracepoint for the return value of the kthread stopping:
 */
TRACE_EVENT(sched_kthread_stop_ret,

	TP_PROTO(int ret),

	TP_ARGS(ret),

	TP_STRUCT__entry(
		__field(	int,	ret	)
	),

	TP_fast_assign(
		__entry->ret	= ret;
	),

	TP_printk("ret=%d", __entry->ret)
);

/*
 * Tracepoint for waking up a task:
 */
DECLARE_EVENT_CLASS(sched_wakeup_template,

	TP_PROTO(struct task_struct *p),

	TP_ARGS(__perf_task(p)),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__field(	int,	success			)
		__field(	int,	target_cpu		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio; /* XXX SCHED_DEADLINE */
		__entry->success	= 1; /* rudiment, kill when possible */
		__entry->target_cpu	= task_cpu(p);
	),

	TP_printk("comm=%s pid=%d prio=%d target_cpu=%03d",
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->target_cpu)
);

/*
 * Tracepoint called when waking a task; this tracepoint is guaranteed to be
 * called from the waking context.
 */
DEFINE_EVENT(sched_wakeup_template, sched_waking,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint called when the task is actually woken; p->state == TASK_RUNNNG.
 * It is not always called from the waking context.
 */
DEFINE_EVENT(sched_wakeup_template, sched_wakeup,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint for waking up a new task:
 */
DEFINE_EVENT(sched_wakeup_template, sched_wakeup_new,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

#ifdef CONFIG_HW_QOS_THREAD
/*
 *  * Tracepoint for sched qos
 *   */
DECLARE_EVENT_CLASS(sched_qos_template,

		TP_PROTO(struct task_struct *p, struct transact_qos *tq, int type),

		TP_ARGS(__perf_task(p), tq, type),

		TP_STRUCT__entry(
			__array(char,   comm,   TASK_COMM_LEN)
			__field(pid_t,  pid)
			__field(int,    prio)
			__field(int,    success)
			__field(int,    target_cpu)
			__field(int,    dynamic_qos)
			__field(int,    vip_prio)
			__field(int,    min_util)
			__field(int,    trans_qos)
			__field(pid_t,  trans_from)
			__field(int,    trans_type)
			__field(int,    trans_flags)
			__field(int,    type)
			),

		TP_fast_assign(
				memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
				__entry->pid         = p->pid;
				__entry->prio        = p->prio;
				__entry->success     = 1; /* rudiment, kill when possible */
				__entry->target_cpu  = task_cpu(p);
				__entry->dynamic_qos = get_task_set_qos(p);
#ifdef CONFIG_SCHED_VIP_OLD
				__entry->vip_prio = task_vip_prio(p);
#else
				__entry->vip_prio = -1;
#endif
				__entry->min_util = get_task_min_util(p);
				__entry->trans_qos   = get_task_trans_qos(tq);
				__entry->trans_from  = (tq == NULL) ? 0 : tq->trans_pid;
				__entry->trans_type  = (tq == NULL) ? 0 : tq->trans_type;
				__entry->trans_flags = atomic_read(&p->trans_flags);
		__entry->type        = type;
		),
		TP_printk("comm=%s pid=%d prio=%d target_cpu=%03d dynamic_qos=%d trans_qos=%d vip_prio=%d min_util=%d trans_from=%d trans_type=%d trans_flags=%d type=%d",
				__entry->comm, __entry->pid, __entry->prio,
				__entry->target_cpu, __entry->dynamic_qos,
				__entry->trans_qos, __entry->vip_prio,
				__entry->min_util,
				__entry->trans_from, __entry->trans_type,
				__entry->trans_flags, __entry->type)
		);

		DEFINE_EVENT(sched_qos_template, sched_qos,
				TP_PROTO(struct task_struct *p, struct transact_qos *tq, int type),
				TP_ARGS(p, tq, type));

#endif

#ifdef CONFIG_HW_VIP_THREAD
/*
 * Tracepoint for sched vip
 */
DECLARE_EVENT_CLASS(sched_vip_template,

	TP_PROTO(struct task_struct *p, char *msg),

	TP_ARGS(__perf_task(p), msg),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__array(	char,	msg, 	VIP_MSG_LEN	)
		__field(	int,	target_cpu		)
		__field(	u64,    dynamic_vip		)
		__field(	int,    vip_depth		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
		memcpy(__entry->msg, msg, min((size_t)VIP_MSG_LEN, strlen(msg)+1));
		__entry->target_cpu	= task_cpu(p);
		__entry->dynamic_vip   = atomic64_read(&p->dynamic_vip);
		__entry->vip_depth     = p->vip_depth;
	),

	TP_printk("comm=%s pid=%d prio=%d msg=%s target_cpu=%03d dynamic_vip:%llx vip_depth:%d",
		__entry->comm, __entry->pid, __entry->prio,
		__entry->msg, __entry->target_cpu, __entry->dynamic_vip, __entry->vip_depth)
);

DEFINE_EVENT(sched_vip_template, sched_vip_queue_op,
	TP_PROTO(struct task_struct *p, char *msg),
	TP_ARGS(p, msg));

DEFINE_EVENT(sched_vip_template, sched_vip_sched,
	TP_PROTO(struct task_struct *p, char *msg),
	TP_ARGS(p, msg));
#endif
#ifdef CONFIG_HW_GRADED_SCHED
DECLARE_EVENT_CLASS(sched_graded_template,

	TP_PROTO(struct task_struct* t, int nice, int increase),

	TP_ARGS(t, nice, increase),

	TP_STRUCT__entry(
		__field(	int,	pid)
		__field(	int,	tgid)
		__field(	int,	original_nice)
		__field(	int,	nice)
		__field(	int,	increase)
		__array(	char,	comm,	TASK_COMM_LEN)
	),

	TP_fast_assign(
		__entry->pid = t->pid;
		__entry->tgid = t->tgid;
		__entry->original_nice = t->original_nice;
		__entry->nice = nice;
		__entry->increase = increase;
		memcpy(__entry->comm, t->comm, TASK_COMM_LEN);
	),

	TP_printk("comm=%s, pid=%d, tgid=%d, original_nice=%d, nice=%d, increase=%d",
		__entry->comm,
		__entry->pid,
		__entry->tgid,
		__entry->original_nice,
		__entry->nice,
		__entry->increase)
);

DEFINE_EVENT(sched_graded_template, sched_update_graded_nice,
		TP_PROTO(struct task_struct *p, int nice, int increase),
		TP_ARGS(p, nice, increase));

DEFINE_EVENT(sched_graded_template, sched_init_graded_nice,
		TP_PROTO(struct task_struct *p, int nice, int increase),
		TP_ARGS(p, nice, increase));
#endif

#ifdef CREATE_TRACE_POINTS
static inline long __trace_sched_switch_state(bool preempt, struct task_struct *p)
{
	unsigned int state;

#ifdef CONFIG_SCHED_DEBUG
	BUG_ON(p != current);
#endif /* CONFIG_SCHED_DEBUG */

	/*
	 * Preemption ignores task state, therefore preempted tasks are always
	 * RUNNING (we will not have dequeued if state != RUNNING).
	 */
	if (preempt)
		return TASK_REPORT_MAX;

	/*
	 * task_state_index() uses fls() and returns a value from 0-8 range.
	 * Decrement it by 1 (except TASK_RUNNING state i.e 0) before using
	 * it for left shift operation to get the correct task->state
	 * mapping.
	 */
	state = task_state_index(p);

	return state ? (1 << (state - 1)) : state;
}
#endif /* CREATE_TRACE_POINTS */

/*
 * Tracepoint for task switches, performed by the scheduler:
 */
TRACE_EVENT(sched_switch,

	TP_PROTO(bool preempt,
		 struct task_struct *prev,
		 struct task_struct *next),

	TP_ARGS(preempt, prev, next),

	TP_STRUCT__entry(
		__array(	char,	prev_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	prev_pid			)
		__field(	int,	prev_prio			)
		__field(	long,	prev_state			)
		__array(	char,	next_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	next_pid			)
		__field(	int,	next_prio			)
	),

	TP_fast_assign(
		memcpy(__entry->next_comm, next->comm, TASK_COMM_LEN);
		__entry->prev_pid	= prev->pid;
		__entry->prev_prio	= prev->prio;
		__entry->prev_state	= __trace_sched_switch_state(preempt, prev);
		memcpy(__entry->prev_comm, prev->comm, TASK_COMM_LEN);
		__entry->next_pid	= next->pid;
		__entry->next_prio	= next->prio;
		/* XXX SCHED_DEADLINE */
	),

	TP_printk("prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d",
		__entry->prev_comm, __entry->prev_pid, __entry->prev_prio,

		(__entry->prev_state & (TASK_REPORT_MAX - 1)) ?
		  __print_flags(__entry->prev_state & (TASK_REPORT_MAX - 1), "|",
				{ TASK_INTERRUPTIBLE, "S" },
				{ TASK_UNINTERRUPTIBLE, "D" },
				{ __TASK_STOPPED, "T" },
				{ __TASK_TRACED, "t" },
				{ EXIT_DEAD, "X" },
				{ EXIT_ZOMBIE, "Z" },
				{ TASK_PARKED, "P" },
				{ TASK_DEAD, "I" }) :
		  "R",

		__entry->prev_state & TASK_REPORT_MAX ? "+" : "",
		__entry->next_comm, __entry->next_pid, __entry->next_prio)
);

/*
 * Tracepoint for a task being migrated:
 */
TRACE_EVENT(sched_migrate_task,

	TP_PROTO(struct task_struct *p, int dest_cpu),

	TP_ARGS(p, dest_cpu),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__field(	int,	orig_cpu		)
		__field(	int,	dest_cpu		)
		__field(	int,	running			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio; /* XXX SCHED_DEADLINE */
		__entry->orig_cpu	= task_cpu(p);
		__entry->dest_cpu	= dest_cpu;
		__entry->running	= (p->state == TASK_RUNNING);
	),

	TP_printk("comm=%s pid=%d prio=%d orig_cpu=%d dest_cpu=%d running=%d",
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->orig_cpu, __entry->dest_cpu,
		  __entry->running)
);

DECLARE_EVENT_CLASS(sched_process_template,

	TP_PROTO(struct task_struct *p),

	TP_ARGS(p),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio; /* XXX SCHED_DEADLINE */
	),

	TP_printk("comm=%s pid=%d prio=%d",
		  __entry->comm, __entry->pid, __entry->prio)
);

/*
 * Tracepoint for freeing a task:
 */
DEFINE_EVENT(sched_process_template, sched_process_free,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint for a task exiting:
 */
DEFINE_EVENT(sched_process_template, sched_process_exit,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint for waiting on task to unschedule:
 */
DEFINE_EVENT(sched_process_template, sched_wait_task,
	TP_PROTO(struct task_struct *p),
	TP_ARGS(p));

/*
 * Tracepoint for a waiting task:
 */
TRACE_EVENT(sched_process_wait,

	TP_PROTO(struct pid *pid),

	TP_ARGS(pid),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, current->comm, TASK_COMM_LEN);
		__entry->pid		= pid_nr(pid);
		__entry->prio		= current->prio; /* XXX SCHED_DEADLINE */
	),

	TP_printk("comm=%s pid=%d prio=%d",
		  __entry->comm, __entry->pid, __entry->prio)
);

/*
 * Tracepoint for do_fork:
 */
TRACE_EVENT(sched_process_fork,

	TP_PROTO(struct task_struct *parent, struct task_struct *child),

	TP_ARGS(parent, child),

	TP_STRUCT__entry(
		__array(	char,	parent_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	parent_pid			)
		__array(	char,	child_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	child_pid			)
	),

	TP_fast_assign(
		memcpy(__entry->parent_comm, parent->comm, TASK_COMM_LEN);
		__entry->parent_pid	= parent->pid;
		memcpy(__entry->child_comm, child->comm, TASK_COMM_LEN);
		__entry->child_pid	= child->pid;
	),

	TP_printk("comm=%s pid=%d child_comm=%s child_pid=%d",
		__entry->parent_comm, __entry->parent_pid,
		__entry->child_comm, __entry->child_pid)
);

/*
 * Tracepoint for exec:
 */
TRACE_EVENT(sched_process_exec,

	TP_PROTO(struct task_struct *p, pid_t old_pid,
		 struct linux_binprm *bprm),

	TP_ARGS(p, old_pid, bprm),

	TP_STRUCT__entry(
		__string(	filename,	bprm->filename	)
		__field(	pid_t,		pid		)
		__field(	pid_t,		old_pid		)
	),

	TP_fast_assign(
		__assign_str(filename, bprm->filename);
		__entry->pid		= p->pid;
		__entry->old_pid	= old_pid;
	),

	TP_printk("filename=%s pid=%d old_pid=%d", __get_str(filename),
		  __entry->pid, __entry->old_pid)
);


#ifdef CONFIG_SCHEDSTATS
#define DEFINE_EVENT_SCHEDSTAT DEFINE_EVENT
#define DECLARE_EVENT_CLASS_SCHEDSTAT DECLARE_EVENT_CLASS
#else
#define DEFINE_EVENT_SCHEDSTAT DEFINE_EVENT_NOP
#define DECLARE_EVENT_CLASS_SCHEDSTAT DECLARE_EVENT_CLASS_NOP
#endif

/*
 * XXX the below sched_stat tracepoints only apply to SCHED_OTHER/BATCH/IDLE
 *     adding sched_stat support to SCHED_FIFO/RR would be welcome.
 */
DECLARE_EVENT_CLASS_SCHEDSTAT(sched_stat_template,

	TP_PROTO(struct task_struct *tsk, u64 delay),

	TP_ARGS(__perf_task(tsk), __perf_count(delay)),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( u64,	delay			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid	= tsk->pid;
		__entry->delay	= delay;
	),

	TP_printk("comm=%s pid=%d delay=%Lu [ns]",
			__entry->comm, __entry->pid,
			(unsigned long long)__entry->delay)
);

/*
 * Tracepoint for accounting wait time (time the task is runnable
 * but not actually running due to scheduler contention).
 */
DEFINE_EVENT_SCHEDSTAT(sched_stat_template, sched_stat_wait,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting sleep time (time the task is not runnable,
 * including iowait, see below).
 */
DEFINE_EVENT_SCHEDSTAT(sched_stat_template, sched_stat_sleep,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting iowait time (time the task is not runnable
 * due to waiting on IO to complete).
 */
DEFINE_EVENT_SCHEDSTAT(sched_stat_template, sched_stat_iowait,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting blocked time (time the task is in uninterruptible).
 */
DEFINE_EVENT_SCHEDSTAT(sched_stat_template, sched_stat_blocked,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for recording the cause of uninterruptible sleep.
 */
TRACE_EVENT(sched_blocked_reason,

	TP_PROTO(struct task_struct *tsk, u64 delay),

	TP_ARGS(tsk, delay),

	TP_STRUCT__entry(
		__field( pid_t,	pid	)
		__field( void*, caller	)
		__field( bool, io_wait	)
		__field( u64, delay	)
	),

	TP_fast_assign(
		__entry->pid	= tsk->pid;
		__entry->caller = (void *)get_wchan(tsk);
		__entry->io_wait = tsk->in_iowait;
		__entry->delay = delay;
	),

	TP_printk("pid=%d iowait=%d caller=%pS delay=%lu",
	__entry->pid, __entry->io_wait, __entry->caller, __entry->delay>>10)
);

/*
 * Tracepoint for accounting runtime (time the task is executing
 * on a CPU).
 */
DECLARE_EVENT_CLASS(sched_stat_runtime,

	TP_PROTO(struct task_struct *tsk, u64 runtime, u64 vruntime),

	TP_ARGS(tsk, __perf_count(runtime), vruntime),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( u64,	runtime			)
		__field( u64,	vruntime			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->runtime	= runtime;
		__entry->vruntime	= vruntime;
	),

	TP_printk("comm=%s pid=%d runtime=%Lu [ns] vruntime=%Lu [ns]",
			__entry->comm, __entry->pid,
			(unsigned long long)__entry->runtime,
			(unsigned long long)__entry->vruntime)
);

DEFINE_EVENT(sched_stat_runtime, sched_stat_runtime,
	     TP_PROTO(struct task_struct *tsk, u64 runtime, u64 vruntime),
	     TP_ARGS(tsk, runtime, vruntime));

/*
 * Tracepoint for showing priority inheritance modifying a tasks
 * priority.
 */
TRACE_EVENT(sched_pi_setprio,

	TP_PROTO(struct task_struct *tsk, struct task_struct *pi_task),

	TP_ARGS(tsk, pi_task),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( int,	oldprio			)
		__field( int,	newprio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->oldprio	= tsk->prio;
		__entry->newprio	= pi_task ?
				min(tsk->normal_prio, pi_task->prio) :
				tsk->normal_prio;
		/* XXX SCHED_DEADLINE bits missing */
	),

	TP_printk("comm=%s pid=%d oldprio=%d newprio=%d",
			__entry->comm, __entry->pid,
			__entry->oldprio, __entry->newprio)
);

#ifdef CONFIG_HW_FUTEX_PI
#ifdef CONFIG_HW_QOS_THREAD
TRACE_EVENT(sched_pi_setqos,

	TP_PROTO(struct task_struct *tsk, int oldqos, int newqos),

	TP_ARGS(tsk, oldqos, newqos),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__field(int, oldqos)
		__field(int, newqos)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid = tsk->pid;
		__entry->oldqos = oldqos;
		__entry->newqos	= newqos;
		/* XXX SCHED_DEADLINE bits missing */
	),

	TP_printk("comm=%s pid=%d oldqos=%d newqos=%d",
			__entry->comm, __entry->pid,
			__entry->oldqos, __entry->newqos)
);
#endif

#ifdef CONFIG_HW_VIP_THREAD
TRACE_EVENT(sched_pi_setvip,

	TP_PROTO(struct task_struct *tsk, int oldvip, int newvip),

	TP_ARGS(tsk, oldvip, newvip),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( int,	oldvip			)
		__field( int,	newvip			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->oldvip		= oldvip;
		__entry->newvip		= newvip;
		/* XXX SCHED_DEADLINE bits missing */
	),

	TP_printk("comm=%s pid=%d oldvip=%d newvip=%d",
			__entry->comm, __entry->pid,
			__entry->oldvip, __entry->newvip)
);
#endif

#ifdef CONFIG_SCHED_VIP_OLD
TRACE_EVENT(sched_pi_setvipprio,

	TP_PROTO(struct task_struct *tsk, int oldvipprio, int newvipprio),

	TP_ARGS(tsk, oldvipprio, newvipprio),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__field(int, oldvipprio)
		__field(int, newvipprio)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid = tsk->pid;
		__entry->oldvipprio = oldvipprio;
		__entry->newvipprio = newvipprio;
		/* XXX SCHED_DEADLINE bits missing */
	),

	TP_printk("comm=%s pid=%d oldvipprio=%d newvipprio=%d",
			__entry->comm, __entry->pid,
			__entry->oldvipprio, __entry->newvipprio)
);
#endif
#endif

#ifdef CONFIG_DETECT_HUNG_TASK
TRACE_EVENT(sched_process_hang,
	TP_PROTO(struct task_struct *tsk),
	TP_ARGS(tsk),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid = tsk->pid;
	),

	TP_printk("comm=%s pid=%d", __entry->comm, __entry->pid)
);
#endif /* CONFIG_DETECT_HUNG_TASK */

/*
 * Tracks migration of tasks from one runqueue to another. Can be used to
 * detect if automatic NUMA balancing is bouncing between nodes.
 */
TRACE_EVENT(sched_move_numa,

	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu),

	TP_ARGS(tsk, src_cpu, dst_cpu),

	TP_STRUCT__entry(
		__field( pid_t,	pid			)
		__field( pid_t,	tgid			)
		__field( pid_t,	ngid			)
		__field( int,	src_cpu			)
		__field( int,	src_nid			)
		__field( int,	dst_cpu			)
		__field( int,	dst_nid			)
	),

	TP_fast_assign(
		__entry->pid		= task_pid_nr(tsk);
		__entry->tgid		= task_tgid_nr(tsk);
		__entry->ngid		= task_numa_group_id(tsk);
		__entry->src_cpu	= src_cpu;
		__entry->src_nid	= cpu_to_node(src_cpu);
		__entry->dst_cpu	= dst_cpu;
		__entry->dst_nid	= cpu_to_node(dst_cpu);
	),

	TP_printk("pid=%d tgid=%d ngid=%d src_cpu=%d src_nid=%d dst_cpu=%d dst_nid=%d",
			__entry->pid, __entry->tgid, __entry->ngid,
			__entry->src_cpu, __entry->src_nid,
			__entry->dst_cpu, __entry->dst_nid)
);

DECLARE_EVENT_CLASS(sched_numa_pair_template,

	TP_PROTO(struct task_struct *src_tsk, int src_cpu,
		 struct task_struct *dst_tsk, int dst_cpu),

	TP_ARGS(src_tsk, src_cpu, dst_tsk, dst_cpu),

	TP_STRUCT__entry(
		__field( pid_t,	src_pid			)
		__field( pid_t,	src_tgid		)
		__field( pid_t,	src_ngid		)
		__field( int,	src_cpu			)
		__field( int,	src_nid			)
		__field( pid_t,	dst_pid			)
		__field( pid_t,	dst_tgid		)
		__field( pid_t,	dst_ngid		)
		__field( int,	dst_cpu			)
		__field( int,	dst_nid			)
	),

	TP_fast_assign(
		__entry->src_pid	= task_pid_nr(src_tsk);
		__entry->src_tgid	= task_tgid_nr(src_tsk);
		__entry->src_ngid	= task_numa_group_id(src_tsk);
		__entry->src_cpu	= src_cpu;
		__entry->src_nid	= cpu_to_node(src_cpu);
		__entry->dst_pid	= dst_tsk ? task_pid_nr(dst_tsk) : 0;
		__entry->dst_tgid	= dst_tsk ? task_tgid_nr(dst_tsk) : 0;
		__entry->dst_ngid	= dst_tsk ? task_numa_group_id(dst_tsk) : 0;
		__entry->dst_cpu	= dst_cpu;
		__entry->dst_nid	= dst_cpu >= 0 ? cpu_to_node(dst_cpu) : -1;
	),

	TP_printk("src_pid=%d src_tgid=%d src_ngid=%d src_cpu=%d src_nid=%d dst_pid=%d dst_tgid=%d dst_ngid=%d dst_cpu=%d dst_nid=%d",
			__entry->src_pid, __entry->src_tgid, __entry->src_ngid,
			__entry->src_cpu, __entry->src_nid,
			__entry->dst_pid, __entry->dst_tgid, __entry->dst_ngid,
			__entry->dst_cpu, __entry->dst_nid)
);

DEFINE_EVENT(sched_numa_pair_template, sched_stick_numa,

	TP_PROTO(struct task_struct *src_tsk, int src_cpu,
		 struct task_struct *dst_tsk, int dst_cpu),

	TP_ARGS(src_tsk, src_cpu, dst_tsk, dst_cpu)
);

DEFINE_EVENT(sched_numa_pair_template, sched_swap_numa,

	TP_PROTO(struct task_struct *src_tsk, int src_cpu,
		 struct task_struct *dst_tsk, int dst_cpu),

	TP_ARGS(src_tsk, src_cpu, dst_tsk, dst_cpu)
);


/*
 * Tracepoint for waking a polling cpu without an IPI.
 */
TRACE_EVENT(sched_wake_idle_without_ipi,

	TP_PROTO(int cpu),

	TP_ARGS(cpu),

	TP_STRUCT__entry(
		__field(	int,	cpu	)
	),

	TP_fast_assign(
		__entry->cpu	= cpu;
	),

	TP_printk("cpu=%d", __entry->cpu)
);

/*
 * Following tracepoints are not exported in tracefs and provide hooking
 * mechanisms only for testing and debugging purposes.
 *
 * Postfixed with _tp to make them easily identifiable in the code.
 */
DECLARE_TRACE(pelt_cfs_tp,
	TP_PROTO(struct cfs_rq *cfs_rq),
	TP_ARGS(cfs_rq));

DECLARE_TRACE(pelt_rt_tp,
	TP_PROTO(struct rq *rq),
	TP_ARGS(rq));

DECLARE_TRACE(pelt_dl_tp,
	TP_PROTO(struct rq *rq),
	TP_ARGS(rq));

DECLARE_TRACE(pelt_thermal_tp,
	TP_PROTO(struct rq *rq),
	TP_ARGS(rq));

DECLARE_TRACE(pelt_irq_tp,
	TP_PROTO(struct rq *rq),
	TP_ARGS(rq));

DECLARE_TRACE(pelt_se_tp,
	TP_PROTO(struct sched_entity *se),
	TP_ARGS(se));

DECLARE_TRACE(sched_cpu_capacity_tp,
	TP_PROTO(struct rq *rq),
	TP_ARGS(rq));

DECLARE_TRACE(sched_overutilized_tp,
	TP_PROTO(struct root_domain *rd, bool overutilized),
	TP_ARGS(rd, overutilized));

DECLARE_TRACE(sched_util_est_cfs_tp,
	TP_PROTO(struct cfs_rq *cfs_rq),
	TP_ARGS(cfs_rq));

DECLARE_TRACE(sched_util_est_se_tp,
	TP_PROTO(struct sched_entity *se),
	TP_ARGS(se));

DECLARE_TRACE(sched_update_nr_running_tp,
	TP_PROTO(struct rq *rq, int change),
	TP_ARGS(rq, change));

#ifdef CONFIG_HISI_EAS_SCHED
/*
 * Tracepoint for eas_store
 */
TRACE_EVENT(eas_attr_store,

	TP_PROTO(const char *name, int value),

	TP_ARGS(name, value),

	TP_STRUCT__entry(
		__array( char,	name,	TASK_COMM_LEN	)
		__field( int,		value		)
	),

	TP_fast_assign(
		memcpy(__entry->name, name, TASK_COMM_LEN);
		__entry->value		= value;
	),

	TP_printk("name=%s value=%d", __entry->name, __entry->value)
);

/*
 * Tracepoint for boost_write
 */
TRACE_EVENT(sched_uclamp_boost,

	TP_PROTO(const char *name, int boost),

	TP_ARGS(name, boost),

	TP_STRUCT__entry(
		__array( char,	name,	TASK_COMM_LEN	)
		__field( int,		boost		)
	),

	TP_fast_assign(
		memcpy(__entry->name, name, TASK_COMM_LEN);
		__entry->boost		= boost;
	),

	TP_printk("name=%s boost=%d", __entry->name, __entry->boost)
);

/*
 * Tracepoint for accounting task boosted utilization
 */
TRACE_EVENT(sched_boost_task,

	TP_PROTO(struct task_struct *tsk, unsigned long util, long margin),

	TP_ARGS(tsk, util, margin),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN		)
		__field( pid_t,		pid			)
		__field( unsigned long,	util			)
		__field( long,		margin			)
		__field( unsigned int,	min_util		)
		__field( unsigned int,	max_util		)

	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid      = tsk->pid;
		__entry->util     = util;
		__entry->margin   = margin;
		__entry->min_util = get_task_min_util(tsk);
		__entry->max_util = get_task_max_util(tsk);
	),

	TP_printk("comm=%s pid=%d util=%lu margin=%ld min_util=%u max_util=%u",
		  __entry->comm, __entry->pid, __entry->util,
		  __entry->margin, __entry->min_util, __entry->max_util)
);

/*
 * Tracepoint for sched_setaffinity
 */
TRACE_EVENT(sched_set_affinity,

	TP_PROTO(struct task_struct *p, const struct cpumask *mask),

	TP_ARGS(p, mask),

	TP_STRUCT__entry(
		__array(   char,	comm,	TASK_COMM_LEN	)
		__field(   pid_t,	pid			)
		__bitmask( cpus,	num_possible_cpus()	)
	),

	TP_fast_assign(
		__entry->pid = p->pid;
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__assign_bitmask(cpus, cpumask_bits(mask), num_possible_cpus());
	),

	TP_printk("comm=%s pid=%d cpus=%s",
		__entry->comm, __entry->pid, __get_bitmask(cpus))
);

/*
 * Tracepoint for sched_setscheduler
 */
TRACE_EVENT(sched_setscheduler,

	TP_PROTO(struct task_struct *p, int oldpolicy, int oldprio,
		 unsigned int sched_flags, bool user, bool pi),

	TP_ARGS(p, oldpolicy, oldprio, sched_flags, user, pi),

	TP_STRUCT__entry(
		__array(char,	comm,	TASK_COMM_LEN	)
		__field(pid_t,		pid		)
		__field(int,		oldpolicy	)
		__field(int,		newpolicy	)
		__field(int,		oldprio		)
		__field(int,		newprio		)
		__field(unsigned int,	flags		)
		__field(bool,		user		)
		__field(bool,		pi		)
	),

	TP_fast_assign(
		__entry->pid = p->pid;
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->oldpolicy = oldpolicy;
		__entry->newpolicy = p->policy;
		__entry->oldprio = oldprio;
		__entry->newprio = p->prio;
		__entry->flags = sched_flags;
		__entry->user = user;
		__entry->pi = pi;
	),

	TP_printk("comm=%s pid=%d prio %d=>%d policy %d=>%d "
		  "flags=%u user=%u pi=%u",
		__entry->comm, __entry->pid,
		__entry->oldprio, __entry->newprio,
		__entry->oldpolicy, __entry->newpolicy,
		__entry->flags, __entry->user, __entry->pi)
);

TRACE_EVENT(sched_find_new_ilb,

	TP_PROTO(int ilb, bool allow_bigger),

	TP_ARGS(ilb, allow_bigger),

	TP_STRUCT__entry(
		__field(int, ilb)
		__field(bool, allow_bigger)
	),

	TP_fast_assign(
		__entry->ilb = ilb;
		__entry->allow_bigger = allow_bigger;
	),

	TP_printk("ilb=%d allow_bigger=%u", __entry->ilb, __entry->allow_bigger)
);
#endif /* CONFIG_HISI_EAS_SCHED */

#ifdef CONFIG_SCHED_TRACEPOINT
TRACE_EVENT(sched_load_balance_fbg,

	TP_PROTO(unsigned long busiest, int bgroup_type, unsigned long bavg_load,
		 unsigned long local, int lgroup_type, unsigned long lavg_load,
		 unsigned long sds_avg_load, unsigned long imbalance, int migration_type),

	TP_ARGS(busiest, bgroup_type, bavg_load,
		local, lgroup_type, lavg_load,
		sds_avg_load, imbalance, migration_type),

	TP_STRUCT__entry(
		__field(unsigned long,		busiest)
		__field(int,			bgp_type)
		__field(unsigned long,		bavg_load)
		__field(unsigned long,		local)
		__field(int,			lgp_type)
		__field(unsigned long,		lavg_load)
		__field(unsigned long,		sds_avg)
		__field(unsigned long,		imbalance)
		__field(int,			migration_type)
	),

	TP_fast_assign(
		__entry->busiest		= busiest;
		__entry->bgp_type		= bgroup_type;
		__entry->bavg_load		= bavg_load;
		__entry->bgp_type		= bgroup_type;
		__entry->local			= local;
		__entry->lgp_type		= lgroup_type;
		__entry->lavg_load		= lavg_load;
		__entry->sds_avg		= sds_avg_load;
		__entry->imbalance		= imbalance;
		__entry->migration_type		= migration_type;
	),

	TP_printk("busiest group=%#lx type=%d avg_load=%ld"
		  " local group=%#lx type=%d avg_load=%ld"
		  " domain_avg_load=%ld imbalance=%ld migration_type=%d",
		  __entry->busiest, __entry->bgp_type, __entry->bavg_load,
		  __entry->local, __entry->lgp_type, __entry->lavg_load,
		  __entry->sds_avg, __entry->imbalance, __entry->migration_type)
);

TRACE_EVENT(sched_load_balance_sg_stats,

	TP_PROTO(unsigned long sg_cpus, int group_type, unsigned int idle_cpus,
		 unsigned int sum_nr_running, unsigned long group_load,
		 unsigned long group_capacity, unsigned long group_util,
		 unsigned long misfit_load, unsigned long runnable_load,
		 unsigned long busiest),

	TP_ARGS(sg_cpus, group_type, idle_cpus, sum_nr_running, group_load,
		group_capacity, group_util, misfit_load, runnable_load, busiest),

	TP_STRUCT__entry(
		__field(unsigned long,		group_mask)
		__field(int,			group_type)
		__field(unsigned int,		group_idle_cpus)
		__field(unsigned int,		sum_nr_running)
		__field(unsigned long,		group_load)
		__field(unsigned long,		group_capacity)
		__field(unsigned long,		group_util)
		__field(unsigned long,		misfit_task_load)
		__field(unsigned long,		runnable_load)
		__field(unsigned long,		busiest)
	),

	TP_fast_assign(
		__entry->group_mask		= sg_cpus;
		__entry->group_type		= group_type;
		__entry->group_idle_cpus	= idle_cpus;
		__entry->sum_nr_running		= sum_nr_running;
		__entry->group_load		= group_load;
		__entry->group_capacity		= group_capacity;
		__entry->group_util		= group_util;
		__entry->misfit_task_load	= misfit_load;
		__entry->runnable_load		= runnable_load;
		__entry->busiest		= busiest;
	),

	TP_printk("sched_group=%#lx type=%d idle_cpus=%u sum_nr_run=%u"
		  " load=%lu capacity=%lu util=%lu"
		  " misfit=%lu runnable=%lu busiest_group=%#lx",
		  __entry->group_mask, __entry->group_type,
		  __entry->group_idle_cpus, __entry->sum_nr_running,
		  __entry->group_load, __entry->group_capacity,
		  __entry->group_util, __entry->misfit_task_load,
		  __entry->runnable_load, __entry->busiest)
);

TRACE_EVENT(sched_load_balance,

	TP_PROTO(int cpu, int idle, int balance,
		long imbalance, unsigned int env_flags, int ld_moved,
		int active_balance, unsigned int balance_interval),

	TP_ARGS(cpu, idle, balance, imbalance,
		env_flags, ld_moved, active_balance, balance_interval),

	TP_STRUCT__entry(
		__field(int,		cpu)
		__field(int,		idle)
		__field(int,		balance)
		__field(long,		imbalance)
		__field(unsigned int,	env_flags)
		__field(int,		active_balance)
		__field(int,		ld_moved)
		__field(unsigned int,	balance_interval)
	),

	TP_fast_assign(
		__entry->cpu                    = cpu;
		__entry->idle                   = idle;
		__entry->balance                = balance;
		__entry->imbalance              = imbalance;
		__entry->env_flags              = env_flags;
		__entry->ld_moved               = ld_moved;
		__entry->active_balance         = active_balance;
		__entry->balance_interval       = balance_interval;
	),

	TP_printk("cpu=%d idle=%d continue=%d imbalance=%ld"
		  " flags=%#x ld_moved=%d active_balance=%d balance_interval=%d",
		  __entry->cpu, __entry->idle, __entry->balance,
		  __entry->imbalance, __entry->env_flags,
		  __entry->ld_moved, __entry->active_balance, __entry->balance_interval)
);

TRACE_EVENT(sched_load_balance_detach_tasks,

	TP_PROTO(int pid, int scpu, int dcpu, unsigned long affinity,
		 long imbalance, unsigned int failed,
		 unsigned long load, unsigned long util),

	TP_ARGS(pid, scpu, dcpu, affinity,
		imbalance, failed, load, util),

	TP_STRUCT__entry(
		__field(int,		pid)
		__field(int,		scpu)
		__field(int,		dcpu)
		__field(unsigned long,	affinity)
		__field(long,		imbalance)
		__field(unsigned int,	failed)
		__field(unsigned long,	load)
		__field(unsigned long,	util)
	),

	TP_fast_assign(
		__entry->pid		= pid;
		__entry->scpu		= scpu;
		__entry->dcpu		= dcpu;
		__entry->affinity	= affinity;
		__entry->imbalance	= imbalance;
		__entry->failed		= failed;
		__entry->load		= load;
		__entry->util		= util;
	),

	TP_printk("pid=%d src=%d dst=%d affinity=%#lx"
		  " imbalance=%ld failed=%u load=%lu util=%lu",
		  __entry->pid, __entry->scpu, __entry->dcpu,
		  __entry->affinity, __entry->imbalance,
		  __entry->failed, __entry->load, __entry->util)
);

TRACE_EVENT(sched_cpu_util,

	TP_PROTO(int cpu, unsigned int nr, long new_util,
		 unsigned int capacity,
		 unsigned int capacity_curr,
		 unsigned int capacity_orig,
		 int idle, int high_irq, int isolated, bool reserved),

	TP_ARGS(cpu, nr, new_util, capacity,
		capacity_curr, capacity_orig,
		idle, high_irq, isolated, reserved),

	TP_STRUCT__entry(
		__field(unsigned int, cpu)
		__field(unsigned int, nr_running)
		__field(long, new_util)
		__field(unsigned int, capacity)
		__field(unsigned int, capacity_curr)
		__field(unsigned int, capacity_orig)
		__field(int, idle)
		__field(int, high_irq)
		__field(int, isolated)
		__field(bool, reserved)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->nr_running		= nr;
		__entry->new_util		= new_util;
		__entry->capacity		= capacity;
		__entry->capacity_curr		= capacity_curr;
		__entry->capacity_orig		= capacity_orig;
		__entry->idle			= idle;
		__entry->high_irq		= high_irq;
		__entry->isolated		= isolated;
		__entry->reserved		= reserved;
	),

	TP_printk("cpu=%d nr=%d new_util=%ld "
		  "cap_curr=%u capacity=%u cap_orig=%u "
		  "idle=%d high_irq=%d isolated=%d reserved=%d",
		  __entry->cpu, __entry->nr_running,
		  __entry->new_util, __entry->capacity_curr,
		  __entry->capacity, __entry->capacity_orig,
		  __entry->idle, __entry->high_irq,
		  __entry->isolated, __entry->reserved)
);

/*
 * Tracepoint for sched_find_energy_efficient_cpu
 */
TRACE_EVENT(sched_find_energy_efficient_cpu,

	TP_PROTO(struct task_struct *tsk, unsigned long task_util,
		 bool boosted, bool latency_sensitive, int prev_cpu,
		 int best_energy_cpu,
		 int best_idle_cpu, unsigned long best_idle_cpu_cap,
		 int max_spare_cap_cpu_ls, unsigned long max_spare_cap_ls),

	TP_ARGS(tsk, task_util, boosted, latency_sensitive, prev_cpu,
		best_energy_cpu, best_idle_cpu,
		best_idle_cpu_cap, max_spare_cap_cpu_ls, max_spare_cap_ls),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__field(unsigned long, task_util)
		__field(bool, boosted)
		__field(bool, latency_sensitive)
		__field(int, prev_cpu)
		__field(int, best_energy_cpu)
		__field(int, best_idle_cpu)
		__field(unsigned long, best_idle_cpu_cap)
		__field(int, max_spare_cap_cpu_ls)
		__field(unsigned long, max_spare_cap_ls)
		),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid = tsk->pid;
		__entry->task_util = task_util;
		__entry->boosted = boosted;
		__entry->latency_sensitive = latency_sensitive;
		__entry->prev_cpu = prev_cpu;
		__entry->best_energy_cpu = best_energy_cpu;
		__entry->best_idle_cpu = best_idle_cpu;
		__entry->best_idle_cpu_cap = best_idle_cpu_cap;
		__entry->max_spare_cap_cpu_ls = max_spare_cap_cpu_ls;
		__entry->max_spare_cap_ls = max_spare_cap_ls;
		),

	TP_printk("pid=%d comm=%s util=%lu boosted=%u "
		  "ls=%u prev_cpu=%d best_energy_cpu=%d "
		  "best_idle_cpu=%d best_idle_cpu_cap=%ld "
		  "max_spare_cap_cpu=%d max_spare_cap=%ld",
		  __entry->pid, __entry->comm, __entry->task_util,
		  __entry->boosted, __entry->latency_sensitive,
		  __entry->prev_cpu, __entry->best_energy_cpu,
		  __entry->best_idle_cpu, __entry->best_idle_cpu_cap,
		  __entry->max_spare_cap_cpu_ls, __entry->max_spare_cap_ls)
);

TRACE_EVENT(sched_compute_energy,

	TP_PROTO(struct task_struct *tsk, unsigned long base_energy,
		 int cpu, unsigned long cur_delta,
		 int best_energy_cpu, long best_delta),

	TP_ARGS(tsk, base_energy, cpu, cur_delta,
		best_energy_cpu, best_delta),

	TP_STRUCT__entry(
		__field(pid_t, pid)
		__field(unsigned long, base_energy)
		__field(int, cpu)
		__field(unsigned long, cur_delta)
		__field(int, best_energy_cpu)
		__field(long, best_delta)
		),

	TP_fast_assign(
		__entry->pid = tsk->pid;
		__entry->base_energy = base_energy;
		__entry->cpu = cpu;
		__entry->cur_delta = cur_delta;
		__entry->best_energy_cpu = best_energy_cpu;
		__entry->best_delta = best_delta;
		),

	TP_printk("pid=%d base_energy=%ld cpu=%d "
		  "cur_delta=%ld best_energy_cpu=%d best_delta=%ld",
		  __entry->pid, __entry->base_energy, __entry->cpu,
		  __entry->cur_delta, __entry->best_energy_cpu, __entry->best_delta)
);

TRACE_EVENT(find_busiest_queue,

	TP_PROTO(int busiest_cpu, unsigned long busiest_util,
		 unsigned long busiest_load, unsigned long busiest_capacity,
		 unsigned long busiest_nr),

	TP_ARGS(busiest_cpu, busiest_util,
		busiest_load, busiest_capacity, busiest_nr),

	TP_STRUCT__entry(
		__field(int, busiest_cpu)
		__field(unsigned long, busiest_util)
		__field(unsigned long, busiest_load)
		__field(unsigned long, busiest_capacity)
		__field(unsigned int, busiest_nr)
		),

	TP_fast_assign(
		__entry->busiest_cpu = busiest_cpu;
		__entry->busiest_util = busiest_util;
		__entry->busiest_load = busiest_load;
		__entry->busiest_capacity = busiest_capacity;
		__entry->busiest_nr = busiest_nr;
		),

	TP_printk("busiest=%d util=%lu load=%lu cap=%lu nr=%u",
		  __entry->busiest_cpu, __entry->busiest_util,
		  __entry->busiest_load, __entry->busiest_capacity,
		  __entry->busiest_nr)
);
#endif /* CONFIG_SCHED_TRACEPOINT */

#ifdef CONFIG_SCHED_PER_SD_OVERUTILIZED
/*
 * Tracepoint for system overutilized flag
 */
struct sched_domain;
TRACE_EVENT_CONDITION(sched_overutilized,

	TP_PROTO(struct sched_domain *sd, bool was_overutilized, bool overutilized),

	TP_ARGS(sd, was_overutilized, overutilized),

	TP_CONDITION(overutilized != was_overutilized),

	TP_STRUCT__entry(
		__field( bool,	overutilized	  )
		__array( char,  cpulist , 32      )
	),

	TP_fast_assign(
		__entry->overutilized	= overutilized;
		scnprintf(__entry->cpulist, sizeof(__entry->cpulist), "%*pbl", cpumask_pr_args(sched_domain_span(sd)));
	),

	TP_printk("overutilized=%d sd_span=%s",
		__entry->overutilized ? 1 : 0, __entry->cpulist)
);
#endif /* CONFIG_SCHED_PER_SD_OVERUTILIZED */

#ifdef CONFIG_SCHED_RTG
/* note msg size is less than TASK_COMM_LEN */
TRACE_EVENT(find_rtg_target_cpu,

	TP_PROTO(struct task_struct *p, const struct cpumask *perferred_cpumask, char *msg, int cpu),

	TP_ARGS(p, perferred_cpumask, msg, cpu),

	TP_STRUCT__entry(
		__array(char, comm, TASK_COMM_LEN)
		__field(pid_t, pid)
		__bitmask(cpus,	num_possible_cpus())
		__array(char, msg, TASK_COMM_LEN)
		__field(int, cpu)
	),

	TP_fast_assign(
		__entry->pid = p->pid;
		memcpy_s(__entry->comm, TASK_COMM_LEN, p->comm, TASK_COMM_LEN);
		__assign_bitmask(cpus, cpumask_bits(perferred_cpumask), num_possible_cpus());
		memcpy_s(__entry->msg, TASK_COMM_LEN, msg, min((size_t)TASK_COMM_LEN, strlen(msg)+1));
		__entry->cpu = cpu;
	),

	TP_printk("comm=%s pid=%d perferred_cpus=%s reason=%s target_cpu=%d",
		__entry->comm, __entry->pid, __get_bitmask(cpus), __entry->msg, __entry->cpu)
);

#ifdef CONFIG_FRAME_RTG
/*
 * Tracepoiny for rtg frame sched
 */
TRACE_EVENT(rtg_frame_sched,

	TP_PROTO(int rtgid, const char *s, s64 value),

	TP_ARGS(rtgid, s, value),
	TP_STRUCT__entry(
		__field(int, rtgid)
		__field(struct frame_info *, frame)
		__field(pid_t, pid)
		__string(str, s)
		__field(s64, value)
	),

	TP_fast_assign(
		__assign_str(str, s);
		__entry->rtgid = rtgid != -1 ? rtgid : (current->grp ? current->grp->id : 0);
		__entry->frame = rtg_frame_info(rtgid);
		__entry->pid = __entry->frame ? ((__entry->frame->pid_task) ? ((__entry->frame->pid_task)->pid) : current->tgid) : current->tgid;
		__entry->value = value;
	),
	TP_printk("C|%d|%s_%d|%lld", __entry->pid, __get_str(str), __entry->rtgid, __entry->value)
);
#endif /* CONFIG_FRAME_RTG */
#endif /* CONFIG_SCHED_RTG */

#ifdef CONFIG_SCHED_RUNNING_TASK_ROTATION
TRACE_EVENT(rotation_checkpoint,

	TP_PROTO(unsigned int nr_big, unsigned int enabled),

	TP_ARGS(nr_big, enabled),

	TP_STRUCT__entry(
		__field(unsigned int,	nr_big)
		__field(unsigned int,	enabled)
	),

	TP_fast_assign(
		__entry->nr_big		= nr_big;
		__entry->enabled	= enabled;
	),

	TP_printk("nr_big=%u enabled=%u",
		__entry->nr_big, __entry->enabled)
);
#endif /* CONFIG_SCHED_RUNNING_TASK_ROTATION */

#ifdef CONFIG_CORE_CTRL
TRACE_EVENT(core_ctl_get_nr_running_avg,

	TP_PROTO(const struct cpumask *cpus, unsigned int avg,
		 unsigned int big_avg, unsigned int iowait_avg,
		 unsigned int nr_max),

	TP_ARGS(cpus, avg, big_avg, iowait_avg, nr_max),

	TP_STRUCT__entry(
		__bitmask( cpumask,	num_possible_cpus()	)
		__field( unsigned int,	avg			)
		__field( unsigned int,	big_avg			)
		__field( unsigned int,	iowait_avg		)
		__field( unsigned int,	nr_max			)
	),

	TP_fast_assign(
		__assign_bitmask(cpumask, cpumask_bits(cpus),
				 num_possible_cpus());
		__entry->avg		= avg;
		__entry->big_avg	= big_avg;
		__entry->iowait_avg	= iowait_avg;
		__entry->nr_max		= nr_max;
	),

	TP_printk("cpu=%s avg=%u big_avg=%u iowait_avg=%u max_nr=%u",
		  __get_bitmask(cpumask), __entry->avg, __entry->big_avg,
		  __entry->iowait_avg, __entry->nr_max)
);

TRACE_EVENT(core_ctl_eval_need,

	TP_PROTO(const struct cpumask *cpus, unsigned int nrrun,
		 unsigned int busy_cpus, unsigned int active_cpus,
		 unsigned int old_need, unsigned int new_need,
		 s64 elapsed, unsigned int updated),
	TP_ARGS(cpus, nrrun, busy_cpus, active_cpus,
		old_need, new_need, elapsed, updated),
	TP_STRUCT__entry(
		__bitmask( cpumask, num_possible_cpus()	)
		__field( u32, nrrun			)
		__field( u32, busy_cpus			)
		__field( u32, active_cpus		)
		__field( u32, old_need			)
		__field( u32, new_need			)
		__field( s64, elapsed			)
		__field( u32, updated			)
	),
	TP_fast_assign(
		__assign_bitmask(cpumask, cpumask_bits(cpus),
				 num_possible_cpus());
		__entry->nrrun		= nrrun;
		__entry->busy_cpus	= busy_cpus;
		__entry->active_cpus	= active_cpus;
		__entry->old_need	= old_need;
		__entry->new_need	= new_need;
		__entry->elapsed	= elapsed;
		__entry->updated	= updated;
	),
	TP_printk("cpu=%s, nrrun:%u, busy:%u, active:%u,"
		  " old_need=%u, new_need=%u, elapsed =%lld, updated=%u",
		  __get_bitmask(cpumask), __entry->nrrun, __entry->busy_cpus,
		  __entry->active_cpus, __entry->old_need, __entry->new_need,
		  __entry->elapsed, __entry->updated)
);

TRACE_EVENT(core_ctl_update_busy,

	TP_PROTO(unsigned int cpu, unsigned int load,
		 unsigned int old_is_busy, unsigned int is_busy),
	TP_ARGS(cpu, load, old_is_busy, is_busy),
	TP_STRUCT__entry(
		__field( u32, cpu		)
		__field( u32, load		)
		__field( u32, old_is_busy	)
		__field( u32, is_busy		)
	),
	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->load		= load;
		__entry->old_is_busy	= old_is_busy;
		__entry->is_busy	= is_busy;
	),
	TP_printk("cpu=%u, load=%u, old_is_busy=%u, new_is_busy=%u",
		  __entry->cpu, __entry->load, __entry->old_is_busy,
		  __entry->is_busy)
);

TRACE_EVENT(core_ctl_set_boost,

	TP_PROTO(u32 refcount),
	TP_ARGS(refcount),
	TP_STRUCT__entry(
		__field( u32, refcount	)
	),
	TP_fast_assign(
		__entry->refcount	= refcount;
	),
	TP_printk("boost=%u", __entry->refcount)
);
#endif /* CONFIG_CORE_CTRL */

#ifdef CONFIG_CPU_ISOLATION_OPT
/*
 * sched_isolate - called when cores are isolated/unisolated
 *
 * @time: amount of time in us it took to isolate/unisolate
 * @isolate: 1 if isolating, 0 if unisolating
 *
 */
TRACE_EVENT(sched_isolate,

	TP_PROTO(unsigned int requested_cpu, unsigned int isolated_cpus,
		 u64 start_time, unsigned char isolate),

	TP_ARGS(requested_cpu, isolated_cpus, start_time, isolate),

	TP_STRUCT__entry(
		__field( u32, requested_cpu	)
		__field( u32, isolated_cpus	)
		__field( u32, time		)
		__field( unsigned char, isolate	)
	),

	TP_fast_assign(
		__entry->requested_cpu	= requested_cpu;
		__entry->isolated_cpus	= isolated_cpus;
		__entry->time		= div64_u64(sched_clock() - start_time, 1000);
		__entry->isolate	= isolate;
	),

	TP_printk("iso cpu=%u cpus=0x%x time=%u us isolated=%d",
		  __entry->requested_cpu, __entry->isolated_cpus,
		  __entry->time, __entry->isolate)
);
#endif /* CONFIG_CPU_ISOLATION_OPT */

#ifdef CONFIG_RT_CAS
extern unsigned long uclamp_task_util(struct task_struct *p);
TRACE_EVENT(rt_energy_aware_wake_cpu,

	TP_PROTO(struct task_struct *task, bool boosted,
		bool check_cap, int best_cpu),

	TP_ARGS(task, boosted, check_cap, best_cpu),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,		pid		)
		__field( int,		prio		)
		__field( unsigned long,	affinity	)
		__field( unsigned long,	util		)
		__field( bool,		boosted		)
		__field( bool,		check_cap	)
		__field( int,		best_cpu	)
	),

	TP_fast_assign(
		memcpy(__entry->comm, task->comm, TASK_COMM_LEN);
		__entry->pid		= task->pid;
		__entry->prio		= task->prio;
		__entry->affinity	= cpumask_bits(task->cpus_ptr)[0];
		__entry->util		= uclamp_task_util(task);
		__entry->boosted	= boosted;
		__entry->check_cap	= check_cap;
		__entry->best_cpu	= best_cpu;
	),

	TP_printk("pid=%d comm=%s prio=%d allowed=%#lx util=%lu"
		" boosted=%d check_cap=%d best_cpu=%d",
		__entry->pid, __entry->comm, __entry->prio,
		__entry->affinity, __entry->util, __entry->boosted,
		__entry->check_cap, __entry->best_cpu)
);

TRACE_EVENT(push_rt_task,

	TP_PROTO(int cpu, pid_t pid),

	TP_ARGS(cpu, pid),

	TP_STRUCT__entry(
		__field( int,		cpu		)
		__field( pid_t,		pid		)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->pid		= pid;
	),

	TP_printk("cpu=%d pid=%d", __entry->cpu, __entry->pid)
);
#endif

#ifdef CONFIG_SCHED_VIP
TRACE_EVENT(set_vip_prio,

	TP_PROTO(struct task_struct *task, unsigned int prio, int ret),

	TP_ARGS(task, prio, ret),

	TP_STRUCT__entry(
		__field( pid_t,		pid		)
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( unsigned int,	prio		)
		__field( int,		ret		)
	),

	TP_fast_assign(
		__entry->pid		= task->pid;
		memcpy(__entry->comm, task->comm, TASK_COMM_LEN);
		__entry->prio		= prio;
		__entry->ret		= ret;
	),

	TP_printk("pid=%d comm=%s prio=%u ret=%d",
		__entry->pid, __entry->comm, __entry->prio, __entry->ret)
);
#endif

#ifdef CONFIG_SCHED_VIP_PI
TRACE_EVENT(vip_boost,

	TP_PROTO(char dir, struct task_struct *p, int type,
		unsigned int vip_prio, int ret),

	TP_ARGS(dir, p, type, vip_prio, ret),

	TP_STRUCT__entry(
		__field( char,		dir		)
		__field( pid_t,		pid		)
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( int,		type		)
		__field( unsigned int,	vip_prio	)
		__field( int,		ret		)
	),

	TP_fast_assign(
		__entry->dir		= dir;
		__entry->pid		= p->pid;
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->type		= type;
		__entry->vip_prio	= vip_prio;
		__entry->ret		= ret;
	),

	TP_printk("%c type=%d task=%s(%d) vip_prio=%u ret=%d",
		__entry->dir, __entry->type, __entry->comm,
		__entry->pid, __entry->vip_prio, __entry->ret)
);
#endif

#ifdef CONFIG_SCHED_DYNAMIC_PRIO
TRACE_EVENT(dynamic_prio_func,

	TP_PROTO(const char *name, struct task_struct *p),

	TP_ARGS(name, p),

	TP_STRUCT__entry(
		__string(str,		name)
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,		pid		)
	),

	TP_fast_assign(
		__assign_str(str, name);
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
	),

	TP_printk("%s task=%s(%d)",
		__get_str(str), __entry->comm, __entry->pid)
);
#endif

#ifdef CONFIG_SCHED_HISI_UTIL_CLAMP
TRACE_EVENT(set_task_uclamp,

	TP_PROTO(struct task_struct *task, int id, int value),

	TP_ARGS(task, id, value),

	TP_STRUCT__entry(
		__field( pid_t,		pid		)
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( int,		id		)
		__field( int,		value		)
	),

	TP_fast_assign(
		__entry->pid		= task->pid;
		memcpy(__entry->comm, task->comm, TASK_COMM_LEN);
		__entry->id		= id;
		__entry->value		= value;
	),

	TP_printk("pid=%d comm=%s uclamp_id=%d util=%d",
		__entry->pid, __entry->comm, __entry->id, __entry->value)
);
#endif

#ifdef CONFIG_HISI_EAS_SCHED
TRACE_EVENT(sched_waking_sync,

	TP_PROTO(int flags),

	TP_ARGS(flags),

	TP_STRUCT__entry(
		__field( int,		flags		)
	),

	TP_fast_assign(
		__entry->flags		= flags;
	),

	TP_printk("WF_SYNC wake_flags=%d", __entry->flags)
);
#endif

#endif /* _TRACE_SCHED_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
