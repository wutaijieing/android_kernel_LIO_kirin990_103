/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: HSDT Trace driver for Kirin SoCs
 * Author: Hisilicon
 * Create: 2019-10-14
 */

#include "hsdt_trace.h"
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>
#include <platform_include/see/bl31_smc.h>

struct hsdt_trace_info g_trace_info[HSDT_BUS_BUTT];
struct platform_device *hsdt_trace_pdev;

u32 g_trace_event_num[HSDT_BUS_BUTT] = {
	HSDT_EVENT_NUM_UFS,
	HSDT_EVENT_NUM_PCIE0,
	HSDT_EVENT_NUM_PCIE1,
	HSDT_EVENT_NUM_USB,
	HSDT_EVENT_NUM_DP
};

noinline void hsdt_trace_smc(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2, u64 _arg3)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	register u64 arg3 asm("x4") = _arg3;

	asm volatile (__asmeq("%0", "x0")
		      __asmeq("%1", "x1")
		      __asmeq("%2", "x2")
		      __asmeq("%3", "x3")
		      __asmeq("%4", "x4")
		      "smc    #0\n"
		      : "+r"(function_id)
		      : "r"(arg0), "r"(arg1), "r"(arg2), "r"(arg3));
};

static int check_type_and_init(u32 type)
{
	if (!is_valid_trace_bus_type(type) || (g_trace_info[type].init != TRACE_INIT))
		return -1;
	return 0;
}

static int check_event_valid(u32 type, u32 event_id_start, u32 event_id_end)
{
	if ((event_id_start > g_trace_event_num[g_trace_info[type].trace_register_type]) ||
		(event_id_end > g_trace_event_num[g_trace_info[type].trace_register_type]) ||
		(event_id_start > event_id_end)) {
		hsdt_trace_err("event id error!\n");
		return -EINVAL;
	}

	return 0;
}

static int trace_validation_check(u32 val, u32 valid_bit)
{
	return (val & (~valid_bit)) ? TRACE_PARAM_INVALID : TRACE_PARAM_VALID;
}

int hsdt_trace_trg_cmd(u32 bus_type, u32 trg_cmd)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	if (trace_validation_check(trg_cmd, TRG_GROUP0_CMD_VALIDBIT)) {
		writel(trg_cmd, g_trace_info[bus_type].trace_cfg_base_addr +
		       TRG_GROUP0_CMD_OFFSET);
		return 0;
	}

	hsdt_trace_err("input param invalid!\n");
	return -EFAULT;
}

int hsdt_trace_trg_event(u32 bus_type, u32 trg_event)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	if (trace_validation_check(trg_event, TRG_GROUP0_EVENT_VALIDBIT)) {
		writel(trg_event, g_trace_info[bus_type].trace_cfg_base_addr +
		       TRG_GROUP0_EVENT_OFFSET);
		return 0;
	}

	hsdt_trace_err("input param invalid!\n");
	return -EFAULT;
}

int hsdt_trace_trg_data1(u32 bus_type, u32 trg_data_low, u32 trg_data_high)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	/* trg_group0_data1_l */
	writel(trg_data_low, g_trace_info[bus_type].trace_cfg_base_addr +
	       TRG_GROUP0_DATA1_LOW_OFFSET);
	/* trg_group0_data1_h */
	writel(trg_data_high, g_trace_info[bus_type].trace_cfg_base_addr +
	       TRG_GROUP0_DATA1_HIGH_OFFSET);

	return 0;
}

int hsdt_trace_trg_data2(u32 bus_type, u32 trg_data_low, u32 trg_data_high)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	/* trg_group0_data2_l */
	writel(trg_data_low, g_trace_info[bus_type].trace_cfg_base_addr +
	       TRG_GROUP0_DATA2_LOW_OFFSET);
	/* trg_group0_data2_h */
	writel(trg_data_high, g_trace_info[bus_type].trace_cfg_base_addr +
	       TRG_GROUP0_DATA2_HIGH_OFFSET);

	return 0;
}

int hsdt_trace_trg_data1_mask(u32 bus_type, u32 trg_data_mask_l, u32 trg_data_mask_h)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	/* trg_group0_data1_msk_l */
	writel(trg_data_mask_l, g_trace_info[bus_type].trace_cfg_base_addr +
	       TRG_GROUP0_DATA1_MASK_L_OFFSET);
	/* trg_group0_data1_msk_h */
	writel(trg_data_mask_h, g_trace_info[bus_type].trace_cfg_base_addr +
	       TRG_GROUP0_DATA1_MASK_H_OFFSET);

	return 0;
}

