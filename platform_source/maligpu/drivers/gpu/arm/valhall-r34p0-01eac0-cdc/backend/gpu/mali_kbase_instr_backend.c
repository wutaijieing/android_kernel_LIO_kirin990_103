// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2014-2021 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
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
 */

/*
 * GPU backend instrumentation APIs.
 */

#include <mali_kbase.h>
#include <gpu/mali_kbase_gpu_regmap.h>
#include <mali_kbase_hwaccess_instr.h>
#include <device/mali_kbase_device.h>
#include <backend/gpu/mali_kbase_instr_internal.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <mali_kbase_regs_history_debugfs.h>


int kbase_instr_hwcnt_enable_internal(struct kbase_device *kbdev,
					struct kbase_context *kctx,
					struct kbase_instr_hwcnt_enable *enable)
{
	unsigned long flags;
	int err = -EINVAL;
	u32 irq_mask;
	u32 prfcnt_config;

	lockdep_assert_held(&kbdev->hwaccess_lock);

	/* alignment failure */
	if ((enable->dump_buffer == 0ULL) || (enable->dump_buffer & (2048 - 1)))
		goto out_err;

	spin_lock_irqsave(&kbdev->hwcnt.lock, flags);

	if (kbdev->hwcnt.backend.state != KBASE_INSTR_STATE_DISABLED) {
		/* Instrumentation is already enabled */
		spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);
		goto out_err;
	}

	if (kbase_is_gpu_removed(kbdev)) {
		/* GPU has been removed by Arbiter */
		spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);
		goto out_err;
	}

	/* Enable interrupt */
	irq_mask = kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK));
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK), irq_mask |
						PRFCNT_SAMPLE_COMPLETED);

	/* In use, this context is the owner */
	kbdev->hwcnt.kctx = kctx;
	/* Remember the dump address so we can reprogram it later */
	kbdev->hwcnt.addr = enable->dump_buffer;
	kbdev->hwcnt.addr_bytes = enable->dump_buffer_bytes;

	spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);

	/* Configure */
	prfcnt_config = kctx->as_nr << PRFCNT_CONFIG_AS_SHIFT;
#ifdef CONFIG_MALI_PRFCNT_SET_SELECT_VIA_DEBUG_FS
	prfcnt_config |= kbdev->hwcnt.backend.override_counter_set
			 << PRFCNT_CONFIG_SETSELECT_SHIFT;
#else
	prfcnt_config |= enable->counter_set << PRFCNT_CONFIG_SETSELECT_SHIFT;
#endif

	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_CONFIG),
			prfcnt_config | PRFCNT_CONFIG_MODE_OFF);

	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_BASE_LO),
					enable->dump_buffer & 0xFFFFFFFF);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_BASE_HI),
					enable->dump_buffer >> 32);

	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_JM_EN),
					enable->fe_bm);

	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_SHADER_EN),
					enable->shader_bm);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_MMU_L2_EN),
					enable->mmu_l2_bm);

	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_TILER_EN),
					enable->tiler_bm);

	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_CONFIG),
			prfcnt_config | PRFCNT_CONFIG_MODE_MANUAL);

	spin_lock_irqsave(&kbdev->hwcnt.lock, flags);

	kbdev->hwcnt.backend.state = KBASE_INSTR_STATE_IDLE;
	kbdev->hwcnt.backend.triggered = 1;
	wake_up(&kbdev->hwcnt.backend.wait);

	spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);

	err = 0;

	dev_dbg(kbdev->dev, "HW counters dumping set-up for context %pK", kctx);
	return err;
 out_err:
	return err;
}

