/*
 *
 * (C) COPYRIGHT 2018-2020 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <mali_kbase.h>
#include "mali_kbase_config_defaults.h"
#include <mali_kbase_ctx_sched.h>
#include <mali_kbase_reset_gpu.h>
#include <mali_kbase_as_fault_debugfs.h>
#include <mali_kbase_bits.h>
#include "mali_kbase_csf.h"
#include "../tl/mali_kbase_tracepoints.h"
#include "backend/gpu/mali_kbase_pm_internal.h"
#include <linux/export.h>
#include "mali_gpu_csf_registers.h"
#include <mali_base_kernel.h>

/* Value to indicate that a queue group is not groups_to_schedule list */
#define KBASEP_GROUP_PREPARED_SEQ_NUM_INVALID (U32_MAX)

/* Waiting timeout for status change acknowledgment, in milliseconds */
#define CSF_STATE_WAIT_TIMEOUT_MS (100) /* 100 milliseconds */

/* Waiting timeout for scheduler state change for descheduling a CSG */
#define CSG_SCHED_STOP_TIMEOUT_MS (50)

#define CSG_SUSPEND_ON_RESET_WAIT_TIMEOUT_MS DEFAULT_RESET_TIMEOUT_MS

/* Maximum number of endpoints which may run tiler jobs. */
#define CSG_TILER_MAX ((u8)1)

/* Maximum dynamic CSG slot priority value */
#define MAX_CSG_SLOT_PRIORITY ((u8)15)

/* CSF scheduler time slice value */
#define CSF_SCHEDULER_TIME_TICK_MS (100) /* 100 milliseconds */
#define CSF_SCHEDULER_TIME_TICK_JIFFIES \
	msecs_to_jiffies(CSF_SCHEDULER_TIME_TICK_MS)

/*
 * CSF scheduler time threshold for converting "tock" requests into "tick" if
 * they come too close to the end of a tick interval. This avoids scheduling
 * twice in a row.
 */
#define CSF_SCHEDULER_TIME_TICK_THRESHOLD_MS \
	CSF_SCHEDULER_TIME_TICK_MS

#define CSF_SCHEDULER_TIME_TICK_THRESHOLD_JIFFIES \
	msecs_to_jiffies(CSF_SCHEDULER_TIME_TICK_THRESHOLD_MS)

/* Nanoseconds per millisecond */
#define NS_PER_MS ((u64)1000 * 1000)

/*
 * CSF minimum time to reschedule for a new "tock" request. Bursts of "tock"
 * requests are not serviced immediately, but shall wait for a minimum time in
 * order to reduce load on the CSF scheduler thread.
 */
#define CSF_SCHEDULER_TIME_TOCK_JIFFIES 1 /* 1 jiffies-time */

/* Command stream suspended and is idle (empty ring buffer) */
#define CS_IDLE_FLAG (1 << 0)

/* Command stream suspended and is wait for a CQS condition */
#define CS_WAIT_SYNC_FLAG (1 << 1)

static int scheduler_group_schedule(struct kbase_queue_group *group);
static void remove_group_from_idle_wait(struct kbase_queue_group *const group);
static
void insert_group_to_runnable(struct kbase_csf_scheduler *const scheduler,
		struct kbase_queue_group *const group,
		enum kbase_csf_group_state run_state);
static struct kbase_queue_group *get_tock_top_group(
	struct kbase_csf_scheduler *const scheduler);

#define kctx_as_enabled(kctx) (!kbase_ctx_flag(kctx, KCTX_AS_DISABLED_ON_FAULT))

static void release_doorbell(struct kbase_device *kbdev, int doorbell_nr)
{
	WARN_ON(doorbell_nr >= CSF_NUM_DOORBELL);

	lockdep_assert_held(&kbdev->csf.scheduler.lock);
	bitmap_clear(kbdev->csf.scheduler.doorbell_inuse_bitmap,
		doorbell_nr, 1);
}

static int acquire_doorbell(struct kbase_device *kbdev)
{
	int doorbell_nr;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	doorbell_nr = find_first_zero_bit(
			kbdev->csf.scheduler.doorbell_inuse_bitmap,
			CSF_NUM_DOORBELL);

	if (doorbell_nr >= CSF_NUM_DOORBELL)
		return KBASEP_USER_DB_NR_INVALID;

	bitmap_set(kbdev->csf.scheduler.doorbell_inuse_bitmap,
		doorbell_nr, 1);

	return doorbell_nr;
}

static void unassign_user_doorbell_from_group(struct kbase_device *kbdev,
		struct kbase_queue_group *group)
{
	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	if (group->doorbell_nr != KBASEP_USER_DB_NR_INVALID) {
		release_doorbell(kbdev, group->doorbell_nr);
		group->doorbell_nr = KBASEP_USER_DB_NR_INVALID;
	}
}

static void unassign_user_doorbell_from_queue(struct kbase_device *kbdev,
		struct kbase_queue *queue)
{
	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	mutex_lock(&kbdev->csf.reg_lock);

	if (queue->doorbell_nr != KBASEP_USER_DB_NR_INVALID) {
		queue->doorbell_nr = KBASEP_USER_DB_NR_INVALID;
		/* After this the dummy page would be mapped in */
		unmap_mapping_range(kbdev->csf.db_filp->f_inode->i_mapping,
			queue->db_file_offset << PAGE_SHIFT, PAGE_SIZE, 1);
	}

	mutex_unlock(&kbdev->csf.reg_lock);
}

static void assign_user_doorbell_to_group(struct kbase_device *kbdev,
		struct kbase_queue_group *group)
{
	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	if (group->doorbell_nr == KBASEP_USER_DB_NR_INVALID)
		group->doorbell_nr = acquire_doorbell(kbdev);
}

static void assign_user_doorbell_to_queue(struct kbase_device *kbdev,
		struct kbase_queue *const queue)
{
	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	mutex_lock(&kbdev->csf.reg_lock);

	/* If bind operation for the queue hasn't completed yet, then the
	 * the command stream interface can't be programmed for the queue
	 * (even in stopped state) and so the doorbell also can't be assigned
	 * to it.
	 */
	if ((queue->bind_state == KBASE_CSF_QUEUE_BOUND) &&
	    (queue->doorbell_nr == KBASEP_USER_DB_NR_INVALID)) {
		WARN_ON(queue->group->doorbell_nr == KBASEP_USER_DB_NR_INVALID);
		queue->doorbell_nr = queue->group->doorbell_nr;

		/* After this the real Hw doorbell page would be mapped in */
		unmap_mapping_range(
				kbdev->csf.db_filp->f_inode->i_mapping,
				queue->db_file_offset << PAGE_SHIFT,
				PAGE_SIZE, 1);
	}

	mutex_unlock(&kbdev->csf.reg_lock);
}

static void scheduler_doorbell_init(struct kbase_device *kbdev)
{
	int doorbell_nr;

	bitmap_zero(kbdev->csf.scheduler.doorbell_inuse_bitmap,
		CSF_NUM_DOORBELL);

	mutex_lock(&kbdev->csf.scheduler.lock);
	/* Reserve doorbell 0 for use by kernel driver */
	doorbell_nr = acquire_doorbell(kbdev);
	mutex_unlock(&kbdev->csf.scheduler.lock);

	WARN_ON(doorbell_nr != CSF_KERNEL_DOORBELL_NR);
}

static u32 get_nr_active_csgs(struct kbase_device *kbdev)
{
	u32 nr_active_csgs;

	mutex_lock(&kbdev->csf.scheduler.lock);
	nr_active_csgs = bitmap_weight(kbdev->csf.scheduler.csg_inuse_bitmap,
				kbdev->csf.global_iface.group_num);
	mutex_unlock(&kbdev->csf.scheduler.lock);

	return nr_active_csgs;
}

/**
 * csgs_active - returns true if any of CSG slots are in use
 *
 * @kbdev: Instance of a GPU platform device that implements a command
 *         stream front-end interface.
 *
 * Return: the interface is actively engaged flag.
 */
bool csgs_active(struct kbase_device *kbdev)
{
	/* Right now if any of the command stream group interfaces are in use
	 * then we need to assume that there is some work pending.
	 * In future when we have IDLE notifications from firmware implemented
	 * then we would have a better idea of the pending work.
	 */
	return (get_nr_active_csgs(kbdev) != 0);
}

/**
 * csg_slot_in_use - returns true if a queue group has been programmed on a
 *                   given CSG slot.
 *
 * @kbdev: Instance of a GPU platform device that implements a command
 *         stream front-end interface.
 * @slot:  Index/number of the CSG slot in question.
 *
 * Return: the interface is actively engaged flag.
 *
 * Note: Caller must hold the scheduler lock.
 */
static inline bool csg_slot_in_use(struct kbase_device *kbdev, int slot)
{
	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	return (kbdev->csf.scheduler.csg_slots[slot].resident_group != NULL);
}

static bool queue_group_suspended_locked(struct kbase_queue_group *group)
{
	lockdep_assert_held(&group->kctx->kbdev->csf.scheduler.lock);

	return (group->run_state == KBASE_CSF_GROUP_SUSPENDED ||
		group->run_state == KBASE_CSF_GROUP_SUSPENDED_ON_IDLE ||
		group->run_state == KBASE_CSF_GROUP_SUSPENDED_ON_WAIT_SYNC);
}

static bool queue_group_idle_locked(struct kbase_queue_group *group)
{
	lockdep_assert_held(&group->kctx->kbdev->csf.scheduler.lock);

	return (group->run_state == KBASE_CSF_GROUP_IDLE ||
		group->run_state == KBASE_CSF_GROUP_SUSPENDED_ON_IDLE ||
		group->run_state == KBASE_CSF_GROUP_SUSPENDED_ON_WAIT_SYNC);
}

static bool queue_group_scheduled(struct kbase_queue_group *group)
{
	return (group->run_state != KBASE_CSF_GROUP_INACTIVE &&
		group->run_state != KBASE_CSF_GROUP_TERMINATED &&
		group->run_state != KBASE_CSF_GROUP_FAULT_EVICTED);
}

static bool queue_group_scheduled_locked(struct kbase_queue_group *group)
{
	lockdep_assert_held(&group->kctx->kbdev->csf.scheduler.lock);

	return queue_group_scheduled(group);
}

int kbase_csf_scheduler_group_get_slot_locked(struct kbase_queue_group *group)
{
	struct kbase_csf_scheduler *scheduler =
			&group->kctx->kbdev->csf.scheduler;
	int slot_num = group->csg_nr;

	lockdep_assert_held(&scheduler->interrupt_lock);

	if (slot_num >= 0) {
		if (WARN_ON(scheduler->csg_slots[slot_num].resident_group !=
			    group))
			return -1;
	}

	return slot_num;
}

int kbase_csf_scheduler_group_get_slot(struct kbase_queue_group *group)
{
	struct kbase_csf_scheduler *scheduler =
			&group->kctx->kbdev->csf.scheduler;
	unsigned long flags;
	int slot_num;

	spin_lock_irqsave(&scheduler->interrupt_lock, flags);
	slot_num = kbase_csf_scheduler_group_get_slot_locked(group);
	spin_unlock_irqrestore(&scheduler->interrupt_lock, flags);

	return slot_num;
}

static bool kbasep_csf_scheduler_group_is_on_slot_locked(
				struct kbase_queue_group *group)
{
	struct kbase_csf_scheduler *scheduler =
			&group->kctx->kbdev->csf.scheduler;
	int slot_num = group->csg_nr;

	lockdep_assert_held(&scheduler->lock);

	if (slot_num >= 0) {
		if (!WARN_ON(scheduler->csg_slots[slot_num].resident_group !=
			     group))
			return true;
	}

	return false;
}

bool kbase_csf_scheduler_group_events_enabled(struct kbase_device *kbdev,
			struct kbase_queue_group *group)
{
	struct kbase_csf_scheduler *scheduler =
			&group->kctx->kbdev->csf.scheduler;
	int slot_num = group->csg_nr;
	bool result = 0;

	lockdep_assert_held(&scheduler->interrupt_lock);

	if (WARN_ON(slot_num < 0))
		return 0;

	result = (scheduler->csgs_events_enable_mask & BIT(slot_num)) ?
		true : false;

	return result;
}

struct kbase_queue_group *kbase_csf_scheduler_get_group_on_slot(
			struct kbase_device *kbdev, int slot)
{
	lockdep_assert_held(&kbdev->csf.scheduler.interrupt_lock);

	return kbdev->csf.scheduler.csg_slots[slot].resident_group;
}

static int halt_stream_sync(struct kbase_queue *queue)
{
	struct kbase_queue_group *group = queue->group;
	struct kbase_device *kbdev = queue->kctx->kbdev;
	struct kbase_csf_global_iface *global_iface = &kbdev->csf.global_iface;
	struct kbase_csf_cmd_stream_group_info *ginfo;
	struct kbase_csf_cmd_stream_info *stream;
	long remaining =
		kbase_csf_timeout_in_jiffies(CSF_STATE_WAIT_TIMEOUT_MS);

	if (kbase_reset_gpu_is_active(kbdev))
		return -EBUSY;

	if (WARN_ON(!group) ||
	    WARN_ON(!kbasep_csf_scheduler_group_is_on_slot_locked(group)))
		return -EINVAL;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);
	ginfo = &global_iface->groups[group->csg_nr];
	stream = &ginfo->streams[queue->csi_index];

	if (CS_REQ_STATE_GET(kbase_csf_firmware_cs_input_read(stream, CS_REQ)) ==
			CS_REQ_STATE_START) {

		remaining = wait_event_timeout(kbdev->csf.event_wait,
			(CS_ACK_STATE_GET(kbase_csf_firmware_cs_output(stream, CS_ACK))
			 == CS_ACK_STATE_START), remaining);

		if (!remaining) {
			dev_warn(kbdev->dev, "Timed out waiting for queue to start on csi %d bound to group %d on slot %d",
				queue->csi_index, group->handle, group->csg_nr);
			if (kbase_prepare_to_reset_gpu(kbdev))
				kbase_reset_gpu(kbdev);

			return -ETIMEDOUT;
		}

		remaining =
			kbase_csf_timeout_in_jiffies(CSF_STATE_WAIT_TIMEOUT_MS);
	}

	/* Set state to STOP */
	kbase_csf_firmware_cs_input_mask(stream, CS_REQ, CS_REQ_STATE_STOP,
					 CS_REQ_STATE_MASK);

	kbase_csf_ring_cs_kernel_doorbell(kbdev, queue);

	/* Timed wait */
	remaining = wait_event_timeout(kbdev->csf.event_wait,
		(CS_ACK_STATE_GET(kbase_csf_firmware_cs_output(stream, CS_ACK))
		 == CS_ACK_STATE_STOP), remaining);

	if (!remaining) {
		dev_warn(kbdev->dev, "Timed out waiting for queue to stop on csi %d bound to group %d on slot %d",
			 queue->csi_index, group->handle, group->csg_nr);
		if (kbase_prepare_to_reset_gpu(kbdev))
			kbase_reset_gpu(kbdev);
	}
	return (remaining) ? 0 : -ETIMEDOUT;
}

static bool can_halt_stream(struct kbase_device *kbdev,
		struct kbase_queue_group *group)
{
	struct kbase_csf_csg_slot *const csg_slot =
			kbdev->csf.scheduler.csg_slots;
	unsigned long flags;
	bool can_halt;
	int slot;

