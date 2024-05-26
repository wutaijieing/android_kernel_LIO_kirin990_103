/*
 * Copyright (c) 2017 AWINIC Technology CO., LTD
 * Description: add FM11N NFC drivers
 * Author: flex He <heyq@tcl.com>
 * Create: 2019-07-22
 */

#include "nfc_kit.h"
#include "securec.h"

#define NFC_ADDR_LEN 2 // 1block is 4page is 16byte
#define NFC_DATA_LEN 16 // 1block is 4page is 16byte
#define NFC_DATA_MAX_LEN 888
#define NFC_DATA_START_REG 0x0010
#define MAX_BUFFER_SIZE 888
#define NFC_FDP_MIRROR_REG_ADDR 0x038c
#define NFC_CHIP_TYPE_ADDR 0x0007
#define NFC_LB0_REG_ADDR 0x000a
#define NFC_LB1_REG_ADDR 0x000b
#define NFC_DYNAMIC_LB_ADDR 0x0388
#define NFC_DYNAMIC_LB_REMAP_ADDR 0x03c8
#define NFC_DELAY_TIME 10
#define NFC_DYNAMIC_LB_LEN 4
#define NFC_RETRYTIMES 3

static struct i2c_client *global_i2c_value = NULL;

static int fm11n_i2c_read_device(struct i2c_client *i2c, unsigned short reg, int bytes, void *dest)
{
	int ret, i;
	unsigned char addr[NFC_ADDR_LEN];

	addr[0] = (unsigned char)(reg >> 8); // reg high addr
	addr[1] = (unsigned char)(reg & 0xff); // reg low addr

	for (i = 0; i < NFC_RETRYTIMES; i++) {
		ret = i2c_master_send(i2c, addr, sizeof(addr));
		if (ret < 0 && i < NFC_RETRYTIMES - 1) {
			NFC_ERR("in func %s ret %d line %d time %d\n", __FUNCTION__, ret, __LINE__, i);
		} else if (ret < 0 && i == NFC_RETRYTIMES - 1) {
			NFC_ERR("in func %s ret %d line %d time %d\n", __FUNCTION__, ret, __LINE__, i);
			return ret;
		} else {
			break;
		}
	}
	mdelay(NFC_DELAY_TIME); // wait 10 ms

	for (i = 0; i < NFC_RETRYTIMES; i++) {
		ret = i2c_master_recv(i2c, dest, bytes);
		if (ret < 0 && i < NFC_RETRYTIMES - 1) {
			NFC_ERR("in func %s ret %d line %d time %d\n", __FUNCTION__, ret, __LINE__, i);
		} else if (ret < 0 && i == NFC_RETRYTIMES - 1) {
			NFC_ERR("in func %s ret %d line %d time %d\n", __FUNCTION__, ret, __LINE__, i);
			return ret;
		} else {
			break;
		}
	}
	if (ret != bytes) {
		NFC_ERR("in func %s ret %d line %d \n", __FUNCTION__, ret,
			__LINE__);
		return -EIO;
	}
	return 0;
}

static int fm11n_i2c_write_device(struct i2c_client *i2c, unsigned short reg, int bytes, const void *src)
{
	int ret, i;
	unsigned char buf[bytes + NFC_ADDR_LEN]; // two byte addr

	buf[1] = (unsigned char)(reg & 0xff); // reg high addr
	buf[0] = (unsigned char)(reg >> 8); // reg low addr
	ret = memcpy_s(&buf[NFC_ADDR_LEN], bytes, src, bytes); // get data
	if (ret != EOK) {
		NFC_ERR("in func %s ret %d line %d\n", __FUNCTION__, ret, __LINE__);
		return NFC_FAIL;
	}

	for (i = 0; i < NFC_RETRYTIMES; i++) {
		ret = i2c_master_send(i2c, buf, bytes + NFC_ADDR_LEN); // add two byte addr
		if (ret < 0 && i < NFC_RETRYTIMES - 1) {
			NFC_ERR("in func %s ret %d line %d time %d\n", __FUNCTION__, ret, __LINE__, i);
		} else if (ret < 0 && i == NFC_RETRYTIMES - 1) {
			NFC_ERR("in func %s ret %d line %d time %d\n", __FUNCTION__, ret, __LINE__, i);
			return ret;
		} else {
			break;
		}
	}
	return 0;
}