static void kbase_instr_hwcnt_dump_info(struct kbase_device *kbdev)
{
	int i;

	dev_warn(kbdev->dev, "HWC Dump Timed out Dump Register");
	if (kbdev->pm.backend.gpu_powered) {
		dev_err(kbdev->dev, "l2_trans 0x%08x l2_ready 0x%08x tiler_trans 0x%08x \
			tiler_ready 0x%08x l2_present 0x%08x tiler_present 0x%08x",
			(unsigned int)kbase_pm_get_trans_cores(kbdev,KBASE_PM_CORE_L2),
			(unsigned int)kbase_pm_get_ready_cores(kbdev, KBASE_PM_CORE_L2),
			(unsigned int)kbase_pm_get_trans_cores(kbdev,KBASE_PM_CORE_TILER),
			(unsigned int)kbase_pm_get_ready_cores(kbdev,KBASE_PM_CORE_TILER),
			(unsigned int)kbdev->gpu_props.props.raw_props.l2_present,
			(unsigned int)kbdev->gpu_props.props.raw_props.tiler_present);
		dev_err(kbdev->dev, "shaders_trans 0x%8x shaders_ready 0x%8x",
			(unsigned int)kbase_pm_get_trans_cores(kbdev, KBASE_PM_CORE_SHADER),
			(unsigned int)kbase_pm_get_ready_cores(kbdev, KBASE_PM_CORE_SHADER));

		/* HWC config related register dump */
		dev_err(kbdev->dev, "perf config = 0x%08x, perf_jm_en = 0x%08x",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(PRFCNT_CONFIG)),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(PRFCNT_JM_EN)));
		dev_err(kbdev->dev, "perf base low = 0x%08x,hi = 0x%08x perf_shader_en = 0x%08x",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(PRFCNT_BASE_LO)),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(PRFCNT_BASE_HI)),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(PRFCNT_SHADER_EN)));
		dev_err(kbdev->dev, "perf tiler = 0x%08x, perf_l2_en = 0x%08x",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(PRFCNT_TILER_EN)),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(PRFCNT_MMU_L2_EN)));

		dev_err(kbdev->dev, "Register state:");
		dev_err(kbdev->dev, "GPU_IRQ_RAWSTAT=0x%08x GPU_STATUS=0x%08x",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_RAWSTAT)),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_STATUS)));
		dev_err(kbdev->dev, "JOB_IRQ_RAWSTAT=0x%08x JOB_IRQ_JS_STATE=0x%08x",
			kbase_reg_read(kbdev, JOB_CONTROL_REG(JOB_IRQ_RAWSTAT)),
			kbase_reg_read(kbdev, JOB_CONTROL_REG(JOB_IRQ_JS_STATE)));
		for (i = 0; i < 3; i++)
			dev_err(kbdev->dev, "JS%d_STATUS=0x%08x JS%d_HEAD_LO=0x%08x",
				i, kbase_reg_read(kbdev, JOB_SLOT_REG(i, JS_STATUS)),
				i, kbase_reg_read(kbdev, JOB_SLOT_REG(i, JS_HEAD_LO)));

		dev_err(kbdev->dev, "MMU_IRQ_RAWSTAT=0x%08x GPU_FAULTSTATUS=0x%08x",
			kbase_reg_read(kbdev, MMU_REG(MMU_IRQ_RAWSTAT)),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_FAULTSTATUS)));
		dev_err(kbdev->dev, "GPU_IRQ_MASK=0x%08x JOB_IRQ_MASK=0x%08x MMU_IRQ_MASK=0x%08x",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK)),
			kbase_reg_read(kbdev, JOB_CONTROL_REG(JOB_IRQ_MASK)),
			kbase_reg_read(kbdev, MMU_REG(MMU_IRQ_MASK)));
		dev_err(kbdev->dev, "PWR_OVERRIDE0=0x%08x PWR_OVERRIDE1=0x%08x",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(PWR_OVERRIDE0)),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(PWR_OVERRIDE1)));
		dev_err(kbdev->dev, "SHADER_CONFIG=0x%08x L2_MMU_CONFIG=0x%08x",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(SHADER_CONFIG)),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(L2_MMU_CONFIG)));
		dev_err(kbdev->dev, "TILER_CONFIG=0x%08x  JM_CONFIG=0x%08x",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(TILER_CONFIG)),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(JM_CONFIG)));
		/* IO history */
		kbase_io_history_dump(kbdev);
	}
}

