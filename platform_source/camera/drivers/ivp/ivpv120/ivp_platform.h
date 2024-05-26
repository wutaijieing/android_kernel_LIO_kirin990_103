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

#include <linux/regulator/consumer.h>
#include "ivp_common.h"

#define LISTENTRY_SIZE                  0x00600000
#define GIC_IRQ_CLEAR_REG               0xE82B11A4

#define IVP_MEM_CTRL3_VAL               0x02600260
#define IVP_MEM_CTRL4_VAL               0x00000260
#define IVP_MST_MID_CFG_VAL             0x00006A69

#define IVP_APB_GATE_CLOCK_VAL          0x00003FFF
#define IVP_TIMER_WDG_RST_DIS_VAL       0x0000007F

#define RST_IVP32_PROCESSOR_EN          0x02
#define RST_IVP32_DEBUG_EN              0x01
#define RST_IVP32_JTAG_EN               0x04

#define MAX_DDR_LEN                     128

struct ivp_iomem_res {
	char __iomem *cfg_base_addr;
	char __iomem *pctrl_base_addr;
	char __iomem *pericrg_base_addr;
};

struct ivp_device {
	struct ivp_common ivp_comm;
	struct ivp_iomem_res io_res;
	struct regulator *ivp_fake_regulator;
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

#endif /* _IVP_PLATFORM_H_ */
