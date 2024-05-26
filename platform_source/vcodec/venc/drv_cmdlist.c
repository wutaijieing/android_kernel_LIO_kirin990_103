/*
 * drv_cmdlist.c
 *
 * This is for Operations Related to cmdlist.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
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

#include "drv_cmdlist.h"
#include <linux/list.h>
#include <linux/workqueue.h>
#include "hal_venc.h"
#include "drv_common.h"
#include "venc_regulator.h"
#include "drv_mem.h"

static struct cmdlist_manager g_cmdlist_manager;
static struct mem_pool *g_cmdlist_pool;

static struct workqueue_struct *g_encode_work_queue;
static struct work_struct g_encode_work;

static struct cmdlist_node *cmdlist_alloc_node(void)
{
	uint64_t iova_addr;
	uint32_t align_size;
	int32_t ret;
	struct cmdlist_node *node = NULL;

	ret = OSAL_WAIT_EVENT_TIMEOUT(&g_cmdlist_pool->event,
			!queue_is_empty(g_cmdlist_pool), ENCODE_DONE_TIMEOUT_MS); /*lint !e578 !e666*/
	if (ret != 0) {
		VCODEC_FATAL_VENC("wait alloc buffer timeout");
		return NULL;
	}

	node = (struct cmdlist_node *)drv_mem_pool_alloc(g_cmdlist_pool, &iova_addr);
	if (node == NULL) {
		VCODEC_FATAL_VENC("alloc cmdlist node memory fail");
		return NULL;
	}

	align_size = align_up(sizeof(struct cmdlist_node), CMDLIST_ALIGN_SIZE);
	node->virt_addr = (void*)(uintptr_t)((uint64_t)(uintptr_t)node + align_size);
	node->iova_addr = iova_addr + align_size;

	return node;
}

static void cmdlist_free_node(struct cmdlist_node *node)
{
	uint32_t offset;
	uint32_t align_size;
	uint64_t iova_addr;

	if (node == NULL) {
		VCODEC_FATAL_VENC("cmdlist node is null");
		return;
	}

	align_size = align_up(sizeof(struct cmdlist_node), CMDLIST_ALIGN_SIZE);
	offset = (uint64_t)(uintptr_t)node->virt_addr - (uint64_t)(uintptr_t)node;
	if (node->iova_addr < offset) {
		VCODEC_FATAL_VENC("iova_addr addr invalid, free cmdlist node memory fail");
		return;
	}

	iova_addr = node->iova_addr - offset;

	drv_mem_pool_free(g_cmdlist_pool, node, iova_addr);
	/* wake up preprocess: The memory pool has free nodes. */
	venc_drv_osal_give_event(&g_cmdlist_pool->event);
}

static struct cmdlist_head *cmdlist_get_head(uint32_t type)
{
	uint32_t i;
	uint32_t wait_idx = g_cmdlist_manager.wait_idx[type];
	struct cmdlist_head *node = NULL;

	if (wait_idx < CMDLIST_MAX_HEAD_NUM) {
		node = &g_cmdlist_manager.head[wait_idx];
		return node;
	}

	for (i = 0; i < CMDLIST_MAX_HEAD_NUM; i++) {
		if (!test_bit(i, &g_cmdlist_manager.bit_map)) {
			node = &g_cmdlist_manager.head[i];
			g_cmdlist_manager.wait_idx[type] = i;
			set_bit(i, &g_cmdlist_manager.bit_map);
			return node;
		}
	}

	return NULL;
}

static void cmdlist_put_head(struct cmdlist_head *head)
{
	if (head < g_cmdlist_manager.head) {
		VCODEC_FATAL_VENC("current head is invalid");
		return;
	}

	if (!list_empty(&head->list))
		VCODEC_FATAL_VENC("current head is not empty");

	INIT_LIST_HEAD(&head->list);
	head->num = 0;
	clear_bit(head - g_cmdlist_manager.head, &g_cmdlist_manager.bit_map);
}

static void cmdlist_cfg_smmu(int32_t core_id, int32_t type)
{
	bool is_protected = (type == CMDLIST_PROTECT);
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	struct venc_context *ctx = &venc->ctx[core_id];

	if (is_protected != ctx->is_protected)
		ctx->first_cfg_flag = true;

	if (ctx->first_cfg_flag) {
		venc_smmu_init(is_protected, core_id);
		ctx->first_cfg_flag = false;
	}

	venc_smmu_cfg(NULL, ctx->reg_base);

	ctx->is_protected = is_protected;
}

