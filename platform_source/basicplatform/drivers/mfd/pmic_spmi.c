/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * pmic_spmi.c
 *
 * Device driver for regulators in PMIC IC
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <platform_include/basicplatform/linux/spmi_platform.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <platform_include/basicplatform/linux/of_platform_spmi.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/regmap.h>
#include "pmic_sim_hpd.h"
#include <securec.h>

#ifndef NO_IRQ
#define NO_IRQ 0
#endif
#ifndef FALSE
#define FALSE 0
#endif
/* 8-bit register offset in PMIC */
#define PMIC_MASK_STATE 0xff
#define PMIC_MASK_FIELD 0xff
#define PMIC_BITS 8
#define PR_LOG_TAG PMIC_TAG
#define PMIC_FPGA_FLAG 1

#define PMIC_IRQ_KEY_NUM 0
#define PMIC_IRQ_KEY_VALUE 0xc0
#define PMIC_IRQ_KEY_DOWN 7
#define PMIC_IRQ_KEY_UP 6

#define VBUS_IRQ_MASK 0x3
#define VBUS_IRQ_UNMASK 0
#define VBUS_IRQ_CLEAR 0x3
#define VBUS_IRQ_UNCLEAR 0
#define VBUS_IRQ_INFO_MEMBERS_NUMS 3
#define PMIC_IRQ_MASK 0
#define PMIC_IRQ_UNMASK 1
#define ONE_IRQ_GROUP_NUM 8
static struct vendor_pmic *g_pmic;
static struct vendor_pmic *g_mmw_pmic;
static struct vendor_pmic *g_sub_pmic;

static struct bit_info g_pmic_vbus;
static struct vbus_irq_info g_pmic_vbus_mask;
static struct vbus_irq_info g_pmic_vbus_clear;

static u32 _spmi_pmic_read(struct vendor_pmic *pmic, int reg)
{
	u32 ret;
	u8 read_value = 0;
	struct spmi_device *pdev = NULL;

	if (pmic == NULL) {
		pr_err("g_pmic is NULL\n");
		return 0;
	}

	pdev = to_spmi_device(pmic->dev);
	if (pdev == NULL) {
		pr_err("%s:pdev get failed!\n", __func__);
		return 0;
	}

	ret = vendor_spmi_ext_register_readl(
		pdev->ctrl, pdev->usid, reg, (unsigned char *)&read_value, 1);
	if (ret) {
		pr_err("%s:spmi_ext_register_readl failed!\n", __func__);
		return 0;
	}
	return (u32)read_value;
}

static void _spmi_pmic_write(struct vendor_pmic *pmic, int reg, u32 val)
{
	u32 ret;
	u8 value;
	struct spmi_device *pdev = NULL;

	if (pmic == NULL) {
		pr_err("g_pmic is NULL\n");
		return;
	}
	pdev = to_spmi_device(pmic->dev);
	if (pdev == NULL) {
		pr_err("%s:sub pdev get failed!\n", __func__);
		return;
	}

	value = val & 0xff;

	ret = vendor_spmi_ext_register_writel(
		pdev->ctrl, pdev->usid, reg, &value, 1);
	if (ret) {
		pr_err("%s:spmi_ext_register_writel failed!\n", __func__);
		return;
	}
}

u32 sub_pmic_read(struct vendor_pmic *pmic, int reg)
{
	return _spmi_pmic_read(g_sub_pmic, reg);
}

void sub_pmic_write(struct vendor_pmic *pmic, int reg, u32 val)
{
	_spmi_pmic_write(g_sub_pmic, reg, val);
}

u32 main_pmic_read(struct vendor_pmic *pmic, int reg)
{
	return _spmi_pmic_read(g_pmic, reg);
}

void main_pmic_write(struct vendor_pmic *pmic, int reg, u32 val)
{
	_spmi_pmic_write(g_pmic, reg, val);
}

#if defined(CONFIG_PMIC_SUB_PMU_SPMI)
unsigned int mmw_pmic_reg_read(int addr)
{
	return _spmi_pmic_read(g_mmw_pmic, addr);
}

void mmw_pmic_reg_write(int addr, int val)
{
	_spmi_pmic_write(g_mmw_pmic, addr, val);
}
#ifndef CONFIG_AB_APCP_NEW_INTERFACE
unsigned int hisi_sub_pmic_reg_read(int addr)
{
	return sub_pmic_reg_read(addr);
}
EXPORT_SYMBOL(hisi_sub_pmic_reg_read);

