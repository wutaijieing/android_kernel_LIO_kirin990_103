#include "npu_bbit_hwts_config.h"
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/dma-buf.h>
#include <linux/io.h>
#include "npu_platform_register.h"
#include "npu_hwts_plat.h"
#include "npu_adapter.h"
#include "npu_platform.h"
#include "npu_pm_framework.h"
#include "npu_common.h"
#include "npu_log.h"

u64 get_hwts_base(void)
{
	u64 hwts_base;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	if (plat_info == NULL) {
		npu_drv_err("get plat info fail.\n");
		return 0;
	}
	hwts_base = (u64)(uintptr_t)plat_info->dts_info.reg_vaddr[NPU_REG_HWTS_BASE];
	npu_drv_debug("hwts_base =0x%llx\n", hwts_base);

	return hwts_base;
}

void config_sq_base(u64 sq_base_addr, u8 sq_idx)
{
	SOC_NPU_HWTS_HWTS_SQ_BASE_ADDR_UNION sq_value;
	u64 hwts_base = get_hwts_base();

	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return;
	}

	npu_drv_debug("sq_base_addr = 0x%llx, sq_idx = 0x%x\n", sq_base_addr,
		sq_idx);
	sq_value.reg.sq_base_addr = sq_base_addr;
	sq_value.reg.sq_base_addr_is_virtual = 1;
	npu_drv_debug("config hwts sq 0x%x base_addr = 0x%llx, sq_value = 0x%llx\n",
		sq_idx, SOC_NPU_HWTS_HWTS_SQ_BASE_ADDR_ADDR(hwts_base, sq_idx),
		sq_value.value);

	writeq(sq_value.value,
		(volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_BASE_ADDR_ADDR(hwts_base, sq_idx));
}

int update_sq_tail(u8 sq_idx, u16 sq_tail)
{
	u64 hwts_base = get_hwts_base();

	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return -1;
	}
	npu_drv_debug("update hwts sq 0x%x base_addr = 0x%llx, sq_tail = 0x%x\n",
		sq_idx,
		SOC_NPU_HWTS_HWTS_SQ_BASE_ADDR_ADDR(hwts_base, sq_idx),
		sq_tail);
	writeq(sq_tail,
		(volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_DB_ADDR(hwts_base, sq_idx));

	return 0;
}

void enable_sq(u8 sq_idx, u16 sq_len)
{
	SOC_NPU_HWTS_HWTS_SQ_CFG0_UNION sq_cfg0 = {0};
	u64 hwts_base = get_hwts_base();

	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return;
	}
	sq_cfg0.reg.sq_length = sq_len;
	sq_cfg0.reg.sq_en = 0x1;
	npu_drv_debug("set hwts sq 0x%x base_addr = 0x%llx, sq_cfg0.value = 0x%x\n",
		sq_idx,
		SOC_NPU_HWTS_HWTS_SQ_BASE_ADDR_ADDR(hwts_base, sq_idx),
		sq_cfg0.value);
	writeq(sq_cfg0.value,
		(volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_CFG0_ADDR(hwts_base, sq_idx));
}

void disable_sq(u8 sq_idx)
{
	u64 hwts_base = get_hwts_base();

	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return;
	}
	writeq(0x0,
		(volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_CFG0_ADDR(hwts_base, sq_idx));
}

int config_hwts_sq(u64 sq_base_addr, u8 sq_idx, u16 sq_len)
{
	npu_drv_debug("sq_base_addr = 0x%llx, sq_idx = 0x%x, sq_len = 0x%x\n",
		sq_base_addr, sq_idx, sq_len);
	disable_sq(sq_idx);
	update_sq_tail(sq_idx, 0x0);
	config_sq_base(sq_base_addr, sq_idx);
	enable_sq(sq_idx, sq_len);

	return 0;
}

int update_hwts_sq(u64 sq_base_addr, u8 sq_idx, u16 sq_tail)
{
	npu_drv_debug("sq_base_addr = 0x%llx, sq_idx = 0x%x, sq_tail = 0x%x\n",
		sq_base_addr, sq_idx, sq_tail);
	disable_sq(sq_idx);
	config_sq_base(sq_base_addr, sq_idx);
	update_sq_tail(sq_idx, sq_tail);
	enable_sq(sq_idx, sq_tail);

	return 0;
}