	if (!queue_group_scheduled(group))
		return true;

	spin_lock_irqsave(&kbdev->csf.scheduler.interrupt_lock, flags);
	slot = kbase_csf_scheduler_group_get_slot_locked(group);
	can_halt = (slot >= 0) &&
		   (atomic_read(&csg_slot[slot].state) == CSG_SLOT_RUNNING);
	spin_unlock_irqrestore(&kbdev->csf.scheduler.interrupt_lock,
				flags);

	return can_halt;
}

/**
 * sched_halt_stream() - Stop a GPU queue when its queue group is not running
 *                       on a CSG slot.
 * @queue: Pointer to the GPU queue to stop.
 *
 * This function handles stopping gpu queues for groups that are either not on
 * a command stream group slot or are on the slot but undergoing transition to
 * resume or suspend states.
 * It waits until the queue group is scheduled on a slot and starts running,
 * which is needed as groups that were suspended may need to resume all queues
 * that were enabled and running at the time of suspension.
 *
 * Return: 0 on success, or negative on failure.
 */
static int sched_halt_stream(struct kbase_queue *queue)
{
	struct kbase_queue_group *group = queue->group;
	struct kbase_device *kbdev = queue->kctx->kbdev;
	struct kbase_csf_scheduler *const scheduler =
			&kbdev->csf.scheduler;
	struct kbase_csf_csg_slot *const csg_slot =
			kbdev->csf.scheduler.csg_slots;
	bool retry_needed = false;
	bool retried = false;
	long remaining;
	int slot;
	int err = 0;

	if (WARN_ON(!group))
		return -EINVAL;

	lockdep_assert_held(&queue->kctx->csf.lock);
	lockdep_assert_held(&scheduler->lock);

	slot = kbase_csf_scheduler_group_get_slot(group);

	if (slot >= 0) {
		WARN_ON(atomic_read(&csg_slot[slot].state) == CSG_SLOT_RUNNING);

		if (atomic_read(&csg_slot[slot].state) == CSG_SLOT_READY2RUN) {
			dev_dbg(kbdev->dev, "Stopping a queue on csi %d when Group-%d is in under transition to running state",
				queue->csi_index, group->handle);
			retry_needed = true;
		}
	}
retry:
	/* First wait for the group to reach a stable state. IDLE state is
	 * an intermediate state that is only set by Scheduler at the start
	 * of a tick (prior to scanout) for groups that received idle
	 * notification, then later the idle group is moved to one of the
	 * suspended states or the runnable state.
	 */
	while (group->run_state == KBASE_CSF_GROUP_IDLE) {
		mutex_unlock(&scheduler->lock);
		remaining = wait_event_timeout(kbdev->csf.event_wait,
				group->run_state != KBASE_CSF_GROUP_IDLE,
				CSF_STATE_WAIT_TIMEOUT_MS);
		mutex_lock(&scheduler->lock);
		if (!remaining) {
			dev_warn(kbdev->dev,
				 "Timed out waiting for state change of Group-%d when stopping a queue on csi %d",
				 group->handle, queue->csi_index);
		}
	}

	WARN_ON(group->run_state == KBASE_CSF_GROUP_IDLE);
	if (group->run_state == KBASE_CSF_GROUP_SUSPENDED_ON_WAIT_SYNC) {
		remove_group_from_idle_wait(group);
		insert_group_to_runnable(scheduler, group,
				KBASE_CSF_GROUP_SUSPENDED);
	} else if (group->run_state == KBASE_CSF_GROUP_SUSPENDED_ON_IDLE) {
		group->run_state = KBASE_CSF_GROUP_SUSPENDED;
	}

	mutex_unlock(&scheduler->lock);

	/* This function is called when the queue group is either not on a CSG
	 * slot or is on the slot but undergoing transition.
	 *
	 * To stop the queue, the function needs to wait either for the queue
	 * group to be assigned a CSG slot (and that slot has to reach the
	 * running state) or for the eviction of the queue group from the
	 * scheduler's list.
	 *
	 * In order to evaluate the latter condition, the function doesn't
	 * really need to lock the scheduler, as any update to the run_state
	 * of the queue group by sched_evict_group() would be visible due
	 * to implicit barriers provided by the kernel waitqueue macros.
	 *
	 * The group pointer cannot disappear meanwhile, as the high level
	 * CSF context is locked. Therefore, the scheduler would be
	 * the only one to update the run_state of the group.
	 */
	remaining = wait_event_timeout(kbdev->csf.event_wait,
		can_halt_stream(kbdev, group),
		kbase_csf_timeout_in_jiffies(20 * CSF_SCHEDULER_TIME_TICK_MS));

	mutex_lock(&scheduler->lock);

	if (remaining && queue_group_scheduled_locked(group)) {
		slot = kbase_csf_scheduler_group_get_slot(group);

		/* If the group is still on slot and slot is in running state
		 * then explicitly stop the command stream interface of the
		 * queue. Otherwise there are different cases to consider
		 *
		 * - If the queue group was already undergoing transition to
		 *   resume/start state when this function was entered then it
		 *   would not have disabled the command stream interface of the
		 *   queue being stopped and the previous wait would have ended
		 *   once the slot was in a running state with command stream
		 *   interface still enabled.
		 *   Now the group is going through another transition either
		 *   to a suspend state or to a resume state (it could have
		 *   been suspended before the scheduler lock was grabbed).
		 *   In both scenarios need to wait again for the group to
		 *   come on a slot and that slot to reach the running state,
		 *   as that would guarantee that firmware will observe the
		 *   command stream interface as disabled.
		 *
		 * - If the queue group was either off the slot or was
		 *   undergoing transition to suspend state on entering this
		 *   function, then the group would have been resumed with the
		 *   queue's command stream interface in disabled state.
		 *   So now if the group is undergoing another transition
		 *   (after the resume) then just need to wait for the state
		 *   bits in the ACK register of command stream interface to be
		 *   set to STOP value. It is expected that firmware will
		 *   process the stop/disable request of the command stream
		 *   interface after resuming the group before it processes
		 *   another state change request of the group.
		 */
		if ((slot >= 0) &&
		    (atomic_read(&csg_slot[slot].state) == CSG_SLOT_RUNNING)) {
			err = halt_stream_sync(queue);
		} else if (retry_needed && !retried) {
			retried = true;
			goto retry;
		} else if (slot >= 0) {
			struct kbase_csf_global_iface *global_iface =
					&kbdev->csf.global_iface;
			struct kbase_csf_cmd_stream_group_info *ginfo =
					&global_iface->groups[slot];
			struct kbase_csf_cmd_stream_info *stream =
					&ginfo->streams[queue->csi_index];
			u32 cs_req =
			    kbase_csf_firmware_cs_input_read(stream, CS_REQ);

			if (!WARN_ON(CS_REQ_STATE_GET(cs_req) !=
				     CS_REQ_STATE_STOP)) {
				/* Timed wait */
				remaining = wait_event_timeout(
					kbdev->csf.event_wait,
					(CS_ACK_STATE_GET(kbase_csf_firmware_cs_output(stream, CS_ACK))
					== CS_ACK_STATE_STOP),
					CSF_STATE_WAIT_TIMEOUT_MS);

				if (!remaining) {
					dev_warn(kbdev->dev,
						 "Timed out waiting for queue stop ack on csi %d bound to group %d on slot %d",
						 queue->csi_index,
						 group->handle, group->csg_nr);
					err = -ETIMEDOUT;
				}
			}
		}
	} else if (!remaining) {
		dev_warn(kbdev->dev, "Group-%d failed to get a slot for stopping the queue on csi %d",
			 group->handle, queue->csi_index);
		err = -ETIMEDOUT;
	}

	return err;
}

int kbase_csf_scheduler_queue_stop(struct kbase_queue *queue)
{
	struct kbase_device *kbdev = queue->kctx->kbdev;
	struct kbase_queue_group *group = queue->group;
	bool const cs_enabled = queue->enabled;
	int err = 0;

	if (WARN_ON(!group))
		return -EINVAL;

	lockdep_assert_held(&queue->kctx->csf.lock);
	mutex_lock(&kbdev->csf.scheduler.lock);

	queue->enabled = false;
	if (cs_enabled && queue_group_scheduled_locked(group)) {
		struct kbase_csf_csg_slot *const csg_slot =
			kbdev->csf.scheduler.csg_slots;
		int slot = kbase_csf_scheduler_group_get_slot(group);

		if ((slot >= 0) &&
		    (atomic_read(&csg_slot[slot].state) == CSG_SLOT_RUNNING))
			err = halt_stream_sync(queue);
		else
			err = sched_halt_stream(queue);

		unassign_user_doorbell_from_queue(kbdev, queue);
	}

	mutex_unlock(&kbdev->csf.scheduler.lock);
	return err;
}

static void program_cs_extract_init(struct kbase_queue *queue)
{
	u64 *input_addr = (u64 *)queue->user_io_addr;
	u64 *output_addr = (u64 *)(queue->user_io_addr + PAGE_SIZE);

	input_addr[CS_EXTRACT_INIT_LO / sizeof(u64)] =
			output_addr[CS_EXTRACT_LO / sizeof(u64)];
}

static void program_cs(struct kbase_device *kbdev,
		struct kbase_queue *queue)
{
	struct kbase_queue_group *group = queue->group;
	struct kbase_csf_cmd_stream_group_info *ginfo;
	struct kbase_csf_cmd_stream_info *stream;
	u64 user_input;
	u64 user_output;

	if (WARN_ON(!group))
		return;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	if (WARN_ON(!kbasep_csf_scheduler_group_is_on_slot_locked(group)))
		return;

	ginfo = &kbdev->csf.global_iface.groups[group->csg_nr];

	if (WARN_ON(queue->csi_index < 0) ||
	    WARN_ON(queue->csi_index >= ginfo->stream_num))
		return;

	if (kbase_reset_gpu_is_active(kbdev))
		return;

	assign_user_doorbell_to_queue(kbdev, queue);
	if (queue->doorbell_nr == KBASEP_USER_DB_NR_INVALID)
		return;

	if (queue->enabled && queue_group_suspended_locked(group))
		program_cs_extract_init(queue);

	stream = &ginfo->streams[queue->csi_index];

	kbase_csf_firmware_cs_input(stream, CS_BASE_LO,
				    queue->base_addr & 0xFFFFFFFF);
	kbase_csf_firmware_cs_input(stream, CS_BASE_HI,
				    queue->base_addr >> 32);
	kbase_csf_firmware_cs_input(stream, CS_SIZE,
				    queue->size);

	user_input = (queue->reg->start_pfn << PAGE_SHIFT);
	kbase_csf_firmware_cs_input(stream, CS_USER_INPUT_LO,
				    user_input & 0xFFFFFFFF);
	kbase_csf_firmware_cs_input(stream, CS_USER_INPUT_HI,
				    user_input >> 32);

	user_output = ((queue->reg->start_pfn + 1) << PAGE_SHIFT);
	kbase_csf_firmware_cs_input(stream, CS_USER_OUTPUT_LO,
				    user_output & 0xFFFFFFFF);
	kbase_csf_firmware_cs_input(stream, CS_USER_OUTPUT_HI,
				    user_output >> 32);

	kbase_csf_firmware_cs_input(stream, CS_CONFIG,
		(queue->doorbell_nr << 8) | (queue->priority & 0xF));

	/* Enable all interrupts for now */
	kbase_csf_firmware_cs_input(stream, CS_ACK_IRQ_MASK, ~((u32)0));

	/*
	 * Enable the CSG idle notification once the stream's ringbuffer
	 * becomes empty or the stream becomes sync_idle, waiting sync update
	 * or protected mode switch.
	 */
	kbase_csf_firmware_cs_input_mask(stream, CS_REQ,
			CS_REQ_IDLE_EMPTY_MASK | CS_REQ_IDLE_SYNC_WAIT_MASK |
			CS_REQ_IDLE_PROTM_PEND_MASK,
			CS_REQ_IDLE_EMPTY_MASK | CS_REQ_IDLE_SYNC_WAIT_MASK |
			CS_REQ_IDLE_PROTM_PEND_MASK);

	/* Set state to START/STOP */
	kbase_csf_firmware_cs_input_mask(stream, CS_REQ,
		queue->enabled ? CS_REQ_STATE_START : CS_REQ_STATE_STOP,
		CS_REQ_STATE_MASK);

	kbase_csf_ring_cs_kernel_doorbell(kbdev, queue);
}

int kbase_csf_scheduler_queue_start(struct kbase_queue *queue)
{
	struct kbase_queue_group *group = queue->group;
	struct kbase_device *kbdev = queue->kctx->kbdev;
	bool const cs_enabled = queue->enabled;
	int err = 0;
	bool evicted = false;

	lockdep_assert_held(&queue->kctx->csf.lock);

	if (WARN_ON(!group || queue->bind_state != KBASE_CSF_QUEUE_BOUND))
		return -EINVAL;

	mutex_lock(&kbdev->csf.scheduler.lock);
	if (group->run_state == KBASE_CSF_GROUP_FAULT_EVICTED) {
		err = -EIO;
		evicted = true;
	} else if ((group->run_state == KBASE_CSF_GROUP_SUSPENDED_ON_WAIT_SYNC)
		   && CS_STATUS_WAIT_SYNC_WAIT_GET(queue->status_wait)) {
		dev_dbg(kbdev->dev, "blocked queue(csi_index=%d) of group %d was kicked",
			queue->csi_index, group->handle);
	} else {
		err = scheduler_group_schedule(group);

		if (!err) {
			queue->enabled = true;
			if (kbasep_csf_scheduler_group_is_on_slot_locked(group)) {
				if (cs_enabled) {
					/* In normal situation, when a queue is
					 * already running, the queue update
					 * would be a doorbell kick on user
					 * side. However, if such a kick is
					 * shortly following a start or resume,
					 * the queue may actually in transition
					 * hence the said kick would enter the
					 * kernel as the hw_active flag is yet
					 * to be set. The sheduler needs to
					 * give a kick to the corresponding
					 * user door-bell on such a case.
					 */
					kbase_csf_ring_cs_user_doorbell(kbdev, queue);
				} else
					program_cs(kbdev, queue);
			}
			queue_delayed_work(system_long_wq,
				&kbdev->csf.scheduler.ping_work,
				msecs_to_jiffies(FIRMWARE_PING_INTERVAL_MS));
		}
	}

	mutex_unlock(&kbdev->csf.scheduler.lock);

	if (evicted)
		kbase_csf_term_descheduled_queue_group(group);

	return err;
}

static enum kbase_csf_csg_slot_state update_csg_slot_status(
				struct kbase_device *kbdev, s8 slot)
{
	struct kbase_csf_csg_slot *csg_slot =
		&kbdev->csf.scheduler.csg_slots[slot];
	struct kbase_csf_cmd_stream_group_info *ginfo =
		&kbdev->csf.global_iface.groups[slot];
	u32 state;
	enum kbase_csf_csg_slot_state slot_state;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	state = CSG_ACK_STATE_GET(kbase_csf_firmware_csg_output(ginfo,
			CSG_ACK));
	slot_state = atomic_read(&csg_slot->state);