void hisi_sub_pmic_reg_write(int addr, int val)
{
	sub_pmic_reg_write(addr, val);
}
EXPORT_SYMBOL(hisi_sub_pmic_reg_write);
#endif
unsigned int sub_pmic_reg_read(int addr)
{
	return _spmi_pmic_read(g_sub_pmic, addr);
}
EXPORT_SYMBOL(sub_pmic_reg_read);

void sub_pmic_reg_write(int addr, int val)
{
	_spmi_pmic_write(g_sub_pmic, addr, val);
}
EXPORT_SYMBOL(sub_pmic_reg_write);

#endif

unsigned int pmic_read_reg(int addr)
{
	return _spmi_pmic_read(g_pmic, addr);
}
EXPORT_SYMBOL(pmic_read_reg);

void pmic_write_reg(int addr, int val)
{
	_spmi_pmic_write(g_pmic, addr, val);
}
EXPORT_SYMBOL(pmic_write_reg);
#ifndef CONFIG_AB_APCP_NEW_INTERFACE
unsigned int hisi_pmic_reg_read(int addr)
{
	return pmic_read_reg(addr);
}
EXPORT_SYMBOL(hisi_pmic_reg_read);

void hisi_pmic_reg_write(int addr, int val)
{
	pmic_write_reg(addr, val);
}
EXPORT_SYMBOL(hisi_pmic_reg_write);
#endif
/* for read dieid of sub pmu before pho */
u32 pmic_read_sub_pmu(u8 sid, int reg)
{
	u8 read_value = 0;

#ifdef CONFIG_PMIC_PLATFORM_DEBUG
	u32 ret;
	struct spmi_device *pdev = NULL;

	if (g_pmic == NULL) {
		pr_err(" g_pmic is NULL\n");
		return 0;
	}

	pdev = to_spmi_device(g_pmic->dev);
	if (pdev == NULL) {
		pr_err("%s:pdev get failed!\n", __func__);
		return 0;
	}

	ret = vendor_spmi_ext_register_readl(
		pdev->ctrl, sid, reg, (unsigned char *)&read_value, 1);
	if (ret) {
		pr_err("%s:spmi_ext_register_readl failed!\n",
			__func__);
		return 0;
	}
#endif
	return (u32)read_value;
}

void pmic_write_sub_pmu(u8 sid, int reg, u32 val)
{
#ifdef CONFIG_PMIC_PLATFORM_DEBUG
	u32 ret;
	struct spmi_device *pdev = NULL;

	if (g_pmic == NULL) {
		pr_err(" g_pmic is NULL\n");
		return;
	}

	pdev = to_spmi_device(g_pmic->dev);
	if (pdev == NULL) {
		pr_err("%s:pdev get failed!\n", __func__);
		return;
	}

	ret = vendor_spmi_ext_register_writel(
		pdev->ctrl, sid, reg, (unsigned char *)&val, 1);
	if (ret) {
		pr_err("%s:spmi_ext_register_writel failed!\n",
			__func__);
		return;
	}
#endif
}

void pmic_rmw(struct vendor_pmic *pmic, int reg, u32 mask, u32 bits)
{
	u32 data;
	unsigned long flags;

	if (g_pmic == NULL) {
		pr_err(" g_pmic  is NULL\n");
		return;
	}

	spin_lock_irqsave(&g_pmic->lock, flags);
	data = main_pmic_read(pmic, reg) & ~mask;
	data |= mask & bits;
	main_pmic_write(pmic, reg, data);
	spin_unlock_irqrestore(&g_pmic->lock, flags);
}

