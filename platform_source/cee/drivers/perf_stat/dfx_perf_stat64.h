/*
 *
 * Analyze the occupancy of ddr bandwidth
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef DFX_PERF_STAT_H
#define DFX_PERF_STAT_H

#include <asm/memory.h>
#include <asm/cacheflush.h>
#include <asm/irq_regs.h>
#include <asm/irqflags.h>
#include <linux/uaccess.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/perf_event.h>
#include <linux/bitmap.h>
#include <linux/vmalloc.h>
#include <linux/gfp.h>
#include <linux/dma-direction.h>
#include <linux/ptrace.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/pm_wakeup.h>
#include <linux/regulator/consumer.h>
#include <linux/syscalls.h>
#include <linux/workqueue.h>
#include <soc_pmctrl_interface.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_crgperiph_interface.h>
#include <soc_perfstat_interface.h>
#include <soc_pctrl_interface.h>

#define PERF_IS_ALIGNED(x, b) (((x) % (b)) == 0)

#define LIST_ADDR_BIT 16
#define LIST_ADDR_OFFSET 32

#define SET_TIME 1000
#define SET_TIME_IN 10

#define RS_DATA_NUM 2

#define CUR_ADDR_H_OFFSET 32
#define CUR_ADDR_L_BIT 0xffffffff

#define PERF_OK 0
#define PERF_ERROR (-1)
#define PERF_FILE_MASK 0060

/* PORT_INDEX offset */
#define PERF_PORT_INDEX_NUM 8
#define PORT_INDEX_ADDR_OFFSET 0x4

/* Sample port */
#define PERF_PORT_DSS0 0
#define PERF_PORT_DSS1 1
#define PERF_PORT_CCI0 2
#define PERF_PORT_CCI1 3
#define PERF_PORT_GPU 4
#define PERF_PORT_MODEM0 5
#define PERF_PORT_MODEM1 6
#define PERF_PORT_ISP0 7
#define PERF_PORT_ISP1 8
#define PERF_PORT_VDEC 9
#define PERF_PORT_VENC 10
#define PERF_PORT_IVP 11
#define PERF_PORT_SYSNOC 12
#define PERF_PORT_AUDIO 13
#define PERF_PORT_EMMC 14
#define PERF_PORT_USBOTG 15
#define PERF_PORT_UFS 16
#define PERF_PORT_PCIE 17
#define PERF_PORT_ISP_SRT_DRM 18
#define PERF_PORT_CCI3 19
#define PERF_PORT_CCI4 20
#define PERF_PORT_DMSS1 21
#define PERF_PORT_DMSS0 22
#define PERF_PORT_GPU1 23
#define PERF_PORT_IPF 24
#define PERF_PORT_DMCA 25
#define PERF_PORT_DMCB 26
#define PERF_PORT_DMCC 27
#define PERF_PORT_DMCD 28
#define PERF_PORT_RESERVE0 29
#define PERF_PORT_RESERVE1 30
#define PERF_PORT_RESERVE2 31
#define PERF_PORT_NUM 64
#define PERF_PORT_ALL (0xFFFFFFFF)
#define PERF_PORT_BIT(x) (1UL << (x))
#define PERF_PORT_L_MASK (0xFFFFFFFF)
#define PERF_PORT_H_MASK (0xFFFFFFFF00000000)
#define PERF_PORT_64BIT (0x3FFFFFFFFFFFFF)
#define PERF_PORT_32BIT (0xFFFFFFFF)
#define SAMPLE_TIME_RANGE 16

/* perfstat irq */
#define PERF_INT_SAMP_DONE (0x1 << 0)
#define PERF_INT_OVER_TIME (0x1 << 1)
#define PERF_INT_LIST_DONE (0x1 << 2)
#define PERF_INT_LIST_ERR (0x1 << 3)
#define PERF_INT_ALL (0x0F)

#define CCI_CLK_GATE_ENABLE_POSITION (0x1 << 28)
#define DMC_CLK_GATE_ENABLE_POSITION (0x1 << 30)

/* sample addr mode */
#ifdef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
#define PERF_CLK_GATE_EN 1
#endif

#define PERF_SEQ_ADDR_MODE 0
#define PERF_LIST_ADDR_MODE 1

#define PERF_MIN_PAGE_NUM 2

#define PERICRG_SIZE (0x2000)

#define PERF_INIT_DONE 1
#define PERF_ADD_DONE 2
#define PERF_DEL_DONE 3
#define PERF_TASK_DONE 0

#define PERF_LIST_CFG_NORMAL 0x21ULL
#define PERF_LIST_CFG_INT 0x25ULL
#define PERF_LIST_CFG_LINK 0x31ULL

#define PERF_SAMPLE_HIGHSPEED 1
#define PERF_SAMPLE_HSCYCLE 2
#define PERF_MALLOC_PAGES_100M 25600
#define PERF_SIZE_1M (1024*1024)
#define PERF_TIME_1US 1000000
#define PERF_TIME_1MS 1000
#ifdef CONFIG_DFX_MNTN_PC
#define PERF_HIGHSPEED_DATA_PATH "/var/log/hisi/perf_data_hs.data"
#else
#define PERF_HIGHSPEED_DATA_PATH "/data/perf_data_hs.data"
#endif
#define PERF_MILLION 1000000