	switch (slot_state) {
	case CSG_SLOT_READY2RUN:
		if ((state == CSG_ACK_STATE_START) ||
		    (state == CSG_ACK_STATE_RESUME)) {
			slot_state = CSG_SLOT_RUNNING;
			atomic_set(&csg_slot->state, slot_state);
			csg_slot->trigger_jiffies = jiffies;
			dev_dbg(kbdev->dev, "Group %u running on slot %d\n",
				csg_slot->resident_group->handle, slot);
		}
		break;
	case CSG_SLOT_DOWN2STOP:
		if ((state == CSG_ACK_STATE_SUSPEND) ||
		    (state == CSG_ACK_STATE_TERMINATE)) {
			slot_state = CSG_SLOT_STOPPED;
			atomic_set(&csg_slot->state, slot_state);
			csg_slot->trigger_jiffies = jiffies;
			dev_dbg(kbdev->dev, "Group %u stopped on slot %d\n",
				csg_slot->resident_group->handle, slot);
		}
		break;
	case CSG_SLOT_DOWN2STOP_TIMEDOUT:
	case CSG_SLOT_READY2RUN_TIMEDOUT:
	case CSG_SLOT_READY:
	case CSG_SLOT_RUNNING:
	case CSG_SLOT_STOPPED:
		break;
	default:
		dev_warn(kbdev->dev, "Unknown CSG slot state %d", slot_state);
		break;
	}

	return slot_state;
}

static bool csg_slot_running(struct kbase_device *kbdev, s8 slot)
{
	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	return (update_csg_slot_status(kbdev, slot) == CSG_SLOT_RUNNING);
}

static bool csg_slot_stopped_locked(struct kbase_device *kbdev, s8 slot)
{
	enum kbase_csf_csg_slot_state slot_state;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	slot_state = update_csg_slot_status(kbdev, slot);

	return (slot_state == CSG_SLOT_STOPPED ||
		slot_state == CSG_SLOT_READY);
}

static bool csg_slot_stopped_raw(struct kbase_device *kbdev, s8 slot)
{
	struct kbase_csf_cmd_stream_group_info *ginfo =
		&kbdev->csf.global_iface.groups[slot];
	u32 state;

	state = CSG_ACK_STATE_GET(kbase_csf_firmware_csg_output(ginfo,
			CSG_ACK));

	if (state == CSG_ACK_STATE_SUSPEND || state == CSG_ACK_STATE_TERMINATE) {
		dev_dbg(kbdev->dev, "(raw status) slot %d stopped\n", slot);
		return true;
	}

	return false;
}

static void halt_csg_slot(struct kbase_queue_group *group, bool suspend)
{
	struct kbase_device *kbdev = group->kctx->kbdev;
	struct kbase_csf_global_iface *global_iface = &kbdev->csf.global_iface;
	struct kbase_csf_csg_slot *csg_slot =
		kbdev->csf.scheduler.csg_slots;
	s8 slot;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	if (WARN_ON(!kbasep_csf_scheduler_group_is_on_slot_locked(group)))
		return;

	slot = group->csg_nr;

	/* When in transition, wait for it to complete */
	if (atomic_read(&csg_slot[slot].state) == CSG_SLOT_READY2RUN) {
		long remaining =
		      kbase_csf_timeout_in_jiffies(CSF_STATE_WAIT_TIMEOUT_MS);

		dev_dbg(kbdev->dev, "slot %d wait for up-running\n", slot);
		remaining = wait_event_timeout(kbdev->csf.event_wait,
				csg_slot_running(kbdev, slot), remaining);
		if (!remaining)
			dev_warn(kbdev->dev,
				 "slot %d timed out on up-running\n", slot);
	}

	if (csg_slot_running(kbdev, slot)) {
		unsigned long flags;
		struct kbase_csf_cmd_stream_group_info *ginfo =
						&global_iface->groups[slot];
		u32 halt_cmd = suspend ? CSG_REQ_STATE_SUSPEND :
					 CSG_REQ_STATE_TERMINATE;

		dev_dbg(kbdev->dev, "Halting(suspend=%d) group %d of context %d_%d on slot %d",
			suspend, group->handle, group->kctx->tgid, group->kctx->id, slot);

		spin_lock_irqsave(&kbdev->csf.scheduler.interrupt_lock, flags);
		/* Set state to SUSPEND/TERMINATE */
		kbase_csf_firmware_csg_input_mask(ginfo, CSG_REQ, halt_cmd,
						  CSG_REQ_STATE_MASK);
		spin_unlock_irqrestore(&kbdev->csf.scheduler.interrupt_lock,
					flags);
		atomic_set(&csg_slot[slot].state, CSG_SLOT_DOWN2STOP);
		csg_slot[slot].trigger_jiffies = jiffies;
		kbase_csf_ring_csg_doorbell(kbdev, slot);
	}
}

static void term_csg_slot(struct kbase_queue_group *group)
{
	halt_csg_slot(group, false);
}

static void suspend_csg_slot(struct kbase_queue_group *group)
{
	halt_csg_slot(group, true);
}

/**
 * evaluate_sync_update() - Evaluate the sync wait condition the GPU command
 *                          queue has been blocked on.
 *
 * @queue: Pointer to the GPU command queue
 *
 * Return: true if sync wait condition is satisfied.
 */
static bool evaluate_sync_update(struct kbase_queue *queue)
{
	enum kbase_csf_group_state run_state;
	struct kbase_vmap_struct *mapping;
	bool updated = false;
	u32 *sync_ptr;
	u32 sync_wait_cond;

	if (WARN_ON(!queue))
		return false;

	run_state = queue->group->run_state;

	if (WARN_ON((run_state != KBASE_CSF_GROUP_IDLE) &&
		    (run_state != KBASE_CSF_GROUP_SUSPENDED_ON_WAIT_SYNC)))
		return false;

	lockdep_assert_held(&queue->kctx->kbdev->csf.scheduler.lock);

	sync_ptr = kbase_phy_alloc_mapping_get(queue->kctx, queue->sync_ptr,
					&mapping);

	if (!sync_ptr) {
		dev_dbg(queue->kctx->kbdev->dev, "sync memory VA 0x%016llX already freed",
			queue->sync_ptr);
		return false;
	}

	sync_wait_cond =
		CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(queue->status_wait);

	WARN_ON((sync_wait_cond != CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GT) &&
		(sync_wait_cond != CS_STATUS_WAIT_SYNC_WAIT_CONDITION_LE));

	if (((sync_wait_cond == CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GT) &&
	     (*sync_ptr > queue->sync_value)) ||
	    ((sync_wait_cond == CS_STATUS_WAIT_SYNC_WAIT_CONDITION_LE) &&
	     (*sync_ptr <= queue->sync_value))) {
		/* The sync wait condition is satisfied so the group to which
		 * queue is bound can be re-scheduled.
		 */
		updated = true;
	} else {
		dev_dbg(queue->kctx->kbdev->dev, "sync memory not updated yet(%u)",
			*sync_ptr);
	}

	kbase_phy_alloc_mapping_put(queue->kctx, mapping);

	return updated;
}

/**
 * save_slot_cs() -  Save the state for blocked GPU command queue.
 *
 * @ginfo: Pointer to the command stream group interface used by the group
 *         the queue is bound to.
 * @queue: Pointer to the GPU command queue.
 *
 * This function will check if GPU command queue is blocked on a sync wait and
 * evaluate the wait condition. If the wait condition isn't satisfied it would
 * save the state needed to reevaluate the condition in future.
 * The group to which queue is bound shall be in idle state.
 *
 * Return: true if the queue is blocked on a sync wait operation.
 */
static
bool save_slot_cs(struct kbase_csf_cmd_stream_group_info const *const ginfo,
		struct kbase_queue *queue)
{
	struct kbase_csf_cmd_stream_info *const stream =
		&ginfo->streams[queue->csi_index];
	u32 status = kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT);
	bool is_waiting = false;

	WARN_ON(queue->group->run_state != KBASE_CSF_GROUP_IDLE);

	if (CS_STATUS_WAIT_SYNC_WAIT_GET(status)) {
		queue->status_wait = status;
		queue->sync_ptr = kbase_csf_firmware_cs_output(stream,
			CS_STATUS_WAIT_SYNC_POINTER_LO);
		queue->sync_ptr |= (u64)kbase_csf_firmware_cs_output(stream,
			CS_STATUS_WAIT_SYNC_POINTER_HI) << 32;
		queue->sync_value = kbase_csf_firmware_cs_output(stream,
			CS_STATUS_WAIT_SYNC_VALUE);

		is_waiting = true;
	} else {
		/* Invalidate wait status info that would have been recorded if
		 * this queue was blocked when the group (in idle state) was
		 * suspended previously. After that the group could have been
		 * unblocked due to the kicking of another queue bound to it &
		 * so the wait status info would have stuck with this queue.
		 */
		queue->status_wait = 0;
	}

	return is_waiting;
}

/**
 * Calculate how far in the future an event should be scheduled.
 *
 * The objective of this function is making sure that a minimum period of
 * time is guaranteed between handling two consecutive events.
 *
 * This function guarantees a minimum period of time between two consecutive
 * events: given the minimum period and the distance between the current time
 * and the last event, the function returns the difference between the two.
 * However, if more time than the minimum period has already elapsed
 * since the last event, the function will return 0 to schedule work to handle
 * the event with the lowest latency possible.
 *
 * @last_event: Timestamp of the last event, in jiffies.
 * @time_now:   Timestamp of the new event to handle, in jiffies.
 *              Must be successive to last_event.
 * @period:     Minimum period between two events, in jiffies.
 *
 * Return:      Time to delay work to handle the current event, in jiffies
 */
static unsigned long get_schedule_delay(unsigned long last_event,
					unsigned long time_now,
					unsigned long period)
{
	const unsigned long t_distance = time_now - last_event;
	const unsigned long delay_t = (t_distance < period) ?
					(period - t_distance) : 0;

	return delay_t;
}

static void schedule_in_cycle(struct kbase_queue_group *group)
{
	struct kbase_context *kctx = group->kctx;
	struct kbase_device *kbdev = kctx->kbdev;
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;

	/* Return if the GPU reset is ongoing as Scheduler would also be reset.
	 * At the end of reset the Scheduler would be kicked and the newly
	 * runnable group would then get considered for scheduling.
	 */
	if (kbase_reset_gpu_is_active(kbdev))
		return;

	lockdep_assert_held(&scheduler->lock);

	/* Only try to schedule work for this event if no requests are pending,
	 * otherwise the function will end up canceling previous work requests.
	 */
	if (likely(kbase_csf_scheduler_timer_is_enabled(kbdev)) &&
			!scheduler->tock_pending_request) {
		const unsigned long delay =
			get_schedule_delay(scheduler->last_schedule, jiffies,
					   CSF_SCHEDULER_TIME_TOCK_JIFFIES);
		scheduler->tock_pending_request = true;
		dev_dbg(kbdev->dev, "Kicking async for group %d\n",
			group->handle);
		mod_delayed_work(scheduler->wq, &scheduler->tock_work, delay);
	}
}

static
void insert_group_to_runnable(struct kbase_csf_scheduler *const scheduler,
		struct kbase_queue_group *const group,
		enum kbase_csf_group_state run_state)
{
	struct kbase_context *const kctx = group->kctx;
	struct kbase_device *const kbdev = kctx->kbdev;

	lockdep_assert_held(&scheduler->lock);

	WARN_ON(group->run_state != KBASE_CSF_GROUP_INACTIVE);

	if (WARN_ON(group->priority >= BASE_QUEUE_GROUP_PRIORITY_COUNT))
		return;

	group->run_state = run_state;

	if (run_state == KBASE_CSF_GROUP_RUNNABLE)
		group->prepared_seq_num = KBASEP_GROUP_PREPARED_SEQ_NUM_INVALID;

	list_add_tail(&group->link,
			&kctx->csf.sched.runnable_groups[group->priority]);
	kctx->csf.sched.num_runnable_grps++;
	/* Add the kctx if not yet in runnable kctxs */
	if (kctx->csf.sched.num_runnable_grps == 1) {
		/* First runnable csg, adds to the runnable_kctxs */
		INIT_LIST_HEAD(&kctx->csf.link);
		list_add_tail(&kctx->csf.link, &scheduler->runnable_kctxs);
	}

	scheduler->total_runnable_grps++;

	if (likely(kbase_csf_scheduler_timer_is_enabled(kbdev)) &&
			(scheduler->total_runnable_grps == 1)) {
		dev_dbg(kbdev->dev, "Kicking scheduler on first runnable group\n");
		/* Fire a scheduling to start the time-slice */
		mod_delayed_work(kbdev->csf.scheduler.wq,
				 &kbdev->csf.scheduler.tick_work, 0);
	} else
		schedule_in_cycle(group);
}

static
void remove_group_from_runnable(struct kbase_csf_scheduler *const scheduler,
		struct kbase_queue_group *const group,
		enum kbase_csf_group_state run_state)
{
	struct kbase_context *kctx = group->kctx;

	lockdep_assert_held(&scheduler->lock);

	WARN_ON(!queue_group_scheduled_locked(group));

	group->run_state = run_state;
	list_del_init(&group->link);

	if (scheduler->top_grp == group) {
		/*
		 * Note: this disables explicit rotation in the next scheduling
		 * cycle. However, removing the top_grp is the same as an
		 * implicit rotation (e.g. if we instead rotated the top_ctx
		 * and then remove top_grp)
		 *
		 * This implicit rotation is assumed by the scheduler rotate
		 * functions.
		 */
		scheduler->top_grp = NULL;
	}

	kctx->csf.sched.num_runnable_grps--;
	if (kctx->csf.sched.num_runnable_grps == 0) {
		/* drop the kctx */
		list_del_init(&kctx->csf.link);
		if (scheduler->top_ctx == kctx)
			scheduler->top_ctx = NULL;
	}

	WARN_ON(scheduler->total_runnable_grps == 0);
	scheduler->total_runnable_grps--;
}

static void insert_group_to_idle_wait(struct kbase_queue_group *const group)
{
	struct kbase_context *kctx = group->kctx;

	lockdep_assert_held(&kctx->kbdev->csf.scheduler.lock);

	WARN_ON(group->run_state != KBASE_CSF_GROUP_IDLE);

	list_add_tail(&group->link, &kctx->csf.sched.idle_wait_groups);
	kctx->csf.sched.num_idle_wait_grps++;
	group->run_state = KBASE_CSF_GROUP_SUSPENDED_ON_WAIT_SYNC;
	dev_dbg(kctx->kbdev->dev,
		"Group-%d suspended on sync_wait, total wait_groups: %u\n",
		group->handle, kctx->csf.sched.num_idle_wait_grps);
}

static void remove_group_from_idle_wait(struct kbase_queue_group *const group)
{
	struct kbase_context *kctx = group->kctx;

	lockdep_assert_held(&kctx->kbdev->csf.scheduler.lock);

	WARN_ON(group->run_state != KBASE_CSF_GROUP_SUSPENDED_ON_WAIT_SYNC);

	list_del_init(&group->link);
	WARN_ON(kctx->csf.sched.num_idle_wait_grps == 0);
	kctx->csf.sched.num_idle_wait_grps--;
	group->run_state = KBASE_CSF_GROUP_INACTIVE;
}