void pmic_reg_write_lock(int addr, int val)
{
	unsigned long flags;

	if (g_pmic == NULL) {
		pr_err(" g_pmic  is NULL\n");
		return;
	}

	spin_lock_irqsave(&g_pmic->lock, flags);
	main_pmic_write(
		g_pmic, g_pmic->normal_lock.addr, g_pmic->normal_lock.val);
	main_pmic_write(
		g_pmic, g_pmic->debug_lock.addr, g_pmic->debug_lock.val);
	main_pmic_write(g_pmic, addr, val);
	main_pmic_write(g_pmic, g_pmic->normal_lock.addr, 0);
	main_pmic_write(g_pmic, g_pmic->debug_lock.addr, 0);
	spin_unlock_irqrestore(&g_pmic->lock, flags);
}
int pmic_array_read(int addr, char *buff, unsigned int len)
{
	unsigned int i;

	if ((len > 32) || (buff == NULL)) /* buf maxlen: 32 */
		return -EINVAL;

	/*
	 * Here is a bug in the pmu die.
	 * the coul driver will read 4 bytes,
	 * but the ssi bus only read 1 byte, and the pmu die
	 * will make sampling 1/10669us about vol cur,so the driver
	 * read the data is not the same sampling
	 */
	for (i = 0; i < len; i++)
		*(buff + i) = pmic_read_reg(addr + i);

	return 0;
}

int pmic_array_write(int addr, const char *buff, unsigned int len)
{
	unsigned int i;

	if ((len > 32) || (buff == NULL)) /* buf maxlen: 32 */
		return -EINVAL;

	for (i = 0; i < len; i++)
		pmic_write_reg(addr + i, *(buff + i));

	return 0;
}

/*
 * The PMIC register is only 8-bit.
 * SoC use hardware to map PMIC register into SoC mapping.
 * At here, we are accessing SoC register with 32-bit.
 */

int pmic_get_irq_byname(unsigned int pmic_irq_list)
{
	if (g_pmic == NULL) {
		pr_err("[%s]g_pmic is NULL\n", __func__);
		return -1;
	}

	if (pmic_irq_list > (unsigned int)g_pmic->irqnum) {
		pr_err("[%s]input pmic irq number is error\n", __func__);
		return -1;
	}
	pr_info("%s:g_pmic->irqs[%u] %u\n", __func__, pmic_irq_list,
		g_pmic->irqs[pmic_irq_list]);
	return (int)g_pmic->irqs[pmic_irq_list];
}

int pmic_get_vbus_status(void)
{
	if (g_pmic_vbus.addr == 0)
		return -1;

	if (pmic_read_reg(g_pmic_vbus.addr) &
		BIT((unsigned int)g_pmic_vbus.bit))
		return 1;

	return 0;
}

static void pmic_vbus_irq(
	struct vbus_irq_info *vbus_info, unsigned int mask)
{
	unsigned int value;
	unsigned int temp;

	if (vbus_info->addr == 0)
		return;

	value = pmic_read_reg(vbus_info->addr);

	temp = (value & (~(vbus_info->mask << vbus_info->shift))) |
	       (mask << vbus_info->shift);
	pmic_write_reg(vbus_info->addr, temp);
}

void pmic_vbus_irq_mask(int enable)
{
	if (enable)
		pmic_vbus_irq(&g_pmic_vbus_mask, VBUS_IRQ_MASK);
	else
		pmic_vbus_irq(&g_pmic_vbus_mask, VBUS_IRQ_UNMASK);
}

void pmic_clear_vbus_irq(int enable)
{
	if (enable)
		pmic_vbus_irq(&g_pmic_vbus_clear, VBUS_IRQ_CLEAR);
	else
		pmic_vbus_irq(&g_pmic_vbus_clear, VBUS_IRQ_UNCLEAR);
}

static int _pmic_get_dieid(
	struct vendor_pmic *pmic, char *dieid, unsigned int len)
{
	int ret;
	unsigned int i;
	unsigned char value;
	unsigned int length;
	char pmu_buf[PMIC_DIEID_BUF] = {0};
	char buf[PMIC_DIEID_TEM_SAVE_BUF] = {0};

	if (!dieid || !pmic || !pmic->dieid_name || !pmic->dieid_regs) {
		pr_err("%s dieid g_pmic  is NULL\n", __func__);
		return -ENOMEM;
	}

	ret = snprintf_s(pmu_buf, sizeof(pmu_buf), sizeof(pmu_buf) - 1,
		"%s%s%s", "\r\n", pmic->dieid_name, ":0x");
	if (ret < 0) {
		pr_err("%s read main pmu dieid head fail\n", __func__);
		return ret;
	}

