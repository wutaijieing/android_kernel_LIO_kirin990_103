/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2012-2018 Huawei Technologies Co., Ltd.
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

#include <linux/string.h>
#include <linux/io.h>
#include "slimbus_drv_ps.h"

/* see slimbus_drv_ps.h */
uint8_t slimbus_drv_ps_uncached_read8(volatile uint8_t* address)
{
	return readb(address);
}

/* see slimbus_drv_ps.h */
uint16_t slimbus_drv_ps_uncached_read16(volatile uint16_t* address)
{
	return readw(address);
}

/* see slimbus_drv_ps.h */
uint32_t slimbus_drv_ps_uncached_read32(volatile uint32_t* address)
{
	return readl(address);
}

/* see slimbus_drv_ps.h */
void slimbus_drv_ps_uncached_write8(volatile uint8_t* address, uint8_t value)
{
	writeb(value, address);
}

/* see slimbus_drv_ps.h */
void slimbus_drv_ps_uncached_write16(volatile uint16_t* address, uint16_t value)
{
	writew(value, address);
}

/* see slimbus_drv_ps.h */
void slimbus_drv_ps_uncached_write32(volatile uint32_t* address, uint32_t value)
{
	writel(value, address);
}

/* see slimbus_drv_ps.h */
void slimbus_drv_ps_write_physaddress32(volatile uint32_t* location, uint32_t addr_value)
{
	writel(addr_value, location);
}

/* see slimbus_drv_ps.h */
void slimbus_drv_ps_buffer_copy(volatile uint8_t *dst, volatile uint8_t *src, uint32_t size)
{
	memcpy((void*)dst, (void*)src, size);
}