static void deschedule_idle_wait_group(struct kbase_csf_scheduler *scheduler,
		struct kbase_queue_group *group)
{
	lockdep_assert_held(&scheduler->lock);

	if (WARN_ON(!group))
		return;

	remove_group_from_runnable(scheduler, group, KBASE_CSF_GROUP_IDLE);
	insert_group_to_idle_wait(group);
}

static bool confirm_cs_idle(struct kbase_queue *queue)
{
	u64 *input_addr = (u64 *)queue->user_io_addr;
	u64 *output_addr = (u64 *)(queue->user_io_addr + PAGE_SIZE);

	return (input_addr[CS_INSERT_LO / sizeof(u64)] ==
		output_addr[CS_EXTRACT_LO / sizeof(u64)]);
}

static void save_csg_slot(struct kbase_queue_group *group)
{
	struct kbase_device *kbdev = group->kctx->kbdev;
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	struct kbase_csf_cmd_stream_group_info *ginfo;
	u32 state;

	lockdep_assert_held(&scheduler->lock);

	if (WARN_ON(!kbasep_csf_scheduler_group_is_on_slot_locked(group)))
		return;

	ginfo = &kbdev->csf.global_iface.groups[group->csg_nr];

	state =
	    CSG_ACK_STATE_GET(kbase_csf_firmware_csg_output(ginfo, CSG_ACK));

	if (!WARN_ON(state != CSG_ACK_STATE_SUSPEND)) {
		if (group->run_state == KBASE_CSF_GROUP_IDLE) {
			bool sync_wait = false;
			bool idle = true;
			int i;

			/* Loop through all bound CSs & save their context */
			for (i = 0; i < MAX_SUPPORTED_STREAMS_PER_GROUP; i++) {
				struct kbase_queue *const queue =
					group->bound_queues[i];

				if (queue && queue->enabled) {
					if (save_slot_cs(ginfo, queue))
						sync_wait = true;
					else if (idle)
						idle = confirm_cs_idle(queue);
				}
			}

			/* Take the suspended group out of the runnable_groups
			 * list of the context and move it to the
			 * idle_wait_groups list.
			 */
			if (sync_wait && idle)
				deschedule_idle_wait_group(scheduler, group);
			else if (idle) {
				group->run_state =
					KBASE_CSF_GROUP_SUSPENDED_ON_IDLE;
				dev_dbg(kbdev->dev, "Group-%d suspended: idle\n",
					group->handle);
			} else
				group->run_state = KBASE_CSF_GROUP_SUSPENDED;
		} else {
			group->run_state = KBASE_CSF_GROUP_SUSPENDED;
		}
	}
}

/* Cleanup_csg_slot after it has been vacated, ready for next csg run.
 * Return whether there is a kctx address fault associated with the group
 * for which the clean-up is done.
 */
static bool cleanup_csg_slot(struct kbase_queue_group *group)
{
	struct kbase_context *kctx = group->kctx;
	struct kbase_device *kbdev = kctx->kbdev;
	struct kbase_csf_global_iface *global_iface = &kbdev->csf.global_iface;
	struct kbase_csf_cmd_stream_group_info *ginfo;
	s8 slot;
	struct kbase_csf_csg_slot *csg_slot;
	unsigned long flags;
	u32 i;
	bool as_fault = false;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	if (WARN_ON(!kbasep_csf_scheduler_group_is_on_slot_locked(group)))
		return as_fault;

	slot = group->csg_nr;
	csg_slot = &kbdev->csf.scheduler.csg_slots[slot];
	ginfo = &global_iface->groups[slot];

	/* Now loop through all the bound CSs, and clean them via a stop */
	for (i = 0; i < ginfo->stream_num; i++) {
		struct kbase_csf_cmd_stream_info *stream = &ginfo->streams[i];

		if (group->bound_queues[i] && group->bound_queues[i]->enabled) {
			kbase_csf_firmware_cs_input_mask(stream, CS_REQ,
				CS_REQ_STATE_STOP, CS_REQ_STATE_MASK);

			unassign_user_doorbell_from_queue(kbdev, group->bound_queues[i]);
		}
	}

	unassign_user_doorbell_from_group(kbdev, group);

	/* The csg does not need cleanup other than drop its AS */
	spin_lock_irqsave(&kctx->kbdev->hwaccess_lock, flags);
	as_fault = kbase_ctx_flag(kctx, KCTX_AS_DISABLED_ON_FAULT);
	kbase_ctx_sched_release_ctx(kctx);
	spin_unlock_irqrestore(&kctx->kbdev->hwaccess_lock, flags);

	/* now marking the slot is vacant */
	spin_lock_irqsave(&kbdev->csf.scheduler.interrupt_lock, flags);
	kbdev->csf.scheduler.csg_slots[slot].resident_group = NULL;
	group->csg_nr = KBASEP_CSG_NR_INVALID;
	bitmap_clear(kbdev->csf.scheduler.csg_slots_idle_mask, slot, 1);
	kbdev->csf.scheduler.csgs_events_enable_mask |= BIT(slot);
	spin_unlock_irqrestore(&kbdev->csf.scheduler.interrupt_lock, flags);

	bitmap_clear(kbdev->csf.scheduler.csg_inuse_bitmap, slot, 1);

	csg_slot->trigger_jiffies = jiffies;
	atomic_set(&csg_slot->state, CSG_SLOT_READY);

	dev_dbg(kbdev->dev, "Cleanup done for group %d on slot %d\n",
		group->handle, slot);

	KBASE_TLSTREAM_TL_KBASE_DEVICE_DEPROGRAM_CSG(kbdev,
		kbdev->gpu_props.props.raw_props.gpu_id, slot);

	return as_fault;
}

static void update_csg_slot_priority(struct kbase_queue_group *group, u8 prio)
{
	struct kbase_device *kbdev = group->kctx->kbdev;
	struct kbase_csf_csg_slot *csg_slot;
	struct kbase_csf_cmd_stream_group_info *ginfo;
	s8 slot;
	u32 ep_cfg;
	u32 csg_req;
	unsigned long flags;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	if (WARN_ON(!kbasep_csf_scheduler_group_is_on_slot_locked(group)))
		return;

	slot = group->csg_nr;
	csg_slot = &kbdev->csf.scheduler.csg_slots[slot];
	ginfo = &kbdev->csf.global_iface.groups[slot];

	WARN_ON(!((group->run_state == KBASE_CSF_GROUP_RUNNABLE) ||
		(group->run_state == KBASE_CSF_GROUP_IDLE)));
	group->run_state = KBASE_CSF_GROUP_RUNNABLE;

	if (csg_slot->priority == prio)
		return;

	/* Read the csg_ep_cfg back for updating the priority field */
	ep_cfg = kbase_csf_firmware_csg_input_read(ginfo, CSG_EP_REQ);
	ep_cfg = CSG_EP_REQ_PRIORITY_SET(ep_cfg, prio);
	kbase_csf_firmware_csg_input(ginfo, CSG_EP_REQ, ep_cfg);

	spin_lock_irqsave(&kbdev->csf.scheduler.interrupt_lock, flags);
	csg_req = kbase_csf_firmware_csg_output(ginfo, CSG_ACK);
	csg_req ^= CSG_REQ_EP_CFG;
	kbase_csf_firmware_csg_input_mask(ginfo, CSG_REQ, csg_req,
					  CSG_REQ_EP_CFG);
	spin_unlock_irqrestore(&kbdev->csf.scheduler.interrupt_lock, flags);

	csg_slot->priority = prio;

	dev_dbg(kbdev->dev, "Priority for group %d of context %d_%d on slot %d to be updated to %u\n",
		group->handle, group->kctx->tgid, group->kctx->id, slot, prio);

	kbase_csf_ring_csg_doorbell(kbdev, slot);
}

static void program_csg_slot(struct kbase_queue_group *group, s8 slot,
		u8 prio)
{
	struct kbase_context *kctx = group->kctx;
	struct kbase_device *kbdev = kctx->kbdev;
	struct kbase_csf_global_iface *global_iface = &kbdev->csf.global_iface;
	const u64 shader_core_mask =
		kbase_pm_get_present_cores(kbdev, KBASE_PM_CORE_SHADER);
	const u64 tiler_core_mask =
		kbase_pm_get_present_cores(kbdev, KBASE_PM_CORE_TILER);
	const u64 compute_mask = shader_core_mask & group->compute_mask;
	const u64 fragment_mask = shader_core_mask & group->fragment_mask;
	const u64 tiler_mask = tiler_core_mask & group->tiler_mask;
	const u8 num_cores = kbdev->gpu_props.num_cores;
	const u8 compute_max = min(num_cores, group->compute_max);
	const u8 fragment_max = min(num_cores, group->fragment_max);
	const u8 tiler_max = min(CSG_TILER_MAX, group->tiler_max);
	struct kbase_csf_cmd_stream_group_info *ginfo;
	u32 ep_cfg = 0;
	u32 csg_req;
	u32 state;
	int i;
	unsigned long flags;
	const u64 normal_suspend_buf =
		group->normal_suspend_buf.reg->start_pfn << PAGE_SHIFT;
	struct kbase_csf_csg_slot *csg_slot =
		&kbdev->csf.scheduler.csg_slots[slot];

	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	if (WARN_ON(slot < 0) &&
	    WARN_ON(slot >= global_iface->group_num))
		return;

	if (kbase_reset_gpu_is_active(kbdev))
		return;

	WARN_ON(atomic_read(&csg_slot->state) != CSG_SLOT_READY);

	ginfo = &global_iface->groups[slot];

	/* Pick an available address space for this context */
	mutex_lock(&kbdev->mmu_hw_mutex);
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_ctx_sched_retain_ctx(kctx);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	mutex_unlock(&kbdev->mmu_hw_mutex);

	if (kctx->as_nr == KBASEP_AS_NR_INVALID) {
		dev_dbg(kbdev->dev, "Could not get a valid AS for group %d of context %d_%d on slot %d\n",
			group->handle, kctx->tgid, kctx->id, slot);
		return;
	}

	bitmap_set(kbdev->csf.scheduler.csg_inuse_bitmap, slot, 1);
	spin_lock_irqsave(&kbdev->csf.scheduler.interrupt_lock, flags);
	kbdev->csf.scheduler.csg_slots[slot].resident_group = group;
	group->csg_nr = slot;
	spin_unlock_irqrestore(&kbdev->csf.scheduler.interrupt_lock, flags);

	assign_user_doorbell_to_group(kbdev, group);

	/* Clear any stale protm info as the group is about to start on-slot.
	 * Note the recorded protm_pending info needs to start fresh on each
	 * load/reload of the group on a slot. Once off-slot, the info becomes
	 * stale. So a clean up is required either at the load-stage, as done
	 * here, or, alternatively the unload-stage.
	 */
	bitmap_zero(group->protm_pending_bitmap,
		MAX_SUPPORTED_STREAMS_PER_GROUP);

	/* Now loop through all the bound & kicked CSs, and program them */
	for (i = 0; i < MAX_SUPPORTED_STREAMS_PER_GROUP; i++) {
		struct kbase_queue *queue = group->bound_queues[i];

		if (queue)
			program_cs(kbdev, queue);
	}


	/* Endpoint programming for CSG */
	kbase_csf_firmware_csg_input(ginfo, CSG_ALLOW_COMPUTE_LO,
				     compute_mask & U32_MAX);
	kbase_csf_firmware_csg_input(ginfo, CSG_ALLOW_COMPUTE_HI,
				     compute_mask >> 32);
	kbase_csf_firmware_csg_input(ginfo, CSG_ALLOW_FRAGMENT_LO,
				     fragment_mask & U32_MAX);
	kbase_csf_firmware_csg_input(ginfo, CSG_ALLOW_FRAGMENT_HI,
				     fragment_mask >> 32);
	kbase_csf_firmware_csg_input(ginfo, CSG_ALLOW_OTHER,
				     tiler_mask & U32_MAX);

	ep_cfg = CSG_EP_REQ_COMPUTE_EP_SET(ep_cfg, compute_max);
	ep_cfg = CSG_EP_REQ_FRAGMENT_EP_SET(ep_cfg, fragment_max);
	ep_cfg = CSG_EP_REQ_TILER_EP_SET(ep_cfg, tiler_max);
	ep_cfg = CSG_EP_REQ_PRIORITY_SET(ep_cfg, prio);
	kbase_csf_firmware_csg_input(ginfo, CSG_EP_REQ, ep_cfg);

	/* Program the address space number assigned to the context */
	kbase_csf_firmware_csg_input(ginfo, CSG_CONFIG, kctx->as_nr);

	kbase_csf_firmware_csg_input(ginfo, CSG_SUSPEND_BUF_LO,
			normal_suspend_buf & U32_MAX);
	kbase_csf_firmware_csg_input(ginfo, CSG_SUSPEND_BUF_HI,
			normal_suspend_buf >> 32);

	if (group->protected_suspend_buf.reg) {
		const u64 protm_suspend_buf =
			group->protected_suspend_buf.reg->start_pfn <<
				PAGE_SHIFT;
		kbase_csf_firmware_csg_input(ginfo, CSG_PROTM_SUSPEND_BUF_LO,
			protm_suspend_buf & U32_MAX);
		kbase_csf_firmware_csg_input(ginfo, CSG_PROTM_SUSPEND_BUF_HI,
			protm_suspend_buf >> 32);
	}

	/* Enable all interrupts for now */
	kbase_csf_firmware_csg_input(ginfo, CSG_ACK_IRQ_MASK, ~((u32)0));

	spin_lock_irqsave(&kbdev->csf.scheduler.interrupt_lock, flags);
	csg_req = kbase_csf_firmware_csg_output(ginfo, CSG_ACK);
	csg_req ^= CSG_REQ_EP_CFG;
	kbase_csf_firmware_csg_input_mask(ginfo, CSG_REQ, csg_req,
					  CSG_REQ_EP_CFG);

	/* Set state to START/RESUME */
	if (queue_group_suspended_locked(group)) {
		state = CSG_REQ_STATE_RESUME;
	} else {
		WARN_ON(group->run_state != KBASE_CSF_GROUP_RUNNABLE);
		state = CSG_REQ_STATE_START;
	}

	kbase_csf_firmware_csg_input_mask(ginfo, CSG_REQ,
			state, CSG_REQ_STATE_MASK);
	spin_unlock_irqrestore(&kbdev->csf.scheduler.interrupt_lock, flags);

	/* Update status before rings the door-bell, marking ready => run */
	atomic_set(&csg_slot->state, CSG_SLOT_READY2RUN);
	csg_slot->trigger_jiffies = jiffies;
	csg_slot->priority = prio;

	/* Trace the programming of the CSG on the slot */
	KBASE_TLSTREAM_TL_KBASE_DEVICE_PROGRAM_CSG(kbdev,
		kbdev->gpu_props.props.raw_props.gpu_id, group->handle, slot);

	dev_dbg(kbdev->dev, "Starting group %d of context %d_%d on slot %d with priority %u\n",
		group->handle, kctx->tgid, kctx->id, slot, prio);

	kbase_csf_ring_csg_doorbell(kbdev, slot);
}

static void remove_scheduled_group(struct kbase_device *kbdev,
		struct kbase_queue_group *group)
{
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;

	lockdep_assert_held(&scheduler->lock);