	for (i = 0; i < pmic->dieid_reg_num; i++) {
		value = _spmi_pmic_read(pmic, pmic->dieid_regs[i]);
		ret = snprintf_s(
			buf, sizeof(buf), sizeof(buf) - 1, "%02x", value);
		if (ret < 0) {
			pr_err("%s read main pmu dieid fail\n", __func__);
			return ret;
		}
		ret = strncat_s(pmu_buf, PMIC_DIEID_BUF, buf, strlen(buf));
		if (ret) {
			pr_err("%s read main pmu dieid buf fail\n", __func__);
			return ret;
		}
	}

	ret = strncat_s(pmu_buf, PMIC_DIEID_BUF, "\r\n", strlen("\r\n"));
	if (ret) {
		pr_err("%s read main pmu dieid buf fail\n", __func__);
		return ret;
	}

	length = strlen(pmu_buf);
	if (len >= length) {
		ret = strncat_s(dieid, PMIC_DIEID_BUF, pmu_buf, length);
		if (ret) {
			pr_err("%s read pmu dieid length fail\n", __func__);
			return ret;
		}
	} else {
		pr_err("%s:dieid buf length is not enough!\n", __func__);
		return length;
	}

	return 0;
}


int pmic_subpmu_get_dieid(char *dieid, unsigned int len)
{
	int ret;

	ret = _pmic_get_dieid(g_sub_pmic, dieid, len);
	if (ret)
		pr_err("%s:fail!\n", __func__);
	return ret;
}

int pmic_get_dieid(char *dieid, unsigned int len)
{
	int ret;

	ret = _pmic_get_dieid(g_pmic, dieid, len);
	if (ret)
		pr_err("%s:fail!\n", __func__);
	return ret;
}

static void _pmic_irq_mask(
	struct irq_data *d, int maskflag)
{
	struct vendor_pmic *pmic = irq_data_get_irq_chip_data(d);
	u32 data, offset;
	unsigned long flags;

	if (pmic == NULL) {
		pr_err(" irq_mask pmic is NULL\n");
		return;
	}
	/* Convert interrupt data to interrupt offset */
	offset = (irqd_to_hwirq(d) / ONE_IRQ_GROUP_NUM);
	offset = pmic->irq_mask_addr_arry[offset];
	spin_lock_irqsave(&pmic->lock, flags);
	data = _spmi_pmic_read(pmic, offset);
	if (maskflag == PMIC_IRQ_MASK)
		data |= (1 << (irqd_to_hwirq(d) & 0x07)); /* offset mask 3 */
	else
		data &= ~(u32)(1 << (irqd_to_hwirq(d) & 0x07)); /* offset mask 3 */

	_spmi_pmic_write(pmic, offset, data);
	spin_unlock_irqrestore(&pmic->lock, flags);
}

static void pmic_irq_mask(struct irq_data *d)
{
	_pmic_irq_mask(d, PMIC_IRQ_MASK);
}

static void pmic_irq_unmask(struct irq_data *d)
{
	_pmic_irq_mask(d, PMIC_IRQ_UNMASK);
}

static int pmic_irq_map(
	struct irq_domain *d, unsigned int virq, irq_hw_number_t hw)
{
	struct vendor_pmic *pmic = d->host_data;
	int ret;

	if (!pmic)
		return -ENOMEM;

	irq_set_chip_and_handler_name(
		virq, &pmic->irq_chip, handle_simple_irq, "vendor");
	irq_set_chip_data(virq, pmic);
	ret = irq_set_irq_type(virq, IRQ_TYPE_NONE);
	if (ret < 0)
		pr_err("irq set type fail\n");

	return 0;
}

static const struct irq_domain_ops pmic_domain_ops = {
	.map = pmic_irq_map,
	.xlate = irq_domain_xlate_twocell,
};

static void get_pmic_device_tree_vbus_data(struct device_node *np)
{
	int ret;

	ret = of_property_read_u32_array(np,
		"pmic-vbus-mask-addr", (u32 *)&g_pmic_vbus_mask,
		VBUS_IRQ_INFO_MEMBERS_NUMS);
	if (ret)
		pr_err("no pmic-vbus-mask-addr property\n");

	ret = of_property_read_u32_array(np,
		"hisilicon,pmic-clear-vbus-irq-addr",
		(u32 *)&g_pmic_vbus_clear, VBUS_IRQ_INFO_MEMBERS_NUMS);
	if (ret)
		pr_err("no pmic-clear-vbus-irq-addr proprety\n");
}

