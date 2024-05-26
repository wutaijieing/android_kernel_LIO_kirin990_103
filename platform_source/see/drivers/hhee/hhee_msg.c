/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: hhee message source file
 * Create: 2017/6/1
 */

#include <linux/errno.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include "hhee.h"
#include "hhee_msg.h"

static hhee_callback_t msg_callbacks[HHEE_MSG_ID_MAX];

static struct hhee_msg_state *msg_state;

static void handle_message(struct hhee_msg *msg)
{
	unsigned int id = msg->id;
	unsigned int len = msg->len;

	if (id >= HHEE_MSG_ID_MAX || len > HHEE_MSG_MAX_PAYLOAD) {
		pr_err("unexpected msg, seq=%llu, msgnum=%u, len=%u\n", msg->seq,
		       id, len);
		return;
	}

	if (!msg_callbacks[id]) {
		pr_err("unhandled msg, seq=%llu, msgnum=%u, len=%u\n", msg->seq, id,
		       len);
		return;
	}

	pr_info("process msg, seq=%llu, msgnum=%u, len=%u\n", msg->seq, id, len);

	msg_callbacks[id](len, (void *)msg->payload);
}

static void hhee_msg_handle(unsigned int start, unsigned int end)
{
	struct hhee_msg *msg = NULL;

	pr_err("hhee irq handler.\n");
	while (start != end) {
		msg = (struct hhee_msg *)(uintptr_t)((uintptr_t)msg_state +
			HHEE_MSG_SIZE * ((uintptr_t)start + 1));
		handle_message(msg);
		start++;
		if (start >= HHEE_MSG_MAX_SLOTS)
			start = 0;
	}
}

#define MAX_IRQ_COUNT 0x3
static int g_irq_counters;

static int hkip_hhee_dispatch(void)
{
	unsigned int start;
	unsigned int end;

	if (!msg_state)
		return -1;

	start = msg_state->read_pos;
	end = msg_state->write_pos;

	if (start >= HHEE_MSG_MAX_SLOTS || end >= HHEE_MSG_MAX_SLOTS) {
		pr_err("unexpected error, pos=%u,%u\n", start, end);
		return -1;
	}

	if (g_irq_counters >= MAX_IRQ_COUNT)
		return -1;

	hhee_msg_handle(start, end);
	msg_state->read_pos = end;
	g_irq_counters += 1;
	return 0;
}

#ifdef CONFIG_HHEE_USING_IPI
static struct work_struct g_hhee_ipi_work;
struct workqueue_struct *g_hhee_ipi_wq;

static void hkip_hhee_ipi_fn(struct work_struct *work)
{
	if (hkip_hhee_dispatch())
		pr_err("%s, msg handle error\n", __func__);

	(void)work;
}

void hhee_irq_receive(void)
{
	queue_work(g_hhee_ipi_wq, &g_hhee_ipi_work);
}
#else
irqreturn_t hhee_irq_handle(int irq, void *data)
{
	if (hkip_hhee_dispatch())
		pr_err("%s, msg handle error\n", __func__);

	return IRQ_HANDLED;
}
#endif

void reset_hkip_irq_counters(void)
{
	g_irq_counters = 0;
}

int hhee_msg_init(void)
{
	struct arm_smccc_res ret_res;
	uintptr_t buf_base;
	uint64_t buf_size;
	void *addr = NULL;
	int ret;

	ret_res = hhee_fn_hvc((unsigned long)HHEE_IRQ_INFOBUF, 0ul, 0ul, 0ul);
	buf_base = ret_res.a0;
	buf_size = ret_res.a1;

	if (!buf_base || buf_size != (HHEE_MSG_MAX_SLOTS + 1) * HHEE_MSG_SIZE) {
		ret = -EFAULT;
		goto err;
	}

	addr = ioremap_cache(buf_base, buf_size);
	if (!addr) {
		pr_err("hhee_msg_init ioremap_cache err\n");
		ret = -EFAULT;
		goto err;
	}
#ifdef CONFIG_HHEE_USING_IPI
	g_hhee_ipi_wq = create_singlethread_workqueue("hhee_ipi_work");
	if (!g_hhee_ipi_wq) {
		pr_err("Create hhee IPI work failed\n");
		ret = -1;
		goto err;
	}
	INIT_WORK(&g_hhee_ipi_work, hkip_hhee_ipi_fn);
#endif
	msg_state = addr;
	return 0;

err:
	pr_err("hhee msg init error.\n");
	return ret;
}

int hhee_msg_register_handler(unsigned int id, hhee_callback_t fn)
{
	if (!fn || id >= HHEE_MSG_ID_MAX)
		return -EINVAL;

	if (!msg_state)
		return -EFAULT;

	msg_callbacks[id] = fn;
	msg_state->enabled[id] = 1;

	return 0;
}