int hsdt_trace_trg_data2_mask(u32 bus_type, u32 trg_data_mask_l, u32 trg_data_mask_h)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	/* trg_group0_data2_msk_l */
	writel(trg_data_mask_l, g_trace_info[bus_type].trace_cfg_base_addr +
	       TRG_GROUP0_DATA2_MASK_L_OFFSET);
	/* trg_group0_data2_msk_h */
	writel(trg_data_mask_h, g_trace_info[bus_type].trace_cfg_base_addr +
	       TRG_GROUP0_DATA2_MASK_H_OFFSET);

	return 0;
}

int hsdt_trace_trg_timeout(u32 bus_type, u32 trg_timeout)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	writel(trg_timeout, g_trace_info[bus_type].trace_cfg_base_addr + TRG_TIMEOUT_OFFSET);

	return 0;
}

int hsdt_trace_trg_wait_num(u32 bus_type, u32 trg_wait_num)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	writel(trg_wait_num, g_trace_info[bus_type].trace_cfg_base_addr + TRG_WAIT_NUM_OFFSET);

	return 0;
}

int hsdt_trace_trigger_manual_cfg(u32 bus_type, u32 trg_manual_content)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	if (trace_validation_check(trg_manual_content, TRG_MANUAL_VALIDBIT)) {
		writel(trg_manual_content, g_trace_info[bus_type].trace_cfg_base_addr +
		       TRG_MANUAL_OFFSET);
		return 0;
	}

	hsdt_trace_err("input param invalid!\n");
	return -EFAULT;
}

int hsdt_trace_eof_int(u32 bus_type, u32 eof_int_content)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	writel(eof_int_content, g_trace_info[bus_type].trace_cfg_base_addr + EOF_INT_OFFSET);

	return 0;
}

int hsdt_trace_cfg_event_pri(u32 bus_type, u32 event_id, u32 priority)
{
	u32 offset_block_idx;
	u32 offset_inner_idx;
	u32 priority_content;
	u32 pri_in_register;
	u32 pri_bit_temp[BYTE_NUM_PER_WORD] = {
		0xffffff00,
		0xffff00ff,
		0xff00ffff,
		0x00ffffff
	};

	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	if (priority > TRACE_PRIORITY_UPPER_BOUND) {
		hsdt_trace_err("priority is bad value!\n");
		return -EFAULT;
	}

	if (event_id > g_trace_event_num[g_trace_info[bus_type].trace_register_type]) {
		hsdt_trace_err("event id error!\n");
		return -EINVAL;
	}

	offset_block_idx = event_id / BYTE_NUM_PER_WORD;
	offset_inner_idx = event_id % BYTE_NUM_PER_WORD;

	priority_content = priority << (offset_inner_idx * BIT_NUM_PER_BYTE);
	hsdt_trace_err("priority = 0x%x, priority_content = 0x%x\n", priority, priority_content);

	pri_in_register = readl(g_trace_info[bus_type].trace_cfg_base_addr + CFG_EVENT_PRI_OFFSET +
				BYTE_NUM_PER_WORD * offset_block_idx);
	pri_in_register &= pri_bit_temp[offset_inner_idx];
	hsdt_trace_err("pri_in_register = 0x%x, pri_in_register|priority_content = 0x%x\n",
			pri_in_register, pri_in_register | priority_content);

	writel(pri_in_register | priority_content, g_trace_info[bus_type].trace_cfg_base_addr +
		CFG_EVENT_PRI_OFFSET + BYTE_NUM_PER_WORD * offset_block_idx);

	hsdt_trace_err("config event priority ok!\n");
	return 0;
}

int hsdt_trace_event_cfg_ex(u32 bus_type, u32 event_id_start, u32 event_id_end, u32 priority)
{
	u32 i;
	int ret = 0;

	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	if (check_event_valid(bus_type, event_id_start, event_id_end)) {
		hsdt_trace_err("event_id_start = %u, event_id_end = %u!\n",
			       event_id_start, event_id_end);
		return -EFAULT;
	}

	for (i = event_id_start; i <= event_id_end; i++)
		ret += hsdt_trace_cfg_event_pri(bus_type, i, priority + i);

	return ret;
}

