#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "Kunit.h"
#include "Stub.h"
#include "hisi_testcase_fb.h"
#include "hisi_fb.h"

void test_hisi_fb_blank_power_on_succ(void)
{
	struct hisi_fb_info *fb_monitor = NULL;
	int ret;

	fb_monitor = (struct hisi_fb_info *)hisi_disp_test_get_module_data(DTE_TEST_MODULE_FB);

	ret = hisi_fb_blank(FB_BLANK_UNBLANK, fb_monitor->fbi_info);

	CHECK_EQUAL(ret, 0);
	CHECK_TRUE(fb_monitor->pwr_on);
}

void test_hisi_fb_blank_power_off_succ(void)
{
	struct hisi_fb_info *fb_monitor = NULL;
	int ret;

	fb_monitor = (struct hisi_fb_info *)hisi_disp_test_get_module_data(DTE_TEST_MODULE_FB);

	ret = hisi_fb_blank(FB_BLANK_POWERDOWN, fb_monitor->fbi_info);

	CHECK_EQUAL(ret, 0);
	CHECK_FALSE(fb_monitor->pwr_on);
}

void test_fb_ioctl_online_play_succ(void)
{

}

KU_TestInfo test_hisi_fb_blank[] = {
	{"test_fb_blank_power_on_succ", test_hisi_fb_blank_power_on_succ},
	{"test_fb_blank_power_off_succ", test_hisi_fb_blank_power_off_succ},
	KU_TEST_INFO_NULL
};

KU_TestInfo test_hisi_fb_ioctl[] = {
	{"test_fb_ioctl_online_play_succ", test_fb_ioctl_online_play_succ},
	KU_TEST_INFO_NULL
};

int hisi_fb_suite_init(void)
{
	return 0;
}

int hisi_fb_suite_clean(void)
{
	return 0;
}

KU_SuiteInfo hisi_fb_test_suites[]=
{
	{"hisi_fb_test_blank", hisi_fb_suite_init, hisi_fb_suite_clean, test_hisi_fb_blank, KU_TRUE},
	{"hisi_fb_test_ioctl", hisi_fb_suite_init, hisi_fb_suite_clean, test_hisi_fb_ioctl, KU_FALSE},
	KU_SUITE_INFO_NULL
};

static int hisi_fb_test_init(void)
{
	pr_info("hello,hisi fb kunit test!\n");

	RUN_ALL_TESTS(hisi_fb_test_suites,"/data/log/hisi_fb");
	return 0;
}

static void hisi_fb_test_exit(void)
{
	pr_info("bye,hisi fb kunit test!\n");
}

module_init(hisi_fb_test_init);
module_exit(hisi_fb_test_exit);

MODULE_LICENSE("GPL v2");


