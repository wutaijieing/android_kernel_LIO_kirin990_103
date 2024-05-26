/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description:bus interface.c
 * Create: 2019-11-11
 */
#include <securec.h>
#include <platform_include/smart/linux/inputhub_as/bus_interface.h>
#include <linux/slab.h>
#include <platform_include/smart/linux/iomcu_ipc.h>
#include "iomcu_log.h"

/*
 * Function    : mcu_i3c_rw
 * Description : i3c rw
 * Input       : [bus_num] bus number
 *               [i2c_add] i3c address
 *               [tx] register address
 *               [tx_len] register address len
 *               [rx_out] output value(chip id)
 *               [rx_len] output value len
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int mcu_i3c_rw(uint8_t bus_num, uint8_t i2c_add, uint8_t *tx, uint32_t tx_len,
	uint8_t *rx_out, uint32_t rx_len)
{
	int ret;
	struct sensor_combo_cfg cfg;

	if (!tx || !rx_out) {
		ctxhub_err("%s: input is null\n", __func__);
		return RET_FAIL;
	}

	cfg.bus_type = TAG_I3C;
	cfg.bus_num = bus_num;
	cfg.i2c_address = i2c_add;

	ret = combo_bus_trans(&cfg, tx, tx_len, rx_out, rx_len);
	return ret < 0 ? RET_FAIL : RET_SUCC;
}

/*
 * Function    : mcu_i2c_rw
 * Description : i2c rw
 * Input       : [bus_num] bus number
 *               [i2c_add] i2c address
 *               [tx] register address
 *               [tx_len] register address len
 *               [rx_out] output value(chip id)
 *               [rx_len] output value len
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int mcu_i2c_rw(uint8_t bus_num, uint8_t i2c_add, uint8_t *tx, uint32_t tx_len,
	uint8_t *rx_out, uint32_t rx_len)
{
	int ret;
	struct sensor_combo_cfg cfg;

	if (!tx || !rx_out) {
		ctxhub_err("%s: input is null\n", __func__);
		return RET_FAIL;
	}

	cfg.bus_type = TAG_I2C;
	cfg.bus_num = bus_num;
	cfg.i2c_address = i2c_add;

	ret = combo_bus_trans(&cfg, tx, tx_len, rx_out, rx_len);
	return ret < 0 ? RET_FAIL : RET_SUCC;
}

/*
 * Function    : combo_bus_trans
 * Description : bus packet send to mcu
 * Input       : [p_cfg] bus info
 *             : [tx] regiseter address
 *             : [tx_len] register address len
 *             : [rx_out] output value(chip id)
 *             : [rx_len] output value len
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int mcu_spi_rw(uint8_t bus_num, union spi_ctrl ctrl, uint8_t *tx,
	uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len)
{
	int ret;
	struct sensor_combo_cfg cfg;

	if (!tx || !rx_out) {
		ctxhub_err("%s: input is null\n", __func__);
		return RET_FAIL;
	}

	cfg.bus_type = TAG_SPI;
	cfg.bus_num = bus_num;
	cfg.ctrl = ctrl;

	ret = combo_bus_trans(&cfg, tx, tx_len, rx_out, rx_len);
	return ret < 0 ? RET_FAIL : RET_SUCC;
}


static int set_bus_type_cmd(struct write_info *pkg_ap)
{
	/* just for internal call, will not be null pointer */
	if (pkg_ap->tag == TAG_I2C) {
		pkg_ap->cmd = CMD_I2C_TRANS_REQ;
	} else if (pkg_ap->tag == TAG_SPI) {
		pkg_ap->cmd = CMD_SPI_TRANS_REQ;
	} else if (pkg_ap->tag == TAG_I3C) {
		pkg_ap->cmd = CMD_I3C_TRANS_REQ;
	} else {
		ctxhub_err("%s: bus_type %d unknown\n", __func__, pkg_ap->tag);
		return RET_FAIL;
	}
	return RET_SUCC;
}


static int combo_bus_trans_send(struct write_info *pkg_ap, struct read_info *pkg_mcu,
			uint8_t *rx_out, uint32_t rx_len, uint32_t real_tx_len,
			struct sensor_combo_cfg *p_cfg)
{
	int ret;

	ret = write_customize_cmd(pkg_ap, pkg_mcu, true);
	if (ret) {
		ctxhub_err("send cmd to mcu fail, data=%u, tx_len=%u,rx_len=%u\n",
			p_cfg->data, real_tx_len, rx_len);
		return RET_FAIL;
	}

	if (pkg_mcu->errno != 0) {
		ctxhub_err("mcu_rw fail, data=%u, real_tx_len=%u,rx_len=%u\n",
			p_cfg->data, real_tx_len, rx_len);
		return RET_FAIL;
	}

