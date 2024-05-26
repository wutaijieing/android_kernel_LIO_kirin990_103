#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/bitops.h>
#include <linux/dma-mapping.h>
#include <linux/sched/clock.h>
#include <soc_dmss_interface.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_ddrc_dmc_interface.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/arm-smccc.h>
#include <global_ddr_map.h>
#include <platform_include/display/linux/dpu_drmdriver.h>
#include "hisi_ddr_ddrcflux_kirin.h"

#define SET_TIMER_INTCLR		1

void hisi_bw_timer_set_time(u32 time, struct ddrc_flux_device* g_dfdev, int *g_stop)
{
	unsigned long ctrl, flags;

	spin_lock_irqsave(&g_dfdev->ddrc_flux_timer->lock, flags);

	ctrl = readl(g_dfdev->ddrc_flux_timer->base + TIMER_CTRL_OFFSET);
	/* first disable timer */
	ctrl &= ~TIMER_CTRL_ENABLE;
	ddrflux_dbg("ctrl=%ld,TIMER_CTRL_ENABLE=%d", ctrl, TIMER_CTRL_ENABLE);
	writel(ctrl, g_dfdev->ddrc_flux_timer->base + TIMER_CTRL_OFFSET);
	/* set TIME_LOAD register together */
	writel(time, g_dfdev->ddrc_flux_timer->base + TIMER_LOAD_OFFSET);
	writel(time, g_dfdev->ddrc_flux_timer->base + TIMER_BGLOAD_OFFSET);
	ddrflux_dbg("time=%d", time);
	/* then enable timer again */
	ctrl |= (TIMER_CTRL_ENABLE | TIMER_CTRL_IE);
	writel(ctrl, g_dfdev->ddrc_flux_timer->base + TIMER_CTRL_OFFSET);
	*g_stop = 0;
	spin_unlock_irqrestore(&g_dfdev->ddrc_flux_timer->lock, flags);
}

/*
 * func: do timer initializtion
 * set timer clk to 4.8MHz
 * if mode is periodic and period is not the same as timer value,
 * set bgload value
 * set timer control: mode, 32bits, no prescale, interrupt enable, timer enable
 */
void hisi_bw_timer_init_config(struct ddrc_flux_device* g_dfdev)
{
	unsigned long flags, ctrl;

	spin_lock_irqsave(&g_dfdev->ddrc_flux_timer->lock, flags);
	/*
	 * 1. timer select 4.8M
	 * 2. clear the interrupt
	 * 3. set timer5 control reg: 32bit, interrupt disable, timer_value,
	 * oneshot mode and disable wakeup_timer
	 */
	writel(SET_TIMER_INTCLR,
	       g_dfdev->ddrc_flux_timer->base + TIMER_INTCLR_OFFSET);

	ctrl = TIMER_CTRL_32BIT;
	writel(BW_TIMER_DEFAULT_LOAD,
	       g_dfdev->ddrc_flux_timer->base + TIMER_LOAD_OFFSET);
	ctrl |= TIMER_CTRL_PERIODIC;
	writel(ctrl, g_dfdev->ddrc_flux_timer->base + TIMER_CTRL_OFFSET);

	spin_unlock_irqrestore(&g_dfdev->ddrc_flux_timer->lock, flags);
}

void hisi_bw_timer_disable(struct ddrc_flux_device* g_dfdev, int *g_stop)
{
	unsigned long ctrl, flags;

	spin_lock_irqsave(&g_dfdev->ddrc_flux_timer->lock, flags);
	ctrl = readl(g_dfdev->ddrc_flux_timer->base + TIMER_CTRL_OFFSET);
	ctrl &= ~(TIMER_CTRL_ENABLE | TIMER_CTRL_IE);
	writel(ctrl, g_dfdev->ddrc_flux_timer->base + TIMER_CTRL_OFFSET);
	*g_stop = 1;
	spin_unlock_irqrestore(&g_dfdev->ddrc_flux_timer->lock, flags);
}

void hisi_bw_timer_clr_interrput(struct ddrc_flux_device* g_dfdev)
{
	writel(1, g_dfdev->ddrc_flux_timer->base + TIMER_INTCLR_OFFSET);
}

u32 hisi_bw_timer_interrput_status(struct ddrc_flux_device* g_dfdev)
{
	u32 status;
	status = (readl(g_dfdev->ddrc_flux_timer->base + TIMER_RIS_OFFSET) & 0x1);
	return status;
}