static int hsdt_trace_register_default_cfg(u32 bus_type)
{
	struct hsdt_trace_info *trace_info = &g_trace_info[bus_type];

	/* event short stamp */
	writel(DEFAULT_TRACE_TIMESTAMP, trace_info->trace_cfg_base_addr +
		TRG_MANUAL_OFFSET);

	writel(DEFAULT_CS_LTIMESTAMP_ENABLE, trace_info->cs_tsgen_base_addr);

	/* trace eof enable */
	writel(DEFAULT_TRACE_EOF_EN, trace_info->trace_cfg_base_addr +
		EOF_INT_OFFSET);

	/* outstanding->2, burst_length->3 */
	writel(DEFAULT_TRACE_OUTSTANDING, trace_info->trace_cfg_base_addr +
		CFG_AXI_BURST_LENGTH_OFFSET);

	/* timeout count */
	writel(DEFAULT_TRACE_TIMEOUT, trace_info->trace_cfg_base_addr +
		CFG_AXI_TIMEOUT_L_OFFSET);

	/* eventid & priority */
	writel(DEFAULT_TRACE_EVENT, trace_info->trace_cfg_base_addr +
		CFG_EVENT_PRI_OFFSET + BYTE_NUM_PER_WORD);

	hsdt_trace_err("trace default config ok!\n");

	return 0;
}