#define PERF_SUGGEST_CLK 120000000
#define PERF_SUGGEST_ACLK 228000000

#define PERF_CHECK_READY_TIMEOUT 200
#define SAMPLE_PERIOD_MASK (0xffffffff)
#define SAMPLE_TIME_MASK (0xffff)

#ifdef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
#define MAX_PORT_NUM 32
#else
#define MAX_PORT_NUM (0x1 << 30)
#endif

#define SAMPLE_DATA_SIZE_SEQ 0x4000 /* 16K */
#define LIST_DATA_SIZE 0x200
#define LIST_NUM 0x2 /* just test, only 2 */
#define LIST_CFG_NORMAL 0x25ULL
#define LIST_CFG_LINK 0x35ULL
#define LIST_CFG_END_ERROR 0x22ULL /* list end bit set to 1 */
#define LIST_CFG_VALID_ERROR 0x20ULL /* list valid bit set to 0 */

/* rs support */
#define RSDATA_MAX 16
#define GET_ENABLE_NUM 2

/* QOS support */
#define RF_STAT_QOS_PRIORI (0x0288)
#define RF_STAT_QOS_PRIORI_VALUE (0x0303)

#define PERF_STAT_QOS_MODE (0x028C)
#define PERF_STAT_QOS_MODE_VALUE (0x0)

#define PERF_STAT_ICS2_PORT_SEL 1
#define PERF_STAT_DP_PORT_SEL 2

#define NSECS_IN_SEC 1000000000ull /* ns */

#define PERF_SPRT_32 (0x0)
#define PERF_SPRT_64 (0x1)

#define POS_OFFSET_4 4
#define POS_OFFSET_6 6

#define SAMPLE_START_OFFSET 32

#define MODE_OFFSET (0x8000)
#define PERF_SAMPLE_NUM_CYCLE 0u

#define OFFSET_16 16
#define OFFSET_32 32
#define OFFSET_48 48
#define BIT_SIZE 8
#define EVENT_ID_BIT (0xffff)
#define EVENT_BASE_BIT (0x0000ffff)
#define SAMPLE_PERIOD_BIT (0xffffffff)
#define ALLOCATE_BIT (0x0000ffff)

#ifndef SOC_PERFSTAT_REAL_PORT_NUM_L_ADDR
	#define DRV_SOC_PERFSTAT_REAL_PORT_NUM_H_ADDR(base) (NULL)
#else
	#define SOC_PERFSTAT_REAL_PORT_NUM_ADDR(base)  SOC_PERFSTAT_REAL_PORT_NUM_L_ADDR(base)
	#define DRV_SOC_PERFSTAT_REAL_PORT_NUM_H_ADDR(base)  SOC_PERFSTAT_REAL_PORT_NUM_H_ADDR(base)
#endif

#ifndef SOC_PERFSTAT_REAL_PORT_NUM_2_ADDR
	#define DRV_SOC_PERFSTAT_REAL_PORT_NUM_3_ADDR(base) (NULL)
#else
	#define SOC_PERFSTAT_REAL_PORT_NUM2_ADDR(base)  SOC_PERFSTAT_REAL_PORT_NUM_2_ADDR(base)
	#define DRV_SOC_PERFSTAT_REAL_PORT_NUM_3_ADDR(base)  SOC_PERFSTAT_REAL_PORT_NUM_3_ADDR(base)
#endif

#if (defined SOC_PERFSTAT_REAL_PORT_NUM2_ADDR) || (defined SOC_PERFSTAT_MAX_PORT_NUM_ADDR)
	#define PERFSTAT_PORT_CAPACITY_128
#endif


#ifdef PERFSTAT_PORT_CAPACITY_128
/* perf sample_data_head struct */
struct perf_sample_data_head {
	u64 sample_cnt_current;
	u64 sample_period;
	u64 sample_cnt_legacy;
	u64 sample_port_low;
	u64 sample_port_high;
	u64 Reserve1;
	u64 Reserve2;
	u64 Reserve3;
};

struct perf_sample_data_end {
	u64 sample_cnt_current;
	u64 sample_pdrst_l;
	u64 sample_pdrst_h;
	u64 sample_clkgt_l;
	u64 sample_clkgt_h;
	u64 sample_port_low;
	u64 sample_port_high;
	u64 Reserve;
};
#else
struct perf_sample_data_head {
	u64 sample_cnt_current;
	u64 sample_period;
	u64 sample_cnt_legacy;
	u64 sample_port;
};

struct perf_sample_data_end {
	u64 sample_cnt_current;
	u64 sample_period;
	u64 sample_data_valid;
	u64 sample_port;
};
#endif

