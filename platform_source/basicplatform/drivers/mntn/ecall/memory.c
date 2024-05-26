/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include <linux/printk.h>
#include <linux/io.h>

#define PR_LOG_TAG EASY_SHELL_TAG

#define reg_vir_addr_map(phy_addr)        ioremap(phy_addr, sizeof(unsigned long))
#define reg_vir_addr_unmap(vir_addr)      iounmap(vir_addr)

void reg_dbg_dump(unsigned long long addr, unsigned long long size,
		  unsigned long long nodesize)
{
	void __iomem *vir_addr = NULL;
	unsigned int i;

	if (((nodesize != sizeof(char)) && (nodesize != sizeof(short))
	     && (nodesize != sizeof(int)) && (nodesize != sizeof(long long)))) {
		pr_info
		    ("dump type should be 1(char),2(short),4(int),8(long long)\n");
		return;
	}

	pr_info("\n");

	vir_addr = ioremap(addr, nodesize * size);
	if (!vir_addr) {
		pr_err("vir_addr is NULL\n");
		return;
	}

	for (i = 0; i < size; i++) {
		if ((i * nodesize) % 32 == 0)
			pr_info("\n[%16llx]: ", addr + (i * nodesize));

		switch (nodesize) {
		case sizeof(char):
			pr_info("%02x ", *((unsigned char *)vir_addr + i));
			break;
		case sizeof(short):
			pr_info("%04x ", *((unsigned short *)vir_addr + i));
			break;
		case sizeof(int):
			pr_info("%08x ", *((unsigned int *)vir_addr + i));
			break;
		case sizeof(long long):
			pr_info("%16llx ", *((unsigned long long *)vir_addr + i));
			break;
		default:
			break;
		}
	}

	pr_info("\n");
	iounmap(vir_addr);
}

/***********************************************************
 Function: reg_write_u64--write an UINT64 value to physical memory
 Input:    the  writed address and data
 return:   void
 see also: write_uint32
 History:
 1.    2018.8.2   Creat
************************************************************/
unsigned long long reg_write_u64(unsigned int addr, unsigned long int value)
{
	void __iomem *vir_addr = NULL;

	vir_addr = reg_vir_addr_map(addr);
	if (!vir_addr) {
		printk(KERN_ERR "vir_addr is NULL\n");
		return 0;
	}

	*(volatile uint64_t*)vir_addr = value;

	reg_vir_addr_unmap(vir_addr);
	return 0;
}

/***********************************************************
 Function: reg_write_u32--write an UINT32 value to physical memory
 Input:    the  writed address and data
 return:   void
 see also: write_uint16
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long reg_write_u32(unsigned int addr, unsigned int value)
{
	void __iomem *vir_addr = NULL;

	vir_addr = reg_vir_addr_map(addr);
	if (!vir_addr) {
		pr_err("vir_addr is NULL\n");
		return 0;
	}

	writel(value, vir_addr);
	reg_vir_addr_unmap(vir_addr);
	return 0;
}

/***********************************************************
 Function: reg_write_u16--write an UINT16 value to physical memory
 Input:    the  writed address and data
 return:   void
 see also: write_u8
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long reg_write_u16(unsigned int addr, unsigned short value)
{
	void __iomem *vir_addr = NULL;

	vir_addr = reg_vir_addr_map(addr);
	if (!vir_addr) {
		pr_err("vir_addr is NULL\n");
		return 0;
	}

	writew(value, vir_addr); /*lint !e144*/
	reg_vir_addr_unmap(vir_addr);
	return 0;
}

/***********************************************************
 Function: reg_write_u8--write an UINT8 value to physical memory
 Input:    the  writed address and data
 return:   void
 see also: write_u32
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long reg_write_u8(unsigned int addr, unsigned char value)
{
	void __iomem *vir_addr = NULL;

	vir_addr = reg_vir_addr_map(addr);
	if (!vir_addr) {
		pr_err("vir_addr is NULL\n");
		return 0;
	}

	writeb(value, vir_addr); /*lint !e144*/
	reg_vir_addr_unmap(vir_addr);
	return 0;
}

unsigned long long phy_write_u32(phys_addr_t phy_addr, unsigned int value)
{
	void *vir_addr = NULL;

	vir_addr = ioremap_wc(phy_addr, sizeof(unsigned long));
	if (!vir_addr) {
		pr_err("vir_addr is NULL\n");
		return 0;
	}

	writel(value, vir_addr);
	iounmap(vir_addr);

	return 0;
}

unsigned int phy_read_u32(phys_addr_t phy_addr)
{
	unsigned int value;
	void *vir_addr = NULL;

	vir_addr = ioremap_wc(phy_addr, sizeof(unsigned long));
	if (!vir_addr) {
		pr_err("vir_addr is NULL\n");
		return 0;
	}

	value = (unsigned int)readl(vir_addr);
	iounmap(vir_addr);

	return value;
}

