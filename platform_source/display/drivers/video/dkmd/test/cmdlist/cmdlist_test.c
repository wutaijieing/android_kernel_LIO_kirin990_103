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
#include <dpu/soc_dpu_define.h>
#include <dpu/dpu_dm.h>
#include <dpu/dpu_vscf.h>
#include <dkmd_cmdlist.h>
#include "kunit.h"
#include "stub.h"
#include "cmdlist_interface.h"
#ifdef CONFIG_BUILD_TESTCASE_SUITE
#include "testcases.h"
#endif

/**
 * @brief create cmdlist node testcase, this test covers
 * the different scenes and different types
 *
 */
static void cmdlist_create_client_test(void)
{
	uint32_t dm_id = 0, cfg_id = 0, reg_id = 0;
	uint32_t header_cmdlist_id = 0;
	uint32_t scene_id = DPU_SCENE_ONLINE_0;
	uint32_t scene0_header_id = cmdlist_create_user_client(scene_id, SCENE_NOP_TYPE, 0, 0);

	for (scene_id = DPU_SCENE_ONLINE_1; scene_id < DPU_SCENE_MAX; scene_id++) {
		header_cmdlist_id = cmdlist_create_user_client(scene_id, SCENE_NOP_TYPE, 0, 0);
		CHECK_EQUAL(header_cmdlist_id, scene0_header_id + (scene_id - DPU_SCENE_ONLINE_0));
	}

	dm_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, DM_TRANSPORT_TYPE,
		DACC_OFFSET + DM_INPUTDATA_ST_ADDR0, PAGE_SIZE);
	CHECK_NOT_EQUAL(dm_id, 0);
	cfg_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, DATA_TRANSPORT_TYPE, 0x0, PAGE_SIZE);
	CHECK_NOT_EQUAL(cfg_id, 0);
	reg_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, REGISTER_CONFIG_TYPE, 0, PAGE_SIZE);
	CHECK_NOT_EQUAL(reg_id, 0);

	cmdlist_flush_client(DPU_SCENE_ONLINE_0, dm_id);
	cmdlist_append_client(DPU_SCENE_ONLINE_0, scene0_header_id, dm_id);
	cmdlist_flush_client(DPU_SCENE_ONLINE_0, cfg_id);
	cmdlist_append_client(DPU_SCENE_ONLINE_0, scene0_header_id, cfg_id);
	cmdlist_append_client(DPU_SCENE_ONLINE_0, scene0_header_id, reg_id);

	cmdlist_flush_client(DPU_SCENE_ONLINE_0, scene0_header_id);

	for (scene_id = DPU_SCENE_ONLINE_0; scene_id < DPU_SCENE_MAX; scene_id++)
		dkmd_cmdlist_release_locked(scene_id, scene0_header_id + (scene_id - DPU_SCENE_ONLINE_0));
}

/**
 * @brief test cmdlist link -- normal link
 *        scene_client
 *             └── dm_client
 *                    └── node_client
 */
