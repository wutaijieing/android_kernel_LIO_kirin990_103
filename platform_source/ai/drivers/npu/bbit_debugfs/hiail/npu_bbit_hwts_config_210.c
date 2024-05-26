/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: for hwts register config
 */
#include "npu_bbit_hwts_config.h"
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/dma-buf.h>
#include <linux/io.h>
#include <securec.h>
#include "npu_platform_register.h"
#include "npu_hwts_plat.h"
#include "npu_adapter.h"
#include "npu_platform.h"
#include "npu_pm_framework.h"
#include "npu_common.h"
#include "npu_log.h"

#define HWTS_HOST_ID_LITE       2

u64 get_hwts_base(void)
{
	u64 hwts_base;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	if (plat_info == NULL) {
		npu_drv_err("get plat info fail.\n");
		return 0;
	}
	hwts_base = (u64)(uintptr_t)plat_info->dts_info.reg_vaddr[NPU_REG_HWTS_BASE];

	return hwts_base;
}

static struct npu_hwts_static_swap_buff* g_hwts_swap_buff_base = NULL;

static struct npu_hwts_static_swap_buff* drv_hwts_get_static_swap_buff(uint16_t channel_id)
{
	/* DO NOT FORGET to unmap after using swap buffer */
	if (channel_id >= HWTS_SQ_SLOT_SIZE) {
		npu_drv_err("channel_id = %u is larger than Max:%u", channel_id, HWTS_SQ_SLOT_SIZE);
		return NULL;
	}
	if (g_hwts_swap_buff_base == NULL)
		g_hwts_swap_buff_base = ioremap_wc(NPU_NS_HWTS_SWAP_BUFF_BASE_ADDR, NPU_NS_HWTS_SWAP_BUFF_SIZE);

	return &g_hwts_swap_buff_base[channel_id];
}

static void drv_hwts_unmap_swap_buff(void)
{
	if (g_hwts_swap_buff_base != NULL) {
		iounmap(g_hwts_swap_buff_base);
		g_hwts_swap_buff_base = NULL;
	}
}

void config_sq_base(u8 sq_idx, u64 sq_base_addr)
{
	struct npu_hwts_static_swap_buff* swap_buff = drv_hwts_get_static_swap_buff(sq_idx);

	if (swap_buff == NULL) {
		npu_drv_err("get hwts swap buffer failed.\n");
		return;
	}

	swap_buff->sq_base_addr0.reg.sq_base_addr_l = sq_base_addr & UINT32_MAX;
	swap_buff->sq_base_addr1.reg.sq_base_addr_h = (sq_base_addr >> 32) & MAX_UINT16_NUM;
	swap_buff->sq_base_addr1.reg.sq_base_addr_is_virtual = 1;

	npu_drv_warn("config hwts sq 0x%x\n", sq_idx);

	drv_hwts_unmap_swap_buff();
}

void set_sqcq_length(u8 sq_idx, u16 sq_len)
{
	struct npu_hwts_static_swap_buff* swap_buff = drv_hwts_get_static_swap_buff(sq_idx);

	if (swap_buff == NULL) {
		npu_drv_err("get hwts swap buffer failed.\n");
		return;
	}

	npu_drv_warn("sq_idx = 0x%x, sq_len = %u", sq_idx, sq_len);

	swap_buff->sq_cfg3.reg.sq_length = sq_len;
	swap_buff->cq_cfg0.reg.cq_length = (sq_len + 1);

	npu_drv_warn("config hwts sq 0x%x sq_length = %u cq_length = %u\n", sq_idx, sq_len, (sq_len + 1));

	drv_hwts_unmap_swap_buff();
}

int update_sq_tail(u8 sq_idx, u16 sq_tail)
{
	u64 hwts_base = get_hwts_base();
	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return -1;
	}
	npu_drv_debug("update hwts sq 0x%x, sq_tail = 0x%x\n", sq_idx, sq_tail);

	writel(sq_tail, (void*)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_DB_ADDR(hwts_base, sq_idx));

	return 0;
}

void enable_sq(u8 sq_idx)
{
	SOC_NPU_HWTS_HWTS_SQ_CFG5_UNION sq_cfg5 = {0};
	u64 hwts_base = get_hwts_base();
	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return;
	}

	sq_cfg5.reg.sq_en = 0x1;
	npu_drv_warn("set hwts sq 0x%x, sq_cfg5.value = 0x%x\n", sq_idx, sq_cfg5.value);
	writel(sq_cfg5.value, (void*)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_CFG5_ADDR(hwts_base, sq_idx));
}

void disable_sq(u8 sq_idx)
{
	u64 hwts_base = get_hwts_base();
	SOC_NPU_HWTS_HWTS_SQ_CFG5_UNION sq_cfg5 = {0};

	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return;
	}

	sq_cfg5.value = readl((void*)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_CFG5_ADDR(hwts_base, sq_idx));
	sq_cfg5.reg.sq_en = 0x0;
	writel(sq_cfg5.value, (void*)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_CFG5_ADDR(hwts_base, sq_idx));
}