static int get_pmic_device_tree_data(
	struct spmi_device *pdev, struct vendor_pmic *pmic)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	bool is_process_irq = FALSE;

	is_process_irq = of_property_read_bool(np, "handle_irq");
	if (!is_process_irq) {
		pr_err(" pmu not need process irq\n");
		return 0;
	}

	/* get pmic irq num */
	ret = of_property_read_u32(
		np, "pmic-irq-num", &(pmic->irqnum));
	if (ret) {
		pr_err("no pmic-irq-num property set\n");
		return -ENODEV;
	}

	/* get pmic irq array number */
	ret = of_property_read_u32(
		np, "pmic-irq-array", &(pmic->irqarray));
	if (ret) {
		pr_err("no pmic-irq-array property set\n");
		return -ENODEV;
	}

	pmic->irq_mask_addr_arry = (int *)devm_kzalloc(
		dev, sizeof(int) * pmic->irqarray, GFP_KERNEL);
	if (!pmic->irq_mask_addr_arry)
		return -ENOMEM;

	/* SOC_PMIC_IRQ_MASK_ADDR */
	ret = of_property_read_u32_array(np,
		"pmic-irq-mask-addr",
		(int *)pmic->irq_mask_addr_arry, pmic->irqarray); /* width:2 */
	if (ret) {
		pr_err("no pmic-irq-mask-addr property set\n");
		return -ENODEV;
	}

	/* SOC_PMIC_IRQ_ADDR */
	pmic->irq_addr_arry = (int *)devm_kzalloc(
		dev, sizeof(int) * pmic->irqarray, GFP_KERNEL);
	if (!pmic->irq_addr_arry)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, "pmic-irq-addr",
		(int *)pmic->irq_addr_arry, pmic->irqarray);
	if (ret) {
		pr_err("no pmic-irq-addr property set\n");
		return -ENODEV;
	}

	if (of_device_is_compatible(np, SPMI_PMIC_COMP)) {
		ret = of_property_read_u32_array(np, "pmic-vbus",
			(u32 *)&g_pmic_vbus, 2); /* width:2 */
		if (ret) {
			pr_err("no pmic-vbus property\n");
			return -ENODEV;
		}
		get_pmic_device_tree_vbus_data(np);
		ret = of_property_read_u32_array(np, "pmic-lock",
			(int *)&pmic->normal_lock, 2); /* width:2 */
		if (ret) {
			pr_err("no pmic-lock property set\n");
			return -ENODEV;
		}

		/* pmic debug lock */
		ret = of_property_read_u32_array(np, "pmic-debug-lock",
			(int *)&pmic->debug_lock, 2); /* width:2 */
		if (ret) {
			pr_err("no pmic-debug-lock property set\n");
			return -ENODEV;
		}
	}
	return ret;
}

static int get_pmic_dieid_tree_data(
	struct spmi_device *pdev, struct vendor_pmic *pmic)
{
	int ret;
	struct device_node *root = NULL;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	root = of_find_compatible_node(np, NULL, "pmic-dieid");
	if (root == NULL) {
		pr_err("[%s]no pmic-dieid root node\n",
			__func__);
		return -ENODEV;
	}

	ret = of_property_read_string(root, "pmic-dieid-name",
		(const char **)&(pmic->dieid_name));
	if (ret) {
		pr_err("no pmic-dieid-name property set\n");
		pmic->dieid_name = NULL;
		return -ENODEV;
	}

	ret = of_property_read_u32(root, "pmic-dieid-reg-num",
		(u32 *)&(pmic->dieid_reg_num));
	if (ret || !pmic->dieid_reg_num) {
		pr_err("no pmic-dieid-reg-num property set\n");
		pmic->dieid_name = NULL;
		return -ENODEV;
	}

	pmic->dieid_regs = (u32 *)devm_kzalloc(
		dev, sizeof(u32) * pmic->dieid_reg_num, GFP_KERNEL);
	if (!pmic->dieid_regs) {
		pmic->dieid_name = NULL;
		return -ENOMEM;
	}
	ret = of_property_read_u32_array(root, "pmic-dieid-regs",
		pmic->dieid_regs, pmic->dieid_reg_num);
	if (ret) {
		pr_err("[%s]get pmic-dieid-regs attribute failed\n",
			__func__);
		pmic->dieid_name = NULL;
		devm_kfree(dev, pmic->dieid_regs);
		return -ENODEV;
	}
	return ret;
}