int hsdt_trace_dump(u32 bus_type)
{
	mm_segment_t fs;
	struct file *fp = NULL;
	loff_t pos;
	u32 i;
	u32 trace_num = TRACE_BUFF_SIZE * BIT_NUM_PER_BYTE / TRACE_SIZE_PER;
	struct hsdt_trace_data_info *trace_data_info = NULL;
	const char *log_path = "data/log/hsdt_trace.log";

	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	fp = filp_open(log_path, O_RDWR | O_CREAT | O_TRUNC, FILE_CHMOD_PRIORITY);
	if (IS_ERR(fp)) {
		hsdt_trace_err("creat file error!\n");
		return -EEXIST;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	trace_data_info = (struct hsdt_trace_data_info *)(g_trace_info[bus_type].buff_virt);

	for (i = 0; i < trace_num; i++) {
		pos = fp->f_pos;
		vfs_write(fp, (const char *)(trace_data_info + i), sizeof(struct hsdt_trace_data_info), &pos);
		fp->f_pos = pos;
	}
	set_fs(fs);
	filp_close(fp, NULL);

	hsdt_trace_err("dump finish!\n");

	return 0;
}

/* dump trace related regs for debugging */
void hsdt_trace_reg_dump(u32 type)
{
	struct hsdt_trace_info *trace_info = NULL;

	if (check_type_and_init(type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", type);
		return;
	}
	trace_info = &g_trace_info[type];

	hsdt_trace_err("CFG_AXI_INIT_ADDR_L_REG [0x%llx] = 0x%x\n",
		       __virt_to_phys(trace_info->trace_cfg_base_addr + CFG_AXI_INIT_ADDR_L_OFFSET),
		       readl(trace_info->trace_cfg_base_addr + CFG_AXI_INIT_ADDR_L_OFFSET));
	hsdt_trace_err("TRG_MANUAL_REG [0x%llx] = 0x%x\n",
		       __virt_to_phys(trace_info->trace_cfg_base_addr + TRG_MANUAL_OFFSET),
		       readl(trace_info->trace_cfg_base_addr + TRG_MANUAL_OFFSET));
	hsdt_trace_err("EOF_INT_REG [0x%llx] = 0x%x\n",
		       __virt_to_phys(trace_info->trace_cfg_base_addr + EOF_INT_OFFSET),
		       readl(trace_info->trace_cfg_base_addr + EOF_INT_OFFSET));
	hsdt_trace_err("CFG_AXI_BURST_LEN_REG [0x%llx] = 0x%x\n",
		       __virt_to_phys(trace_info->trace_cfg_base_addr + CFG_AXI_BURST_LENGTH_OFFSET),
		       readl(trace_info->trace_cfg_base_addr + CFG_AXI_BURST_LENGTH_OFFSET));
	hsdt_trace_err("CFG_AXI_TO_REG [0x%llx] = 0x%x\n",
		       __virt_to_phys(trace_info->trace_cfg_base_addr + CFG_AXI_TIMEOUT_L_OFFSET),
		       readl(trace_info->trace_cfg_base_addr + CFG_AXI_TIMEOUT_L_OFFSET));
	hsdt_trace_err("CFG_EVENT_PRI_REG [0x%llx] = 0x%x\n",
		       __virt_to_phys(trace_info->trace_cfg_base_addr + CFG_EVENT_PRI_OFFSET),
		       readl(trace_info->trace_cfg_base_addr + CFG_EVENT_PRI_OFFSET));
	hsdt_trace_err("CFG_AXI_INIT_ADDR_H_REG [0x%llx] = 0x%x\n",
		       __virt_to_phys(trace_info->trace_cfg_base_addr + CFG_AXI_INIT_ADDR_H_OFFSET),
		       readl(trace_info->trace_cfg_base_addr + CFG_AXI_INIT_ADDR_H_OFFSET));
}

void hsdt_trace_ddr_dump(u32 type, u32 count)
{
	u32 i;
	struct hsdt_trace_info *trace_info = NULL;

	if (check_type_and_init(type) || (count > TRACE_COUNT_MAX)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", type);
		return;
	}
	trace_info = &g_trace_info[type];

	for (i = 0; i < count; i++) {
		if ((i % 4) == 0)
			pr_err("[0x%llx]\t", trace_info->buff_phy + i);
		pr_err("0x%x\t\t", readl(trace_info->buff_virt + i));
	}
}

static int hsdt_trace_register_baseaddr_cfg(struct hsdt_trace_info *trace_info)
{
	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,kirin-hsdt-trace");
	if (!np) {
		hsdt_trace_err("Failed to kirin-hsdt-trace node");
		return -EFAULT;
	}

	trace_info->trace_cfg_base_addr = of_iomap(np, trace_info->trace_register_type);
	if (!trace_info->trace_cfg_base_addr) {
		hsdt_trace_err("Failed to iomap hsdt trace base addr iomap");
		return -EFAULT;
	}

	trace_info->cs_tsgen_base_addr = of_iomap(np, REG_INDEX_IN_DTSI);
	if (!trace_info->cs_tsgen_base_addr) {
		hsdt_trace_err("Failed to iomap base addr iomap");
		iounmap(trace_info->trace_cfg_base_addr);
		return -EFAULT;
	}

	hsdt_trace_err("trace cfg base addr is 0x%pK.\n", trace_info->trace_cfg_base_addr);

	return 0;
}

static void hsdt_trace_buff_cfg(struct hsdt_trace_info *trace_info)
{
	hsdt_trace_err("trace ddr logical addr is 0x%pK\n", trace_info->buff_virt);
	hsdt_trace_err("trace ddr physical addr is 0x%llx\n", trace_info->buff_phy);
	writel(trace_info->buff_phy, trace_info->trace_cfg_base_addr +
	       CFG_AXI_INIT_ADDR_L_OFFSET);
	writel(TRACE_BUFF_SIZE, trace_info->trace_cfg_base_addr + CFG_AXI_INIT_ADDR_H_OFFSET);
}

static int hsdt_trace_ddr_alloc(struct hsdt_trace_info *trace_info)
{
	if (trace_info->buff_virt != NULL) {
		hsdt_trace_err("trace ddr buffer have been alloced, free frist!\n");
		dma_free_coherent(&hsdt_trace_pdev->dev, TRACE_BUFF_SIZE,
				  trace_info->buff_virt, trace_info->buff_phy);
	}

	trace_info->buff_virt = dma_alloc_coherent(&hsdt_trace_pdev->dev, TRACE_BUFF_SIZE,
						   &(trace_info->buff_phy), GFP_KERNEL | __GFP_ZERO);

	if (trace_info->buff_virt == NULL) {
		hsdt_trace_err("trace ddr memory alloc fail!\n");
		return -ENOMEM;
	}

	hsdt_trace_buff_cfg(trace_info);
	return 0;
}

/* this function must be executed first */
int hsdt_trace_init(u32 bus_type)
{
	int ret = 0;

	if (!is_valid_trace_bus_type(bus_type)) {
		hsdt_trace_err("bus type is %u, error!\n", bus_type);
		return -EFAULT;
	}

	if (g_trace_info[bus_type].init == TRACE_INIT) {
		hsdt_trace_err("bus type %u has been inited!\n", bus_type);
		return -EFAULT;
	}

	/* clk and reset, dis reset */
	hsdt_trace_smc(HSDT_TRACE_FID_VALUE, bus_type,
		       TRACE_SMC_INVALID_PARAM, TRACE_SMC_INVALID_PARAM,
		       TRACE_SMC_CLK_ENABLE);

	g_trace_info[bus_type].init = TRACE_INIT;
	g_trace_info[bus_type].trace_register_type = bus_type;
	ret += hsdt_trace_register_baseaddr_cfg(&g_trace_info[bus_type]);
	ret += hsdt_trace_ddr_alloc(&g_trace_info[bus_type]);
	ret += hsdt_trace_register_default_cfg(bus_type);
	if (ret != EOK) {
		hsdt_trace_err("hsdt trace init fail!\n");
		return -EFAULT;
	}

	hsdt_trace_err("hsdt trace init ok!\n");
	return ret;
}

int hsdt_trace_start(u32 bus_type)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	hsdt_trace_smc(HSDT_TRACE_FID_VALUE, g_trace_info[bus_type].trace_register_type,
		       HSDT_TRACE_ENABLE, TRACE_SMC_INVALID_PARAM, TRACE_SMC_ENABLE);

	hsdt_trace_err("hsdt trace start success!\n");
	return 0;
}

int hsdt_trace_stop(u32 bus_type)
{
	if (check_type_and_init(bus_type)) {
		hsdt_trace_err("bus type or init error, type is %u!\n", bus_type);
		return -EFAULT;
	}

	hsdt_trace_smc(HSDT_TRACE_FID_VALUE, g_trace_info[bus_type].trace_register_type,
		       HSDT_TRACE_DISABLE, TRACE_SMC_INVALID_PARAM, TRACE_SMC_ENABLE);

	hsdt_trace_err("hsdt trace stop success!\n");
	return 0;
}

static int kirin_hsdt_trace_probe(struct platform_device *pdev)
{
	int ret;

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(TRACE_DMA_MASK));
	if (ret) {
		hsdt_trace_err("set dmamask to MASK 64 failed\n");
		return ret;
	}
	hsdt_trace_pdev = pdev;
	hsdt_trace_err("hsdt trace driver has been loaded!\n");
	return 0;
}

