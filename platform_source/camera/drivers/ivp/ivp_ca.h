/*
 * This file define interfaces for dev-ivp secure ca
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

#ifndef _IVP_CA_H_
#define _IVP_CA_H_

#include <linux/types.h>
#include "ivp.h"

int teek_secivp_ca_probe(void);
void teek_secivp_ca_remove(void);
int teek_secivp_sec_cfg(int sharefd, unsigned int size);
int teek_secivp_sec_uncfg(int sharefd, unsigned int size);
unsigned int teek_secivp_secmem_map(unsigned int sharefd, unsigned int size);
int teek_secivp_secmem_unmap(unsigned int sharefd, unsigned int size);
unsigned int teek_secivp_nonsecmem_map(int sharefd, unsigned int size,
	struct sglist *sgl);
int teek_secivp_nonsecmem_unmap(int sharefd, unsigned int size);
void teek_secivp_clear(void);

#endif /* _IVP_CA_H_ */