static void pmic_irq_prc(struct vendor_pmic *pmic)
{
	int i;
	unsigned int pending;

	for (i = 0; i < pmic->irqarray; i++)
		_spmi_pmic_write(pmic, pmic->irq_mask_addr_arry[i],
			PMIC_MASK_STATE);

	for (i = 0; i < pmic->irqarray; i++) {
		pending = _spmi_pmic_read(pmic, pmic->irq_addr_arry[i]);
		pr_debug("PMU IRQ address value:irq[0x%x] = 0x%x\n",
			pmic->irq_addr_arry[i], pending);
		_spmi_pmic_write(
			pmic, pmic->irq_addr_arry[i], PMIC_MASK_STATE);
	}
}

static unsigned int pmic_power_key_irq_handler(struct vendor_pmic *pmic,
	int num, unsigned int pending)
{
	if (pmic->powerkey_irq_down_up) {
		if ((num == PMIC_IRQ_KEY_NUM) &&
			((pending & PMIC_IRQ_KEY_VALUE) ==
				PMIC_IRQ_KEY_VALUE)) {
			generic_handle_irq(pmic->irqs[PMIC_IRQ_KEY_DOWN]);
			generic_handle_irq(pmic->irqs[PMIC_IRQ_KEY_UP]);
			pending &= (~PMIC_IRQ_KEY_VALUE);
		}
	}
	return pending;
}

static void sim_hpd_proc(struct vendor_pmic *pmic)
{
	struct device *dev = pmic->dev;
	struct device_node *np = dev->of_node;

	if (of_device_is_compatible(np, SPMI_PMIC_COMP))
		pmic_sim_hpd_proc();
	else if (of_device_is_compatible(np, SPMI_SUB_PMIC_COMP))
		sub_pmic_sim_hpd_proc();
}

static irqreturn_t pmic_irq_handler(int irq, void *data)
{
	struct vendor_pmic *pmic = (struct vendor_pmic *)data;
	unsigned int pending;
	unsigned long pending_s;
	int i, offset;

	sim_hpd_proc(pmic);

	for (i = 0; i < pmic->irqarray; i++) {
		pending = _spmi_pmic_read(pmic, pmic->irq_addr_arry[i]);
		pending &= PMIC_MASK_FIELD;
		if (pending != 0)
			pr_info("pending[%d]=0x%x\n\r", i, pending);

		_spmi_pmic_write(pmic, pmic->irq_addr_arry[i], pending);
		/* solve powerkey order */
		pending_s = (unsigned long)pmic_power_key_irq_handler(
				pmic, i, pending);
		if (pending_s) {
			for_each_set_bit(offset, &pending_s, PMIC_BITS)
			generic_handle_irq(
				pmic->irqs[offset + i * PMIC_BITS]);
		}
	}
	return IRQ_HANDLED;
}

static int pmic_irq_create_mapping(struct vendor_pmic *pmic)
{
	int i;
	unsigned int virq;

	for (i = 0; i < pmic->irqnum; i++) {
		virq = irq_create_mapping(pmic->domain, i);
		if (virq == NO_IRQ) {
			pr_debug("Failed mapping hwirq\n");
			return -ENOSPC;
		}
		pmic->irqs[i] = virq;
		pr_info("[%s]. pmic->irqs[%d] = %u\n", __func__, i,
			pmic->irqs[i]);
	}
	return 0;
}

static int pmic_irq_register(
	struct spmi_device *pdev, struct vendor_pmic *pmic, const char *name)
{
	int ret;
	enum of_gpio_flags flags;
	struct device *dev = NULL;
	struct device_node *np = NULL;

	if (!pdev || !pmic || !name) {
		pr_info("[%s] pointer null\n", __func__);
		return -EINVAL;
	}

	dev = &pdev->dev;
	np = dev->of_node;
	pmic->gpio = of_get_gpio_flags(np, 0, &flags);
	if (pmic->gpio < 0) {
		dev_err(dev, "[%s] of_get_gpio_flags %d\n", __func__, pmic->gpio);
		return pmic->gpio;
	}

	if (!gpio_is_valid(pmic->gpio)) {
		dev_err(dev, "[%s] gpio_is_valid\n", __func__);
		return -EINVAL;
	}

	ret = gpio_request_one(pmic->gpio, GPIOF_IN, name);
	if (ret < 0) {
		dev_err(dev, "failed to request gpio %d\n", pmic->gpio);
		return ret;
	}