static void cmdlist_normal_link_test(void)
{
	uint32_t scene0_header_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, SCENE_NOP_TYPE, 0, 0);
	uint32_t dm_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, DM_TRANSPORT_TYPE,
		DACC_OFFSET + DM_INPUTDATA_ST_ADDR0, PAGE_SIZE);
	struct dpu_dm_param *dm_param = (struct dpu_dm_param *)cmdlist_get_payload_addr(DPU_SCENE_ONLINE_0, dm_id);
	uint32_t cfg_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, DATA_TRANSPORT_TYPE, 0x0, 64);
	uint32_t *cfg_data = (uint32_t *)cmdlist_get_payload_addr(DPU_SCENE_ONLINE_0, cfg_id);
	uint32_t reg_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, REGISTER_CONFIG_TYPE, 0, PAGE_SIZE);

	CHECK_NOT_EQUAL(scene0_header_id, 0);
	CHECK_NOT_EQUAL(dm_id, 0);
	ASSERT_PTR_NOT_NULL(dm_param);
	CHECK_NOT_EQUAL(cfg_id, 0);
	ASSERT_PTR_NOT_NULL(cfg_data);
	CHECK_NOT_EQUAL(reg_id, 0);

	dm_param->scene_info.srot_number.reg.scene_id = DPU_SCENE_ONLINE_0;
	dm_param->scene_info.scene_reserved.reg.scene_mode = 0;
	memset(&dm_param->cmdlist_addr.cmdlist_h_addr, 0xFF, sizeof(struct dpu_dm_cmdlist_addr));
	dm_param->cmdlist_addr.cmdlist_h_addr.value = 0;
	*cfg_data++ = 0x3FF;
	*cfg_data++ = 0;
	*cfg_data++ = 1080;
	*cfg_data++ = 0;
	*cfg_data++ = 1920;

	dkmd_set_reg(DPU_SCENE_ONLINE_0, reg_id, DPU_ITF_CH_DPP_CLIP_EN_ADDR(DPU_ITF_CH0_OFFSET), 0);
	dkmd_set_reg(DPU_SCENE_ONLINE_0, reg_id, DPU_ITF_CH_DPP_CLRBAR_CTRL_ADDR(DPU_ITF_CH0_OFFSET), 0);
	dkmd_set_reg(DPU_SCENE_ONLINE_0, reg_id, DPU_ITF_CH_REG_CTRL_ADDR(DPU_ITF_CH0_OFFSET), 0);
	dkmd_set_reg(DPU_SCENE_ONLINE_0, reg_id, DPU_ITF_CH_ITFSW_DATA_SEL_ADDR(DPU_ITF_CH0_OFFSET), 0);
	dkmd_set_reg(DPU_SCENE_ONLINE_0, reg_id, DPU_ITF_CH_REG_CTRL_FLUSH_EN_ADDR(DPU_ITF_CH0_OFFSET), 1);

	cmdlist_flush_client(DPU_SCENE_ONLINE_0, dm_id);
	cmdlist_append_client(DPU_SCENE_ONLINE_0, scene0_header_id, dm_id);
	cmdlist_flush_client(DPU_SCENE_ONLINE_0, cfg_id);
	cmdlist_append_client(DPU_SCENE_ONLINE_0, scene0_header_id, cfg_id);
	cmdlist_append_client(DPU_SCENE_ONLINE_0, scene0_header_id, reg_id);

	cmdlist_flush_client(DPU_SCENE_ONLINE_0, scene0_header_id);
	CHECK_EQUAL(cmdlist_get_phy_addr(DPU_SCENE_ONLINE_0, dm_id),
		dkmd_cmdlist_get_dma_addr(DPU_SCENE_ONLINE_0, scene0_header_id));

	/**
	 * @brief Through the head node release all nodes in memory
	 *
	 */
	dkmd_cmdlist_release_locked(DPU_SCENE_ONLINE_0, scene0_header_id);
}

/**
 * @brief test cmdlist link -- operator reuse link
 *        scene_client
 *             └── dm_client ------ scl0 ------ scl0
 *                                   └── scl0_cfg └── scl1_cfg
 */