/* perf perf_list_descriptor struct */
/*
 * bit[63:32] | bit[31:10] |  bit[9:6]  | bit5 | bit4 | bit3 | bit2 | bit1 | bit0
 * address   |  length    |   reserve  | act2 | act1 |  NA  | int  | end  | valid
 * --------------------------------------------------------------------------------
 * valid : valid = 1 indicates this line of descriptor is effective,
 * if valid = 0 generates error interrupt.
 * end  : end = 1 indicates to end of descriptors, transfer completed
 * interrupt is generated when operation of this descriptor is completed.
 * int  : int = 1 generates DMA interrupt when operation of this descriptor
 * is completed.
 *
 * ---------------------------------------------------------------------------------
 * act2 | act1 | sympol |    comment    |               operation
 * ---------------------------------------------------------------------------------
 * 0   |  0   |  Nop   | No operation  | Do not excute current line and go to next
 * ---------------------------------------------------------------------------------
 * 0   |  1   |  Rsv   |    Reserve    | Do not excute current line and go to next
 * ---------------------------------------------------------------------------------
 * 1   |  0   |  Tran  | Transfer Data | Transfer data of one descriptor line
 * --------------------------------------------------------------------------------
 * 1   |  1   |  Link  |Link Descriptor| Link to another descriptor
 * --------------------------------------------------------------------------------
 * length  : transfer data size of this descriptor line
 * address : when sympol = Tran, it indicates the start address of transfer data
 * when sympol = Link, it indicates the address of the link descriptor
 *
 */
typedef union {
	struct {
		u64 value0;
		u64 value1;
	} value;

	struct {
		u64 valid : 1;
		u64 end : 1;
		u64 interrupt : 1;
		u64 reserve0 : 1;
		u64 act : 2;
		u64 reserve1 : 4;
		u64 length : 22;
		u64 reserve2 : 32;
		u64 address;
	} bit_config;
} perf_list_descriptor;

struct perfstat_dev_info {
	struct resource *res;
	struct perf_event *event_bk; /* backup when user creat a perf_event, needed when overflow called */
	struct perfstat_clk_data *clk_data;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	struct wakeup_source wakelock;
#else
	struct wakeup_source *wakelock;
#endif
	spinlock_t synclock; /* sync perf_interrupt and perf_del */

	int status; /* perf_status flag, when it is set rest, interrupt returns directly */
	int irq;
	unsigned long clk_perf_stat_rate;
	unsigned long aclk_perf_stat_rate;
/* perf attibute get from dts
 * event_id    : perf event_id, when a evnet is initialized,
 * we deal with the perf_event only if the evend_id matches ours.
 * per_data_sz : perf per data size,
 * perf_add use it to count the per_size,
 * per_int_size  : when sampling, we should notice the user to copy the data,
 * so every per_int_size we send a notice, it means we should
 * config perf to generate a (list-done)interrupt every per_int_size
 */
	unsigned int event_id;
	unsigned int per_data_sz;
	int per_int_size;
	int sample_per_sz;
	int sample_port_num;
	unsigned long sprt;
	u64 vldmsk_of_sprt;
	u64 vldmsk_of_sprt_h;
	unsigned int rs_enable_support;
	int samp_type; /* HIGHSPEED : 1, NORMALSPEED : 0 */
	struct delayed_work hs_record_data_work; /* Record Data work in HighSpeed Mode */

/* We report the sample data in each (list-done)interrupt,
 * of course we should know the data start address & size.
 * The fisrt start address of data is virt_addr, once a interrupt
 * comes, we report PAGE_SIZE*pages_u data, then the cur_page_n += pages_u,
 * so we could know the next data start address according to the cur_page_n,
 * however, when cur_page_n beyonds the total page numbers(pages_n),
 * it becomes zero again.
 */
	int pages_n;
	void *virt_addr;

/* perf generates interrupt by analyzing the perf_list_descriptors,
 * we config a serial of descriptors in a continuous address, and
 * tell perf the first descriptor's address
 */
	perf_list_descriptor *list;
	struct page **pages;
	unsigned int rs_data[RSDATA_MAX];
};

struct perfstat_clk_data {
	int num;
	const char **name;
	struct clk **perfstat_clk;
};

#ifdef CONFIG_DFX_PERF_STAT_LAT_MON
/* Latency monitor */
#define SUBSYS_PERIOD_MAX 31
#define SUBSYS_PERIOD_MIN 1
#define UNREGISTER_DEF_PERIOD 2
#define SUBSYS_NAME_LEN 48
#define SAMPLE_PERIOD_MIN 2

typedef int (*perfstat_get_sub_period_cb) (void);

struct perfstat_subsystem_info {
	int period;
	bool is_valid; /* whether or not subsystem is registered, except VDEC/VENC/DMSS */
	perfstat_get_sub_period_cb cb;
	unsigned int read;
	unsigned int write;
	char name[SUBSYS_NAME_LEN];
};

/* interface for subsystem registering period get callback */
unsigned int perfstat_latency_monitor_register(unsigned int id, perfstat_get_sub_period_cb cb);
#endif
#endif
