/*
 * Copyright (c) 2017-2019 Huawei Technologies Co., Ltd.
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
#ifndef __PERI_VOLT_POLL_H_
#define __PERI_VOLT_POLL_H_

#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/version.h>

struct peri_volt_poll;

struct peri_volt_ops {
	int (*set_volt)(struct peri_volt_poll *pvp, unsigned int volt);
	unsigned int (*get_volt)(struct peri_volt_poll *pvp);
	unsigned int (*recalc_volt)(struct peri_volt_poll *pvp);
	int (*set_avs)(struct peri_volt_poll *pvp, unsigned int avs);
	int (*wait_avs_update)(struct peri_volt_poll *pvp);
	int (*get_temperature)(struct peri_volt_poll *pvp);
	void (*init)(struct peri_volt_poll *pvp, unsigned int en);
};

/*
 * struct peri_volt_poll - a device which usually  dvfs
 * @name: device poll name, will be used to invoke peri volt
 * @addr: platform-specific peri volt poll unit base address
 * @bitsmask: platform-specific peri volt poll ctrl reg
 * @ops: platform-specific  peri_volt_poll handlers
 * @volt: peri zone current volt
 * @flags: platform-specific tag
 * @stat: peri volt poll reg  stat
 * @lock:
 * @pri: private data;
 * @poll_count: device poll num
 */
struct peri_volt_poll {
	const char *name;
	unsigned int dev_id;
	unsigned int perivolt_avs_ip;
	void __iomem *addr;
	void __iomem *addr_0;      /* lpmcu poll reg */
	void __iomem *sysreg_base; /* lpmcu pmctrl reg */
	unsigned int bitsmask;
	unsigned int bitsshift;
	const struct peri_volt_ops *ops;
	unsigned int volt;
	unsigned int flags;
	unsigned int stat;
	struct list_head node;
	spinlock_t lock;
	void *priv;
	unsigned int poll_count;
#ifdef CONFIG_HW_PERI_DVS
	void *priv_date;
#endif
};

enum {
	PERI_VOLT_0 = 0,
	PERI_VOLT_1,
	PERI_VOLT_2,
	PERI_VOLT_3,
	PERI_VOLT_MAX,
};

enum {
	PERI_AVS_DISABLE = 0,
	PERI_AVS_ENABLE,
};

enum {
	DVFS_ISP_CHANEL = 3, /* buck13 need vote 0.8V when isp working */
	DVFS_DDR_CHANEL = 15,
	DVFS_CHANEL_MAX,
};

enum {
	DVFS_BUCK13_VOLT0 = 0,
	DVFS_BUCK13_VOLT1 = 1,
	DVFS_BUCK13_MAX,
};

struct peri_volt_poll *peri_volt_poll_get(unsigned int dev_id, const char *name);
unsigned int peri_get_volt(struct peri_volt_poll *pvp);
int peri_set_volt(struct peri_volt_poll *pvp, unsigned int volt);
int peri_get_temperature(struct peri_volt_poll *pvp);
int peri_set_avs(struct peri_volt_poll *pvp, unsigned int avs);
int peri_wait_avs_update(struct peri_volt_poll *pvp);
#ifdef CONFIG_CLK_BUCK13_DVS
void peri_buck13_mem_volt_set(unsigned int channel, unsigned int buck13_volt);
#else
inline static void peri_buck13_mem_volt_set(unsigned int channel, unsigned int buck13_volt) { return; }
#endif
#endif /* __PERI_VOLT_POLL_H */
