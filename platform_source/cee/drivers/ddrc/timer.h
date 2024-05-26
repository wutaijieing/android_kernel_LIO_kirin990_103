#ifndef __HISI_TIMER_H__
#define __HISI_TIMER_H__

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

void hisi_bw_timer_set_time(u32 time, struct ddrc_flux_device* g_dfdev, int *g_stop);
void hisi_bw_timer_init_config(struct ddrc_flux_device* g_dfdev);
void hisi_bw_timer_disable(struct ddrc_flux_device* g_dfdev, int *g_stop);
void hisi_bw_timer_clr_interrput(struct ddrc_flux_device* g_dfdev);
u32 hisi_bw_timer_interrput_status(struct ddrc_flux_device* g_dfdev);

#endif