/*
 * provide initialization of the the platform individual part
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _IVP_PLATFORM_H_
#define _IVP_PLATFORM_H_

#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/regulator/consumer.h>
#include "ivp_common.h"

#define LISTENTRY_SIZE         0x00600000

#define IVP_SEC_BUFF_SIZE      0x200000
#define IVP_SEC_SHARE_ADDR     0x11600000
#define IVP_SEC_LOG_ADDR       0x11680000

#define IVP_CLK_LEVEL_DEFAULT         0
#define IVP_CLK_LEVEL_ULTRA_LOW       1
#define IVP_CLK_LEVEL_LOW             2
#define IVP_CLK_LEVEL_MEDIUM          3
#define IVP_CLK_LEVEL_HIGH            4

#define GIC_IRQ_CLEAR_REG      0xEA0001A4
#define IVP_CORE_RD_QOS_MODE   0x000C
#define IVP_CORE_WR_QOS_MODE   0x008C
#define IVP_IDMA_RD_QOS_MODE   0x010C
#define IVP_IDMA_WR_QOS_MODE   0x018C
#define IVP_SYS_QOS_CFG_VALUE  0x30000
#define NOC_IVP_QOS_CFG_VALUE  0x02
#define V250_CS2_ID            0x36903100
#define V250_CS1_ID            0x36901100

#define READ_BACK_IVP_SYS_QOS_CFG_ERROR        (-1)
#define READ_BACK_IVP_CORE_RD_QOS_MODE_ERROR   (-2)
#define READ_BACK_IVP_CORE_WR_QOS_MODE_ERROR   (-3)
#define READ_BACK_IVP_IDMA_RD_QOS_MODE_ERROR   (-4)
#define READ_BACK_IVP_IDMA_WR_QOS_MODE_ERROR   (-5)

#define IVP_REG_OFF_MST_MID_CFG         0x0338
#define IVP_MST_MID_CFG_VAL             0x00696A74

#define IVP_APB_GATE_CLOCK_VAL          0x0000003F
#define IVP_TIMER_WDG_RST_DIS_VAL       0x00000007

#define RST_IVP32_PROCESSOR_EN          0x02
#define RST_IVP32_DEBUG_EN              0x01
#define RST_IVP32_JTAG_EN               0x04
#define IVP_DSP_PWAITMODE               0x01

#define MAX_DDR_LEN                     128
#define IVP_REMP_ADDR_SHIFT             16
#define IVP_REMP_LEN_SHIFT              8

struct ivp_iomem_res {
	char __iomem *cfg_base_addr;
	char __iomem *pctrl_base_addr;
	char __iomem *pericrg_base_addr;
	char __iomem *noc_ivp_base_addr;
};

struct ivp_sec_device {
	struct task_struct *secivp_kthread;
	wait_queue_head_t secivp_wait;
	bool secivp_wake;
	atomic_t ivp_image_success;
	struct completion load_completion;
	unsigned long ivp_sec_phymem_addr;
	bool thread_exit;
};

struct ivp_device {
	struct ivp_common ivp_comm;
	unsigned int middle_clk_rate;
	unsigned int low_clk_rate;
	unsigned int ultra_low_clk_rate;
	unsigned int lowtemp_clk_rate;
	struct ivp_iomem_res io_res;
	struct regulator *ivp_media2_regulator;
	struct ivp_sec_device *sec_dev;
};

int ivp_poweron_pri(struct ivp_device *ivp_devp);
int ivp_poweroff_pri(struct ivp_device *ivp_devp);
int ivp_change_clk(struct ivp_device *ivp_devp, unsigned int level);
int ivp_setup_clk(struct platform_device *plat_dev,
	struct ivp_device *ivp_devp);
int ivp_setup_regulator(struct platform_device *plat_dev,
	struct ivp_device *ivp_devp);
int ivp_get_memory_section(struct platform_device *plat_dev,
	struct ivp_device *ivp_devp);
void ivp_free_memory_section(struct ivp_device *ivp_devp);
int ivp_remap_addr_ivp2ddr(struct ivp_device *ivp_devp,
	unsigned int ivp_addr, int len, unsigned long ddr_addr);
int ivp_get_secure_attribute(struct platform_device *plat_dev,
	struct ivp_device *ivp_devp);

#endif /* _IVP_PLATFORM_H_ */
