/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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
#include "ktz8864.h"
#include <linux/module.h>
#include <linux/leds.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <linux/semaphore.h>
#include <platform_include/basicplatform/linux/hw_cmdline_parse.h>  // for runmode_is_factory
#include "dkmd_blpwm_api.h"

struct class *ktz8864_class = NULL;
struct ktz8864_chip_data *ktz8864_g_chip = NULL;
static bool ktz8864_be_inited;
static unsigned int g_bl_level_enhance_mode;
struct ktz8864_backlight_information g_bl_info;
static int ktz8864_fault_check_support;
static struct backlight_work_mode_reg_info g_bl_work_mode_reg_indo;
static int ktz8864_reg[KTZ8864_RW_REG_MAX] = { 0 };
unsigned int ktz8864_msg_level = 5;
module_param_named(debug_ktz8864_msg_level, ktz8864_msg_level, int, 0644);
MODULE_PARM_DESC(debug_ktz8864_msg_level, "backlight ktz8864 msg level");

static int ktz8864_parse_dts(struct device_node *np)
{
	int ret;
	int i;

	if (np == NULL) {
		ktz8864_err("np is NULL pointer\n");
		return -1;
	}

	for (i = 0; i < KTZ8864_RW_REG_MAX; i++) {
		ret = of_property_read_u32(np, ktz8864_dts_string[i], &ktz8864_reg[i]);
		if (ret < 0) {
			ktz8864_reg[i] = INVALID_REG_VAL;
			ktz8864_info("can not find config:%s\n", ktz8864_dts_string[i]);
		} else {
			ktz8864_info("get %s from dts value = 0x%x\n", ktz8864_dts_string[i], ktz8864_reg[i]);
		}
	}
	if (of_property_read_u32(np, "ktz8864_check_fault_support", &ktz8864_fault_check_support) < 0)
		ktz8864_info("No need to detect fault flags!\n");

	return ret;
}

static int ktz8864_config_write(struct ktz8864_chip_data *pchip,
	unsigned int reg[], unsigned int val[], unsigned int size)
{
	int ret = 0;
	unsigned int i;

	if ((pchip == NULL) || (reg == NULL) || (val == NULL)) {
		ktz8864_err("pchip or reg or val is NULL pointer\n");
		return -1;
	}

	for (i = 0; i < size; i++) {
		/* judge reg is invalid */
		if (val[i] == INVALID_REG_VAL)
			continue;
		ret = regmap_write(pchip->regmap, reg[i], val[i]);
		if (ret < 0) {
			ktz8864_err("write ktz8864 backlight config register 0x%x failed\n", reg[i]);
			goto exit;
		} else {
			ktz8864_info("register 0x%x value = 0x%x\n", reg[i], val[i]);
		}
	}

exit:
	return ret;
}

static int ktz8864_config_read(struct ktz8864_chip_data *pchip,
	unsigned int reg[], unsigned int val[], unsigned int size)
{
	int ret;
	unsigned int i;

	if ((pchip == NULL) || (reg == NULL) || (val == NULL)) {
		ktz8864_err("pchip or reg or val is NULL pointer\n");
		return -1;
	}

	for (i = 0; i < size; i++) {
		ret = regmap_read(pchip->regmap, reg[i], &val[i]);
		if (ret < 0) {
			ktz8864_err("read ktz8864 backlight config register 0x%x failed", reg[i]);
			goto exit;
		} else {
			ktz8864_info("read 0x%x value = 0x%x\n", reg[i], val[i]);
		}
	}

exit:
	return ret;
}

static ssize_t ktz8864_reg_bl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ktz8864_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	ssize_t ret;
	unsigned int bl_lsb = 0;
	unsigned int bl_msb = 0;
	int bl_level;

	if (!dev)
		return snprintf(buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf(buf, PAGE_SIZE, "data is null\n");

	client = pchip->client;
	if (!client)
		return snprintf(buf, PAGE_SIZE, "client is null\n");

	ret = regmap_read(pchip->regmap, REG_BL_BRIGHTNESS_MSB, &bl_msb);
	if (ret < 0)
		return snprintf(buf, PAGE_SIZE, "KTZ8864 I2C read error\n");

	ret = regmap_read(pchip->regmap, REG_BL_BRIGHTNESS_LSB, &bl_lsb);
	if (ret < 0)
		return snprintf(buf, PAGE_SIZE, "KTZ8864 I2C read error\n");

	bl_level = (bl_msb << MSB) | bl_lsb;

	return snprintf(buf, PAGE_SIZE, "KTZ8864 bl_level:%d\n", bl_level);
}