int check_hwts_sq(u8 sq_idx, u16 sq_tail, u32 time_out)
{
	u32 time_wait = 1;
	u32 timer = 0;
	SOC_NPU_HWTS_HWTS_SQ_CFG0_UNION sq_cfg0 = {0};
	u64 hwts_base = get_hwts_base();

	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return -1;
	}
	npu_drv_debug("sq_idx = 0x%x, sq_tail = 0x%x, time_out = 0x%x\n",
		sq_idx, sq_tail, time_out);
	while (timer < time_out) {
		npu_drv_debug("check status, timer = %llu/%llu\n", timer, time_out);
		read_uint64(sq_cfg0.value, SOC_NPU_HWTS_HWTS_SQ_CFG0_ADDR(hwts_base,
			sq_idx));
		if (sq_cfg0.reg.sq_head == sq_tail)
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

	if (fd == -1) {
		npu_drv_err("invalid fd is -1.\n");
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

	if (fd == -1) {
		npu_drv_err("invalid fd is -1.\n");
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

	if (msg.buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}
	ret = (int)copy_from_user(&para, msg.buf, sizeof(para));
	if (ret != 0) {
		npu_drv_err("copy from user failed ret %d\n", ret);
		return -1;
	}
	ret = config_hwts_sq(para.sq_base_addr, para.sq_idx, para.sq_tail);
	if (ret != 0)
		npu_drv_err("check hwts sq fail ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_update_hwts_sq(struct user_msg msg)
{
	int ret = 0;
	hwts_para_t para = {0};

	if (msg.buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}
	ret = (int)copy_from_user(&para, msg.buf, sizeof(para));
	if (ret != 0) {
		npu_drv_err("copy from user failed ret %d\n", ret);
		return -1;
	}
	ret = update_hwts_sq(para.sq_base_addr, para.sq_idx, para.sq_tail);
	if (ret != 0)
		npu_drv_err("check hwts sq fail ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_update_sq_tail(struct user_msg msg)
{
	int ret = 0;
	hwts_para_t para = {0};

	if (msg.buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}
	ret = (int)copy_from_user(&para, msg.buf, sizeof(para));
	if (ret != 0) {
		npu_drv_err("copy from user failed ret %d.\n", ret);
		return -1;
	}
	ret = update_sq_tail(para.sq_idx, para.sq_tail);
	if (ret != 0)
		npu_drv_err("check hwts sq fail ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_check_hwts_sq(struct user_msg msg)
{
	int ret = 0;
	hwts_para_t para = {0};

	if (msg.buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}
	ret = copy_from_user_safe(&para, msg.buf, sizeof(para));
	if (ret != 0) {
		npu_drv_err("copy from user failed ret %d.\n", ret);
		return -1;
	}
	ret = check_hwts_sq(para.sq_idx, para.sq_tail, para.time_out);
	if (ret != 0)
		npu_drv_err("check hwts sq fail ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_attach_ion_sc(struct user_msg msg)
{
	int ret = 0;
	syscache_para_t para = {0};

	if (msg.buf == NULL) {
		npu_drv_err("msg.buf is NULL.\n");
		return -1;
	}
	ret = (int)copy_from_user(&para, msg.buf, sizeof(para));
	if (ret != 0) {
		npu_drv_err("copy from user failed ret %d.\n", ret);
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

	if (msg.buf == NULL) {
		npu_drv_err("msg.buf is NULL.\n");
		return -1;
	}
	ret = (int)copy_from_user(&para, msg.buf, sizeof(para));
	if (ret != 0) {
		npu_drv_err("copy from user failed ret %d.\n", ret);
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

	if (buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}
	ret = (int)copy_from_user(&msg, buf, sizeof(msg));
	if (ret != 0) {
		npu_drv_err("copy from user failed ret %d.\n", ret);
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
		npu_pm_safe_call_with_return(ctx, NPU_SUBSYS, func(msg), ret);
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
	ret = (int)copy_to_user(buf, &result, sizeof(result));
	if (ret != 0) {
		npu_drv_err("copy to user buffer failed.\n");
		return -1;
	}

	return count;
}