static int fm11n_bitlock_enable(void)
{
	unsigned char val0, val1, chip_type;
	int ret, dy_val;

	// read chip type
	ret = fm11n_i2c_read_device(global_i2c_value, NFC_CHIP_TYPE_ADDR, 1, &chip_type);
	if (ret != 0) {
		NFC_ERR(" in func %s read chip type failed\n", __FUNCTION__);
		return ret;
	}
	mdelay(NFC_DELAY_TIME);

	ret = fm11n_i2c_read_device(global_i2c_value, NFC_LB0_REG_ADDR, 1, &val0);
	if (ret != 0) {
		NFC_ERR(" in func %s read device failed\n", __FUNCTION__);
		return ret;
	}
	NFC_INFO(" in func %s read device, val0 = 0x%x, chip type = 0x%x", __func__, val0, chip_type);

	val0 |= 0xf8;
	val1 = 0xff;
	dy_val = 0xffff;
	mdelay(NFC_DELAY_TIME);

	if (chip_type == 0x00) {
		ret = fm11n_i2c_write_device(global_i2c_value, NFC_LB0_REG_ADDR, 1, &val0);
		if (ret < 0) {
			NFC_ERR("%s i2c_write fail ret %d line %d \n", __FUNCTION__, ret, __LINE__);
			return ret;
		}
		mdelay(NFC_DELAY_TIME);

		ret = fm11n_i2c_write_device(global_i2c_value, NFC_LB1_REG_ADDR, 1, &val1);
		if (ret < 0) {
			NFC_ERR("%s i2c_write fail ret %d line %d \n", __FUNCTION__, ret, __LINE__);
			return ret;
		}
		mdelay(NFC_DELAY_TIME);

		ret = fm11n_i2c_write_device(global_i2c_value, NFC_DYNAMIC_LB_ADDR, NFC_DYNAMIC_LB_LEN,
			&dy_val);
		if (ret < 0) {
			NFC_ERR("%s i2c_write fail ret %d line %d \n", __FUNCTION__, ret, __LINE__);
			return ret;
		}
	} else {
		ret = fm11n_i2c_write_device(global_i2c_value, NFC_DYNAMIC_LB_REMAP_ADDR, NFC_DYNAMIC_LB_LEN,
			&dy_val);
		if (ret < 0) {
			NFC_ERR("%s i2c_write fail ret %d line %d \n", __FUNCTION__, ret, __LINE__);
			return ret;
		}
	}
	mdelay(NFC_DELAY_TIME);

	NFC_INFO("%s set bitlock done\n", __func__);
	return NFC_OK;
}

static int fm11n_reg_init(void)
{
	unsigned char reg;
	int ret;

	ret = fm11n_i2c_read_device(global_i2c_value, NFC_FDP_MIRROR_REG_ADDR, 1, &reg);
	if (ret != 0) {
		NFC_ERR("%s fm11n_i2c_read_device failed line %d \n", __FUNCTION__,
			__LINE__);
		return ret;
	}
	mdelay(NFC_DELAY_TIME); // must  delay. otherwise can't receive ACK

	if ((reg & 0x07) != 0x06) { // 0x06 is select intr
		NFC_ERR("%s config FD select mode, reg = 0x%x \n", __func__, reg);
		reg = reg & 0x06;
		ret = fm11n_i2c_write_device(global_i2c_value, NFC_FDP_MIRROR_REG_ADDR, 1, &reg); // write 1byte
	}
	reg = 0x0;
	mdelay(NFC_DELAY_TIME); // 10ms delay. otherwise can't receive ACK
	ret = fm11n_i2c_read_device(global_i2c_value, NFC_FDP_MIRROR_REG_ADDR, 1, &reg);
	if (ret != 0) {
		NFC_ERR("%s fm11n_i2c_read_device failed line %d \n", __FUNCTION__,
			__LINE__);
		return ret;
	}
	mdelay(NFC_DELAY_TIME);
	NFC_INFO(" in func %s get reg value 0x%x\n", __FUNCTION__, reg);
	return NFC_OK;
}

static int i2c_fm11n_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	NFC_INFO("i2c_fm11n_probe enter");

	ret = fm11n_reg_init();
	if (ret != 0) {
		NFC_ERR("%s failed line %d \n", __FUNCTION__, __LINE__);
		return ret;
	}
	ret = fm11n_bitlock_enable();
	if (ret != 0) {
		NFC_ERR("%s failed line %d \n", __FUNCTION__, __LINE__);
		return ret;
	}
	NFC_INFO("i2c_fm11n_probe ok.\n");
	return NFC_OK;
}