	WARN_ON(group->prepared_seq_num ==
		KBASEP_GROUP_PREPARED_SEQ_NUM_INVALID);
	WARN_ON(list_empty(&group->link_to_schedule));

	list_del_init(&group->link_to_schedule);
	scheduler->ngrp_to_schedule--;
	group->prepared_seq_num = KBASEP_GROUP_PREPARED_SEQ_NUM_INVALID;
	group->kctx->csf.sched.ngrp_to_schedule--;
}

static void sched_evict_group(struct kbase_queue_group *group, bool fault)
{
	struct kbase_context *kctx = group->kctx;
	struct kbase_device *kbdev = kctx->kbdev;
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	if (queue_group_scheduled_locked(group)) {
		u32 i;

		for (i = 0; i < MAX_SUPPORTED_STREAMS_PER_GROUP; i++) {
			if (group->bound_queues[i])
				group->bound_queues[i]->enabled = false;
		}

		if (group->run_state == KBASE_CSF_GROUP_SUSPENDED_ON_WAIT_SYNC)
			remove_group_from_idle_wait(group);
		else {
			if (group->prepared_seq_num !=
					KBASEP_GROUP_PREPARED_SEQ_NUM_INVALID)
				remove_scheduled_group(kbdev, group);

			remove_group_from_runnable(scheduler, group,
						KBASE_CSF_GROUP_INACTIVE);
		}

		WARN_ON(group->run_state != KBASE_CSF_GROUP_INACTIVE);

		if (fault)
			group->run_state = KBASE_CSF_GROUP_FAULT_EVICTED;

		dev_dbg(kbdev->dev, "group %d exited scheduler, num_runnable_grps %d\n",
			group->handle, scheduler->total_runnable_grps);
		/* Notify a group has been evicted */
		wake_up_all(&kbdev->csf.event_wait);
	}
}

static int term_group_sync(struct kbase_queue_group *group)
{
	struct kbase_device *kbdev = group->kctx->kbdev;
	long remaining =
		kbase_csf_timeout_in_jiffies(CSF_STATE_WAIT_TIMEOUT_MS);
	int err = 0;

	term_csg_slot(group);

	remaining = wait_event_timeout(kbdev->csf.event_wait,
		csg_slot_stopped_locked(kbdev, group->csg_nr), remaining);

	if (!remaining) {
		dev_warn(kbdev->dev, "term request timed out for group %d on slot %d",
			 group->handle, group->csg_nr);
		if (kbase_prepare_to_reset_gpu(kbdev))
			kbase_reset_gpu(kbdev);
		err = -ETIMEDOUT;
	}

	return err;
}

void kbase_csf_scheduler_group_deschedule(struct kbase_queue_group *group)
{
	struct kbase_device *kbdev = group->kctx->kbdev;
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	long remaining =
		kbase_csf_timeout_in_jiffies(CSG_SCHED_STOP_TIMEOUT_MS);
	bool force = false;

	lockdep_assert_held(&group->kctx->csf.lock);
	mutex_lock(&scheduler->lock);

	while (queue_group_scheduled_locked(group)) {
		u32 saved_state = scheduler->state;
		bool reset = kbase_reset_gpu_is_active(kbdev);

		if (!kbasep_csf_scheduler_group_is_on_slot_locked(group)) {
			sched_evict_group(group, false);
		} else if (reset || saved_state == SCHED_INACTIVE || force) {
			bool as_faulty;

			if (!reset)
				term_group_sync(group);
			/* Treat the csg been terminated */
			as_faulty = cleanup_csg_slot(group);
			/* remove from the scheduler list */
			sched_evict_group(group, as_faulty);
		}

		/* waiting scheduler state to change */
		if (queue_group_scheduled_locked(group)) {
			mutex_unlock(&scheduler->lock);
			remaining = wait_event_timeout(
					kbdev->csf.event_wait,
					saved_state != scheduler->state,
					remaining);
			if (!remaining) {
				dev_warn(kbdev->dev, "Scheduler state change wait timed out for group %d on slot %d",
					 group->handle, group->csg_nr);
				force = true;
			}
			mutex_lock(&scheduler->lock);
		}
	}

	mutex_unlock(&scheduler->lock);
}

/**
 * scheduler_group_schedule() - Schedule a GPU command queue group on firmware
 *
 * @group: Pointer to the queue group to be scheduled.
 *
 * This function would enable the scheduling of GPU command queue group on
 * firmware.
 *
 * Return: 0 on success, or negative on failure.
 */
static int scheduler_group_schedule(struct kbase_queue_group *group)
{
	struct kbase_context *kctx = group->kctx;
	struct kbase_device *kbdev = kctx->kbdev;

	lockdep_assert_held(&kctx->csf.lock);
	lockdep_assert_held(&kbdev->csf.scheduler.lock);

	if (queue_group_idle_locked(group)) {
		if (group->run_state ==
				KBASE_CSF_GROUP_SUSPENDED_ON_WAIT_SYNC) {
			remove_group_from_idle_wait(group);
			insert_group_to_runnable(&kbdev->csf.scheduler, group,
					KBASE_CSF_GROUP_SUSPENDED);
		} else {
			WARN_ON(kctx->csf.sched.num_runnable_grps == 0);
			WARN_ON(kbdev->csf.scheduler.total_runnable_grps == 0);

			group->run_state =
				(group->run_state == KBASE_CSF_GROUP_IDLE) ?
					KBASE_CSF_GROUP_RUNNABLE :
					KBASE_CSF_GROUP_SUSPENDED;
		}
	} else if (!queue_group_scheduled_locked(group)) {
		insert_group_to_runnable(&kbdev->csf.scheduler, group,
			KBASE_CSF_GROUP_RUNNABLE);
	}

	return 0;
}

/**
 * set_max_csg_slots() - Set the number of available command stream group slots
 *
 * @kbdev: Pointer of the GPU device.
 *
 * This function would set/limit the number of command stream group slots that
 * can be used in the given tick/tock. It would be less than the total command
 * stream group slots supported by firmware if the number of GPU address space
 * slots required to utilize all the CSG slots is more than the available
 * address space slots.
 */
static inline void set_max_csg_slots(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	unsigned int total_csg_slots = kbdev->csf.global_iface.group_num;
	unsigned int max_address_space_slots = kbdev->nr_hw_address_spaces - 1;

	WARN_ON(scheduler->num_active_address_spaces > total_csg_slots);

	if (likely(scheduler->num_active_address_spaces <=
		   max_address_space_slots))
		scheduler->num_csg_slots_for_tick = total_csg_slots;
}

/**
 * count_active_address_space() - Count the number of GPU address space slots
 *
 * @kbdev: Pointer of the GPU device.
 * @kctx: Pointer of the Kbase context.
 *
 * This function would update the counter that is tracking the number of GPU
 * address space slots that would be required to program the command stream
 * group slots from the groups at the head of groups_to_schedule list.
 */
static inline void count_active_address_space(struct kbase_device *kbdev,
		struct kbase_context *kctx)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	unsigned int total_csg_slots = kbdev->csf.global_iface.group_num;
	unsigned int max_address_space_slots = kbdev->nr_hw_address_spaces - 1;

	if (scheduler->ngrp_to_schedule <= total_csg_slots) {
		if (kctx->csf.sched.ngrp_to_schedule == 1) {
			scheduler->num_active_address_spaces++;

			if (scheduler->num_active_address_spaces <=
			    max_address_space_slots)
				scheduler->num_csg_slots_for_tick++;
		}
	}
}

/**
 * update_resident_groups_priority() - Update the priority of resident groups
 *
 * @kbdev:    The GPU device.
 *
 * This function will update the priority of all resident queue groups
 * that are at the head of groups_to_schedule list, preceding the first
 * non-resident group.
 *
 * This function will also adjust kbase_csf_scheduler.head_slot_priority on
 * the priority update.
 */
static void update_resident_groups_priority(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	u32 num_groups = scheduler->num_csg_slots_for_tick;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);
	while (!list_empty(&scheduler->groups_to_schedule)) {
		struct kbase_queue_group *group =
			list_first_entry(&scheduler->groups_to_schedule,
					struct kbase_queue_group,
					 link_to_schedule);
		bool resident =
			kbasep_csf_scheduler_group_is_on_slot_locked(group);

		if (!resident || (group->prepared_seq_num >= num_groups))
			break;

		update_csg_slot_priority(group,
					 scheduler->head_slot_priority);

		/* Drop the head group from the list */
		remove_scheduled_group(kbdev, group);
		scheduler->head_slot_priority--;
	}
}

/**
 * program_group_on_vacant_csg_slot() - Program a non-resident group on the
 *                                      given vacant CSG slot.
 * @kbdev:    Pointer to the GPU device.
 * @slot:     Vacant command stream group slot number.
 *
 * This function will program a non-resident group at the head of
 * kbase_csf_scheduler.groups_to_schedule list on the given vacant command
 * stream group slot, provided the initial position of the non-resident
 * group in the list is less than the number of CSG slots and there is
 * an available GPU address space slot.
 * kbase_csf_scheduler.head_slot_priority would also be adjusted after
 * programming the slot.
 */
static void program_group_on_vacant_csg_slot(struct kbase_device *kbdev,
		s8 slot)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	struct kbase_queue_group *const group =
		list_empty(&scheduler->groups_to_schedule) ? NULL :
			list_first_entry(&scheduler->groups_to_schedule,
					struct kbase_queue_group,
					link_to_schedule);
	u32 num_groups = scheduler->num_csg_slots_for_tick;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);
	if (group && (group->prepared_seq_num < num_groups)) {
		bool ret = kbasep_csf_scheduler_group_is_on_slot_locked(group);

		if (!WARN_ON(ret)) {
			if (kctx_as_enabled(group->kctx)) {
				program_csg_slot(group,
					 slot,
					 scheduler->head_slot_priority);

				if (likely(csg_slot_in_use(kbdev, slot))) {
					/* Drop the head group from the list */
					remove_scheduled_group(kbdev, group);
					scheduler->head_slot_priority--;
				}
			} else
				remove_scheduled_group(kbdev, group);
		}
	}
}

/**
 * program_vacant_csg_slot() - Program the vacant CSG slot with a non-resident
 *                             group and update the priority of resident groups.
 *
 * @kbdev:    Pointer to the GPU device.
 * @slot:     Vacant command stream group slot number.
 *
 * This function will first update the priority of all resident queue groups
 * that are at the head of groups_to_schedule list, preceding the first
 * non-resident group, it will then try to program the given command stream
 * group slot with the non-resident group. Finally update the priority of all
 * resident queue groups following the non-resident group.
 *
 * kbase_csf_scheduler.head_slot_priority would also be adjusted.
 */
static void program_vacant_csg_slot(struct kbase_device *kbdev, s8 slot)
{
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	struct kbase_csf_csg_slot *const csg_slot =
				scheduler->csg_slots;

	lockdep_assert_held(&kbdev->csf.scheduler.lock);
	WARN_ON(atomic_read(&csg_slot[slot].state) != CSG_SLOT_READY);

	/* First update priority for already resident groups (if any)
	 * before the non-resident group
	 */
	update_resident_groups_priority(kbdev);

	/* Now consume the vacant slot for the non-resident group */
	program_group_on_vacant_csg_slot(kbdev, slot);

	/* Now update priority for already resident groups (if any)
	 * following the non-resident group
	 */
	update_resident_groups_priority(kbdev);
}

static bool slots_state_changed(struct kbase_device *kbdev,
		unsigned long *slots_mask,
		bool (*state_check_func)(struct kbase_device *, s8))
{
	u32 num_groups = kbdev->csf.global_iface.group_num;
	unsigned long changed = 0;
	u32 i;

	i = find_first_bit(slots_mask, num_groups);

	while (i < num_groups) {
		if (state_check_func(kbdev, (s8)i))
			changed |= BIT(i);

		i = find_next_bit(slots_mask, num_groups, i + 1);
	}

	if (changed)
		*slots_mask = changed;

	return changed > 0;
}

/**
 * program_suspending_csg_slots() - Program the CSG slots vacated on suspension
 *                                  of queue groups running on them.
 *
 * @kbdev:    Pointer to the GPU device.
 *
 * This function will first wait for the ongoing suspension to complete on a
 * command stream group slot and will then program the vacant slot with the
 * non-resident queue group inside the groups_to_schedule list.
 * The programming of the non-resident queue group on the vacant slot could
 * fail due to unavailability of free GPU address space slot and so the
 * programming is re-attempted after the ongoing suspension has completed
 * for all the command stream group slots.
 * The priority of resident groups before and after the non-resident group
 * in the groups_to_schedule list would also be updated.
 * This would be repeated for all the slots undergoing suspension.
 * GPU reset would be initiated if the wait for suspend times out.
 */
static void program_suspending_csg_slots(struct kbase_device *kbdev)
{
	u32 num_groups = kbdev->csf.global_iface.group_num;
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	unsigned long slot_mask = ~(scheduler->csgs_events_enable_mask);
	unsigned long idle_slots_mask = 0;
	unsigned long evicted_mask = 0;
	bool suspend_wait_failed = false;
	long remaining =
		kbase_csf_timeout_in_jiffies(CSF_STATE_WAIT_TIMEOUT_MS);

	while (slot_mask && !kbase_reset_gpu_is_active(kbdev)) {
		unsigned long changed = slot_mask;

		remaining = wait_event_timeout(kbdev->csf.event_wait,
			slots_state_changed(kbdev, &changed, csg_slot_stopped_raw),
			remaining);

		if (remaining) {
			u32 i = find_first_bit(&changed, num_groups);

			while (i < num_groups) {
				struct kbase_queue_group *group =
					scheduler->csg_slots[i].resident_group;

				if (WARN_ON(!csg_slot_stopped_locked(kbdev, (s8)i))) {
					i = find_next_bit(&changed, num_groups, i + 1);
					continue;
				}
				/* The on slot csg is now stopped */
				slot_mask &= ~BIT(i);

				if (likely(group)) {
					bool as_fault;
					/* Only do save/cleanup if the
					 * group is not terminated during
					 * the sleep.
					 */
					save_csg_slot(group);
					as_fault = cleanup_csg_slot(group);
					/* If AS fault detected, evict it */
					if (as_fault) {
						sched_evict_group(group, true);
						evicted_mask |= BIT(i);
					}
				}

				program_vacant_csg_slot(kbdev, (s8)i);
				if (unlikely(!csg_slot_in_use(kbdev, (int)i)))
					idle_slots_mask |= BIT(i);

				i = find_next_bit(&changed, num_groups, i + 1);
			}
		} else {
			dev_warn(kbdev->dev, "Timed out waiting for CSG slots to suspend, slot mask=0x%lx\n",
				 slot_mask);

			if (kbase_prepare_to_reset_gpu(kbdev))
				kbase_reset_gpu(kbdev);
			suspend_wait_failed = true;
			break;
		}
	}

	if (evicted_mask)
		dev_info(kbdev->dev, "Scheduler evicting slots: mask=0x%lx\n",
			 evicted_mask);

	if (unlikely(!suspend_wait_failed)) {
		u32 i = find_first_bit(&idle_slots_mask, num_groups);

		while (i < num_groups) {
			program_vacant_csg_slot(kbdev, (s8)i);
			i = find_next_bit(&idle_slots_mask, num_groups, i + 1);
		}
	}
}

