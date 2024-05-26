/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: Provide log to find lock's owner.
 * Author: Zhang Kuangqi <zhangkuangqi1@huawei.com>
 * Create: 2022-04-19
 */
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/sched/task.h>
#include <linux/spinlock_types.h>
#include <linux/sysrq.h>
#include <log/log_usertype.h>
#include <platform_include/basicplatform/linux/dfx_fiq.h>

#include "sched/sched.h"

#define TIMER_PERIOD 5
#define DECIMAL 10
#define LOCK_BUF_MAX 30

enum LOCK_TYPE {
	MUTEX,
	SPIN_LOCK,
	TYPE_MAX
};

extern struct mutex kernfs_mutex;
extern void *get_rtnl_mutex(void);

static void *get_kernfs_mutex(void)
{
	return (void *)&kernfs_mutex;
}

static struct task_struct *g_lock_owner;

struct lock_operations {
	void (*lock)(void *lock);
	void (*unlock)(void *lock);
	int (*trylock)(void *lock);
	struct task_struct *(*get_owner)(void *lock);
};

struct lock_info {
	const char *name;
	void *(*get_lock)(void);
	enum LOCK_TYPE type;
};

static void _mutex_lock(void *lock)
{
	mutex_lock(lock);
}

static void _mutex_unlock(void *lock)
{
	mutex_unlock(lock);
}

static int _mutex_trylock(void *lock)
{
	return mutex_trylock(lock);
}

static struct task_struct *_mutex_owner(void *lock)
{
	return mutex_owner(lock);
}

struct lock_operations mutex_ops = {
	.lock = _mutex_lock,
	.unlock = _mutex_unlock,
	.trylock = _mutex_trylock,
	.get_owner = _mutex_owner,
};

#ifdef CONFIG_DEBUG_SPINLOCK
void dump_rq_lock_owner(void)
{
	unsigned int i;
	struct rq *rq = NULL;
	struct task_struct *curr = NULL;
	struct task_struct *owner = NULL;

	rcu_read_lock();
	for_each_online_cpu(i) {
		rq = cpu_rq(i);
		curr = rq->curr;
		if (curr) {
			pr_err("cpu%d curr info: comm %s pid %d prio %d\n",
				i, curr->comm, curr->pid, curr->prio);
			show_stack(curr, NULL, KERN_ERR);
		}

		owner = (struct task_struct *)(rq->__lock.owner);
		if (owner && owner != SPINLOCK_OWNER_INIT) {
			pr_err("cpu%d rq_lock is holding by name=%s,PID=%d owner_cpu: %d\n", i,
				owner->comm, owner->pid, rq->__lock.owner_cpu);
			show_stack(owner, NULL, KERN_ERR);
		}
	}
	rcu_read_unlock();
}

struct task_struct *spinlock_owner(struct spinlock *lock)
{
	return (struct task_struct *)READ_ONCE(lock->rlock.owner);
}

static void _spin_lock(void *lock)
{
	spin_lock(lock);
}

static void _spin_unlock(void *lock)
{
	spin_unlock(lock);
}

static int _spin_trylock(void *lock)
{
	return spin_trylock(lock);
}

static struct task_struct *_spinlock_owner(void *lock)
{
	return spinlock_owner(lock);
}

struct lock_operations spinlock_ops = {
	.lock = _spin_lock,
	.unlock = _spin_unlock,
	.trylock = _spin_trylock,
	.get_owner = _spinlock_owner,
};
#endif

static struct lock_operations *g_ops[TYPE_MAX] = {
	&mutex_ops,
#ifdef CONFIG_DEBUG_SPINLOCK
	&spinlock_ops
#endif
};

static struct lock_info g_locks[] = {
	{"kernfs_mutex", get_kernfs_mutex, MUTEX},
	{"rtnl_mutex", get_rtnl_mutex, MUTEX},
};

static inline void *get_lock(unsigned int index)
{
	return g_locks[index].get_lock();
}

static inline void lock(unsigned int index)
{
	g_ops[g_locks[index].type]->lock(get_lock(index));
}

static inline void unlock(unsigned int index)
{
	g_ops[g_locks[index].type]->unlock(get_lock(index));
}

static inline int trylock(unsigned int index)
{
	return g_ops[g_locks[index].type]->trylock(get_lock(index));
}