void reset_hwts_swap_buff(u8 sq_idx)
{
	struct npu_hwts_static_swap_buff* swap_buff = drv_hwts_get_static_swap_buff(sq_idx);

	if (swap_buff == NULL) {
		npu_drv_err("get hwts swap buffer failed.\n");
		return;
	}

	npu_drv_warn("reset swap buffer, sq_idx = 0x%x\n", sq_idx);

	memset_s(swap_buff, sizeof (struct npu_hwts_static_swap_buff), 0, sizeof (struct npu_hwts_static_swap_buff));

	drv_hwts_unmap_swap_buff();
}

void set_sq_host(u8 sq_idx)
{
	u64 hwts_base = get_hwts_base();
	SOC_NPU_HWTS_HWTS_SQ_CFG6_UNION sq_cfg6 = {0};

	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return;
	}

	sq_cfg6.reg.sq_host_id = HWTS_HOST_ID_LITE;
	writel(sq_cfg6.value, (void*)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_CFG6_ADDR(hwts_base, sq_idx));
}


int config_hwts_sq(u64 sq_base_addr, u8 sq_idx, u16 sq_len)
{
	npu_drv_debug("sq_idx = 0x%x, sq_len = 0x%x\n", sq_base_addr, sq_idx, sq_len);
	set_sq_host(sq_idx);
	disable_sq(sq_idx);
	reset_hwts_swap_buff(sq_idx);
	config_sq_base(sq_idx, sq_base_addr);
	set_sqcq_length(sq_idx, sq_len);
	update_sq_tail(sq_idx, 0x0);
	enable_sq(sq_idx);

	return 0;
}

int update_hwts_sq(u64 sq_base_addr, u8 sq_idx, u16 sq_tail)
{
	npu_drv_debug("sq_idx = 0x%x, sq_tail = 0x%x\n", sq_base_addr, sq_idx, sq_tail);
	disable_sq(sq_idx);
	config_sq_base(sq_idx, sq_base_addr);
	set_sqcq_length(sq_idx, sq_tail);
	update_sq_tail(sq_idx, sq_tail);
	enable_sq(sq_idx);

	return 0;
}

int check_hwts_sq(u8 sq_idx, u16 sq_tail, u32 time_out)
{
	u32 time_wait = 1;
	u32 timer = 0;
	SOC_NPU_HWTS_HWTS_SQ_CFG4_UNION sq_cfg4 = {0};
	u64 hwts_base = get_hwts_base();
	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return -1;
	}
	npu_drv_debug("sq_idx = 0x%x, sq_tail = 0x%x, time_out = 0x%x\n", sq_idx, sq_tail, time_out);
	while (timer < time_out) {
		npu_drv_debug("check status, timer = %llu/%llu\n", timer, time_out);
		npu_drv_debug("sq_idx = 0x%x\n", sq_idx);

		read_uint32(sq_cfg4.value, SOC_NPU_HWTS_HWTS_SQ_CFG4_ADDR(hwts_base, sq_idx));

		npu_drv_debug("sq_cfg4.reg.sq_head = 0x%x\n", sq_cfg4.reg.sq_head);
		if (sq_cfg4.reg.sq_head == sq_tail)
			return 0;
		timer += time_wait;
		msleep(time_wait);
	}
	npu_drv_err("check hwts sq status time out after %dms\n", time_out);

	return -1;
}

int attach_ion_sc(int fd, u32 pid, u64 offset, u64 size)
{
	int ret = 0;

	if (fd < 0) {
		npu_drv_err("invalid fd is %d.\n", fd);
		return -1;
	}
	npu_drv_debug("fd = 0x%x, offset = 0x%lx, size = 0x%lx\n",
		fd, offset, size);
#ifdef CONFIG_MM_LB
	ret = dma_buf_attach_lb(fd, pid, offset, (size_t)size);
#endif
	if (ret != 0) {
		npu_drv_err("fail to dma_buf_attach_lb, ret = %d\n", ret);
		return -1;
	}
	return 0;
}

int detach_ion_sc(int fd)
{
	int ret = 0;

	if (fd < 0) {
		npu_drv_err("invalid fd is %d.\n", fd);
		return -1;
	}
	ret = dma_buf_detach_lb(fd);
	if (ret != 0) {
		npu_drv_err("fail to dma_buf_detach_lb, ret = %d\n", ret);
		return -1;
	}
	return 0;
}