	if (!rx_out || rx_len == 0) {
		// if out is null, no need to copy result to out
		ctxhub_warn("%s, ouput is null", __func__);
		return RET_SUCC;
	}

	if (sizeof(pkg_mcu->data) < rx_len) {
		ctxhub_err("[%s], copy len is %u, src len is %lu.\n",
				__func__, rx_len, sizeof(pkg_mcu->data));
		return RET_FAIL;
	}

	if (memcpy_s((void *)rx_out, rx_len,
		(void *)pkg_mcu->data, rx_len) != EOK) {
		ctxhub_err("[%s], memcpy failed.\n", __func__);
		return RET_FAIL;
	}

	return RET_SUCC;
}

/*
 * Function    : get_combo_bus_tag
 * Description : according string of bus, get tag
 * Input       : [bus] bus type string
 *             : [tag] output tag
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int combo_bus_trans(struct sensor_combo_cfg *p_cfg, uint8_t *tx,
		uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len)
{
	int ret;
	struct write_info pkg_ap;
	struct read_info pkg_mcu;
	pkt_combo_bus_trans_req_t *pkt_combo_trans = NULL;
	uint32_t cmd_wd_len;
	uint32_t real_tx_len;

	if (p_cfg == NULL) {
		ctxhub_err("[%s]: p_cfg null\n", __func__);
		return RET_FAIL;
	}

	(void)memset_s((void *)&pkg_ap, sizeof(pkg_ap), 0, sizeof(pkg_ap));
	(void)memset_s((void *)&pkg_mcu, sizeof(pkg_mcu), 0, sizeof(pkg_mcu));

	pkg_ap.tag = p_cfg->bus_type;

	/* check and get bus type */
	ret = set_bus_type_cmd(&pkg_ap);
	if (ret != RET_SUCC)
		return RET_FAIL;

	/* get the real tx_len */
	real_tx_len = (tx_len & 0x80) ? (tx_len & 0x7F) : tx_len;

	if (real_tx_len >= (uint32_t) 0xFFFF - sizeof(pkt_combo_bus_trans_req_t)) {
		ctxhub_err("%s: tx_len %x too big\n", __func__, real_tx_len);
		return RET_FAIL;
	}
	cmd_wd_len = real_tx_len + sizeof(pkt_combo_bus_trans_req_t);
	pkt_combo_trans = kzalloc((size_t) cmd_wd_len, GFP_KERNEL);
	if (pkt_combo_trans == NULL) {
		ctxhub_err("alloc failed in %s\n", __func__);
		return RET_FAIL;
	}

	pkt_combo_trans->busid = p_cfg->bus_num;
	pkt_combo_trans->ctrl = p_cfg->ctrl;
	pkt_combo_trans->rx_len = (uint16_t) rx_len;
	pkt_combo_trans->tx_len = (uint16_t) real_tx_len;
	if (real_tx_len != 0 && tx != NULL) {
		if (memcpy_s((void *)pkt_combo_trans->tx, real_tx_len,
			(void *)tx, real_tx_len) != EOK) {
		    kfree(pkt_combo_trans);
		    ctxhub_err("[%s], combo bus memcpy failed!\n", __func__);
		    return RET_FAIL;
		}
	}

	pkg_ap.wr_buf = ((struct pkt_header *) pkt_combo_trans + 1);
	pkg_ap.wr_len = (int)(cmd_wd_len - sizeof(pkt_combo_trans->hd));

	ctxhub_info("%s: tag %d cmd %d data=%u, tx_len=%u,rx_len=%u\n", __func__,
		pkg_ap.tag, pkg_ap.cmd, p_cfg->data, real_tx_len, rx_len);

	ret = combo_bus_trans_send(&pkg_ap, &pkg_mcu, rx_out, rx_len,
		real_tx_len, p_cfg);
	if (ret != RET_SUCC)
		ctxhub_err("[%s], combo bus send failed!\n", __func__);

	kfree(pkt_combo_trans);
	return ret;
}

int get_combo_bus_tag(const char *bus, uint8_t *tag)
{
	enum obj_tag tag_tmp = TAG_END;

	if (bus == NULL || tag == NULL) {
		ctxhub_err("%s: input is null\n", __func__);
		return RET_FAIL;
	}

	if (!strcmp(bus, "i2c"))
		tag_tmp = TAG_I2C;
	else if (!strcmp(bus, "spi"))
		tag_tmp = TAG_SPI;
	else if (!strcmp(bus, "i3c"))
		tag_tmp = TAG_I3C;

	if (tag_tmp == TAG_END)
		return RET_FAIL;
	*tag = (uint8_t) tag_tmp;
	return RET_SUCC;
}