static inline struct task_struct *get_owner(unsigned int index)
{
	return g_ops[g_locks[index].type]->get_owner(get_lock(index));
}

static inline const char *get_name(unsigned int index)
{
	return g_locks[index].name;
}

void dump_lock_trace(struct timer_list *timer)
{
	unsigned int index;
	struct task_struct *owner = g_lock_owner;

	if (owner) {
		index = (unsigned int)timer->data;
		task_lock(owner);
		pr_err("%s is holding by name=%s,PID=%d\n",
			get_name(index), owner->comm, owner->pid);
		sched_show_task(owner);
		task_unlock(owner);
	}

	mod_timer(timer, (jiffies + TIMER_PERIOD * HZ));
}

struct timer_list lock_timer;
DEFINE_TIMER(lock_timer, dump_lock_trace);

static void check_lock_state(unsigned int index)
{
	if (trylock(index)) {
		unlock(index);
		return;
	}

	g_lock_owner = get_owner(index);
	if (g_lock_owner) {
		lock_timer.data = (unsigned long)index;
		lock(index);
		unlock(index);
		g_lock_owner = NULL;
	}
}

static bool is_beta_user(void)
{
	int i;
	unsigned int beta_flag;

	/* Try 60 times, wait 120s at most */
	for (i = 0; i < 60; i++) {
		beta_flag = get_logusertype_flag();
		if (beta_flag != 0)
			break;
		ssleep(2);
	}

	return (beta_flag == BETA_USER);
}

static int start_lock_trace(void *info)
{
	unsigned int i;
	unsigned int size;

	if (!is_beta_user()) {
		pr_err("non-beta user\n");
		return 0;
	}

	mod_timer(&lock_timer, (jiffies + TIMER_PERIOD * HZ));
	size = ARRAY_SIZE(g_locks);
	while (1) {
		for (i = 0; i < size; i++)
			check_lock_state(i);

		ssleep(30);
	}
	return 0;
}

#ifdef CONFIG_LOCK_TRACE_DEBUG
static ssize_t lock_trace_write(struct file *file,
	const char __user *buf, size_t size, loff_t *ppos)
{
	int i;
	int ret;
	unsigned int index;
	char val[LOCK_BUF_MAX] = {0};

	ret = simple_write_to_buffer(val, sizeof(val), ppos, buf, size);
	if (ret < 0)
		return size;

	ret = kstrtouint(val, DECIMAL, &index);
	if ((ret < 0) || (index >= ARRAY_SIZE(g_locks))) {
		pr_err("%s: Invalid input (%d)\n", __func__, ret);
		return size;
	}

	lock(index);
	i = 0;
	while (1) {
		mdelay(1000);
		i++;
		pr_err("%s : %s locked for %d seconds\n", __func__, get_name(index), i);
	}
	return size;
}

static int lock_trace_show(struct seq_file *s, void *unused)
{
	int i;

	seq_printf(s, "%s\n", "set index to start lock_debug");
	seq_printf(s, "%-5s %-20s\n", "index", "name");
	for (i = 0; i < ARRAY_SIZE(g_locks); i++)
		seq_printf(s, "%-5d %-20s\n", i, get_name(i));

	return 0;
}

static int lock_trace_open(struct inode *inode, struct file *file)
{
	return single_open(file, lock_trace_show, NULL);
}

static const struct file_operations lock_trace_proc_fops = {
	.open		= lock_trace_open,
	.read		= seq_read,
	.write		= lock_trace_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static int __init lock_trace_init(void)
{
	struct dentry *debug_file = NULL;
	struct task_struct *thread = NULL;

#ifdef CONFIG_LOCK_TRACE_DEBUG
	debug_file = debugfs_create_file("lock_trace_debug", 0600, NULL, NULL,
		&lock_trace_proc_fops);
	if (IS_ERR_OR_NULL(debug_file)) {
		pr_err("%s: failed to create %s\n", __func__, "lock_trace_debug");
		return -EIO;
	}
#endif

	thread = kthread_run(start_lock_trace, NULL, "klocktraced");
	if (IS_ERR_OR_NULL(thread))
		pr_err("%s: create thread fail!\n", __func__);

	return 0;
}

module_init(lock_trace_init);