/***********************************************************
 Function: reg_read_u64 --read an UINT64 value from physical memory
 Input:    the  read address
 return:   the value
 see also: read_u16
 History:
 1.    2018.8.2   Creat
************************************************************/
unsigned long int reg_read_u64(unsigned long int addr)
{
	unsigned long int value;
	void __iomem *vir_addr = NULL;

	vir_addr = reg_vir_addr_map(addr);
	if (!vir_addr) {
		printk(KERN_ERR "vir_addr is NULL\n");
		return 0;
	}

	value = *(volatile uint64_t*)vir_addr;
	reg_vir_addr_unmap(vir_addr);

	return value;
}

/***********************************************************
 Function: reg_read_u32 --read an UINT32 value from physical memory
 Input:    the  read address
 return:   the value
 see also: read_u16
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned int reg_read_u32(unsigned int addr)
{
	unsigned int value;
	void __iomem *vir_addr = NULL;

	vir_addr = reg_vir_addr_map(addr);
	if (!vir_addr) {
		pr_err("vir_addr is NULL\n");
		return 0;
	}

	value = readl(vir_addr);
	reg_vir_addr_unmap(vir_addr);

	return value;
}


/***********************************************************
 Function: reg_read_u16 --read an UINT16 value from physical memory
 Input:    the  read address
 return:   the value
 see also: read_u8
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned short reg_read_u16(unsigned int addr)
{
	unsigned short value;
	void __iomem *vir_addr = NULL;

	vir_addr = reg_vir_addr_map(addr);
	if (!vir_addr) {
		pr_err("vir_addr is NULL\n");
		return 0;
	}

	value = readw(vir_addr); /*lint !e578*/
	reg_vir_addr_unmap(vir_addr);

	return value;
}

/***********************************************************
 Function: reg_read_u8 --read an UINT8 value from physical memory
 Input:    the  read address
 return:   the value
 see also: read_u32
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned char reg_read_u8(unsigned int addr)
{
	unsigned char value;
	void __iomem *vir_addr = NULL;

	vir_addr = reg_vir_addr_map(addr);
	if (!vir_addr) {
		pr_err("vir_addr is NULL\n");
		return 0;
	}

	value = readb(vir_addr); /*lint !e578*/
	reg_vir_addr_unmap(vir_addr);

	return value;
}

/***********************************************************
 Function: write_uint32--write an UINT32 value to memory
 Input:    the  writed address and data
 return:   void
 see also: write_uint16
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long write_u64(uintptr_t addr, unsigned long long value)
{
	*(volatile unsigned long long *)(addr) = value;
	return 0;
}

/***********************************************************
 Function: write_uint32--write an UINT32 value to memory
 Input:    the  writed address and data
 return:   void
 see also: write_uint16
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long write_u32(uintptr_t addr, unsigned int value)
{
	*(volatile unsigned int *)(addr) = value;
	return 0;
}

/***********************************************************
 Function: write_u16--write an UINT16 value to memory
 Input:    the  writed address and data
 return:   void
 see also: write_u8
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long write_u16(uintptr_t addr, unsigned short value)
{
	*(volatile unsigned short *)(addr) = value;
	return 0;
}

/***********************************************************
 Function: write_u8--write an UINT8 value to memory
 Input:    the  writed address and data
 return:   void
 see also: write_u32
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long write_u8(uintptr_t addr, unsigned char value)
{
	*(volatile unsigned char *)(addr) = value;
	return 0;
}

/***********************************************************
 Function: read_u32 --read an UINT32 value from memory
 Input:    the  read address
 return:   the value
 see also: read_u16
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long read_u64(uintptr_t addr)
{
	unsigned long long value;

	value = *(volatile unsigned long long *)(addr);

	return value;
}

/***********************************************************
 Function: read_u32 --read an UINT32 value from memory
 Input:    the  read address
 return:   the value
 see also: read_u16
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned int read_u32(uintptr_t addr)
{
	unsigned int value;

	value = *(volatile unsigned int *)(addr);
	return value;
}

/***********************************************************
 Function: read_u16 --read an UINT16 value from memory
 Input:    the  read address
 return:   the value
 see also: read_u8
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned short read_u16(uintptr_t addr)
{
	unsigned short value;

	value = *(volatile unsigned short *)(addr);
	return value;
}

/***********************************************************
 Function: read_u8 --read an UINT8 value from memory
 Input:    the  read address
 return:   the value
 see also: read_u32
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned char read_u8(uintptr_t addr)
{
	unsigned char value;

	value = *(volatile unsigned char *)(addr);
	return value;
}