static ssize_t ktz8864_reg_bl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret;
	struct ktz8864_chip_data *pchip = NULL;
	unsigned int bl_level = 0;
	unsigned int bl_msb;
	unsigned int bl_lsb;

	if (!dev) {
		ktz8864_err("dev is null\n");
		return -1;
	}

	pchip = dev_get_drvdata(dev);
	if (!pchip) {
		ktz8864_err("data is null\n");
		return -1;
	}

	ret = kstrtouint(buf, 10, &bl_level); // 10 means decimal
	if (ret) {
		ktz8864_err("input conversion fail\n");
		return -1;
	}

	ktz8864_info("buf=%s,state=%d\n", buf, bl_level);

	if (bl_level > BL_MAX)
		bl_level = BL_MAX;

	/* 11-bit brightness code */
	bl_msb = bl_level >> MSB;
	bl_lsb = bl_level & LSB;

	ktz8864_info("bl_level = %d, bl_msb = %d, bl_lsb = %d\n", bl_level, bl_msb, bl_lsb);

	ret = regmap_update_bits(pchip->regmap, REG_BL_BRIGHTNESS_LSB, MASK_BL_LSB, bl_lsb);
	if (ret < 0)
		goto i2c_error;

	ret = regmap_write(pchip->regmap, REG_BL_BRIGHTNESS_MSB, bl_msb);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	ktz8864_err("i2c access fail to register\n");
	return -1;
}

static DEVICE_ATTR(reg_bl, 0644, ktz8864_reg_bl_show, ktz8864_reg_bl_store);

static ssize_t ktz8864_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ktz8864_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	ssize_t ret;
	unsigned char val[REG_MAX] = { 0 };

	if (!dev)
		return snprintf(buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf(buf, PAGE_SIZE, "data is null\n");

	client = pchip->client;
	if (!client)
		return snprintf(buf, PAGE_SIZE, "client is null\n");

	ret = regmap_bulk_read(pchip->regmap, REG_REVISION, &val[0], REG_MAX);
	if (ret < 0)
		goto i2c_error;

	return snprintf(buf, PAGE_SIZE, "Revision(0x01)= 0x%x\nBacklight Configuration1(0x02)= 0x%x\n"
			"\rBacklight Configuration2(0x03) = 0x%x\nBacklight Brightness LSB(0x04) = 0x%x\n"
			"\rBacklight Brightness MSB(0x05) = 0x%x\nBacklight Auto-Frequency Low(0x06) = 0x%x\n"
			"\rBacklight Auto-Frequency High(0x07) = 0x%x\nBacklight Enable(0x08) = 0x%x\n"
			"\rDisplay Bias Configuration 1(0x09)  = 0x%x\nDisplay Bias Configuration 2(0x0A)  = 0x%x\n"
			"\rDisplay Bias Configuration 3(0x0B) = 0x%x\nLCM Boost Bias Register(0x0C) = 0x%x\n"
			"\rVPOS Bias Register(0x0D) = 0x%x\nVNEG Bias Register(0x0E) = 0x%x\n"
			"\rFlags Register(0x0F) = 0x%x\nBacklight Option 1 Register(0x10) = 0x%x\n"
			"\rBacklight Option 2 Register(0x11) = 0x%x\nPWM-to-Digital Code Readback LSB(0x12) = 0x%x\n"
			"\rPWM-to-Digital Code Readback MSB(0x13) = 0x%x\nReg Ramp Time(0x14) = 0x%x\n"
			"\rReg Led Current(0x15) = 0x%x\n",
			val[0], val[1], val[2], val[3], val[4], val[5], val[6],
			val[7], val[8], val[9], val[10], val[11], val[12],
			val[13], val[14], val[15], val[16], val[17], val[18], val[19], val[20]);
			/* 0~18 means number of registers */

i2c_error:
	return snprintf(buf, PAGE_SIZE, "%s: i2c access fail to register\n", __func__);
}

static ssize_t ktz8864_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret;
	struct ktz8864_chip_data *pchip = NULL;
	unsigned int reg = 0;
	unsigned int mask = 0;
	unsigned int val = 0;

	if (!dev) {
		ktz8864_err("dev is null\n");
		return -1;
	}

	pchip = dev_get_drvdata(dev);
	if (!pchip) {
		ktz8864_err("data is null\n");
		return -1;
	}

	ret = sscanf(buf, "reg=0x%x, mask=0x%x, val=0x%x", &reg, &mask, &val);
	if (ret < 0) {
		ktz8864_err("check your input!!!\n");
		return -1;
	}

	if (reg > REG_MAX) {
		ktz8864_err("Invalid argument!!!\n");
		return -1;
	}

	ktz8864_info("reg=0x%x,mask=0x%x,val=0x%x\n", reg, mask, val);

	ret = regmap_update_bits(pchip->regmap, reg, mask, val);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	ktz8864_err("i2c access fail to register\n");
	return -1;
}

