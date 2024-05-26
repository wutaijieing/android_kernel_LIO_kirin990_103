
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/types.h>

static int g_s32_slt_feature = 0;

/*
is slt ,return 1
other ,return 0
*/
int is_running_kernel_slt(void)
{
	return g_s32_slt_feature;
}

#define SLT_CMDLINE_STRING "slt_feature"
static int __init early_parse_slt_cmdline(char *p)
{
	if (p) {
		if (!strcmp(p, "true")) {//lint !e421
			g_s32_slt_feature = 1;
		}
	}
	pr_err("p is %s, is_running_kernel_slt return %d\n", p, is_running_kernel_slt());
	return 0;
}

early_param(SLT_CMDLINE_STRING, early_parse_slt_cmdline);

static int __init hitest_init(void)
{
	pr_err("hitest_init in\n");
	return 0;
}
arch_initcall(hitest_init);


MODULE_DESCRIPTION("Hisilicon Hitest_SLT Module");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("caixi <jean.caixi@huawei.com>");
