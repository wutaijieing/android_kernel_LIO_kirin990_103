/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: for npu kprobe
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <securec.h>
#include "npu_log.h"
#include "npu_common.h"
#include "npu_kprobe.h"
#define MAX_NAME_LEN 256
#define MAX_KPROBE_NUM 10

static struct kprobe *g_kprobe_node[MAX_KPROBE_NUM];
static int g_probe_num;

static int g_kprobe_is_load;

static int handler_fault_default(struct kprobe *p, struct pt_regs *regs,
	int trapnr)
{
	unused(p);
	unused(regs);
	unused(trapnr);
	npu_drv_warn("into fault default");
	return 0;
}

static void handler_post_default(struct kprobe *p, struct pt_regs *regs,
		unsigned long flags)
{
	unused(p);
	unused(regs);
	unused(flags);
	npu_drv_warn("into post default");
}

/* If a function has two probes or more,
 * use npu_search_kprobe to get the index.
 */
static int npu_search_kprobe(const char *func_name,
	kprobe_pre_handler_t handler_pre)
{
	int i = 0;

	for (i = 0; i < g_probe_num; i++) {
		if ((!strncmp(func_name, g_kprobe_node[i]->symbol_name,
			strlen(func_name))) &&
			(g_kprobe_node[i]->pre_handler == handler_pre))
			return i;
	}

	return -1;
}

/* Add this kprobe into the global struct array kp. */
int npu_add_kprobe(char *func_name, kprobe_pre_handler_t handler_pre,
	kprobe_post_handler_t handler_post, kprobe_fault_handler_t handler_fault)
{
	struct kprobe *kprobe_node = NULL;

	npu_drv_debug("into");
	if (g_probe_num < 0 || g_probe_num >= MAX_KPROBE_NUM) {
		npu_drv_warn("current kprobe num is %d, reached the limit",
			g_probe_num);
		return -1;
	}
	kprobe_node = g_kprobe_node[g_probe_num];

	if (func_name == NULL || strlen(func_name) > MAX_NAME_LEN) {
		npu_drv_warn("the func name is null or too long");
		return -1;
	}

	kprobe_node->symbol_name = (const char *)func_name;
	kprobe_node->pre_handler = handler_pre;
	kprobe_node->fault_handler =
		((handler_fault == NULL) ? handler_fault_default : handler_fault);
	kprobe_node->post_handler =
		((handler_post == NULL) ? handler_post_default : handler_post);

	if (register_kprobe(kprobe_node)) {
		npu_drv_warn("register kprobe(%s) failed", func_name);
		return -1;
	}
	g_probe_num++;
	npu_drv_warn("register kprobe(%s) index %d success, cur_num = %d",
		func_name, g_probe_num - 1, g_probe_num);

	return g_probe_num - 1;
}

static void free_kprobe(int num)
{
	int i = 0;

	for (i = 0; i < num; i++)
		kfree(g_kprobe_node[i]);
}

static int alloc_kprobe(void)
{
	int i = 0;

	npu_drv_debug("into");
	for (i = 0; i < MAX_KPROBE_NUM; i++) {
		g_kprobe_node[i] = (struct kprobe *)kzalloc(sizeof(struct kprobe),
			GFP_ATOMIC);
		if (g_kprobe_node[i] == NULL)
			goto failed;
	}
	npu_drv_debug("out");

	return 0;

failed:
	free_kprobe(i);
	return -1;
}

int npu_load_kprobe(void)
{
	int ret;

	g_probe_num = 0;
	npu_drv_debug("into");
	ret = memset_s(g_kprobe_node, MAX_KPROBE_NUM * sizeof(struct kprobe *),
			0, MAX_KPROBE_NUM * sizeof(struct kprobe *));
	if (ret != 0) {
		npu_drv_warn("memset_s failed");
		return -1;
	}
	if (alloc_kprobe()) {
		npu_drv_warn("alloc kprobe failed");
		return -1;
	}
	npu_drv_debug("out");
	return 0;
}

void npu_unload_kprobe(void)
{
	if (g_probe_num)
		(void)unregister_kprobes(g_kprobe_node, g_probe_num);

	free_kprobe(MAX_KPROBE_NUM);
}

int npu_kprobe_is_load(void)
{
	return g_kprobe_is_load;
}

void npu_kprobe_set_status(int status)
{
	g_kprobe_is_load = status;
}

/* The probe is in kp array, just enable it. */
int npu_enable_kprobe(char *func_name, kprobe_pre_handler_t handler_pre)
{
	int index = 0;

	npu_drv_debug("into");
	if (func_name == NULL) {
		npu_drv_warn("invalid function name");
		return -1;
	}
	if (npu_kprobe_is_load() == 0) {
		if (npu_load_kprobe() != 0) {
			npu_drv_warn("npu load kprobe fail");
			return -1;
		}
		npu_kprobe_set_status(1);
	}

	index = npu_search_kprobe(func_name, handler_pre);
	if (index < 0) {
		index = npu_add_kprobe(func_name, handler_pre, NULL, NULL);
		if (index < 0) {
			npu_drv_warn("npu add kprobe fail");
			return -1;
		}
	} else {
		cond_return_error(index >= MAX_KPROBE_NUM, -1, "invalid index is %d\n", index);
		if (enable_kprobe(g_kprobe_node[index])) {
			npu_drv_warn("enable kprobe(%s) failed", func_name);
			return -1;
		}
		npu_drv_warn("out");
	}
	npu_drv_debug("index is %d", index);
	return 0;
}

/* The probe is in kp array, just disable it, not del it from the array. */
int npu_disable_kprobe(char *func_name, kprobe_pre_handler_t handler_pre)
{
	int index = 0;

	npu_drv_debug("into");
	if (func_name == NULL) {
		npu_drv_warn("invalid function name");
		return -1;
	}

	if (npu_kprobe_is_load() == 0) {
		npu_drv_warn("npu kprobe is not loaded");
		return -1;
	}

	index = npu_search_kprobe(func_name, handler_pre);
	if (index < 0) {
		npu_drv_warn("search kprobe(%s) failed", func_name);
		return -1;
	}

	cond_return_error(index >= MAX_KPROBE_NUM, -1, "invalid index is %d\n", index);
	if (disable_kprobe(g_kprobe_node[index])) {
		npu_drv_warn("disable kprobe(%s) failed", func_name);
		return -1;
	}
	npu_drv_debug("out");
	return 0;
}