static int32_t cmdlist_encode(int32_t core_id, uint32_t type)
{
	struct cmdlist_head *head = NULL;
	int32_t ret = 0;
	uint32_t wait_idx;
	uint32_t cur_idx;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	mutex_lock(&g_cmdlist_manager.lock);

	wait_idx = g_cmdlist_manager.wait_idx[type];
	cur_idx = g_cmdlist_manager.cur_idx;
	if (wait_idx >= CMDLIST_MAX_HEAD_NUM) {
		VCODEC_FATAL_VENC("no new node is added to the list");
		ret = VCODEC_FAILURE;
		goto exit;
	}

	if (cur_idx < CMDLIST_MAX_HEAD_NUM) {
		VCODEC_FATAL_VENC("cur num is %u, core id %u",
			g_cmdlist_manager.head[cur_idx].num, core_id);
		ret = VCODEC_FAILURE;
		goto exit;
	}

	head = &g_cmdlist_manager.head[wait_idx];
	if (list_empty(&head->list)) {
		VCODEC_FATAL_VENC("current list is empty");
		ret = VCODEC_FAILURE;
		goto exit;
	}

	VCODEC_DBG_VENC("status %u, cur_idx %u, type %u, wait_idx %u",
		venc->ctx[core_id].status, cur_idx, type, wait_idx);

	g_cmdlist_manager.cur_idx = wait_idx;
	g_cmdlist_manager.wait_idx[type] = CMDLIST_MAX_HEAD_NUM;

	pm_hardware_busy_enter(&venc->ctx[core_id].pm);

	cmdlist_cfg_smmu(core_id, type);

	osal_add_timer(&venc->ctx[core_id].timer, INTERRUPT_TIMEOUT_MS);
	/* start cmdlist */
	venc->ctx[core_id].status = VENC_BUSY;
	head->start_time = osal_get_sys_time_in_us();
	VCODEC_DBG_VENC("start encode head num %u, type %u", head->num, type);

	hal_cmdlist_encode(head, venc->ctx[core_id].reg_base);

exit:
	mutex_unlock(&g_cmdlist_manager.lock);

	return ret;
}

static void cmdlist_encode_handle(struct work_struct *work)
{
	int32_t ret;
	int32_t core_id;
	uint32_t i;
	uint32_t wait_idx;
	struct clock_info info;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	if (!g_cmdlist_manager.bit_map) {
		if (venc_regulator_disable_by_low_power() != 0)
			VCODEC_WARN_VENC("power off regulator failed");
		return;
	}

	venc_get_clock_info(&info);

	if (venc_regulator_update(&info) != 0) {
		VCODEC_ERR_VENC("update regulator status failed");
		return;
	}

	for (i = 0; i < CMDLIST_BUTT; i++) {
		// The index of the waiting linked list may be changed by the cmdlist_prepare_encode.
		// But the waiting linked still can be processed by the next task.
		wait_idx = g_cmdlist_manager.wait_idx[i];
		if (wait_idx >= CMDLIST_MAX_HEAD_NUM ||
			g_cmdlist_manager.head[wait_idx].num == 0)
			continue;

		/* wait cmdlist available */
		core_id = venc_regulator_select_idle_core(&venc->event);
		ret = venc_check_coreid(core_id);
		if (ret != 0) {
			VCODEC_ERR_VENC("CORE_ERROR:invalid core ID is %d", core_id);
			break;
		}

		ret = cmdlist_encode(core_id, i);
		if (ret != 0)
			VCODEC_ERR_VENC("start encode failed");
	}
}

