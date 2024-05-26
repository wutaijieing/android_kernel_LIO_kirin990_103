/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "Kunit.h"
#include "Stub.h"

#include "cmdlist.h"
#include "hisi_dm.h"

void cmdlist_alloc_test(void)
{
	struct hisi_dm_param *dm = NULL;
	struct cmdlist_client *client_mode0 = NULL;
	struct cmdlist_client *client_mode1 = NULL;
	struct cmdlist_client *client_mode2 = NULL;

	pr_info("here is cmdlist alloc buffer test!\n");
	client_mode0 = dpu_cmdlist_create_client(DPU_SCENE_ONLINE_0, TRANSPORT_MODE);
	client_mode1 = dpu_cmdlist_create_client(DPU_SCENE_ONLINE_0, TRANSPORT_MODE);

	CHECK_PTR_NOT_NULL(client_mode0);
	CHECK_PTR_NOT_NULL(client_mode1);

	dpu_cmdlist_set_reg(client_mode0, 0x12000, 0);
	dpu_cmdlist_set_reg(client_mode0, 0x12004, 1);
	dpu_cmdlist_set_reg(client_mode0, 0x12008, 2);
	dpu_cmdlist_set_reg(client_mode0, 0x1200c, 3);
	dpu_cmdlist_set_reg(client_mode0, 0x12010, 4);
	dpu_cmdlist_set_reg(client_mode0, 0x12014, 5);
	dpu_cmdlist_set_reg(client_mode0, 0x12018, 6);
	dpu_cmdlist_set_reg(client_mode0, 0x1201c, 7);
	dpu_cmdlist_set_reg(client_mode0, 0x12020, 8);
	dpu_cmdlist_set_reg(client_mode0, 0x12024, 9);
	dpu_cmdlist_set_reg(client_mode0, 0x12028, 10);

	dpu_cmdlist_set_reg(client_mode0, 0x12000, 0);
	dpu_cmdlist_set_reg(client_mode0, 0x12004, 1);
	dpu_cmdlist_set_reg(client_mode0, 0x12008, 2);
	dpu_cmdlist_set_reg(client_mode0, 0x1200c, 3);
	dpu_cmdlist_set_reg(client_mode0, 0x12010, 4);
	dpu_cmdlist_set_reg(client_mode0, 0x12014, 5);
	dpu_cmdlist_set_reg(client_mode0, 0x12018, 6);
	dpu_cmdlist_set_reg(client_mode0, 0x1201c, 7);
	dpu_cmdlist_set_reg(client_mode0, 0x12020, 8);
	dpu_cmdlist_set_reg(client_mode0, 0x12024, 9);
	dpu_cmdlist_set_reg(client_mode0, 0x12028, 10);

	dpu_cmdlist_dump_client(client_mode0);

	dm = (struct hisi_dm_param *)client_mode1->list_cmd_item;

	dm->scene_info.ddic_number_union.value = 17;
	dm->scene_info.layer_number_union.value = 18;
	dm->scene_info.srot_number_union.value = 19;

	dpu_cmdlist_dump_client(client_mode1);

	client_mode2 = dpu_cmdlist_create_client(DPU_SCENE_ONLINE_1, TRANSPORT_MODE);
	dm = (struct hisi_dm_param *)client_mode2->list_cmd_item;
	dm->scene_info.ddic_number_union.value = 17;
	dm->scene_info.layer_number_union.value = 18;
	dm->scene_info.srot_number_union.value = 19;

	dpu_cmdlist_dump_client(client_mode2);

	cmdlist_release_client(client_mode1);
	cmdlist_release_client(client_mode2);
	cmdlist_release_client(client_mode0);
}

