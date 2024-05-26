/*
 * ASoc adapter layer for diff version kernel
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
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

#include "asoc_adapter.h"

#include <sound/soc.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
unsigned int snd_soc_component_read32(struct snd_soc_component *component,
	unsigned int reg)
{
	if (component && component->driver && component->driver->read)
		return component->driver->read(component, reg);

	return 0;
}
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
unsigned int snd_soc_component_read32(struct snd_soc_component *component,
	unsigned int reg)
{
	return snd_soc_read(snd_soc_component_to_codec(component), reg);
}
#else
// in kernel 5.0.4, alsa provide snd_soc_component_read32 function
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
int snd_soc_component_write_adapter(struct snd_soc_component *component,
	unsigned int reg, unsigned int val)
{
	if (component && component->driver && component->driver->write)
		return component->driver->write(component, reg, val);

	return 0;
}
#else
int snd_soc_component_write_adapter(struct snd_soc_component *component,
	unsigned int reg, unsigned int val)
{
	return snd_soc_component_write(component, reg, val);
}
#endif