	pmic->irq = gpio_to_irq(pmic->gpio);

	/* mask && clear IRQ status */
	pmic_irq_prc(pmic);

	pmic->irqs = (unsigned int *)devm_kmalloc(
		dev, pmic->irqnum * sizeof(int), GFP_KERNEL);
	if (!pmic->irqs) {
		dev_err(dev, "[%s] devm_kmalloc fail\n", __func__);
		return -ENODEV;
	}

	/* Dynamic obtain struct irq_chip */
	pmic->irq_chip.name = pmic->chip_irq_name;
	pmic->irq_chip.irq_mask = pmic_irq_mask;
	pmic->irq_chip.irq_unmask = pmic_irq_unmask;
	pmic->irq_chip.irq_disable = pmic_irq_mask;
	pmic->irq_chip.irq_enable = pmic_irq_unmask;

	pmic->domain = irq_domain_add_simple(
		np, pmic->irqnum, 0, &pmic_domain_ops, pmic);

	if (!pmic->domain) {
		dev_err(dev, "failed irq domain add simple!\n");
		return -ENODEV;
	}

	ret = pmic_irq_create_mapping(pmic);
	if (ret < 0) {
		dev_err(dev, "[%s] pmic_irq_create_mapping fail\n", __func__);
		return ret;
	}

	ret = request_threaded_irq(pmic->irq, pmic_irq_handler, NULL,
		IRQF_TRIGGER_LOW | IRQF_SHARED | IRQF_NO_SUSPEND, name, pmic);
	if (ret < 0) {
		dev_err(dev, "could not claim pmic %d\n", ret);
		return -ENODEV;
	}

	return 0;
}

static int get_pmic_device_info(
	struct spmi_device *pdev, struct vendor_pmic *pmic)
{
	int ret;
	struct device *dev = &pdev->dev;

	/* get pmic dieid dts info */
	ret = get_pmic_dieid_tree_data(pdev, pmic);
	if (ret)
		dev_err(&pdev->dev, "Error reading pmic dieid dts\n");

	/* get pmic dts irq */
	ret = get_pmic_device_tree_data(pdev, pmic);
	if (ret) {
		dev_err(&pdev->dev, "Error reading pmic device dts\n");
		return ret;
	}
	pmic->dev = dev;
	return 0;
}
static int pmic_get_chip_and_irq_name(struct vendor_pmic *pmic)
{
	int ret;
	struct spmi_device *pdev = NULL;

	if (!pmic || !pmic->dev)
		return -ENOMEM;

	pdev = to_spmi_device(pmic->dev);
	if (!pdev)
		return -ENOMEM;

	ret = snprintf_s(pmic->irq_name, PMIC_IRQ_NAME_SIZE,
		PMIC_IRQ_NAME_SIZE - 1, "%s%u%s",
		"pmic-sid", pdev->usid, "-irq");
	if (ret < 0)
		pr_err("snprintf error, ret = %d!\n", ret);

	ret = snprintf_s(pmic->chip_irq_name, PMIC_IRQ_NAME_SIZE,
		PMIC_IRQ_NAME_SIZE - 1, "%s%u%s",
		"pmu-sid", pdev->usid, "-chip-irq");
	if (ret < 0)
		pr_err("snprintf error, ret = %d!\n", ret);
	return 0;
}

static int pmic_regmap_read(void *context, unsigned int reg, unsigned int *val)
{
	struct vendor_pmic *pmic = context;

	if (!pmic || !val) {
		pr_err("could not find dev, %s failed!", __func__);
		return -1;
	}
	*val = _spmi_pmic_read(pmic, reg);

	return 0;
}

static int pmic_regmap_write(void *context, unsigned int reg, unsigned int val)
{
	struct vendor_pmic *pmic = context;

	if (!pmic) {
		pr_err("could not find dev, %s failed!", __func__);
		return -1;
	}

	_spmi_pmic_write(pmic, reg, val);

	return 0;
}

static const struct regmap_config pmic_spmi_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.max_register = 0xffff,
	.fast_io = true,
	.reg_read = pmic_regmap_read,
	.reg_write = pmic_regmap_write,
};