static void trace_mem_free(void)
{
	int i;
	struct hsdt_trace_info *trace_info = NULL;

	for (i = 0; i < HSDT_BUS_BUTT; i++) {
		trace_info = &g_trace_info[i];
		if (trace_info->init == TRACE_NO_INIT)
			continue;

		dma_free_coherent(&hsdt_trace_pdev->dev, TRACE_BUFF_SIZE,
				  trace_info->buff_virt, trace_info->buff_phy);
		iounmap(trace_info->trace_cfg_base_addr);
		iounmap(trace_info->cs_tsgen_base_addr);
	}
}

static struct of_device_id kirin_hsdt_trace_match_table[] = {
	{ .compatible = "hisilicon,kirin-hsdt-trace",
	  .data = NULL,
	},
	{},
};

static struct platform_driver kirin_hsdt_trace_driver = {
	.driver	= { .name = "kirin-hsdt-trace",
		    .owner = THIS_MODULE,
		    .of_match_table = kirin_hsdt_trace_match_table,
	},
	.probe = kirin_hsdt_trace_probe,
};

static int kirin_hsdt_trace_init(void)
{
	int ret;

	ret = platform_driver_register(&kirin_hsdt_trace_driver);
	if (ret)
		hsdt_trace_err("Failed to register kirin hsdt trace driver");

	return ret;
}

static void kirin_hsdt_trace_exit(void)
{
	trace_mem_free();

	platform_driver_unregister(&kirin_hsdt_trace_driver);
	hsdt_trace_inf("hsdt trace exit!\n");
}

module_init(kirin_hsdt_trace_init);
module_exit(kirin_hsdt_trace_exit);

MODULE_AUTHOR("Hisilicon");
MODULE_DESCRIPTION("HSDT Trace Driver");
MODULE_LICENSE("GPL");
