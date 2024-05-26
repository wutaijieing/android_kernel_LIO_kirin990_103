/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: CFI harden test ko.
 * Create: 2020/10/19
 */

#include <linux/module.h>
#include <linux/printk.h>

void test_cfi_func1(int para)
{
	pr_err("%s para: %d\n", __func__, para);
}
EXPORT_SYMBOL(test_cfi_func1);

void test_cfi_func2(unsigned int para)
{
	pr_err("%s para: 0x%x\n", __func__, para);
}
EXPORT_SYMBOL(test_cfi_func2);

void test_cfi_func3(unsigned long para)
{
	pr_err("%s para: 0x%lx\n", __func__, para);;
}
EXPORT_SYMBOL(test_cfi_func3);

static int __init test_cfi_init(void)
{
	pr_info("cfi test ko init.\n");
	return 0;
}

static void __exit test_cfi_exit(void)
{
	pr_info("cfi test ko exit.\n");
}


module_init(test_cfi_init);
module_exit(test_cfi_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Test cfi harden feature");