static void suspend_queue_group(struct kbase_queue_group *group)
{
	unsigned long flags;
	struct kbase_csf_scheduler *const scheduler =
		&group->kctx->kbdev->csf.scheduler;

	spin_lock_irqsave(&scheduler->interrupt_lock, flags);
	scheduler->csgs_events_enable_mask &= ~BIT(group->csg_nr);
	spin_unlock_irqrestore(&scheduler->interrupt_lock, flags);

	/* If AS fault detected, terminate the group */
	if (!kctx_as_enabled(group->kctx))
		term_csg_slot(group);
	else
		suspend_csg_slot(group);
}

static void wait_csg_slots_start(struct kbase_device *kbdev)
{
	u32 num_groups = kbdev->csf.global_iface.group_num;
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	long remaining =
		kbase_csf_timeout_in_jiffies(CSF_STATE_WAIT_TIMEOUT_MS);
	unsigned long slot_mask = 0;
	u32 i;

	/* extract start slot flags for check */
	for (i = 0; i < num_groups; i++) {
		if (atomic_read(&scheduler->csg_slots[i].state) ==
		    CSG_SLOT_READY2RUN)
			slot_mask |= BIT(i);
	}

	while (slot_mask && !kbase_reset_gpu_is_active(kbdev)) {
		unsigned long changed = slot_mask;

		remaining = wait_event_timeout(kbdev->csf.event_wait,
			slots_state_changed(kbdev, &changed, csg_slot_running),
			remaining);

		if (remaining) {
			i = find_first_bit(&changed, num_groups);
			while (i < num_groups) {
				struct kbase_queue_group *group =
					scheduler->csg_slots[i].resident_group;

				/* The on slot csg is now running */
				slot_mask &= ~BIT(i);
				group->run_state = KBASE_CSF_GROUP_RUNNABLE;

				i = find_next_bit(&changed, num_groups, i + 1);
			}
		} else {
			dev_warn(kbdev->dev, "Timed out waiting for CSG slots to start, slot mask=0x%lx\n",
				 slot_mask);

			if (kbase_prepare_to_reset_gpu(kbdev))
				kbase_reset_gpu(kbdev);
			break;
		}
	}
}

void kbase_csf_scheduler_evict_ctx_slots(struct kbase_device *kbdev,
		struct kbase_context *kctx, struct list_head *evicted_groups)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	struct kbase_queue_group *group;
	u32 num_groups = kbdev->csf.global_iface.group_num;
	u32 slot;
	unsigned long slot_mask = 0;
	unsigned long terminated_slot_mask;
	long remaining =
		kbase_csf_timeout_in_jiffies(DEFAULT_RESET_TIMEOUT_MS);

	lockdep_assert_held(&kctx->csf.lock);
	mutex_lock(&scheduler->lock);

	for (slot = 0; slot < num_groups; slot++) {
		group = kbdev->csf.scheduler.csg_slots[slot].resident_group;
		if (group && group->kctx == kctx) {
			term_csg_slot(group);
			slot_mask |= (1 << slot);
		}
	}

	dev_info(kbdev->dev, "Evicting context %d_%d slots, mask=0x%lx\n",
			kctx->tgid, kctx->id, slot_mask);

	terminated_slot_mask = slot_mask;
	while (remaining && slot_mask && !kbase_reset_gpu_is_active(kbdev)) {
		unsigned long changed = slot_mask;

		remaining = wait_event_timeout(kbdev->csf.event_wait,
			slots_state_changed(kbdev, &changed, csg_slot_stopped_raw),
			remaining);

		if (remaining)
			slot_mask &= ~changed;
	}

	for (slot = find_first_bit(&terminated_slot_mask, num_groups);
		slot < num_groups;
		slot = find_next_bit(&terminated_slot_mask, num_groups, slot + 1)) {
		bool as_fault;

		terminated_slot_mask &= ~BIT(slot);
		group = scheduler->csg_slots[slot].resident_group;
		as_fault = cleanup_csg_slot(group);
		/* remove the group from the scheduler list */
		sched_evict_group(group, as_fault);
		/* return the evicted group to the caller */
		list_add_tail(&group->link, evicted_groups);
	}

	if (!remaining) {
		dev_warn(kbdev->dev, "Timeout on evicting ctx slots, mask=0x%lx\n",
				slot_mask);
		if (kbase_prepare_to_reset_gpu(kbdev))
			kbase_reset_gpu(kbdev);
	}

	mutex_unlock(&scheduler->lock);
}

/**
 * scheduler_slot_protm_ack - Acknowledging the protected region requests
 * from the resident group on a given slot.
 *
 * The function assumes that the given slot is in stable running state and
 * has already been judged by the caller on that any pending protected region
 * requests of the resident group should be acknowledged.
 *
 * @kbdev:  Pointer to the GPU device.
 * @group:  Pointer to the resident group on the give slot.
 * @slot:   The slot that the given group is actively operating on.
 *
 * Return: true if the group has pending protm request(s) and is acknowledged.
 *         The caller should arrange to enter the protected mode for servicing
 *         it. Otherwise return false, indicating the group has no pending protm
 *         request.
 */
static bool scheduler_slot_protm_ack(struct kbase_device *const kbdev,
		struct kbase_queue_group *const group,
		const int slot)
{
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	bool protm_ack = false;
	struct kbase_csf_cmd_stream_group_info *ginfo =
		&kbdev->csf.global_iface.groups[slot];
	u32 max_csi;
	int i;

	if (WARN_ON(scheduler->csg_slots[slot].resident_group != group))
		return protm_ack;

	lockdep_assert_held(&scheduler->lock);
	lockdep_assert_held(&group->kctx->kbdev->csf.scheduler.interrupt_lock);

	max_csi = ginfo->stream_num;
	i = find_first_bit(group->protm_pending_bitmap, max_csi);
	while (i < max_csi) {
		struct kbase_queue *queue = group->bound_queues[i];

		bitmap_clear(group->protm_pending_bitmap, i, 1);

		if (!WARN_ON(!queue) && queue->enabled) {
			struct kbase_csf_cmd_stream_info *stream =
						&ginfo->streams[i];
			u32 cs_protm_ack = kbase_csf_firmware_cs_output(
						stream, CS_ACK) &
						CS_ACK_PROTM_PEND_MASK;
			u32 cs_protm_req = kbase_csf_firmware_cs_input_read(
						stream, CS_REQ) &
						CS_REQ_PROTM_PEND_MASK;

			if (WARN_ON(cs_protm_ack == cs_protm_req))
				break;

			kbase_csf_firmware_cs_input_mask(stream, CS_REQ,
						cs_protm_ack,
						CS_ACK_PROTM_PEND_MASK);
			protm_ack = true;
			dev_dbg(kbdev->dev,
				"PROTM-ack for queue-%d, group-%d slot-%d",
				queue->csi_index, group->handle, slot);
		}

		i = find_next_bit(group->protm_pending_bitmap,
				max_csi, i + 1);
	}

	return protm_ack;
}


/**
 * scheduler_group_check_protm_enter - Request the given group to be evaluated
 * for triggering the protected mode.
 *
 * The function assumes the given group is either an active running group or
 * the scheduler internally maintained field scheduler->top_grp.
 *
 * If the given group matches the scheduler->top_grp, the function will
 * evaluate the given group, if not NULL, for pending protected region
 * request(s). If the scheduler->top_grp is NULL, which may happen when the
 * top-group is evicted during the tick, its would be replacement (i.e. the
 * second top-group) is used for evaluation as a replacement.
 *
 * If the scheduler->top_grp being NULL and the given group differs from it, the
 * second top-group (as a replacement of the top-group) is used for match check.
 * On success, the given group is evaluated for triggering the protected mode.
 *
 * If the GPU is not already running in protected mode and the selected group
 * for evaluation has protected region requests from its bound queues, the
 * requests are acknowledged and the GPU is instructed to enter the protected
 * mode.
 *
 * @kbdev:     Pointer to the GPU device.
 * @scheduler: Pointer to the CSF scheduler.
 * @group:     Pointer to the given group for evaluation.
 */
static void scheduler_group_check_protm_enter(struct kbase_device *const kbdev,
				struct kbase_csf_scheduler *const scheduler,
				struct kbase_queue_group *const group)
{
	unsigned long flags;

	if (WARN_ON(group == NULL && (group != scheduler->top_grp)))
		return;

	lockdep_assert_held(&scheduler->lock);

	if (WARN_ON(!(scheduler->state == SCHED_INACTIVE)))
		return;

	spin_lock_irqsave(&scheduler->interrupt_lock, flags);

	/* Firmware samples the PROTM_PEND ACK bit for command streams when
	 * Host sends PROTM_ENTER global request. So if PROTM_PEND ACK bit
	 * is set for a command stream after Host has sent the PROTM_ENTER
	 * Global request, then there is no guarantee that firmware will
	 * notice that prior to switching to protected mode. And firmware
	 * may not again raise the PROTM_PEND interrupt for that command
	 * stream later on. To avoid that uncertainty PROTM_PEND ACK bit
	 * is not set for a command stream if the request to enter protected
	 * mode has already been sent. It will be set later (after the exit
	 * from protected mode has taken place) when the group to which
	 * command stream is bound becomes the top group.
	 *
	 * The actual decision of entering protected mode is hinging on the
	 * input group is the top priority group, or, in case the previous
	 * top-group is evicted from the scheduler during the tick, its would
	 * be replacement, and that it is currently in a stable state (i.e. the
	 * slot state is running).
	 */
	if (!scheduler->protected_mode_in_use) {
		struct kbase_queue_group *match_grp = scheduler->top_grp ?
						scheduler->top_grp :
						get_tock_top_group(scheduler);
		struct kbase_queue_group *input_grp = group ? group : match_grp;

		if (input_grp && (input_grp == match_grp)) {
			const int slot =
				kbase_csf_scheduler_group_get_slot_locked(
					input_grp);

			/* check the input_grp is running and requesting
			 * protected mode
			 */
			if (slot >= 0 &&
				atomic_read(
					&scheduler->csg_slots[slot].state) ==
					CSG_SLOT_RUNNING) {
				if (kctx_as_enabled(input_grp->kctx) &&
					scheduler_slot_protm_ack(kbdev,
							input_grp, slot)) {
					/* Option of acknowledging to multiple
					 * CSGs from the same kctx is dropped,
					 * after consulting with the
					 * architecture team. See the comment in
					 * GPUCORE-21394.
					 */

					/* Switch to protected mode */
					scheduler->protected_mode_in_use = true;
					spin_unlock_irqrestore(&scheduler->interrupt_lock, flags);
					kbase_csf_enter_protected_mode(kbdev);
					return;
				}
			}
		}
	}

	spin_unlock_irqrestore(&scheduler->interrupt_lock, flags);
}

static void scheduler_apply(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	const u32 total_csg_slots = kbdev->csf.global_iface.group_num;
	const u32 available_csg_slots = scheduler->num_csg_slots_for_tick;
	u32 suspend_cnt = 0;
	u32 remain_cnt = 0;
	u32 resident_cnt = 0;
	struct kbase_queue_group *group;
	u32 i;
	u32 spare;

	lockdep_assert_held(&scheduler->lock);

	/* Suspend those resident groups not in the run list */
	for (i = 0; i < total_csg_slots; i++) {
		group = scheduler->csg_slots[i].resident_group;
		if (group) {
			resident_cnt++;
			if (group->prepared_seq_num >= available_csg_slots) {
				suspend_queue_group(group);
				suspend_cnt++;
			} else
				remain_cnt++;
		}
	}

	/* If there are spare slots, apply heads in the list */
	spare = (available_csg_slots > resident_cnt) ?
		(available_csg_slots - resident_cnt) : 0;
	while (!list_empty(&scheduler->groups_to_schedule)) {
		group = list_first_entry(&scheduler->groups_to_schedule,
				struct kbase_queue_group,
				link_to_schedule);

		if (kbasep_csf_scheduler_group_is_on_slot_locked(group)) {
			/* One of the resident remainders */
			update_csg_slot_priority(group,
						scheduler->head_slot_priority);
		} else if (spare != 0) {
			s8 slot = (s8)find_first_zero_bit(
				     kbdev->csf.scheduler.csg_inuse_bitmap,
				     total_csg_slots);

			if (WARN_ON(slot >= (s8)total_csg_slots))
				break;

			if (!kctx_as_enabled(group->kctx)) {
				/* Drop the head group and continue */
				remove_scheduled_group(kbdev, group);
				continue;
			}
			program_csg_slot(group, slot,
					 scheduler->head_slot_priority);
			if (unlikely(!csg_slot_in_use(kbdev, slot)))
				break;

			spare--;
		} else
			break;

		/* Drop the head csg from the list */
		remove_scheduled_group(kbdev, group);
		if (scheduler->head_slot_priority)
			scheduler->head_slot_priority--;
	}

	/* Dealing with groups currently going through suspend */
	program_suspending_csg_slots(kbdev);
}

static void scheduler_ctx_scan_groups(struct kbase_device *kbdev,
		struct kbase_context *kctx, int priority)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	struct kbase_queue_group *group;

	lockdep_assert_held(&scheduler->lock);
	if (WARN_ON(priority < 0) ||
	    WARN_ON(priority >= BASE_QUEUE_GROUP_PRIORITY_COUNT))
		return;

	if (!kctx_as_enabled(kctx))
		return;

	list_for_each_entry(group, &kctx->csf.sched.runnable_groups[priority],
			    link) {
		if (WARN_ON(!list_empty(&group->link_to_schedule)))
			/* This would be a bug */
			list_del_init(&group->link_to_schedule);

		if (queue_group_idle_locked(group)) {
			list_add_tail(&group->link_to_schedule,
				      &scheduler->idle_groups_to_schedule);
			continue;
		}

		if (!scheduler->ngrp_to_schedule) {
			/* keep the top csg's origin */
			scheduler->top_ctx = kctx;
			scheduler->top_grp = group;
		}

		list_add_tail(&group->link_to_schedule,
			      &scheduler->groups_to_schedule);
		group->prepared_seq_num = scheduler->ngrp_to_schedule++;

		kctx->csf.sched.ngrp_to_schedule++;
		count_active_address_space(kbdev, kctx);
	}
}

