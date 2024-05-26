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
#include "soc_acpu_baseaddr_interface.h"

#define LISTENTRY_SIZE                               0x00600000

#define IVP_SEC_BUFF_SIZE                            0x200000
#define IVP_SEC_SHARE_ADDR                           0x11600000
#define IVP_SEC_LOG_ADDR                             0x11680000

#define IVP_CLK_LEVEL_DEFAULT                        0
#define IVP_CLK_LEVEL_ULTRA_LOW                      1
#define IVP_CLK_LEVEL_LOW                            2
#define IVP_CLK_LEVEL_MEDIUM                         3
#define IVP_CLK_LEVEL_HIGH                           4

#define IVP_SYS_QOS_CFG_VALUE                        0x30000

#define READ_BACK_IVP_SYS_QOS_CFG_ERROR              (-1)

#define IVP_POWEROFF_TRANS_CLK_RATE                  238000000

#define IVP_APB_GATE_CLOCK_VAL                       0x0000003F
#define IVP_TIMER_WDG_RST_DIS_VAL                    0x00000007

#define RST_IVP32_PROCESSOR_EN                       0x02
#define RST_IVP32_DEBUG_EN                           0x01
#define RST_IVP32_JTAG_EN                            0x04
#define IVP_DSP_PWAITMODE                            0x01

#define MAX_DDR_LEN                                  128
#define IVP_ADDR_SHIFT                               16
#define IVP_REMP_LEN_SHIFT                           8

#define SIZE_512K                                    (512 * 1024)
#define SIZE_1M                                      (1024 * 1024)
#define SIZE_2M                                      0x200000

#define IVP_ALGO_NODE_MAX                            2
#define IVP_DYNAMIC_CORE_NAME                        "ivp_core.elf"

#define IVP_ALGO_MAX_SIZE            (3 * SIZE_1MB)
#define IVP_IRAM_TEXT_SEGMENT_OFFSET 0x2000
#define IVP_DRAM0_BSS_SEGMENT_OFFSET 0x2400
#define IVP_IRAM_TEXT_SEGMENT_ADDR   (SOC_ACPU_IVP32_IRAM_BASE_ADDR + IVP_IRAM_TEXT_SEGMENT_OFFSET)
#define IVP_DRAM0_DATA_SEGMENT_ADDR  SOC_ACPU_IVP32_DRAM0_BASE_ADDR
#define IVP_DRAM0_BSS_SEGMENT_ADDR   (SOC_ACPU_IVP32_DRAM0_BASE_ADDR + IVP_DRAM0_BSS_SEGMENT_OFFSET)

#define ALGO_DDR_NODE_SIZE           (25 * 1024)
#define ALGO_DDR_RECORD_MAX          (IVP_ALGO_MAX_SIZE / ALGO_DDR_NODE_SIZE)
#define ALGO_IRAM_RECORD_MAX         (32 * IVP_ALGO_NODE_MAX)  /* 8K * 2 */
#define ALGO_DRAM_DATA_RECORD_MAX    (18 * IVP_ALGO_NODE_MAX)  /* 4.5K * 2 */
#define ALGO_DRAM_BSS_RECORD_MAX     (10 * IVP_ALGO_NODE_MAX)  /* 2.5K * 2 */
#define ALGO_MEM_NODE_SIZE           256

struct ivp_iomem_res {
	char __iomem *cfg_base_addr;
	char __iomem *pctrl_base_addr;
	char __iomem *pericrg_base_addr;
	char __iomem *noc_ivp_base_addr;
};

enum MULTIPLE_ALGO_SECTION_INDEX {
	DRAM0_SECT_INDEX = 0,
	DRAM1_SECT_INDEX,
	IRAM_SECT_INDEX,
	BASE_SECT_INDEX,
	ALGO_SECT_INDEX,
	SHARE_SECT_INDEX,
	LOG_SECT_INDEX,
	IVP_SECT_MAX
};

enum IVP_CORE_STATUS {
	INVALID = 0,    /* power off status */
	FREE = 1,       /* power on but not load core */
	WORK = 2        /* power on and loaded core */
};

struct ivp_algo_mem_info {
	int occupied;
	unsigned int algo_start_addr;
	unsigned int algo_rel_addr;
	unsigned int algo_rel_count;
	unsigned int algo_ddr_addr;   /* algo ddr text address */
	unsigned int algo_ddr_vaddr;  /* algo ddr text vitual address */
	unsigned int algo_ddr_size;   /* algo ddr text size */
	unsigned int algo_func_addr;  /* algo hot function address */
	unsigned int algo_func_vaddr; /* algo hot function vitual address */
	unsigned int algo_func_size;  /* algo hot function size */
	unsigned int algo_data_addr;  /* algo data section address */
	unsigned int algo_data_vaddr; /* algo data section vitual address */
	unsigned int algo_data_size;  /* algo data section size */
	unsigned int algo_bss_addr;   /* algo bss section address */
	unsigned int algo_bss_vaddr;  /* algo bss section vitual address */
	unsigned int algo_bss_size;   /* algo bss section size */
};

struct ivp_device {
	struct ivp_common ivp_comm;
	unsigned int core_id;
	unsigned int core_status;
	unsigned int base_offset;
	unsigned int middle_clk_rate;
	unsigned int low_clk_rate;
	unsigned int ultra_low_clk_rate;
	unsigned int sid;
	unsigned int ssid;
	struct ivp_iomem_res io_res;
	struct ivp_algo_mem_info *algo_mem_info;
	struct regulator *ivp_media2_regulator;
	struct regulator *ivp_smmu_tcu_regulator;
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
