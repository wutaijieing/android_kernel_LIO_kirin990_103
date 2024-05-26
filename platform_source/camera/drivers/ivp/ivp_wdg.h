/*
 * This file is ivp watchdog register and value.
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

#ifndef _IVP_WDG_H_
#define _IVP_WDG_H_

struct register_info {
	unsigned int offset;
	unsigned int value;
	const char *info;
};

#ifdef IVP_WDG_V500
#define WDG_REG_UNLOCK_KEY            0x1AAEE533
#define WDG_REG_CLR_IRQ               0x1
#define WDG_REG_IRQ_MASK              0x1
#define WDG_REG_DISABLE               0x0
#define WDG_REG_LOCK_KEY              0x0

#define IVP32_WDG_OFFSET_LOCKEN       0x0000
#define IVP32_WDG_OFFSET_INTR_WD_CLR  0x001C
#define IVP32_WDG_OFFSET_INTR_WD_MASK 0x0020
#define IVP32_WDG_OFFSET_WD_EN        0x0014
#define IVP32_WDG_OFFSET_LOCKDIS      0x0004

#define IVP32_WDG_OFFSET_STATE        IVP32_WDG_OFFSET_WD_EN

static struct register_info wdg_clr_irq[] = {
	{ IVP32_WDG_OFFSET_LOCKEN, WDG_REG_UNLOCK_KEY, "unlock wdg register" },
	{ IVP32_WDG_OFFSET_INTR_WD_CLR, WDG_REG_CLR_IRQ, "clear wdg irq" },
	{ IVP32_WDG_OFFSET_INTR_WD_MASK, WDG_REG_IRQ_MASK, "wdg irq mask" },
	{ IVP32_WDG_OFFSET_WD_EN, WDG_REG_DISABLE, "disable wdg" },
	{ IVP32_WDG_OFFSET_LOCKDIS, WDG_REG_LOCK_KEY, "lock wdg register" },
};
#else
#define WDG_REG_UNLOCK_KEY            0x1ACCE551
#define WDG_REG_CLR_IRQ               0x1
#define WDG_REG_DISABLE               0x0
#define WDG_REG_LOCK_KEY              0x0

#define IVP32_WDG_OFFSET_LOCK         0x0C00
#define IVP32_WDG_OFFSET_INTCLR       0x000C
#define IVP32_WDG_OFFSET_CONTROL      0x0008

#define IVP32_WDG_OFFSET_STATE        IVP32_WDG_OFFSET_CONTROL

static struct register_info wdg_clr_irq[] = {
	{ IVP32_WDG_OFFSET_LOCK, WDG_REG_UNLOCK_KEY, "unlock wdg register" },
	{ IVP32_WDG_OFFSET_INTCLR, WDG_REG_CLR_IRQ, "clear wdg irq" },
	{ IVP32_WDG_OFFSET_CONTROL, WDG_REG_DISABLE, "disable wdg" },
	{ IVP32_WDG_OFFSET_LOCK, WDG_REG_LOCK_KEY, "lock wdg register" },
};
#endif
#endif