void cmdlist_link_test(void)
{
	struct hisi_dm_param *dm = NULL;
	struct cmdlist_client *client = NULL;

	pr_info("here is cmdlist link buffer test!\n");

	client = dpu_cmdlist_create_client(DPU_SCENE_ONLINE_0, TRANSPORT_MODE);
	CHECK_PTR_NOT_NULL(client);

	pr_info("config  client set+++++!\n");
	dpu_cmdlist_set_reg(client, 0x12000, 0);
	dpu_cmdlist_set_reg(client, 0x12004, 1);
	dpu_cmdlist_set_reg(client, 0x12008, 2);
	dpu_cmdlist_set_reg(client, 0x1200c, 3);
	dpu_cmdlist_set_reg(client, 0x12010, 4);
	dpu_cmdlist_set_reg(client, 0x12014, 5);
	dpu_cmdlist_set_reg(client, 0x12018, 6);
	dpu_cmdlist_set_reg(client, 0x1201c, 7);
	dpu_cmdlist_set_reg(client, 0x12020, 8);
	dpu_cmdlist_set_reg(client, 0x12024, 9);
	dpu_cmdlist_set_reg(client, 0x12028, 10);

	dpu_cmdlist_set_reg(client, 0x12000, 0);
	dpu_cmdlist_set_reg(client, 0x12004, 1);
	dpu_cmdlist_set_reg(client, 0x12008, 2);
	dpu_cmdlist_set_reg(client, 0x1200c, 3);
	dpu_cmdlist_set_reg(client, 0x12010, 4);
	dpu_cmdlist_set_reg(client, 0x12014, 5);
	dpu_cmdlist_set_reg(client, 0x12018, 6);
	dpu_cmdlist_set_reg(client, 0x1201c, 7);
	dpu_cmdlist_set_reg(client, 0x12020, 8);
	dpu_cmdlist_set_reg(client, 0x12024, 9);
	dpu_cmdlist_set_reg(client, 0x12028, 10);

	dpu_cmdlist_set_reg(client, 0x12000, 0);
	dpu_cmdlist_set_reg(client, 0x12004, 1);
	dpu_cmdlist_set_reg(client, 0x12008, 2);
	dpu_cmdlist_set_reg(client, 0x1200c, 3);
	dpu_cmdlist_set_reg(client, 0x12010, 4);
	dpu_cmdlist_set_reg(client, 0x12014, 5);
	dpu_cmdlist_set_reg(client, 0x12018, 6);
	dpu_cmdlist_set_reg(client, 0x1201c, 7);
	dpu_cmdlist_set_reg(client, 0x12020, 8);
	dpu_cmdlist_set_reg(client, 0x12024, 9);
	dpu_cmdlist_set_reg(client, 0x12028, 10);

	dpu_cmdlist_dump_client(client);

	dm = (struct hisi_dm_param *)client->list_cmd_item;
	dm->scene_info.ddic_number_union.value = 17;
	dm->scene_info.layer_number_union.value = 18;
	dm->scene_info.srot_number_union.value = 19;

	dpu_cmdlist_dump_client(client);

	pr_info("cmdlist release client 1!\n");
	cmdlist_release_client(client);
}

KU_TestInfo test_cmdlist[] = {
	{"test_cmdlist_alloc", cmdlist_alloc_test},
	{"test_cmdlist_link", cmdlist_link_test},
	KU_TEST_INFO_NULL
};

int cmdlist_test_suite_init(void)
{
	pr_info("hisi cmdlist device init!\n");
	return 0;
}

int cmdlist_test_suite_clean(void)
{
	pr_info("hisi cmdlist device deinit!\n");
	return 0;
}

KU_SuiteInfo cmdlist_test_suites[]=
{
	{"cmdlist_test_init", cmdlist_test_suite_init, cmdlist_test_suite_clean, test_cmdlist, KU_TRUE},
	KU_SUITE_INFO_NULL
};

static int cmdlist_test_init(void)
{
	pr_info("+++++++++++++++++++++++++++++++++++++++ hello, cmdlist kunit test ++++++++++++++++++++++++++++++++++!\n");

	RUN_ALL_TESTS(cmdlist_test_suites,"/data/log/cmdlist");
	return 0;
}

static void cmdlist_test_exit(void)
{
	pr_info("--------------------------------------- bye, cmdlist kunit test ----------------------------------!\n");
}

module_init(cmdlist_test_init);
module_exit(cmdlist_test_exit);

MODULE_AUTHOR("Kirin Graphics Display");
MODULE_DESCRIPTION("Hisi Cmdlist Test Driver");
MODULE_LICENSE("GPL");
