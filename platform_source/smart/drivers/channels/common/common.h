/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: Contexthub common driver.
 * Create: 2017-07-21
 */

#ifndef __CONTEXTHUB_COMMON_H__
#define __CONTEXTHUB_COMMON_H__

#define hm_en(n)                (0x10001 << (n))
#define hm_dis(n)               (0x10000 << (n))

#define writel_mask(mask, data, addr) \
	do { /*lint -save -e717 */ \
		writel((((u32)readl(addr)) & (~((u32)(mask)))) | ((data) & (mask)), (addr)); \
	} while (0) /*lint -restore */

union dump_info {
	unsigned int value;
	struct {
		unsigned int tag : 8; /* bit[0-7]:  ipc tag */
		unsigned int cmd : 8; /* bit[8-15]: ipc cmd */
		unsigned int reserved : 16; /* bit[16-31]: reserved */
	}reg;
	unsigned int modid;
};

static inline unsigned int is_bits_clr(unsigned int  mask, volatile void __iomem *addr)
{
	return (((*(volatile unsigned int *) (addr)) & (mask)) == 0);
}

static inline unsigned int is_bits_set(unsigned int  mask, volatile void __iomem *addr)
{
	return (((*(volatile unsigned int*) (addr))&(mask)) == (mask));
}

static inline void set_bits(unsigned int  mask, volatile void __iomem *addr)
{
	(*(volatile unsigned int *) (addr)) |= mask;
}

static inline void clr_bits(unsigned int mask, volatile void __iomem *addr)
{
	(*(volatile unsigned int *) (addr)) &= ~(mask);
}

int get_contexthub_dts_status(void);

int get_ext_contexthub_dts_status(void);

void save_dump_info_to_history(union dump_info info);
#endif