static void cmdlist_process_readback_info(struct cmdlist_head *head,
	uint32_t *reg_base, U_FUNC_VCPI_INTSTAT int_status)
{
	struct cmdlist_node *cmdlist_node = NULL;
	struct cmdlist_node *tmp_cmdlist_node = NULL;
	struct encode_done_info info = {0};
	vedu_osal_event_t *event = NULL;
	int32_t ret;

	if (list_empty(&head->list))
		return;

#ifdef SLICE_INT_EN
	/* only process slice end */
	if (int_status.bits.vcpi_int_vedu_slice_end &&
		!int_status.bits.vcpi_cmdlst_eop &&
		!int_status.bits.vcpi_soft_int0) {
		cmdlist_node = list_first_entry(&head->list, struct cmdlist_node, list);
		(void)memcpy_s(&info.channel_info, sizeof(struct channel_info),
			&cmdlist_node->channel_info, sizeof(struct channel_info));
		venc_hal_get_slice_reg(&info.stream_info, reg_base);
		ret = push(cmdlist_node->buffer, &info);
		if (ret != 0)
			VCODEC_ERR_VENC("write buffer fail");
		/* wake up postprocess */
		venc_drv_osal_give_event(&cmdlist_node->buffer->event);
	}
#endif

	/* process softint0/cmdlist eop/time out */
	list_for_each_entry_safe(cmdlist_node, tmp_cmdlist_node, &head->list, list) {
		if (!(int_status.bits.vcpi_int_vedu_timeout ||
			int_status.bits.vcpi_cmdlst_eop ||
			hal_cmdlist_is_encode_done((uint32_t *)cmdlist_node->virt_addr)))
			break;

		(void)memset_s(&info, sizeof(struct encode_done_info), 0, sizeof(struct encode_done_info));
		(void)memcpy_s(&info.channel_info, sizeof(struct channel_info),
			&cmdlist_node->channel_info, sizeof(struct channel_info));

		if (int_status.bits.vcpi_int_vedu_timeout ||
			!hal_cmdlist_is_encode_done((uint32_t *)cmdlist_node->virt_addr)) {
			info.is_timeout = true;
			VCODEC_WARN_VENC("current isr status %x", int_status.u32);
		} else {
			hal_cmdlist_get_reg(&info.stream_info, (uint32_t *)cmdlist_node->virt_addr);
		}

		ret = push(cmdlist_node->buffer, &info);
		if (ret != 0)
			VCODEC_ERR_VENC("write buffer fail");

		list_del(&cmdlist_node->list);
		event = &cmdlist_node->buffer->event;
		cmdlist_free_node(cmdlist_node);
		/* wake up postprocess */
		venc_drv_osal_give_event(event);
	}
}

static int32_t cmdlist_init(void)
{
	uint32_t i;
	int32_t ret;
	struct platform_device *venc_dev = venc_get_device();

	g_cmdlist_pool = drv_mem_create_pool(&venc_dev->dev, CMDLIST_BUFFER_SIZE,
						CMDLIST_BUFFER_NUM, MIN_PAGE_ALIGN_SIZE);
	if (g_cmdlist_pool == NULL) {
		VCODEC_FATAL_VENC("create cmdlist pool failed");
		return VCODEC_FAILURE;
	}

	g_encode_work_queue = create_freezable_workqueue("encode_work");
	if (g_encode_work_queue == NULL) {
		VCODEC_FATAL_VENC("create encode work queue failed");
		drv_mem_destory_pool(g_cmdlist_pool);
		return VCODEC_FAILURE;
	}
	INIT_WORK(&g_encode_work, cmdlist_encode_handle);

	ret = drv_mem_init();
	if (ret != 0) {
		destroy_workqueue(g_encode_work_queue);
		drv_mem_destory_pool(g_cmdlist_pool);
		return VCODEC_FAILURE;
	}
	g_cmdlist_manager.bit_map = 0;

	for (i = 0; i < CMDLIST_MAX_HEAD_NUM; i++) {
		INIT_LIST_HEAD(&g_cmdlist_manager.head[i].list);
		g_cmdlist_manager.head[i].num = 0;
	}

	for (i = 0; i < CMDLIST_BUTT; i++)
		g_cmdlist_manager.wait_idx[i] = CMDLIST_MAX_HEAD_NUM;

	g_cmdlist_manager.cur_idx = CMDLIST_MAX_HEAD_NUM;
	mutex_init(&g_cmdlist_manager.lock);

	return ret;
}

static void cmdlist_deinit(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	cancel_work(&g_encode_work);
#endif
	flush_workqueue(g_encode_work_queue);
	destroy_workqueue(g_encode_work_queue);
	drv_mem_destory_pool(g_cmdlist_pool);
	drv_mem_exit();
}

