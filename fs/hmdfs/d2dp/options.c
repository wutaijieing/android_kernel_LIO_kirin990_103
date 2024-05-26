// SPDX-License-Identifier: GPL-2.0
/* D2D protocol options */

#include <asm-generic/errno-base.h>
#include <linux/atomic.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/kconfig.h>
#include <linux/pid.h>
#include <linux/types.h>

#include "d2d.h"
#include "options.h"
#include "protocol.h"

struct affinity_work_data {
	struct work_struct work;
	pid_t pid;
	struct cpumask mask;
};

#if IS_BUILTIN(CONFIG_D2D_PROTOCOL)
/*
 * The kernel symbols `sched_setaffinity` and `sched_getaffinity` are available
 * for built-in modules only
 */
static void set_affinity_work_handler(struct work_struct *work)
{
	struct affinity_work_data *data = NULL;

	data = container_of(work, struct affinity_work_data, work);
	sched_setaffinity(data->pid, &data->mask);
}

static void get_affinity_work_handler(struct work_struct *work)
{
	struct affinity_work_data *data = NULL;

	data = container_of(work, struct affinity_work_data, work);
	sched_getaffinity(data->pid, &data->mask);
}
#else /* IS_BUILTIN(CONFIG_D2D_PROTOCOL) */
static void set_affinity_work_handler(struct work_struct *work)
{
	/* if built as a module, do nothing */
}

static void get_affinity_work_handler(struct work_struct *work)
{
	struct affinity_work_data *data = NULL;

	/* if built as a module, fill bitmask with ones */
	data = container_of(work, struct affinity_work_data, work);
	cpumask_setall(&data->mask);
}
#endif /* IS_BUILTIN(CONFIG_D2D_PROTOCOL) */

void d2d_options_init(struct d2d_options *options)
{
	atomic_set_release(&options->timeout, 0);
	mutex_init(&options->opt_mtx);
}

int d2d_set_opt(struct d2d_protocol *proto, int optname, const void *optval,
		size_t optlen)
{
	struct d2d_options *options = &proto->options;

	switch (optname) {
	case D2D_OPT_RCVTIMEO: {
		int value = 0;

		value = *(const int *)optval;
		if (value < 0)
			return -EINVAL;

		atomic_set_release(&options->timeout, value);
		break;
	}
	case D2D_OPT_TXCPUMASK: {
		/*
		 * Setting cpumask is done on workqueue because
		 * current thread may not have enough permissions
		 * to do it since its credentials are overwritten.
		 */
		int ret = 0;
		struct affinity_work_data data = { };
		struct timers *timers = &proto->timers;
		struct pid *txpid = NULL;
		pid_t txpid_nr = 0;

		mutex_lock(&options->opt_mtx);

		txpid = get_task_pid(proto->transport.tx_task, PIDTYPE_PID);
		txpid_nr = pid_nr(txpid);
		if (!txpid_nr) {
			ret = -ESRCH;
			goto unlock;
		}

		INIT_WORK_ONSTACK(&data.work, set_affinity_work_handler);
		data.mask = *(const struct cpumask *)optval;
		data.pid = proto->transport.tx_task->pid;

		queue_work(timers->timer_wq, &data.work);
		flush_work(&data.work);
unlock:
		put_pid(txpid);
		mutex_unlock(&options->opt_mtx);
		return ret;
	}
	default:
		return -EINVAL;
	}

	return 0;
}

int d2d_get_opt(struct d2d_protocol *proto, int optname, void *optval)
{
	struct d2d_options *options = &proto->options;

	switch (optname) {
	case D2D_OPT_RCVTIMEO: {
		int *val = optval;

		*val = atomic_read_acquire(&options->timeout);
		break;
	}
	case D2D_OPT_TXCPUMASK: {
		/*
		 * Getting cpumask is done on workqueue because
		 * current thread may not have enough permissions
		 * to do it since its credentials are overwritten.
		 */
		int ret = 0;
		struct timers *timers = &proto->timers;
		struct affinity_work_data data = { };
		struct pid *txpid = NULL;
		pid_t txpid_nr = 0;

		mutex_lock(&options->opt_mtx);

		txpid = get_task_pid(proto->transport.tx_task, PIDTYPE_PID);
		txpid_nr = pid_nr(txpid);
		if (!txpid_nr) {
			ret = -ESRCH;
			goto unlock;
		}

		INIT_WORK_ONSTACK(&data.work, get_affinity_work_handler);
		data.pid = proto->transport.tx_task->pid;

		queue_work(timers->timer_wq, &data.work);
		flush_work(&data.work);
		memcpy(optval, &data.mask, sizeof(struct cpumask));
unlock:
		put_pid(txpid);
		mutex_unlock(&options->opt_mtx);
		return ret;
	}
	default:
		return -EINVAL;
	}

	return 0;
}