static DEVICE_ATTR(reg, 0644, ktz8864_reg_show, ktz8864_reg_store);

static void ktz8864_bl_mode_reg_init(unsigned int reg[], unsigned int val[], unsigned int size)
{
	unsigned int i;
	unsigned int reg_element_num;

	if ((reg == NULL) || (val == NULL)) {
		ktz8864_err("reg or val is NULL pointer\n");
		return;
	}

	ktz8864_info("ktz8864_bl_mode_reg_init in\n");
	(void)memset(&g_bl_work_mode_reg_indo, 0, sizeof(struct backlight_work_mode_reg_info));

	for (i = 0; i < size; i++) {
		switch (reg[i]) {
		case REG_BL_CONFIG_1:
		case REG_BL_CONFIG_2:
		case REG_BL_OPTION_2:
			reg_element_num = g_bl_work_mode_reg_indo.
				bl_mode_config_reg.reg_element_num;
			if (reg_element_num >= BL_MAX_CONFIG_REG_NUM)
				break;
			g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_addr[reg_element_num] = reg[i];
			g_bl_work_mode_reg_indo.bl_mode_config_reg.normal_reg_var[reg_element_num] = val[i];
			if (reg[i] == REG_BL_CONFIG_1)
				g_bl_work_mode_reg_indo.bl_mode_config_reg.enhance_reg_var[reg_element_num] =
					BL_OVP_29V;
			else if (reg[i] == REG_BL_CONFIG_2)
				g_bl_work_mode_reg_indo.bl_mode_config_reg.enhance_reg_var[reg_element_num] =
					CURRENT_RAMP_5MS;
			else
				g_bl_work_mode_reg_indo.bl_mode_config_reg.enhance_reg_var[reg_element_num] =
					BL_OCP_2;

			ktz8864_info("reg_addr:0x%x, normal_val=0x%x, enhance_val=0x%x\n",
				g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_addr[reg_element_num],
				g_bl_work_mode_reg_indo.bl_mode_config_reg.normal_reg_var[reg_element_num],
				g_bl_work_mode_reg_indo.bl_mode_config_reg.enhance_reg_var[reg_element_num]);

			g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_element_num++;
			break;
		case REG_BL_ENABLE:
			reg_element_num = g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_element_num;
			if (reg_element_num >= BL_MAX_CONFIG_REG_NUM)
				break;
			g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_addr[reg_element_num] = reg[i];
			g_bl_work_mode_reg_indo.bl_enable_config_reg.normal_reg_var[reg_element_num] = val[i];
			g_bl_work_mode_reg_indo.bl_enable_config_reg.enhance_reg_var[reg_element_num] =
				EN_4_SINK;

			ktz8864_info("reg_addr:0x%x, normal_val=0x%x, enhance_val=0x%x\n",
				g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_addr[reg_element_num],
				g_bl_work_mode_reg_indo.bl_enable_config_reg.normal_reg_var[reg_element_num],
				g_bl_work_mode_reg_indo.bl_enable_config_reg.enhance_reg_var[reg_element_num]);

			g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_element_num++;
			break;
		default:
			break;
		}
	}

	reg_element_num =
		g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num;
	g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr[reg_element_num] = REG_BL_BRIGHTNESS_LSB;
	g_bl_work_mode_reg_indo.bl_current_config_reg.normal_reg_var[reg_element_num] = LSB_10MA;
	g_bl_work_mode_reg_indo.bl_current_config_reg.enhance_reg_var[reg_element_num] = g_bl_level_enhance_mode & LSB;

	ktz8864_info("reg_addr:0x%x, normal_val=0x%x, enhance_val=0x%x\n",
		g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr[reg_element_num],
		g_bl_work_mode_reg_indo.bl_current_config_reg.normal_reg_var[reg_element_num],
		g_bl_work_mode_reg_indo.bl_current_config_reg.enhance_reg_var[reg_element_num]);

	g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num++;
	reg_element_num =
		g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num;
	g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr[reg_element_num] = REG_BL_BRIGHTNESS_MSB;
	g_bl_work_mode_reg_indo.bl_current_config_reg.normal_reg_var[reg_element_num] = MSB_10MA;
	g_bl_work_mode_reg_indo.bl_current_config_reg.enhance_reg_var[reg_element_num] =
		g_bl_level_enhance_mode >> MSB;

	ktz8864_info("reg_addr:0x%x, normal_val=0x%x, enhance_val=0x%x\n",
		g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr[reg_element_num],
		g_bl_work_mode_reg_indo.bl_current_config_reg.normal_reg_var[reg_element_num],
		g_bl_work_mode_reg_indo.bl_current_config_reg.enhance_reg_var[reg_element_num]);

	g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num++;

	ktz8864_info("ktz8864_bl_mode_reg_init success\n");
}