int npu_bbit_config_hwts_sq(struct user_msg msg)
{
	int ret = 0;
	hwts_para_t para = {0};

	cond_return_error(msg.buf == NULL, -1, "config hwts sq msg.buf is null\n");

	if (copy_from_user(&para, msg.buf, sizeof(para)) != 0) {
		npu_drv_err("config hwts sq copy failed.\n");
		return -1;
	}

	ret = config_hwts_sq(para.sq_base_addr, para.sq_idx + 8, para.sq_tail);
	if (ret != 0)
		npu_drv_err("check hwts sq fail ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_update_hwts_sq(struct user_msg msg)
{
	int ret = 0;
	hwts_para_t para = {0};

	cond_return_error(msg.buf == NULL, -1, "update hwts sq msg.buf is null\n");

	if (copy_from_user(&para, msg.buf, sizeof(para)) != 0) {
		npu_drv_err("update hwts sq copy failed.\n");
		return -1;
	}

	ret = update_hwts_sq(para.sq_base_addr, para.sq_idx + 8, para.sq_tail);
	if (ret != 0)
		npu_drv_err("check hwts sq fail ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_update_sq_tail(struct user_msg msg)
{
	int ret = 0;
	hwts_para_t para = {0};

	cond_return_error(msg.buf == NULL, -1, "update sq tail msg.buf is null\n");

	if (copy_from_user(&para, msg.buf, sizeof(para)) != 0) {
		npu_drv_err("update sq tail copy failed.\n");
		return -1;
	}

	ret = update_sq_tail(para.sq_idx + 8, para.sq_tail);
	if (ret != 0)
		npu_drv_err("check hwts sq fail ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_check_hwts_sq(struct user_msg msg)
{
	int ret = 0;
	hwts_para_t para = {0};

	cond_return_error(msg.buf == NULL, -1, "check hwts sq msg.buf is null\n");

	ret = copy_from_user_safe(&para, msg.buf, sizeof(para));
	cond_return_error(ret != 0, -1, "check hwts sq copy failed ret %d.\n", ret);

	ret = check_hwts_sq(para.sq_idx + 8, para.sq_tail, para.time_out);
	if (ret != 0)
		npu_drv_err("check hwts sq fail ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_attach_ion_sc(struct user_msg msg)
{
	int ret = 0;
	syscache_para_t para = {0};

	cond_return_error(msg.buf == NULL, -1, "attach ion sc msg.buf is null\n");

	if (copy_from_user(&para, msg.buf, sizeof(para)) != 0) {
		npu_drv_err("attach ion sc copy failed.\n");
		return -1;
	}

	ret = attach_ion_sc(para.fd, para.pid, para.offset, para.size);
	if (ret != 0)
		npu_drv_err("attach_ion_sc ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_detach_ion_sc(struct user_msg msg)
{
	int ret = 0;
	syscache_para_t para = {0};

	cond_return_error(msg.buf == NULL, -1, "detach ion sc msg.buf is null\n");

	if (copy_from_user(&para, msg.buf, sizeof(para)) != 0) {
		npu_drv_err("detach ion sc copy failed.\n");
		return -1;
	}

	ret = detach_ion_sc(para.fd);
	if (ret != 0)
		npu_drv_err("detach_ion_sc ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

// if you want to add an entry, please add to the end
hwts_func hwts_func_map[] = {
	npu_bbit_config_hwts_sq,
	npu_bbit_update_hwts_sq,
	npu_bbit_update_sq_tail,
	npu_bbit_check_hwts_sq,
	npu_bbit_attach_ion_sc,
	npu_bbit_detach_ion_sc,
	npu_bbit_enable_prof,
	npu_bbit_disable_prof,
	npu_bbit_get_prof_log,
	npu_bbit_print_prof_log,
};

ssize_t npu_bbit_hwts_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *ppos)
{
	int ret = 0;
	hwts_func func = NULL;
	struct user_msg msg = {0};
	struct npu_dev_ctx *ctx = NULL;
	unused(filp);
	unused(ppos);

	npu_drv_warn("npu_bbit_hwts_write enter.\n");

	if (buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}
	count = (sizeof(msg) < count) ? sizeof(msg) : count;
	if (copy_from_user(&msg, buf, count) != 0) {
		npu_drv_err("copy from user failed.\n");
		return -1;
	}
	if (msg.interface_id >= HWTS_MAX) {
		npu_drv_err("invalid interface_id %d.\n", msg.interface_id);
		return -1;
	}
	ctx = get_dev_ctx_by_id(0);
	if (ctx == NULL) {
		npu_drv_err("get dev ctx failed.\n");
		return -1;
	}
	func = hwts_func_map[msg.interface_id];
	npu_drv_debug("hwts_func_map[%d]\n", msg.interface_id);
	switch (msg.interface_id) {
	case 4:
	case 5:
		ret = func(msg);
		break;
	default:
		npu_pm_safe_call_with_return(ctx, NPU_NON_TOP_COMMON, func(msg), ret);
		break;
	}

	if (ret != 0) {
		npu_drv_err("npu bbit core write failed ret %d.\n", ret);
		return -1;
	}

	return count;
}

ssize_t npu_bbit_hwts_read(struct file *filp, char __user *buf,
	size_t count, loff_t *ppos)
{
	int result;
	int ret;
	unused(filp);
	unused(ppos);

	npu_drv_warn("npu_bbit_hwts_read enter.\n");

	if (buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}
	ret = npu_bbit_get_result(&result);
	npu_drv_debug("npu_bbit_get_result, result=%d\n", result);
	if (ret != 0) {
		npu_drv_err("get bbit result fail\n");
		result = -1;
	}
	count = (sizeof(result) < count) ? sizeof(result) : count;
	if (copy_to_user(buf, &result, count) != 0) {
		npu_drv_err("copy to user buffer failed.\n");
		return -1;
	}

	return count;
}