/**
 * scheduler_rotate_groups() - Rotate the runnable queue groups to provide
 *                             fairness of scheduling within a single
 *                             kbase_context.
 *
 * Since only kbase_csf_scheduler's top_grp (i.e. the queue group assigned
 * the highest slot priority) is guaranteed to get the resources that it
 * needs we only rotate the kbase_context corresponding to it -
 * kbase_csf_scheduler's top_ctx.
 *
 * The priority level chosen for rotation is the one containing the previous
 * scheduling cycle's kbase_csf_scheduler's top_grp.
 *
 * In a 'fresh-slice-cycle' this always corresponds to the highest group
 * priority in use by kbase_csf_scheduler's top_ctx. That is, it's the priority
 * level of the previous scheduling cycle's first runnable kbase_context.
 *
 * We choose this priority level because when higher priority work is
 * scheduled, we should always cause the scheduler to run and do a scan. The
 * scan always enumerates the highest priority work first (whether that be
 * based on process priority or group priority), and thus
 * kbase_csf_scheduler's top_grp will point to the first of those high priority
 * groups, which necessarily must be the highest priority group in
 * kbase_csf_scheduler's top_ctx. The fresh-slice-cycle will run later and pick
 * up that group appropriately.
 *
 * If kbase_csf_scheduler's top_grp was instead evicted (and thus is NULL),
 * then no explicit rotation occurs on the next fresh-slice-cycle schedule, but
 * will set up kbase_csf_scheduler's top_ctx again for the next scheduling
 * cycle. Implicitly, a rotation had already occurred by removing
 * the kbase_csf_scheduler's top_grp
 *
 * If kbase_csf_scheduler's top_grp became idle and all other groups belonging
 * to kbase_csf_scheduler's top_grp's priority level in kbase_csf_scheduler's
 * top_ctx are also idle, then the effect of this will be to rotate idle
 * groups, which might not actually become resident in the next
 * scheduling slice. However this is acceptable since a queue group becoming
 * idle is implicitly a rotation (as above with evicted queue groups), as it
 * automatically allows a new queue group to take the maximum slot priority
 * whilst the idle kbase_csf_scheduler's top_grp ends up near the back of
 * the kbase_csf_scheduler's groups_to_schedule list. In this example, it will
 * be for a group in the next lowest priority level or in absence of those the
 * next kbase_context's queue groups.
 *
 * @kbdev:    Pointer to the GPU device.
 */
static void scheduler_rotate_groups(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	struct kbase_context *const top_ctx = scheduler->top_ctx;
	struct kbase_queue_group *const top_grp = scheduler->top_grp;

	lockdep_assert_held(&scheduler->lock);
	if (top_ctx && top_grp) {
		struct list_head *list =
			&top_ctx->csf.sched.runnable_groups[top_grp->priority];

		WARN_ON(top_grp->kctx != top_ctx);
		if (!WARN_ON(list_empty(list))) {
			list_move_tail(&top_grp->link, list);
			dev_dbg(kbdev->dev,
			    "groups rotated for a context, num_runnable_groups: %u\n",
			    scheduler->top_ctx->csf.sched.num_runnable_grps);
		}
	}
}

static void scheduler_rotate_ctxs(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	struct list_head *list = &scheduler->runnable_kctxs;

	lockdep_assert_held(&scheduler->lock);
	if (scheduler->top_ctx) {
		if (!WARN_ON(list_empty(list))) {
			struct kbase_context *pos;
			bool found = false;

			/* Locate the ctx on the list */
			list_for_each_entry(pos, list, csf.link) {
				if (scheduler->top_ctx == pos) {
					found = true;
					break;
				}
			}

			if (!WARN_ON(!found)) {
				list_move_tail(&pos->csf.link, list);
				dev_dbg(kbdev->dev, "contexts rotated\n");
			}
		}
	}
}

static void scheduler_handle_idle_slots(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	u32 num_groups = kbdev->csf.global_iface.group_num;
	DECLARE_BITMAP(idle_slots, MAX_SUPPORTED_CSGS);
	unsigned long i, flags;

	lockdep_assert_held(&scheduler->lock);

	spin_lock_irqsave(&scheduler->interrupt_lock, flags);
	bitmap_copy(idle_slots, scheduler->csg_slots_idle_mask, num_groups);
	bitmap_clear(scheduler->csg_slots_idle_mask, 0, num_groups);
	spin_unlock_irqrestore(&scheduler->interrupt_lock, flags);

	for_each_set_bit(i, idle_slots, num_groups) {
		struct kbase_csf_csg_slot *csg_slot = &scheduler->csg_slots[i];
		struct kbase_queue_group *group = csg_slot->resident_group;

		if (WARN_ON(atomic_read(&csg_slot->state) != CSG_SLOT_RUNNING))
			continue;
		if (WARN_ON(!group))
			continue;
		if (WARN_ON(group->run_state != KBASE_CSF_GROUP_RUNNABLE))
			continue;
		if (WARN_ON(group->priority >= BASE_QUEUE_GROUP_PRIORITY_COUNT))
			continue;

		group->run_state = KBASE_CSF_GROUP_IDLE;
	}
}

static void scheduler_scan_idle_groups(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	struct kbase_queue_group *group, *n;

	list_for_each_entry_safe(group, n, &scheduler->idle_groups_to_schedule,
				 link_to_schedule) {

		WARN_ON(!queue_group_idle_locked(group));

		if (!scheduler->ngrp_to_schedule) {
			/* keep the top csg's origin */
			scheduler->top_ctx = group->kctx;
			scheduler->top_grp = group;
		}

		group->prepared_seq_num = scheduler->ngrp_to_schedule++;
		list_move_tail(&group->link_to_schedule,
			       &scheduler->groups_to_schedule);

		group->kctx->csf.sched.ngrp_to_schedule++;
		count_active_address_space(kbdev, group->kctx);
	}
}

static void scheduler_rotate(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;

	lockdep_assert_held(&scheduler->lock);

	/* Dealing with rotation */
	scheduler_rotate_groups(kbdev);
	scheduler_rotate_ctxs(kbdev);
}

static struct kbase_queue_group *get_tock_top_group(
	struct kbase_csf_scheduler *const scheduler)
{
	struct kbase_context *kctx;
	int i;

	lockdep_assert_held(&scheduler->lock);
	for (i = 0; i < BASE_QUEUE_GROUP_PRIORITY_COUNT; ++i) {
		list_for_each_entry(kctx,
			&scheduler->runnable_kctxs, csf.link) {
			struct kbase_queue_group *group;

			list_for_each_entry(group,
					&kctx->csf.sched.runnable_groups[i],
					link) {
				if (queue_group_idle_locked(group))
					continue;

				return group;
			}
		}
	}

	return NULL;
}

static void scheduler_prepare(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	int i;

	lockdep_assert_held(&scheduler->lock);

	/* Empty the groups_to_schedule */
	while (!list_empty(&scheduler->groups_to_schedule)) {
		struct kbase_queue_group *grp =
			list_first_entry(&scheduler->groups_to_schedule,
					 struct kbase_queue_group,
					 link_to_schedule);

		remove_scheduled_group(kbdev, grp);
	}

	/* Pre-scan init scheduler fields */
	if (WARN_ON(scheduler->ngrp_to_schedule != 0))
		scheduler->ngrp_to_schedule = 0;
	scheduler->top_ctx = NULL;
	scheduler->top_grp = NULL;
	scheduler->head_slot_priority = MAX_CSG_SLOT_PRIORITY;
	WARN_ON(!list_empty(&scheduler->idle_groups_to_schedule));
	scheduler->num_active_address_spaces = 0;
	scheduler->num_csg_slots_for_tick = 0;

	/* Scan out to run groups */
	for (i = 0; i < BASE_QUEUE_GROUP_PRIORITY_COUNT; ++i) {
		struct kbase_context *kctx;

		list_for_each_entry(kctx, &scheduler->runnable_kctxs, csf.link)
			scheduler_ctx_scan_groups(kbdev, kctx, i);
	}

	scheduler_scan_idle_groups(kbdev);
	set_max_csg_slots(kbdev);
	dev_dbg(kbdev->dev, "prepared groups length: %u, num_active_address_spaces: %u\n",
		scheduler->ngrp_to_schedule, scheduler->num_active_address_spaces);
}

static void schedule_on_tock(struct work_struct *work)
{
	struct kbase_device *kbdev = container_of(work, struct kbase_device,
					csf.scheduler.tock_work.work);
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	unsigned long flags;
	bool protm;

	if (kbase_reset_gpu_is_active(kbdev))
		return;

	mutex_lock(&scheduler->lock);

	WARN_ON(!(scheduler->state == SCHED_INACTIVE));
	scheduler->state = SCHED_BUSY;

	/* Avoid tock update if the top-group remains unchanged and is active in
	 * protected mode
	 */
	spin_lock_irqsave(&scheduler->interrupt_lock, flags);
	protm = scheduler->protected_mode_in_use;
	spin_unlock_irqrestore(&scheduler->interrupt_lock, flags);
	if (!protm || get_tock_top_group(scheduler) != scheduler->top_grp) {
		/* Do scheduling stuff */
		scheduler_handle_idle_slots(kbdev);
		scheduler_prepare(kbdev);
		scheduler_apply(kbdev);
		wait_csg_slots_start(kbdev);
	}

	/* Record time information */
	scheduler->last_schedule = jiffies;

	/* Tock is serviced */
	scheduler->tock_pending_request = false;

	scheduler->state = SCHED_INACTIVE;
	scheduler_group_check_protm_enter(kbdev, scheduler, scheduler->top_grp);

	mutex_unlock(&scheduler->lock);

	dev_dbg(kbdev->dev,
		"Waking up for event after schedule-on-tock completes.");
	wake_up_all(&kbdev->csf.event_wait);
}

static void schedule_on_tick(struct work_struct *work)
{
	struct kbase_device *kbdev = container_of(work, struct kbase_device,
					csf.scheduler.tick_work.work);
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	unsigned long flags;
	bool protm;

	if (kbase_reset_gpu_is_active(kbdev))
		return;

	mutex_lock(&scheduler->lock);
	scheduler->state = SCHED_BUSY;

	/* Do scheduling stuff */
	scheduler_rotate(kbdev);

	/* Avoid tick update if the top-group remains unchanged and in protected
	 * mode. For the said case, all the slots update is effectively
	 * competing against the protected mode directed by the top-group. If we
	 * update other slots, even on leaving the top-group slot untouched, the
	 * firmware would exit the protected mode for interacting with the
	 * host-driver. After it, as the top-group would again raise the request
	 * for entering protected mode, we would be actively doing the switching
	 * over twice without progressing the queue jobs.
	 */
	spin_lock_irqsave(&scheduler->interrupt_lock, flags);
	protm = scheduler->protected_mode_in_use;
	spin_unlock_irqrestore(&scheduler->interrupt_lock, flags);
	if (!protm || get_tock_top_group(scheduler) != scheduler->top_grp) {
		scheduler_handle_idle_slots(kbdev);
		scheduler_prepare(kbdev);
		scheduler_apply(kbdev);
		wait_csg_slots_start(kbdev);
	}

	/* Record time information */
	scheduler->last_schedule = jiffies;

	/* Kicking next scheduling if needed */
	if (likely(kbase_csf_scheduler_timer_is_enabled(kbdev)) &&
			(scheduler->total_runnable_grps > 0)) {
		mod_delayed_work(scheduler->wq, &scheduler->tick_work,
				  CSF_SCHEDULER_TIME_TICK_JIFFIES);
		dev_dbg(kbdev->dev, "scheduling for next tick, num_runnable_groups:%u\n",
			scheduler->total_runnable_grps);
	}

	scheduler->state = SCHED_INACTIVE;
	scheduler_group_check_protm_enter(kbdev, scheduler, scheduler->top_grp);
	mutex_unlock(&scheduler->lock);

	dev_dbg(kbdev->dev, "Waking up for event after schedule-on-tick completes.");
	wake_up_all(&kbdev->csf.event_wait);
}

int wait_scheduler_inactive(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	long remaining =
		kbase_csf_timeout_in_jiffies(CSG_SCHED_STOP_TIMEOUT_MS);
	int err = 0;

	lockdep_assert_held(&scheduler->lock);

	while ((scheduler->state != SCHED_INACTIVE) && remaining) {
		mutex_unlock(&scheduler->lock);

		remaining = wait_event_timeout(kbdev->csf.event_wait,
				scheduler->state == SCHED_INACTIVE,
				remaining);

		mutex_lock(&scheduler->lock);

		if (!remaining)
			err = -ETIMEDOUT;
	}

	return err;
}

int wait_csg_slots_suspend(struct kbase_device *kbdev, unsigned long slot_mask,
			   unsigned int timeout_ms)
{
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	long remaining = kbase_csf_timeout_in_jiffies(timeout_ms);
	u32 num_groups = kbdev->csf.global_iface.group_num;
	int err = 0;

	lockdep_assert_held(&scheduler->lock);

	while (slot_mask && remaining) {
		unsigned long changed = slot_mask;

		remaining = wait_event_timeout(kbdev->csf.event_wait,
			slots_state_changed(kbdev, &changed, csg_slot_stopped_locked),
			remaining);

		if (remaining) {
			u32 i;

			for_each_set_bit(i, &changed, num_groups) {
				struct kbase_queue_group *group;

				if (WARN_ON(!csg_slot_stopped_locked(kbdev, (s8)i)))
					continue;

				/* The on slot csg is now stopped */
				clear_bit(i, &slot_mask);

				group = scheduler->csg_slots[i].resident_group;
				if (likely(group)) {
					/* Only do save/cleanup if the
					 * group is not terminated during
					 * the sleep.
					 */
					save_csg_slot(group);
					if (cleanup_csg_slot(group))
						sched_evict_group(group, true);
				}
			}
		} else {
			dev_warn(kbdev->dev, "Timed out waiting for CSG slots to suspend before reset, slot mask=0x%lx\n",
				 slot_mask);
			err = -ETIMEDOUT;
		}
	}

	return err;
}

static void suspend_active_queue_groups(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	u32 num_groups = kbdev->csf.global_iface.group_num;
	unsigned long slot_mask = 0;
	u32 slot_num;

	WARN_ON(!kbase_reset_gpu_is_active(kbdev));

	mutex_lock(&scheduler->lock);

	if (wait_scheduler_inactive(kbdev))
		dev_warn(kbdev->dev, "Timed out waiting for scheduler state change when suspending queue groups before reset");

	for (slot_num = 0; slot_num < num_groups; slot_num++) {
		struct kbase_queue_group *group =
			scheduler->csg_slots[slot_num].resident_group;

		if (group) {
			suspend_queue_group(group);
			set_bit(slot_num, &slot_mask);
		}
	}

	if (wait_csg_slots_suspend(kbdev, slot_mask,
			CSG_SUSPEND_ON_RESET_WAIT_TIMEOUT_MS)) {
		dev_warn(kbdev->dev, "Timed out waiting for CSG slots to suspend before reset, slot mask=0x%lx\n",
			 slot_mask);
	}

	if (slot_mask) {
		/* Need to flush the GPU cache to ensure suspend buffer
		 * contents are not lost on reset of GPU.
		 */
		kbase_gpu_start_cache_clean(kbdev);
		if (kbase_gpu_wait_cache_clean_timeout(kbdev,
				DEFAULT_RESET_TIMEOUT_MS)) {
			dev_warn(kbdev->dev, "Timed out waiting for cache clean to complete before reset");
		}
	}

	mutex_unlock(&scheduler->lock);
}

static void scheduler_inner_reset(struct kbase_device *kbdev)
{
	u32 const num_groups = kbdev->csf.global_iface.group_num;
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;

	WARN_ON(csgs_active(kbdev));

	/* Cancel any potential queued delayed work(s) */
	cancel_delayed_work_sync(&scheduler->tick_work);
	cancel_delayed_work_sync(&scheduler->tock_work);
	cancel_delayed_work_sync(&scheduler->ping_work);

	mutex_lock(&scheduler->lock);

	scheduler->state = SCHED_INACTIVE;
	scheduler->csgs_events_enable_mask = ULONG_MAX;

	scheduler->top_ctx = NULL;
	scheduler->top_grp = NULL;

	bitmap_zero(kbdev->csf.scheduler.csg_inuse_bitmap, num_groups);

	memset(kbdev->csf.scheduler.csg_slots, 0,
	       num_groups * sizeof(struct kbase_csf_csg_slot));

	mutex_unlock(&scheduler->lock);
}