static int32_t cmdlist_prepare_encode(struct encode_info *info, struct venc_fifo_buffer *buffer)
{
	struct cmdlist_head *head = NULL;
	struct cmdlist_node *cur_node = NULL;
	int32_t ret = 0;
	uint32_t type;

	venc_set_clock_info(&info->clock_info);

	mutex_lock(&g_cmdlist_manager.lock);

	type = info->is_protected ? CMDLIST_PROTECT : CMDLIST_NORMAL;

	head = cmdlist_get_head(type);
	if (head == NULL) {
		VCODEC_FATAL_VENC("get instance head fail, current type is %d", type);
		ret = VCODEC_FAILURE;
		goto exit;
	}

	cur_node = cmdlist_alloc_node();
	if (cur_node == NULL) {
		/* If the list contains other nodes, head cannot be directly released. */
		if (list_empty(&head->list)) {
			cmdlist_put_head(head);
			g_cmdlist_manager.wait_idx[type] = CMDLIST_MAX_HEAD_NUM;
		}
		VCODEC_FATAL_VENC("alloc cmdlist node fail");
		ret = VCODEC_FAILURE;
		goto exit;
	}

	cur_node->buffer = buffer;

	(void)memcpy_s(&cur_node->channel_info, sizeof(struct channel_info), &info->channel, sizeof(struct channel_info));

	hal_cmdlist_cfg(head, cur_node, info);

	list_add_tail(&cur_node->list, &head->list);

	head->num++;

	queue_work(g_encode_work_queue, &g_encode_work);

exit:
	mutex_unlock(&g_cmdlist_manager.lock);
	return ret;
}

static void cmdlist_encode_done(struct venc_context *ctx)
{
	U_FUNC_VCPI_INTSTAT int_status;
	struct cmdlist_head *head = NULL;
	S_HEVC_AVC_REGS_TYPE *reg_base = NULL;
	uint32_t cur_idx = g_cmdlist_manager.cur_idx;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	unsigned long flag;

	reg_base = (S_HEVC_AVC_REGS_TYPE *)ctx->reg_base;
	spin_lock_irqsave(&ctx->lock, flag);
	if (ctx->status != VENC_BUSY) {
		spin_unlock_irqrestore(&ctx->lock, flag);
		/* The hardware status is incorrect. Do not access the register.Timeout processing clears the interrupt status. */
		VCODEC_FATAL_VENC("current core status invalid, core status: %d", ctx->status);
		return;
	}

	int_status.u32 = reg_base->FUNC_VCPI_INTSTAT.u32;
	if (!int_status.bits.vcpi_soft_int0 && !int_status.bits.vcpi_cmdlst_eop &&
		!int_status.bits.vcpi_int_vedu_slice_end) {
		reg_base->VEDU_VCPI_INTCLR.u32 = int_status.u32;
		spin_unlock_irqrestore(&ctx->lock, flag);
		VCODEC_WARN_VENC("not support isr %x", int_status.u32);
		return;
	}

	if (cur_idx >= CMDLIST_MAX_HEAD_NUM) {
		reg_base->VEDU_VCPI_INTCLR.u32 = int_status.u32;
		spin_unlock_irqrestore(&ctx->lock, flag);
		VCODEC_FATAL_VENC("not find current head");
		return;
	}

	head = &g_cmdlist_manager.head[cur_idx];
	cmdlist_process_readback_info(head, ctx->reg_base, int_status);

	/* process softint0 */
	if (!int_status.bits.vcpi_cmdlst_eop && !int_status.bits.vcpi_int_vedu_slice_end) {
		reg_base->VEDU_VCPI_INTCLR.u32 = int_status.u32;
		spin_unlock_irqrestore(&ctx->lock, flag);
		/* reset timer */
		osal_add_timer(&ctx->timer, INTERRUPT_TIMEOUT_MS);
		return;
	}

	/* process cmdlist eop */
	if (int_status.bits.vcpi_cmdlst_eop) {
#ifdef VENC_DEBUG_ENABLE
		if (venc->debug_flag & (1LL << PRINT_ENC_COST_TIME))
			VCODEC_INFO_VENC("cmdlist num %u, hardware cost time %llu us", head->num,
					osal_get_during_time(head->start_time));
#endif
		if (osal_del_timer(&ctx->timer, false) != 0)
			VCODEC_FATAL_VENC("delete timer fail");

		/* The VEDU_VCPI_INTCLR register must be configured before the hardware is set to the idle status */
		reg_base->VEDU_VCPI_INTCLR.u32 = int_status.u32;

		pm_hardware_busy_exit(&ctx->pm);
		g_cmdlist_manager.cur_idx = CMDLIST_MAX_HEAD_NUM;
		ctx->status = VENC_IDLE;
		cmdlist_put_head(head);

		/* try to power off */
		if (!g_cmdlist_manager.bit_map && pm_if_need_power_off(&ctx->pm))
			queue_work(g_encode_work_queue, &g_encode_work);

		/* wake up cmdlist encode work queue */
		venc_drv_osal_give_event(&venc->event);
		spin_unlock_irqrestore(&ctx->lock, flag);
		return;
	}

	reg_base->VEDU_VCPI_INTCLR.u32 = int_status.u32;
	spin_unlock_irqrestore(&ctx->lock, flag);
}