static int pmic_probe(struct spmi_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct vendor_pmic *pmic = NULL;
	int ret;
	unsigned int fpga_flag = 0;
	bool is_process_irq = FALSE;

	pmic = devm_kzalloc(dev, sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	ret = get_pmic_device_info(pdev, pmic);
	if (ret)
		return ret;

	spin_lock_init(&pmic->lock);

	/* get spmi sid */
	ret = of_property_read_u32(np, "slave_id", (u32 *)&(pdev->usid));
	if (ret)
		return -ENOMEM;

	pmic->regmap = devm_regmap_init(dev, NULL, pmic, &pmic_spmi_regmap_config);
	if (IS_ERR(pmic->regmap)) {
		ret = PTR_ERR(pmic->regmap);
		dev_err(dev, "regmap init failed %d\n", ret);
		return ret;
	}

	is_process_irq = of_property_read_bool(np, "handle_irq");
	if (!is_process_irq) {
		dev_err(&pdev->dev, " not need process irq\n");
		goto success;
	}

	ret = of_property_read_u32_array(
		np, "pmic_fpga_flag", &fpga_flag, 1);
	if (ret)
		pr_err("no pmic_fpga_flag!\n");

	if (fpga_flag == PMIC_FPGA_FLAG)
		goto success;

	if (of_device_is_compatible(np, SPMI_PMIC_COMP))
		pmic->powerkey_irq_down_up = 1;

	ret = pmic_get_chip_and_irq_name(pmic);
	if (ret) {
		pr_err("get chip irq name or irq name fail!\n");
		return ret;
	}

	ret = pmic_irq_register(pdev, pmic, pmic->irq_name);
	if (ret)
		goto request_theaded_irq;

success:
	if (of_device_is_compatible(np, SPMI_MMW_PMIC_COMP))
		g_mmw_pmic = pmic;
	else if (of_device_is_compatible(np, SPMI_SUB_PMIC_COMP))
		g_sub_pmic = pmic;
	else
		g_pmic = pmic;

	return devm_of_platform_populate(&pdev->dev);

request_theaded_irq:
	gpio_free(pmic->gpio);
	return ret;
}

static int _pmic_remove(struct spmi_device *pdev)
{
	struct vendor_pmic *pmic = dev_get_drvdata(&pdev->dev);

	free_irq(pmic->irq, pmic);
	gpio_free(pmic->gpio);
	devm_kfree(&pdev->dev, pmic);

	return 0;
}

static void pmic_remove(struct spmi_device *pdev)
{
	(void)_pmic_remove(pdev);
}

static int pmic_suspend(struct device *dev, pm_message_t state)
{
	struct vendor_pmic *pmic = dev_get_drvdata(dev);

	if (pmic == NULL) {
		pr_err("%s:pmic is NULL\n", __func__);
		return -ENOMEM;
	}

	pr_info("%s:+\n", __func__);
	pr_info("%s:-\n", __func__);

	return 0;
}

static int pmic_resume(struct device *dev)
{
	struct vendor_pmic *pmic = dev_get_drvdata(dev);

	if (pmic == NULL) {
		pr_err("%s:pmic is NULL\n", __func__);
		return -ENOMEM;
	}

	pr_info("%s:+\n", __func__);
	pr_info("%s:-\n", __func__);

	return 0;
}

static const struct of_device_id of_spmi_pmic_match_tbl[] = {
	{ .compatible = SPMI_PMIC_COMP, },
	{ .compatible = SPMI_SUB_PMIC_COMP, },
	{ .compatible = SPMI_MMW_PMIC_COMP, },
	{  }   /* end */
};

static const struct spmi_device_id pmic_spmi_id[] = {
	{ "pmic_driver", 0 },
	{ }   /* end */
};

MODULE_DEVICE_TABLE(spmi, pmic_spmi_id);

static struct spmi_driver pmic_driver = {
	.driver = {
			.name = "pmic_driver",
			.owner = THIS_MODULE,
			.of_match_table = of_spmi_pmic_match_tbl,
			.suspend = pmic_suspend,
			.resume = pmic_resume,
		},
	.id_table = pmic_spmi_id,
	.probe = pmic_probe,
	.remove = pmic_remove,
};

static int __init pmic_init(void)
{
	return spmi_driver_register(&pmic_driver);
}

static void __exit pmic_exit(void)
{
	spmi_driver_unregister(&pmic_driver);
}

subsys_initcall_sync(pmic_init);
module_exit(pmic_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PMIC driver");