void kbase_csf_scheduler_reset(struct kbase_device *kbdev)
{
	struct kbase_context *kctx;

	WARN_ON(!kbase_reset_gpu_is_active(kbdev));

	suspend_active_queue_groups(kbdev);

	mutex_lock(&kbdev->kctx_list_lock);

	/* The loop to iterate over the kbase contexts is present due to lock
	 * ordering issue between kctx->csf.lock & kbdev->csf.scheduler.lock.
	 * CSF ioctls first take kctx->csf.lock which is context-specific and
	 * then take kbdev->csf.scheduler.lock for global actions like assigning
	 * a CSG slot.
	 * If the lock ordering constraint was not there then could have
	 * directly looped over the active queue groups.
	 */
	list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
		/* Firmware reload would reinitialize the CSG & CS interface IO
		 * pages, so just need to internally mark the currently active
		 * queue groups as terminated (similar to the unexpected OoM
		 * event case).
		 * No further work can now get executed for the active groups
		 * (new groups would have to be created to execute work) and
		 * in near future Clients would be duly informed of this
		 * reset. The resources (like User IO pages, GPU queue memory)
		 * allocated for the associated queues would be freed when the
		 * Clients do the teardown when they become aware of the reset.
		 */
		kbase_csf_active_queue_groups_reset(kbdev, kctx);
	}

	mutex_unlock(&kbdev->kctx_list_lock);

	/* After queue groups reset, the scheduler data fields clear out */
	scheduler_inner_reset(kbdev);
}

static void firmware_aliveness_monitor(struct work_struct *work)
{
	struct kbase_device *kbdev = container_of(work, struct kbase_device,
					csf.scheduler.ping_work.work);
	int err;

#ifdef CONFIG_MALI_DEBUG
	if (fw_debug) {
		/* ping requests cause distraction in firmware debugging */
		return;
	}
#endif

	if (kbase_reset_gpu_is_active(kbdev))
		return;

	if (get_nr_active_csgs(kbdev) != 1)
		return;

	if (kbdev->csf.scheduler.protected_mode_in_use)
		return;

	if (kbase_pm_context_active_handle_suspend(kbdev,
			KBASE_PM_SUSPEND_HANDLER_DONT_INCREASE)) {
		/* Suspend pending - no real need to ping */
		return;
	}

	err = kbase_csf_firmware_ping(kbdev);

	if (err) {
		if (kbase_prepare_to_reset_gpu(kbdev))
			kbase_reset_gpu(kbdev);
	} else if (get_nr_active_csgs(kbdev) == 1) {
		queue_delayed_work(system_long_wq,
			&kbdev->csf.scheduler.ping_work,
			msecs_to_jiffies(FIRMWARE_PING_INTERVAL_MS));
	}

	kbase_pm_context_idle(kbdev);
}

int kbase_csf_scheduler_group_copy_suspend_buf(struct kbase_queue_group *group,
		struct kbase_suspend_copy_buffer *sus_buf)
{
	struct kbase_context *const kctx = group->kctx;
	struct kbase_device *const kbdev = kctx->kbdev;
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	int err = 0;

	lockdep_assert_held(&kctx->csf.lock);
	mutex_lock(&scheduler->lock);

	err = wait_scheduler_inactive(kbdev);
	if (err) {
		dev_warn(kbdev->dev, "Timed out waiting for scheduler state change when suspending group %d on slot %d",
			 group->handle, group->csg_nr);
		goto exit;
	}

	if (kbasep_csf_scheduler_group_is_on_slot_locked(group)) {
		unsigned long slot_mask =
			BIT(kbase_csf_scheduler_group_get_slot(group));

		suspend_queue_group(group);
		err = wait_csg_slots_suspend(kbdev, slot_mask,
				CSF_STATE_WAIT_TIMEOUT_MS);
		if (err) {
			dev_warn(kbdev->dev, "Timed out waiting for the group %d to suspend on slot %d",
					group->handle, group->csg_nr);
			goto exit;
		}
	}

	if (queue_group_suspended_locked(group)) {
		unsigned int target_page_nr = 0, i = 0;
		u64 offset = sus_buf->offset;
		size_t to_copy = sus_buf->size;

		/* Similar to the case of HW counters, need to flush the
		 * GPU cache before reading from the suspend buffer pages
		 * as they are mapped and cached on GPU side.
		 */
		kbase_gpu_start_cache_clean(kbdev);
		kbase_gpu_wait_cache_clean(kbdev);

		for (i = 0; i < PFN_UP(sus_buf->size) &&
				target_page_nr < sus_buf->nr_pages; i++) {
			struct page *pg =
				as_page(group->normal_suspend_buf.phy[i]);
			void *sus_page = kmap(pg);

			if (sus_page) {
				kbase_sync_single_for_cpu(kbdev,
					kbase_dma_addr(pg),
					PAGE_SIZE, DMA_BIDIRECTIONAL);

				err = kbase_mem_copy_to_pinned_user_pages(
						sus_buf->pages, sus_page,
						&to_copy, sus_buf->nr_pages,
						&target_page_nr, offset);
				kunmap(pg);
				if (err)
					goto exit;
			}
		}
		schedule_in_cycle(group);
	} else
		/* If addr-space fault, the group may have been evicted */
		err = -EIO;

exit:
	mutex_unlock(&scheduler->lock);
	return err;
}

KBASE_EXPORT_TEST_API(kbase_csf_scheduler_group_copy_suspend_buf);

/**
 * group_sync_updated() - Evaluate sync wait condition of all blocked command
 *                        queues of the group.
 *
 * @group: Pointer to the command queue group that has blocked command queue(s)
 *         bound to it.
 *
 * Return: true if sync wait condition is satisfied for at least one blocked
 *         queue of the group.
 */
static bool group_sync_updated(struct kbase_queue_group *group)
{
	bool updated = false;
	int stream;

	WARN_ON(group->run_state != KBASE_CSF_GROUP_SUSPENDED_ON_WAIT_SYNC);

	for (stream = 0; stream < MAX_SUPPORTED_STREAMS_PER_GROUP; ++stream) {
		struct kbase_queue *const queue = group->bound_queues[stream];

		/* For idle_sync_wait command streams */
		if (queue && queue->enabled &&
		    CS_STATUS_WAIT_SYNC_WAIT_GET(queue->status_wait))
			if (evaluate_sync_update(queue)) {
				updated = true;
				queue->status_wait = 0;
			}
	}

	return updated;
}

void kbase_csf_scheduler_group_protm_enter(struct kbase_queue_group *group)
{
	struct kbase_device *const kbdev = group->kctx->kbdev;
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;

	mutex_lock(&scheduler->lock);

	/* Synchronize the entering with the scheduler */
	if (SCHED_INACTIVE == scheduler->state) {
		scheduler_group_check_protm_enter(kbdev, scheduler, group);
	}

	mutex_unlock(&scheduler->lock);
}

/**
 * check_group_sync_update_worker() - Check the sync wait condition for all the
 *                                    blocked queue groups
 *
 * @work:    Pointer to the context-specific work item for evaluating the wait
 *           condition for all the queue groups in idle_wait_groups list.
 *
 * This function checks the gpu queues of all the groups present in
 * idle_wait_groups list of a context. If the sync wait condition
 * for at least one queue bound to the group has been satisfied then
 * the group is moved to the per context list of runnable groups so
 * that Scheduler can consider scheduling the group in next tick.
 */
static void check_group_sync_update_worker(struct work_struct *work)
{
	struct kbase_context *const kctx = container_of(work,
		struct kbase_context, csf.sched.sync_update_work);
	struct kbase_csf_scheduler *const scheduler =
		&kctx->kbdev->csf.scheduler;

	mutex_lock(&scheduler->lock);

	if (kctx->csf.sched.num_idle_wait_grps != 0) {
		struct kbase_queue_group *group, *temp;

		list_for_each_entry_safe(group, temp,
				&kctx->csf.sched.idle_wait_groups, link) {
			if (group_sync_updated(group)) {
				remove_group_from_idle_wait(group);
				/* Move this group back in to the runnable
				 * groups list of the context.
				 */
				insert_group_to_runnable(scheduler, group,
					KBASE_CSF_GROUP_SUSPENDED);
			}
		}
	} else {
		WARN_ON(!list_empty(&kctx->csf.sched.idle_wait_groups));
	}

	mutex_unlock(&scheduler->lock);
}

static
enum kbase_csf_event_callback_action check_group_sync_update_cb(void *param)
{
	struct kbase_context *const kctx = param;

	queue_work(kctx->csf.sched.sync_update_wq,
		&kctx->csf.sched.sync_update_work);

	return KBASE_CSF_EVENT_CALLBACK_KEEP;
}

int kbase_csf_scheduler_context_init(struct kbase_context *kctx)
{
	int priority;
	int err;

	for (priority = 0; priority < BASE_QUEUE_GROUP_PRIORITY_COUNT;
	     ++priority) {
		INIT_LIST_HEAD(&kctx->csf.sched.runnable_groups[priority]);
	}

	kctx->csf.sched.num_runnable_grps = 0;
	INIT_LIST_HEAD(&kctx->csf.sched.idle_wait_groups);
	kctx->csf.sched.num_idle_wait_grps = 0;
	kctx->csf.sched.ngrp_to_schedule = 0;

	kctx->csf.sched.sync_update_wq =
		alloc_ordered_workqueue("mali_kbase_csf_sync_update_wq",
			WQ_HIGHPRI);
	if (!kctx->csf.sched.sync_update_wq) {
		dev_err(kctx->kbdev->dev,
			"Failed to initialize scheduler context workqueue");
		return -ENOMEM;
	}

	INIT_WORK(&kctx->csf.sched.sync_update_work,
		check_group_sync_update_worker);

	err = kbase_csf_event_wait_add(kctx, check_group_sync_update_cb, kctx);

	if (err) {
		dev_err(kctx->kbdev->dev,
			"Failed to register a sync update callback");
		destroy_workqueue(kctx->csf.sched.sync_update_wq);
	}

	return err;
}

void kbase_csf_scheduler_context_term(struct kbase_context *kctx)
{
	kbase_csf_event_wait_remove(kctx, check_group_sync_update_cb, kctx);
	cancel_work_sync(&kctx->csf.sched.sync_update_work);
	destroy_workqueue(kctx->csf.sched.sync_update_wq);
}

int kbase_csf_scheduler_init(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	u32 num_groups = kbdev->csf.global_iface.group_num;

	bitmap_zero(scheduler->csg_inuse_bitmap, num_groups);
	bitmap_zero(scheduler->csg_slots_idle_mask, num_groups);

	scheduler->csg_slots = kcalloc(num_groups,
				sizeof(*scheduler->csg_slots), GFP_KERNEL);
	if (!scheduler->csg_slots) {
		dev_err(kbdev->dev,
			"Failed to allocate memory for csg slot status array\n");
		return -ENOMEM;
	}

	atomic_set(&scheduler->timer_enabled, true);

	scheduler->wq = alloc_ordered_workqueue("csf_scheduler_wq", WQ_HIGHPRI);
	if (!scheduler->wq) {
		dev_err(kbdev->dev, "Failed to allocate scheduler workqueue\n");

		kfree(scheduler->csg_slots);
		scheduler->csg_slots = NULL;

		return -ENOMEM;
	}

	INIT_DEFERRABLE_WORK(&scheduler->tick_work, schedule_on_tick);
	INIT_DEFERRABLE_WORK(&scheduler->tock_work, schedule_on_tock);

	INIT_DEFERRABLE_WORK(&scheduler->ping_work, firmware_aliveness_monitor);
	BUILD_BUG_ON(GLB_REQ_WAIT_TIMEOUT_MS >= FIRMWARE_PING_INTERVAL_MS);

	mutex_init(&scheduler->lock);
	spin_lock_init(&scheduler->interrupt_lock);

	/* Internal lists */
	INIT_LIST_HEAD(&scheduler->runnable_kctxs);
	INIT_LIST_HEAD(&scheduler->groups_to_schedule);
	INIT_LIST_HEAD(&scheduler->idle_groups_to_schedule);

	BUILD_BUG_ON(MAX_SUPPORTED_CSGS >
		(sizeof(scheduler->csgs_events_enable_mask) * BITS_PER_BYTE));
	scheduler->csgs_events_enable_mask = ULONG_MAX;
	scheduler->state = SCHED_INACTIVE;
	scheduler->ngrp_to_schedule = 0;
	scheduler->total_runnable_grps = 0;
	scheduler->top_ctx = NULL;
	scheduler->top_grp = NULL;
	scheduler->last_schedule = 0;
	scheduler->tock_pending_request = false;
	scheduler->protected_mode_in_use = false;
	scheduler_doorbell_init(kbdev);

	return 0;
}

void kbase_csf_scheduler_term(struct kbase_device *kbdev)
{
	if (kbdev->csf.scheduler.csg_slots) {
		WARN_ON(csgs_active(kbdev));
		cancel_delayed_work_sync(&kbdev->csf.scheduler.ping_work);
		destroy_workqueue(kbdev->csf.scheduler.wq);
		mutex_destroy(&kbdev->csf.scheduler.lock);
		kfree(kbdev->csf.scheduler.csg_slots);
		kbdev->csf.scheduler.csg_slots = NULL;
	}
}

int kbase_csf_scheduler_firmware_reinit(struct kbase_device *kbdev)
{
	return kbase_csf_firmware_reinit(kbdev);
}

void kbase_csf_scheduler_enable_tick_timer(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;

	mutex_lock(&scheduler->lock);

	WARN_ON(scheduler->state != SCHED_INACTIVE);
	WARN_ON(delayed_work_pending(&scheduler->tick_work));

	if (scheduler->total_runnable_grps > 0) {
		mod_delayed_work(scheduler->wq, &scheduler->tick_work, 0);
		dev_dbg(kbdev->dev, "Re-enabling the scheduler timer\n");
	}

	mutex_unlock(&scheduler->lock);
}

bool kbase_csf_scheduler_timer_is_enabled(struct kbase_device const *kbdev)
{
	return atomic_read(&kbdev->csf.scheduler.timer_enabled);
}

void kbase_csf_scheduler_timer_set_enabled(struct kbase_device *kbdev,
		bool enable)
{
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	bool const currently_enabled =
		kbase_csf_scheduler_timer_is_enabled(kbdev);

	if (currently_enabled && !enable) {
		atomic_set(&scheduler->timer_enabled, false);

		cancel_delayed_work_sync(&scheduler->tick_work);
		cancel_delayed_work_sync(&scheduler->tock_work);
	} else if (!currently_enabled && enable) {
		atomic_set(&scheduler->timer_enabled, true);

		kbase_csf_scheduler_enable_tick_timer(kbdev);
	}
}

void kbase_csf_scheduler_kick(struct kbase_device *kbdev)
{
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;

	if (!unlikely(kbase_csf_scheduler_timer_is_enabled(kbdev)))
		return;

	mutex_lock(&scheduler->lock);

	if (scheduler->total_runnable_grps > 0) {
		mod_delayed_work(scheduler->wq, &scheduler->tick_work, 0);
		dev_dbg(kbdev->dev, "Kicking the scheduler manually\n");
	}

	mutex_unlock(&scheduler->lock);
}