/* initialize chip */
static int ktz8864_chip_init(struct ktz8864_chip_data *pchip)
{
	int ret = -1;
	struct device_node *np = NULL;

	ktz8864_info("in!\n");

	if (pchip == NULL) {
		ktz8864_err("pchip is NULL pointer\n");
		return -1;
	}

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_KTZ8864);
	if (!np) {
		ktz8864_err("NOT FOUND device node %s!\n", DTS_COMP_KTZ8864);
		goto out;
	}

	ret = ktz8864_parse_dts(np);
	if (ret < 0) {
		ktz8864_err("parse ktz8864 dts failed");
		goto out;
	}

	ret = of_property_read_u32(np, "ktz8864_bl_max_level", &g_bl_level_enhance_mode);
	if (ret != 0)
		g_bl_level_enhance_mode = BL_MAX;
	ktz8864_info("g_bl_level_enhance_mode = %d\n", g_bl_level_enhance_mode);

	ret = ktz8864_config_write(pchip, ktz8864_reg_addr, ktz8864_reg, KTZ8864_RW_REG_MAX);
	if (ret < 0) {
		ktz8864_err("ktz8864 config register failed");
		goto out;
	}

	ret = ktz8864_config_read(pchip, ktz8864_reg_addr, ktz8864_reg, KTZ8864_RW_REG_MAX);
	if (ret < 0) {
		ktz8864_err("ktz8864 config read failed");
		goto out;
	}

	ktz8864_bl_mode_reg_init(ktz8864_reg_addr, ktz8864_reg, KTZ8864_RW_REG_MAX);

	ktz8864_info("ok!\n");

	return ret;

out:
	dev_err(pchip->dev, "i2c failed to access register\n");
	return ret;
}

static void ktz8864_check_fault(struct ktz8864_chip_data *pchip, int last_level, int level)
{
	unsigned int val = 0;
	int ret;

	(void)last_level;
	(void)level;

	ktz8864_info("backlight check FAULT_FLAG!\n");

	ret = regmap_read(pchip->regmap, REG_FLAGS, &val);
	if (ret < 0) {
		ktz8864_info("read lm36274 FAULT_FLAG failed!\n");
		return;
	}
}


static int ktz8864_blpwm_on(void *data)
{
	(void)data;

	if (!ktz8864_be_inited) {
		ktz8864_chip_init(ktz8864_g_chip);
	} else { /* backlight enable gpio keep on，so no need do init */
		ktz8864_info("has already initied!\n");
	}

	return 0;
}

/*
 * ktz8864_set_backlight_reg(): Set Backlight working mode
 *
 * @bl_level: value for backlight ,range from 0 to 2047
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
static int ktz8864_set_backlight_reg(void *bl_ctrl, uint32_t bl_level)
{
	ssize_t ret = -1;
	uint32_t level;
	int bl_msb;
	int bl_lsb;
	static int last_level = -1;

	(void)bl_ctrl;

	if (!ktz8864_be_inited) {
		ktz8864_err("init fail, return.\n");
		return ret;
	}

	if (down_trylock(&(ktz8864_g_chip->test_sem))) {
		ktz8864_info("Now in test mode\n");
		return 0;
	}

	level = bl_level;
	if (level > BL_MAX)
		level = BL_MAX;

	/* 11-bit brightness code */
	bl_msb = level >> MSB;
	bl_lsb = level & LSB;

	ktz8864_info("[backlight] level = %d, bl_msb = %d, bl_lsb = %d\n", level, bl_msb, bl_lsb);

	ret = regmap_update_bits(ktz8864_g_chip->regmap, REG_BL_BRIGHTNESS_LSB, MASK_BL_LSB, bl_lsb);
	if (ret < 0)
		goto i2c_error;

	ret = regmap_write(ktz8864_g_chip->regmap, REG_BL_BRIGHTNESS_MSB, bl_msb);
	if (ret < 0)
		goto i2c_error;

	/* Judge power on or power off */
	if (ktz8864_fault_check_support && ((last_level <= 0 && level != 0) ||
		(last_level > 0 && level == 0)))
		ktz8864_check_fault(ktz8864_g_chip, last_level, level);

	last_level = level;
	up(&(ktz8864_g_chip->test_sem));
	ktz8864_info("ktz8864_set_backlight_reg exit succ\n");
	return ret;