int kbase_instr_hwcnt_disable_internal(struct kbase_context *kctx)
{
	unsigned long flags, pm_flags;
	int err = -EINVAL;
	u32 irq_mask;
	struct kbase_device *kbdev = kctx->kbdev;
	long remaining = 100;

	while (1) {
		spin_lock_irqsave(&kbdev->hwaccess_lock, pm_flags);
		spin_lock_irqsave(&kbdev->hwcnt.lock, flags);

		if (kbdev->hwcnt.backend.state == KBASE_INSTR_STATE_DISABLED) {
			/* Instrumentation is not enabled */
			spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);
			spin_unlock_irqrestore(&kbdev->hwaccess_lock, pm_flags);
			goto out;
		}

		if (kbdev->hwcnt.kctx != kctx) {
			/* Instrumentation has been setup for another context */
			spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);
			spin_unlock_irqrestore(&kbdev->hwaccess_lock, pm_flags);
			goto out;
		}

		if (kbdev->hwcnt.backend.state == KBASE_INSTR_STATE_IDLE)
			break;
		if (!remaining) {
			kbase_instr_hwcnt_dump_info(kbdev);
			break;
		}

		spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, pm_flags);

		/* Ongoing dump/setup - wait for its completion */
		remaining = wait_event_timeout(kbdev->hwcnt.backend.wait,
					kbdev->hwcnt.backend.triggered != 0,
					msecs_to_jiffies(100));
	}

	kbdev->hwcnt.backend.state = KBASE_INSTR_STATE_DISABLED;
	kbdev->hwcnt.backend.triggered = 0;

	if (kbase_is_gpu_removed(kbdev)) {
		/* GPU has been removed by Arbiter */
		spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, pm_flags);
		err = 0;
		goto out;
	}

	/* Disable interrupt */
	irq_mask = kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK));
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK),
				irq_mask & ~PRFCNT_SAMPLE_COMPLETED);

	/* Disable the counters */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_CONFIG), 0);

	kbdev->hwcnt.kctx = NULL;
	kbdev->hwcnt.addr = 0ULL;
	kbdev->hwcnt.addr_bytes = 0ULL;

	spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, pm_flags);

	dev_dbg(kbdev->dev, "HW counters dumping disabled for context %pK",
									kctx);

	err = 0;
 out:
	return err;
}

int kbase_instr_hwcnt_request_dump(struct kbase_context *kctx)
{
	unsigned long flags;
	int err = -EINVAL;
	struct kbase_device *kbdev = kctx->kbdev;

	spin_lock_irqsave(&kbdev->hwcnt.lock, flags);

	if (kbdev->hwcnt.kctx != kctx) {
		/* The instrumentation has been setup for another context */
		goto unlock;
	}

	if (kbdev->hwcnt.backend.state != KBASE_INSTR_STATE_IDLE) {
		/* HW counters are disabled or another dump is ongoing, or we're
		 * resetting
		 */
		goto unlock;
	}

	if (kbase_is_gpu_removed(kbdev)) {
		/* GPU has been removed by Arbiter */
		goto unlock;
	}

	kbdev->hwcnt.backend.triggered = 0;

	/* Mark that we're dumping - the PF handler can signal that we faulted
	 */
	kbdev->hwcnt.backend.state = KBASE_INSTR_STATE_DUMPING;

	/* Reconfigure the dump address */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_BASE_LO),
					kbdev->hwcnt.addr & 0xFFFFFFFF);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(PRFCNT_BASE_HI),
					kbdev->hwcnt.addr >> 32);

	/* Start dumping */
	KBASE_KTRACE_ADD(kbdev, CORE_GPU_PRFCNT_SAMPLE, NULL,
			kbdev->hwcnt.addr);

	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND),
					GPU_COMMAND_PRFCNT_SAMPLE);

	dev_dbg(kbdev->dev, "HW counters dumping done for context %pK", kctx);

	err = 0;

 unlock:
	spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);

	return err;
}
KBASE_EXPORT_SYMBOL(kbase_instr_hwcnt_request_dump);

bool kbase_instr_hwcnt_dump_complete(struct kbase_context *kctx,
						bool * const success)
{
	unsigned long flags;
	bool complete = false;
	struct kbase_device *kbdev = kctx->kbdev;

	spin_lock_irqsave(&kbdev->hwcnt.lock, flags);

	if (kbdev->hwcnt.backend.state == KBASE_INSTR_STATE_IDLE) {
		*success = true;
		complete = true;
	} else if (kbdev->hwcnt.backend.state == KBASE_INSTR_STATE_FAULT) {
		*success = false;
		complete = true;
		kbdev->hwcnt.backend.state = KBASE_INSTR_STATE_IDLE;
	}

	spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);

	return complete;
}
KBASE_EXPORT_SYMBOL(kbase_instr_hwcnt_dump_complete);

void kbase_instr_hwcnt_sample_done(struct kbase_device *kbdev)
{
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwcnt.lock, flags);

	if (kbdev->hwcnt.backend.state == KBASE_INSTR_STATE_FAULT) {
		kbdev->hwcnt.backend.triggered = 1;
		wake_up(&kbdev->hwcnt.backend.wait);
	} else if (kbdev->hwcnt.backend.state == KBASE_INSTR_STATE_DUMPING) {
		/* All finished and idle */
		kbdev->hwcnt.backend.state = KBASE_INSTR_STATE_IDLE;
		kbdev->hwcnt.backend.triggered = 1;
		wake_up(&kbdev->hwcnt.backend.wait);
	}

	spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);
}