static ssize_t fm11n_write(const char *buf, size_t count)
{
	unsigned short reg_addr = NFC_DATA_START_REG;
	int wr_len;
	int rem_len;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;
	NFC_INFO("%s enter \n", __func__);

	rem_len = count;
	do {
		wr_len = (rem_len > NFC_DATA_LEN) ? NFC_DATA_LEN : rem_len;
		rem_len -= wr_len;
		if (fm11n_i2c_write_device(global_i2c_value, reg_addr, wr_len, buf)) {
			NFC_ERR("%s fm11n_i2c_write fail, addr=%x, len=%d", __func__, reg_addr, wr_len);
			return NFC_FAIL;
		}
		reg_addr += wr_len;
		buf += wr_len;
		mdelay(NFC_DELAY_TIME);
	} while (rem_len != 0);

	NFC_INFO("%s success \n", __func__);
	return count;
}

static ssize_t  fm11n_read(char *buf, size_t count)
{
	unsigned short reg_addr = NFC_DATA_START_REG;
	int rd_len;
	int rem_len;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;
	NFC_INFO("%s enter \n", __func__);

	rem_len = count;
	do {
		rd_len = (rem_len > NFC_DATA_LEN) ? NFC_DATA_LEN : rem_len;
		rem_len -= rd_len;
		if (fm11n_i2c_read_device(global_i2c_value, reg_addr, rd_len, buf)) {
			NFC_ERR("%s fm11n_i2c_read fail, addr=%x, len=%d", __func__, reg_addr, rd_len);
			return NFC_FAIL;
		}
		reg_addr += rd_len;
		buf += rd_len;
		mdelay(NFC_DELAY_TIME);
	} while (rem_len != 0);

	NFC_INFO("%s success \n", __func__);
	return count;
}

#define FM11N_CHIP_NAME           "fm,fm11n"

struct nfc_device_data fm11n_data = {0x0};

static int fm11n_chip_init(void)
{
	int ret;
	struct nfc_kit_data *pdata = NULL;

	pdata = get_nfc_kit_data();
	if (pdata == NULL) {
		NFC_ERR("%s: pdata is NULL\n", __func__);
		return NFC_FAIL;
	}

	ret = i2c_fm11n_probe(pdata->client, NULL);
	NFC_INFO("%s: finished, ret = %d\n", __func__, ret);
	return ret;
}

static int fm11n_parse_dts(struct nfc_kit_data *pdata, struct nfc_device_dts *dts)
{
	int ret;
	bool found = false;
	struct device_node *node = NULL;
	u32 slave_addr = 0;

	/* 查询fm11n芯片设备树节点信息 */
	for_each_child_of_node(pdata->client->dev.of_node, node) {
	    if (of_device_is_compatible(node, FM11N_CHIP_NAME)) {
			found = true;
			break;
	    }
	}
	if (!found) {
		NFC_ERR("%s:%s node not found\n", __func__, FM11N_CHIP_NAME);
		return NFC_FAIL;
	}

	/* 获取芯片的slave_address */
	ret = nfc_of_read_u32(node, "slave_address", &slave_addr);
	if (ret != NFC_OK) {
		return NFC_FAIL;
	}
	pdata->client->addr = slave_addr;

	return NFC_OK;
}

static int fm11n_ndef_store(const char *buf, size_t count)
{
	int cnt;
	char tmp[MAX_BUFFER_SIZE + 1] = {0x0};

	// erase data area first
	cnt = fm11n_write(tmp, MAX_BUFFER_SIZE);
	if (cnt != MAX_BUFFER_SIZE) {
		NFC_ERR("%s: erase failed cnt = %d\n", __func__, cnt);
		return NFC_FAIL;
	}

	cnt = fm11n_write(buf, count);
	return (cnt == count) ? NFC_OK : cnt;
}

static int fm11n_ndef_show(char *buf, size_t *count)
{
	int cnt;

	cnt = fm11n_read(buf, *count);
	return (cnt == *count) ? NFC_OK : cnt;
}

static int fm11n_irq_handler(void)
{
	struct nfc_kit_data *pdata = NULL;

	pdata = get_nfc_kit_data();
	if (pdata == NULL) {
		NFC_ERR("%s: pdata is NULL\n", __func__);
		return NFC_FAIL;
	}

	return gpio_get_value(pdata->irq_gpio);
}