static void cmdlist_operator_reuse_link_test(void)
{
	uint32_t scene0_header_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, SCENE_NOP_TYPE, 0, 0);
	uint32_t dm_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, DM_TRANSPORT_TYPE,
		DACC_OFFSET + DM_INPUTDATA_ST_ADDR0, PAGE_SIZE);
	struct dpu_dm_param *dm_param = (struct dpu_dm_param *)cmdlist_get_payload_addr(DPU_SCENE_ONLINE_0, dm_id);
	uint32_t scl0_cfg = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, DATA_TRANSPORT_TYPE, 0x78000, 64);
	uint32_t *cfg_data = (uint32_t *)cmdlist_get_payload_addr(DPU_SCENE_ONLINE_0, scl0_cfg);
	uint32_t scl1_cfg = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, REGISTER_CONFIG_TYPE, 0, PAGE_SIZE);

	CHECK_NOT_EQUAL(scene0_header_id, 0);
	CHECK_NOT_EQUAL(dm_id, 0);
	ASSERT_PTR_NOT_NULL(dm_param);
	CHECK_NOT_EQUAL(scl0_cfg, 0);
	ASSERT_PTR_NOT_NULL(cfg_data);
	CHECK_NOT_EQUAL(scl1_cfg, 0);

	dm_param->scene_info.srot_number.reg.scene_id = 0;
	dm_param->scene_info.scene_reserved.reg.scene_mode = 0;
	memset(&dm_param->cmdlist_addr.cmdlist_h_addr, 0xFF, sizeof(struct dpu_dm_cmdlist_addr));
	dm_param->cmdlist_addr.cmdlist_h_addr.value = 0;

	dm_param->scl_info[0].scl0_input_img_width.reg.scl0_input_img_width = 1440;
	dm_param->scl_info[0].scl0_input_img_width.reg.scl0_input_img_height = 2560;
	dm_param->scl_info[0].scl0_output_img_width.reg.scl0_output_img_width = 1080;
	dm_param->scl_info[0].scl0_output_img_width.reg.scl0_output_img_height = 1920;
	dm_param->scl_info[0].scl0_type.reg.scl0_layer_id = 0;
	dm_param->scl_info[0].scl0_type.reg.scl0_order0 = 1;
	dm_param->scl_info[0].scl0_type.reg.scl0_sel = 1;
	dm_param->scl_info[0].scl0_type.reg.scl0_type = 1;
	dm_param->scl_info[0].scl0_threshold.reg.scl0_output_fmt = 0x6;
	dm_param->scl_info[0].scl0_threshold.reg.scl0_input_fmt = 0x6;
	dm_param->cmdlist_addr.cmdlist_scl0_addr.reg.cmdlist_addr5 = cmdlist_get_phy_addr(DPU_SCENE_ONLINE_0, scl0_cfg);
	*cfg_data++ = 0x3FF;
	*cfg_data++ = 0;
	*cfg_data++ = 1080;
	*cfg_data++ = 0;
	*cfg_data++ = 1920;

	dm_param->scl_info[1].scl0_input_img_width.reg.scl0_input_img_width = 1440;
	dm_param->scl_info[1].scl0_input_img_width.reg.scl0_input_img_height = 2560;
	dm_param->scl_info[1].scl0_output_img_width.reg.scl0_output_img_width = 1080;
	dm_param->scl_info[1].scl0_output_img_width.reg.scl0_output_img_height = 1920;
	dm_param->scl_info[1].scl0_type.reg.scl0_layer_id = 1;
	dm_param->scl_info[1].scl0_type.reg.scl0_order0 = 1;
	dm_param->scl_info[1].scl0_type.reg.scl0_sel = 1;
	dm_param->scl_info[1].scl0_type.reg.scl0_type = 1;
	dm_param->scl_info[1].scl0_threshold.reg.scl0_output_fmt = 0x6;
	dm_param->scl_info[1].scl0_threshold.reg.scl0_input_fmt = 0x6;
	dm_param->cmdlist_addr.cmdlist_scl1_addr.reg.cmdlist_addr6 = cmdlist_get_phy_addr(DPU_SCENE_ONLINE_0, scl1_cfg);
	dkmd_set_reg(DPU_SCENE_ONLINE_0, scl1_cfg, DPU_ARSR_SCF_INC_HSCL_ADDR(DPU_VSCF0_OFFSET), 0x123);
	dkmd_set_reg(DPU_SCENE_ONLINE_0, scl1_cfg, DPU_ARSR_SCF_ACC_HSCL_ADDR(DPU_VSCF0_OFFSET), 0x456);
	dkmd_set_reg(DPU_SCENE_ONLINE_0, scl1_cfg, DPU_ARSR_SCF_ACC_HSCL1_ADDR(DPU_VSCF0_OFFSET), 0x789);
	dkmd_set_reg(DPU_SCENE_ONLINE_0, scl1_cfg, DPU_ARSR_SCF_INC_HSCL_ADDR(DPU_VSCF0_OFFSET), 0xabc);
	dkmd_set_reg(DPU_SCENE_ONLINE_0, scl1_cfg, DPU_ARSR_SCF_ACC_VSCL_ADDR(DPU_VSCF0_OFFSET), 0x1);

	cmdlist_flush_client(DPU_SCENE_ONLINE_0, dm_id);
	/* only append dm cmdlist id */
	cmdlist_append_client(DPU_SCENE_ONLINE_0, scene0_header_id, dm_id);
	cmdlist_flush_client(DPU_SCENE_ONLINE_0, scene0_header_id);

	CHECK_EQUAL(cmdlist_get_phy_addr(DPU_SCENE_ONLINE_0, dm_id),
		dkmd_cmdlist_get_dma_addr(DPU_SCENE_ONLINE_0, scene0_header_id));

	cmdlist_flush_client(DPU_SCENE_ONLINE_0, scl0_cfg);
	dkmd_cmdlist_dump_by_id(DPU_SCENE_ONLINE_0, scl0_cfg);
	dkmd_cmdlist_dump_by_id(DPU_SCENE_ONLINE_0, scl1_cfg);

	dkmd_cmdlist_dump_scene(DPU_SCENE_ONLINE_0);
	/**
	 * @brief Through the head node release all nodes in memory, but operator reuse node
	 *        need release by its own call process.
	 *
	 */
	dkmd_cmdlist_release_locked(DPU_SCENE_ONLINE_0, scene0_header_id);

	dkmd_cmdlist_release_locked(DPU_SCENE_ONLINE_0, scl0_cfg);
	dkmd_cmdlist_release_locked(DPU_SCENE_ONLINE_0, scl1_cfg);
}