static void cmdlist_encode_timeout_proc(int32_t core_id)
{
	int32_t ret;
	U_FUNC_VCPI_INTSTAT int_status;
	uint32_t cur_idx = g_cmdlist_manager.cur_idx;
	struct cmdlist_head *head = NULL;
	struct venc_context *ctx = NULL;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	unsigned long flag;

	VCODEC_WARN_VENC("coreid:%d encode timeout", core_id);

	ret = venc_check_coreid(core_id);
	if (ret != 0) {
		VCODEC_ERR_VENC("CORE_ERROR:invalid core ID is %d", core_id);
		return;
	}

	ctx = &venc->ctx[core_id];

	spin_lock_irqsave(&ctx->lock, flag);
	if (ctx->status != VENC_BUSY) {
		spin_unlock_irqrestore(&ctx->lock, flag);
		VCODEC_FATAL_VENC("core %d: current core status invalid, status: %d",
				core_id, ctx->status);
		return;
	}

	if (cur_idx >= CMDLIST_MAX_HEAD_NUM) {
		spin_unlock_irqrestore(&ctx->lock, flag);
		VCODEC_WARN_VENC("not find current head");
		return;
	}

	head = &g_cmdlist_manager.head[cur_idx];
	int_status.u32 = 0;
	int_status.bits.vcpi_int_vedu_timeout = 1;
	cmdlist_process_readback_info(head, ctx->reg_base, int_status);

	/* Masks and clears interrupts before setting the timeout status. */
	vedu_hal_request_bus_idle(ctx->reg_base);
	venc_hal_disable_all_int((S_HEVC_AVC_REGS_TYPE *)(ctx->reg_base));
	venc_hal_clr_all_int((S_HEVC_AVC_REGS_TYPE *)(ctx->reg_base));

	pm_hardware_busy_exit(&ctx->pm);
	g_cmdlist_manager.cur_idx = CMDLIST_MAX_HEAD_NUM;
	ctx->status = VENC_TIME_OUT;
	cmdlist_put_head(head);

	/* wake up cmdlist encode work queue */
	venc_drv_osal_give_event(&venc->event);
	spin_unlock_irqrestore(&ctx->lock, flag);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static void cmdlist_encode_timeout(struct timer_list *data)
{
	struct venc_context *ctx = from_timer(ctx, data, timer);
	if (!ctx) {
		VCODEC_ERR_VENC("ctx null error");
		return;
	}

	cmdlist_encode_timeout_proc(ctx->core_id);
}
#else
static void cmdlist_encode_timeout(unsigned long core_id)
{
	cmdlist_encode_timeout_proc((int32_t)core_id);
}
#endif

void cmdlist_init_ops(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	venc->ops.init = cmdlist_init;
	venc->ops.deinit = cmdlist_deinit;
	venc->ops.encode = cmdlist_prepare_encode;
	venc->ops.encode_done = cmdlist_encode_done;
	venc->ops.encode_timeout = cmdlist_encode_timeout;

	VCODEC_INFO_VENC("config register by cmdlist");
}