i2c_error:
	up(&(ktz8864_g_chip->test_sem));
	dev_err(ktz8864_g_chip->dev, "%s:i2c access fail to register\n", __func__);
	ktz8864_info("ktz8864_set_backlight_reg exit fail\n");
	return ret;
}

static struct blpwm_dev_ops ktz8864_bl_ops = {
	.on = ktz8864_blpwm_on,
	.set_backlight = ktz8864_set_backlight_reg,
};

static const struct regmap_config ktz8864_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
};

/* pointers to created device attributes */
static struct attribute *ktz8864_attributes[] = {
	&dev_attr_reg_bl.attr,
	&dev_attr_reg.attr,
	NULL,
};

static const struct attribute_group ktz8864_group = {
	.attrs = ktz8864_attributes,
};

static int ktz8864_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = NULL;
	struct ktz8864_chip_data *pchip = NULL;
	int ret;

	ktz8864_info("in!\n");
	if (client == NULL) {
		ktz8864_err("client is NULL pointer\n");
		return -1;
	}

	adapter = client->adapter;
	if (adapter == NULL) {
		ktz8864_err("adapter is NULL pointer\n");
		return -1;
	}

	if (i2c_check_functionality(adapter, I2C_FUNC_I2C) == 0) {
		dev_err(&client->dev, "i2c functionality check fail.\n");
		return -EOPNOTSUPP;
	}

	pchip = devm_kzalloc(&client->dev, sizeof(struct ktz8864_chip_data), GFP_KERNEL);
	if (!pchip)
		return -ENOMEM;

#ifdef CONFIG_REGMAP_I2C
	pchip->regmap = devm_regmap_init_i2c(client, &ktz8864_regmap);
	if (IS_ERR(pchip->regmap)) {
		ret = PTR_ERR(pchip->regmap);
		dev_err(&client->dev, "fail : allocate register map: %d\n", ret);
		goto err_out;
	}
#endif

	pchip->client = client;
	i2c_set_clientdata(client, pchip);

	sema_init(&(pchip->test_sem), 1);

	/* chip initialize */
	ret = ktz8864_chip_init(pchip);
	if (ret < 0) {
		dev_err(&client->dev, "fail : chip init\n");
		goto err_out;
	}

	pchip->dev = device_create(ktz8864_class, NULL, 0, "%s", client->name);
	if (IS_ERR(pchip->dev)) {
		/* Not fatal */
		ktz8864_err("Unable to create device; errno = %ld\n", PTR_ERR(pchip->dev));
		pchip->dev = NULL;
	} else {
		dev_set_drvdata(pchip->dev, pchip);
		ret = sysfs_create_group(&pchip->dev->kobj, &ktz8864_group);
		if (ret)
			goto err_sysfs;
	}

	ktz8864_g_chip = pchip;

	ktz8864_info("name: %s, address: (0x%x) ok!\n", client->name, client->addr);

	ktz8864_be_inited = true;

	dkmd_blpwm_register_bl_device(KTZ8864_NAME, &ktz8864_bl_ops);
	return ret;

err_sysfs:
	device_destroy(ktz8864_class, 0);
err_out:
	devm_kfree(&client->dev, pchip);
	return ret;
}

static const struct i2c_device_id ktz8864_id[] = {
	{ KTZ8864_NAME, 0 },
	{}
};

static const struct of_device_id ktz8864_of_id_table[] = {
	{ .compatible = "ktz,ktz8864" },
	{},
};

MODULE_DEVICE_TABLE(i2c, ktz8864_id);
static struct i2c_driver ktz8864_i2c_driver = {
	.driver = {
		.name = "ktz8864",
		.owner = THIS_MODULE,
		.of_match_table = ktz8864_of_id_table,
	},
	.probe = ktz8864_probe,
	.id_table = ktz8864_id,
};

static int __init ktz8864_module_init(void)
{
	int ret;

	ktz8864_info("in!\n");

	ktz8864_class = class_create(THIS_MODULE, "ktz8864");
	if (IS_ERR(ktz8864_class)) {
		ktz8864_err("Unable to create ktz8864 class; errno = %ld\n", PTR_ERR(ktz8864_class));
		ktz8864_class = NULL;
	}

	ret = i2c_add_driver(&ktz8864_i2c_driver);
	if (ret != 0)
		ktz8864_err("Unable to register ktz8864 driver\n");

	ktz8864_info("ok!\n");
	return ret;
}

device_initcall(ktz8864_module_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Backlight driver for ktz8864");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