/**
 * @brief test cmdlist link -- block link
 *        scene_client
 *             ├── dm_client
 *             │       └── reg_id
 *             ├── dm_client
 *             │       └── reg_id
 *             └── dm_client
 *                     └── reg_id
 */
static void cmdlist_block_link_test(void)
{
	uint32_t i = 0, total_block_num = 4;
	struct dpu_dm_param *last_dm_param = NULL;
	struct dpu_dm_param *dm_param = NULL;
	uint32_t frame_addr = 0;
	uint32_t dm_id = 0;
	uint32_t reg_id = 0;

	uint32_t scene0_header_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, SCENE_NOP_TYPE, 0, 0);
	CHECK_NOT_EQUAL(scene0_header_id, 0);

	for (i = 0; i < total_block_num; i++) {
		dm_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, DM_TRANSPORT_TYPE,
			DACC_OFFSET + DM_INPUTDATA_ST_ADDR0, PAGE_SIZE);
		CHECK_NOT_EQUAL(dm_id, 0);

		if (last_dm_param != NULL) {
			last_dm_param->cmdlist_addr.cmdlist_h_addr.reg.cmdlist_next_en = 1;
			last_dm_param->cmdlist_addr.cmdlist_next_addr.reg.cmdlist_addr0 =
				cmdlist_get_phy_addr(DPU_SCENE_ONLINE_0, dm_id);
		}

		dm_param = (struct dpu_dm_param *)cmdlist_get_payload_addr(DPU_SCENE_ONLINE_0, dm_id);
		ASSERT_PTR_NOT_NULL(dm_param);
		dm_param->scene_info.srot_number.reg.scene_id = 0;
		dm_param->scene_info.scene_reserved.reg.scene_mode = 0;
		memset(&dm_param->cmdlist_addr.cmdlist_h_addr, 0xFF, sizeof(struct dpu_dm_cmdlist_addr));
		dm_param->cmdlist_addr.cmdlist_h_addr.value = 0;

		reg_id = cmdlist_create_user_client(DPU_SCENE_ONLINE_0, REGISTER_CONFIG_TYPE, 0, PAGE_SIZE);
		CHECK_NOT_EQUAL(reg_id, 0);
		dkmd_set_reg(DPU_SCENE_ONLINE_0, reg_id, DPU_ITF_CH_DPP_CLIP_EN_ADDR(DPU_ITF_CH0_OFFSET), 0);
		dkmd_set_reg(DPU_SCENE_ONLINE_0, reg_id, DPU_ITF_CH_DPP_CLRBAR_CTRL_ADDR(DPU_ITF_CH0_OFFSET), 0);
		dkmd_set_reg(DPU_SCENE_ONLINE_0, reg_id, DPU_ITF_CH_REG_CTRL_ADDR(DPU_ITF_CH0_OFFSET), 0);
		dkmd_set_reg(DPU_SCENE_ONLINE_0, reg_id, DPU_ITF_CH_ITFSW_DATA_SEL_ADDR(DPU_ITF_CH0_OFFSET), 0);
		dkmd_set_reg(DPU_SCENE_ONLINE_0, reg_id, DPU_ITF_CH_REG_CTRL_FLUSH_EN_ADDR(DPU_ITF_CH0_OFFSET), 1);

		cmdlist_flush_client(DPU_SCENE_ONLINE_0, dm_id);
		cmdlist_append_client(DPU_SCENE_ONLINE_0, scene0_header_id, dm_id);
		cmdlist_append_client(DPU_SCENE_ONLINE_0, scene0_header_id, reg_id);

		last_dm_param = dm_param;
		if (i == 0)
			frame_addr = cmdlist_get_phy_addr(DPU_SCENE_ONLINE_0, dm_id);
	}
	CHECK_EQUAL(dkmd_cmdlist_get_dma_addr(DPU_SCENE_ONLINE_0, scene0_header_id), frame_addr);

	cmdlist_flush_client(DPU_SCENE_ONLINE_0, scene0_header_id);
	dkmd_cmdlist_release_locked(DPU_SCENE_ONLINE_0, scene0_header_id);
}

