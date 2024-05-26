/*
 * QIC dump Module.
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#include "dfx_sec_qic_dump.h"
#include "dfx_sec_qic_err_probe.h"
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <linux/printk.h>

static int dfx_sec_qic_dump(void *dump_addr, u32 size)
{
	struct dfx_sec_qic_device *qic_dev = dfx_sec_qic_get_dev();
	static u32 dump_done = 0;

	if (!qic_dev) {
		pr_err("qic init fail\n");
		return 0;
	}
	if (!dump_done) {
		dfx_sec_qic_err_probe_handler(qic_dev);
		dump_done = 1;
	}
	return 1;
}

int dfx_sec_qic_dump_init(void)
{
	int ret;

	ret = register_module_dump_mem_func(dfx_sec_qic_dump, "QIC", MODU_QIC);

	return ret;
}