int kbase_instr_hwcnt_wait_for_dump(struct kbase_context *kctx)
{
	struct kbase_device *kbdev = kctx->kbdev;
	unsigned long flags, pm_flags;
	int err;
	long remaining = 0;

	/* Wait for dump & cache clean to complete, add timeout 2s */
	remaining = wait_event_timeout(kbdev->hwcnt.backend.wait,
					kbdev->hwcnt.backend.triggered != 0,
					msecs_to_jiffies(100));
	if (!remaining) {
		/* dump GPU register */
		spin_lock_irqsave(&kbdev->hwaccess_lock, pm_flags);
		kbase_instr_hwcnt_dump_info(kbdev);
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, pm_flags);
	}

	spin_lock_irqsave(&kbdev->hwcnt.lock, flags);
	if (!remaining)
		/* Set state to fault state */
		kbdev->hwcnt.backend.state = KBASE_INSTR_STATE_FAULT;

	if (kbdev->hwcnt.backend.state == KBASE_INSTR_STATE_FAULT) {
		err = -EINVAL;
		kbdev->hwcnt.backend.state = KBASE_INSTR_STATE_IDLE;
	} else {
		/* Dump done */
		KBASE_DEBUG_ASSERT(kbdev->hwcnt.backend.state ==
							KBASE_INSTR_STATE_IDLE);
		err = 0;
	}

	spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);

	return err;
}

int kbase_instr_hwcnt_clear(struct kbase_context *kctx)
{
	unsigned long flags;
	int err = -EINVAL;
	struct kbase_device *kbdev = kctx->kbdev;

	spin_lock_irqsave(&kbdev->hwcnt.lock, flags);

	/* Check it's the context previously set up and we're not already
	 * dumping
	 */
	if (kbdev->hwcnt.kctx != kctx || kbdev->hwcnt.backend.state !=
							KBASE_INSTR_STATE_IDLE)
		goto out;

	if (kbase_is_gpu_removed(kbdev)) {
		/* GPU has been removed by Arbiter */
		goto out;
	}

	/* Clear the counters */
	KBASE_KTRACE_ADD(kbdev, CORE_GPU_PRFCNT_CLEAR, NULL, 0);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND),
						GPU_COMMAND_PRFCNT_CLEAR);

	err = 0;

out:
	spin_unlock_irqrestore(&kbdev->hwcnt.lock, flags);
	return err;
}
KBASE_EXPORT_SYMBOL(kbase_instr_hwcnt_clear);

int kbase_instr_backend_init(struct kbase_device *kbdev)
{
	spin_lock_init(&kbdev->hwcnt.lock);

	kbdev->hwcnt.backend.state = KBASE_INSTR_STATE_DISABLED;

	init_waitqueue_head(&kbdev->hwcnt.backend.wait);

	kbdev->hwcnt.backend.triggered = 0;

#ifdef CONFIG_MALI_PRFCNT_SET_SELECT_VIA_DEBUG_FS
/* Use the build time option for the override default. */
#if defined(CONFIG_MALI_PRFCNT_SET_SECONDARY)
	kbdev->hwcnt.backend.override_counter_set = KBASE_HWCNT_SET_SECONDARY;
#elif defined(CONFIG_MALI_PRFCNT_SET_TERTIARY)
	kbdev->hwcnt.backend.override_counter_set = KBASE_HWCNT_SET_TERTIARY;
#else
	/* Default to primary */
	kbdev->hwcnt.backend.override_counter_set = KBASE_HWCNT_SET_PRIMARY;
#endif
#endif
	return 0;
}

void kbase_instr_backend_term(struct kbase_device *kbdev)
{
	CSTD_UNUSED(kbdev);
}

#ifdef CONFIG_MALI_PRFCNT_SET_SELECT_VIA_DEBUG_FS
void kbase_instr_backend_debugfs_init(struct kbase_device *kbdev)
{
	/* No validation is done on the debugfs input. Invalid input could cause
	 * performance counter errors. This is acceptable since this is a debug
	 * only feature and users should know what they are doing.
	 *
	 * Valid inputs are the values accepted bythe SET_SELECT bits of the
	 * PRFCNT_CONFIG register as defined in the architecture specification.
	*/
	debugfs_create_u8("hwcnt_set_select", S_IRUGO | S_IWUSR,
			  kbdev->mali_debugfs_directory,
			  (u8 *)&kbdev->hwcnt.backend.override_counter_set);
}
#endif