static int fm11n_rf_enter(void)
{
	int ret;
	struct nfc_kit_data *pdata = NULL;
	struct device *dev = NULL;
	char *envp[2] = {"ISFIELDON=ON", NULL};

	pdata = get_nfc_kit_data();
	if (pdata == NULL) {
		NFC_ERR("%s: pdata is NULL\n", __func__);
		return NFC_FAIL;
	}
	NFC_INFO("%s begin  \n", __func__);

	if (pdata->is_first_irq) {
		dev = &pdata->client->dev;
		ret = kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
		if (ret < 0) {
			NFC_ERR("%s send uevent fail, ret=%d\n", __func__, ret);
			return NFC_FAIL;
		}

		pdata->is_first_irq = false;
		pdata->irq_cnt = 0x1;
	} else {
		pdata->irq_cnt++;
	}

	NFC_INFO("%s end, irq_cnt=%u\n", __func__, pdata->irq_cnt);
	return NFC_OK;
}

static int fm11n_rf_leave(void)
{
	int ret;
	struct nfc_kit_data *pdata = NULL;
	struct device *dev = NULL;
	char *envp[2] = {"ISFIELDON=OFF", NULL};
	int irq_val[3];
	u32 irq_cnt[3];
	int cnt;
	bool is_left = false;
	u32 i;

	pdata = get_nfc_kit_data();
	if (pdata == NULL || pdata->client == NULL) {
		NFC_ERR("%s: pdata or pdata->client is NULL\n", __func__);
		return NFC_FAIL;
	}
	NFC_INFO("%s begin  \n", __func__);

	while (true) {
		msleep(500);
		if (pdata->is_first_irq) {
			continue;
		}

		/* 连续三次检测中断为1，且中断计数不增加，判断已经离场 */
		NFC_INFO("%s: detect start\n", __func__);
		is_left = false;
		cnt = 0x0;
		memset_s(irq_val, sizeof(irq_val), 0x0, sizeof(irq_val));
		memset_s(irq_cnt, sizeof(irq_cnt), 0x0, sizeof(irq_cnt));
		while (is_left == false) {
			irq_val[cnt] = gpio_get_value(pdata->irq_gpio);
			irq_cnt[cnt] = pdata->irq_cnt;
			cnt = (cnt + 1) % 3;
			for (i = 0x0; i < 3; i++) {
				if (irq_val[i] == 0x0) {
					break;
				}
			}
			if (i < 3) {
				msleep(500);
				continue;
			}
			for (i = 0x0; i < 2; i++) {
				if (irq_cnt[i] != irq_cnt[i + 1]) {
					break;
				}
			}
			if (i < 2) {
				msleep(500);
				continue;
			}
			is_left = true;
		}
		NFC_INFO("%s: detect end, irq_cnt=%u\n", __func__, pdata->irq_cnt);

		dev = &pdata->client->dev;
		ret = kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
		if (ret < 0) {
			NFC_ERR("%s send uevent fail, ret=%d\n", __func__, ret);
			return NFC_FAIL;
		}
		pdata->is_first_irq = true;
	}

	NFC_INFO("%s end  \n", __func__);
	return NFC_OK;
}

struct nfc_device_ops nfc_fm11n_ops = {
	.irq_handler = fm11n_irq_handler,
	.irq0_sch_work = fm11n_rf_enter,
	.irq1_sch_work = fm11n_rf_leave,
	.chip_init = fm11n_chip_init,
	.ndef_store = fm11n_ndef_store,
	.ndef_show = fm11n_ndef_show,
};

int fm11n_probe(struct nfc_kit_data *pdata)
{
	int ret;

	if (pdata == NULL || pdata->client == NULL) {
		NFC_ERR("%s: nfc_kit_data is NULL, error\n", __func__);
		return NFC_FAIL;
	}

	NFC_INFO("%s: called\n", __func__);
	/* 检查I2C功能 */
	if (!i2c_check_functionality(pdata->client->adapter, I2C_FUNC_I2C)) {
		NFC_ERR("%s: i2c function check error\n", __func__);
		return NFC_FAIL;
	}
	global_i2c_value = pdata->client;

	/* 解析设备树 */
#ifdef CONFIG_OF
	ret = fm11n_parse_dts(pdata, &fm11n_data.dts);
	if (ret != NFC_OK) {
		NFC_ERR("%s: parse dts error, ret = %d\n", __func__, ret);
		return NFC_FAIL;
	}
	NFC_INFO("%s: dts parse success\n", __func__);
#else
#error: fm11n only support device tree platform
#endif

	/* 注册芯片数据 */
	fm11n_data.ops = &nfc_fm11n_ops;
	ret = nfc_chip_register(&fm11n_data);
	if (ret != NFC_OK) {
		NFC_ERR("%s:chip register failed, ret = %d\n", __func__, ret);
		return NFC_FAIL;
	}

	NFC_INFO("%s:success\n", __func__);
	return NFC_OK;
}
EXPORT_SYMBOL(fm11n_probe);