ku_test_info test_cmdlist[] = {
	{"test_cmdlist_create", cmdlist_create_client_test},
	{"test_cmdlist_normal_link", cmdlist_normal_link_test},
	{"test_cmdlist_reuse_link", cmdlist_operator_reuse_link_test},
	{"test_cmdlist_block_link", cmdlist_block_link_test},
	KU_TEST_INFO_NULL
};

int cmdlist_test_suite_init(void)
{
	pr_info("cmdlist device init!\n");
	return 0;
}

int cmdlist_test_suite_clean(void)
{
	pr_info("cmdlist device deinit!\n");
	return 0;
}

#ifndef CONFIG_BUILD_TESTCASE_SUITE
ku_suite_info cmdlist_test_suites[]=
{
	{"cmdlist_test_init", cmdlist_test_suite_init, cmdlist_test_suite_clean, test_cmdlist, KU_TRUE},
	KU_SUITE_INFO_NULL
};

static int cmdlist_test_init(void)
{
	pr_info("++++++++++++++++ hello, cmdlist kunit test +++++++++++++++++!\n");

	run_all_tests(cmdlist_test_suites, "/data/log/cmdlist");
	return 0;
}

static void cmdlist_test_exit(void)
{
	pr_info("---------------- bye, cmdlist kunit test ----------------!\n");
}

module_init(cmdlist_test_init);
module_exit(cmdlist_test_exit);

MODULE_AUTHOR("Graphics Display");
MODULE_DESCRIPTION("Cmdlist Test Driver");
#endif

MODULE_LICENSE("GPL");