// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_auth.c
 *
 * authenticate for wireless rx charge
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
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

#include <chipset_common/hwpower/wireless_charge/wireless_rx_auth.h>
#include <chipset_common/hwpower/protocol/wireless_protocol.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_genl.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/completion.h>

#define HWLOG_TAG wireless_rx_auth
HWLOG_REGIST();

#define WLRX_AUTH_RX_RANDOM_LEN    8
#define WLRX_AUTH_TX_HASH_LEN      8
#define WLRX_AUTH_HASH_LEN         (WLRX_AUTH_RX_RANDOM_LEN + WLRX_AUTH_TX_HASH_LEN)
#define WLRX_AUTH_SRV_TIMEOUT      2000 /* 2s */
#define WLRX_AUTH_GENL_OPS_NUM     1

struct wlrx_auth_dev {
	struct completion completion;
	bool srv_ready;
	u8 random[WLRX_AUTH_RX_RANDOM_LEN];
	u8 tx_hash[WLRX_AUTH_TX_HASH_LEN];
	u8 auth_hash[WLRX_AUTH_HASH_LEN]; /* 8bytes random + 8bytes tx_hash */
	int auth_res;
};

static struct wlrx_auth_dev *g_rx_auth_di;

bool wlrx_auth_srv_ready(void)
{
	return g_rx_auth_di && g_rx_auth_di->srv_ready;
}

static int wlrx_auth_wait_completion(struct wlrx_auth_dev *di)
{
	di->auth_res = 0;
	reinit_completion(&di->completion);

	power_genl_easy_send(POWER_GENL_TP_AF, POWER_GENL_CMD_WLRX_AUTH_HASH, 0,
		di->auth_hash, WLRX_AUTH_HASH_LEN);

	if (!wait_for_completion_timeout(&di->completion,
		msecs_to_jiffies(WLRX_AUTH_SRV_TIMEOUT))) {
		hwlog_err("wait_completion: service timeout\n");
		return WLRX_ACC_AUTH_SRV_ERR;
	}

	if (di->auth_res == 0) {
		hwlog_err("wait_completion: hash calculate failed\n");
		return WLRX_ACC_AUTH_SRV_ERR;
	}

	return WLRX_ACC_AUTH_SUCC;
}

int wlrx_auth_handler(unsigned int ic_type, int prot, int kid)
{
	int ret, i;
	struct wlrx_auth_dev *di = g_rx_auth_di;

	if (!di)
		return WLRX_ACC_AUTH_DEV_ERR;

	ret = wireless_auth_encrypt_start(ic_type, WIRELESS_PROTOCOL_QI, kid,
		di->random, WLRX_AUTH_RX_RANDOM_LEN, di->tx_hash, WLRX_AUTH_TX_HASH_LEN);
	if (ret) {
		hwlog_err("auth_handler: get hash from tx failed\n");
		return WLRX_ACC_AUTH_CM_ERR;
	}

	memset(di->auth_hash, 0x00, WLRX_AUTH_HASH_LEN);
	for (i = 0; i < WLRX_AUTH_RX_RANDOM_LEN; i++)
		di->auth_hash[i] = di->random[i];
	for (i = 0; i < WLRX_AUTH_TX_HASH_LEN; i++)
		di->auth_hash[WLRX_AUTH_RX_RANDOM_LEN + i] = di->tx_hash[i];

	return wlrx_auth_wait_completion(di);
}

static int wlrx_auth_srv_callback(unsigned char version, void *data, int len)
{
	struct wlrx_auth_dev *di = g_rx_auth_di;

	if (!di || !data || (len != 1))
		return -EPERM;

	di->auth_res = *(int *)data; /* 1:succ 0: failed */
	complete(&di->completion);

	hwlog_info("[callback] ver=%u res=%d\n", version, di->auth_res);
	return 0;
}

static int wlrx_auth_srv_on_callback(void)
{
	struct wlrx_auth_dev *di = g_rx_auth_di;

	if (!di)
		return -ENODEV;

	di->srv_ready = true;
	hwlog_info("[srv_on_cb] ok\n");
	return 0;
}

static const struct power_genl_easy_ops wlrx_auth_easy_ops[] = {
	{
		.cmd = POWER_GENL_CMD_WLRX_AUTH_HASH,
		.doit = wlrx_auth_srv_callback,
	}
};

static struct power_genl_node wlrx_auth_genl_node = {
	.target = POWER_GENL_TP_AF,
	.name = "WLRX_AUTH",
	.easy_ops = wlrx_auth_easy_ops,
	.n_easy_ops = WLRX_AUTH_GENL_OPS_NUM,
	.srv_on_cb = wlrx_auth_srv_on_callback,
};

static int __init wlrx_auth_init(void)
{
	struct wlrx_auth_dev *di = kzalloc(sizeof(*di), GFP_KERNEL);

	if (!di)
		return -ENOMEM;

	g_rx_auth_di = di;
	di->srv_ready = false;
	init_completion(&di->completion);
	power_genl_easy_node_register(&wlrx_auth_genl_node);
	return 0;
}

static void __exit wlrx_auth_exit(void)
{
	kfree(g_rx_auth_di);
	g_rx_auth_di = NULL;
}

subsys_initcall(wlrx_auth_init);
module_exit(wlrx_auth_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("auth for wireless rx charge module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
