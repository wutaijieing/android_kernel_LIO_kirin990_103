/*
 *
 * kernel AP maintenance test interface
 *
 * Copyright (c) 2001-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/threads.h>
#include <linux/kallsyms.h>
#include <linux/reboot.h>
#include <linux/input.h>
#include <linux/syscalls.h>
#include <linux/of.h>

#include <asm/ptrace.h>
#include <securec.h>
#include <platform_include/basicplatform/linux/util.h>
#include <platform_include/basicplatform/linux/powerkey_event.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <platform_include/basicplatform/linux/dfx_bootup_keypoint.h>
#include "../rdr_print.h"
#include <platform_include/basicplatform/linux/pr_log.h>
#ifdef CONFIG_DFX_MNTN_GT_WATCH_SUPPORT
#include <sensor/ext_inputhub/default/ext_sensorhub_api.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/reboot.h>
#endif
#ifdef CONFIG_BOOT_DETECTOR
#include <hwbootfail/chipsets/bootfail_hisi.h>
#endif

#define PR_LOG_TAG BLACKBOX_TAG
#define POWERKEY_ONLY_PRODUCT 0x55aa
const char *g_dts_powerkey_only_status_name = "hisilicon,powerhold";

#ifdef CONFIG_DFX_BB
static u32 reboot_reason_flag;
#endif

#ifdef CONFIG_DFX_MNTN_GT_WATCH_SUPPORT
#define AP_MAX_NOTIFY_COUNT 2
#define AP_MCU_SERVICE_ID 0x01
#define AP_NOTIFY_MCU_COMMAND_ID 0x85
#define GPIO_PULL_DOWN                0
static unsigned int g_gt_watch_type = GT_WATCH_DISABLE;
#endif

#ifdef CONFIG_DFX_IRQ_REGISTER
#define LINK_REGISTER_INDEX 30

struct pt_regs g_regs[NR_CPUS];

void irq_register_hook(struct pt_regs *reg)
{
	int cpu;

	if (!reg)
		return;

	cpu = (u8)smp_processor_id();
	if (memcpy_s(&g_regs[cpu], sizeof(g_regs[cpu]), reg, sizeof(*reg)) != EOK)
		BB_PRINT_ERR("%s:%d memcpy_s fail\n", __func__, __LINE__);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
void print_symbol(const char *fmt, unsigned long address)
{
	char buffer[KSYM_SYMBOL_LEN];
	sprint_symbol(buffer, address);
	printk(fmt, buffer);
}
#endif

static void dfx_show_regs(struct pt_regs *reg)
{
	u64 lr, sp;

	if (!reg) {
		BB_PRINT_ERR("%s:%d param reg is NULL\n", __func__, __LINE__);
		return;
	}

	if (compat_user_mode(reg)) {
		lr = reg->compat_lr;
		sp = reg->compat_sp;
	} else {
		lr = reg->regs[LINK_REGISTER_INDEX];
		sp = reg->sp;
	}

	print_symbol("PC is at %s\n", instruction_pointer(reg));
	print_symbol("LR is at %s\n", lr);
	pr_info("pc : [<%016llx>] lr : [<%016llx>] pstate: %08llx\n",
	       reg->pc, lr, reg->pstate);
	pr_info("sp : %016llx\n", sp);
}

void show_irq_register(void)
{
	unsigned int cpu;

	BB_PRINT_PN("show irq register start:\n");
	for (cpu = 0; cpu < num_possible_cpus(); cpu++) {
		BB_PRINT_PN("show cpu%u irq register:\n", cpu);
		dfx_show_regs(&g_regs[cpu]);
	}
	BB_PRINT_PN("show irq register end\n");
}
#endif

#ifdef CONFIG_DFX_REENTRANT_EXCEPTION
#define REENTRANT_EXCEPTION_MAX 5

static int reentrant_exception_num;

void reentrant_exception(void)
{
	reentrant_exception_num++;

	if (reentrant_exception_num > REENTRANT_EXCEPTION_MAX)
		machine_restart("AP_S_PANIC");
}
#endif

static int g_powerkey_only_status;

static int dfx_pmic_powerkey_only_flag(void)
{
	return g_powerkey_only_status;
}

#ifdef CONFIG_DFX_MNTN_GT_WATCH_SUPPORT
unsigned int dfx_pm_gt_watch_type(void)
{
	return g_gt_watch_type;
}
int dfx_pm_ap_pull_down_state(void)
{
	int ret = -1;
	const struct device_node *node = NULL;
	unsigned int gpio_ap_ready;

	node = of_find_compatible_node(NULL, NULL, "hisilicon,ap_ready");
	if (node == NULL) {
		BB_PRINT_ERR("%s[%d]: ap_ready no compatible node found\n",
				__func__, __LINE__);
		return ret;
	}
	ret = of_property_read_u32(node, "gpio", &gpio_ap_ready);
	if (ret || !gpio_ap_ready) {
		BB_PRINT_ERR("%s[%d]: gpio not found, not support ap ready notice\n",
				__func__, __LINE__);
		return ret;
	}
	ret = gpio_request(gpio_ap_ready, "ap_ready");
	if (ret < 0) {
		BB_PRINT_ERR("%s[%u]: request gpio %u fail!\n",
				__func__, __LINE__, gpio_ap_ready);
		return ret;
	}
	ret = gpio_direction_output(gpio_ap_ready, GPIO_PULL_DOWN);
	if (ret < 0) {
		BB_PRINT_ERR("%s[%d]: pull down ap ready fail\n", __func__, __LINE__);
		gpio_free(gpio_ap_ready);
		return ret;
	}
	gpio_free(gpio_ap_ready);
	BB_PRINT_ERR("pull down ap ready to notice kernel reset end\n");
	return ret;
}
static int dfx_pm_pull_down_poweroff(void)
{
	int ret = -1;
	const struct device_node *node = NULL;
	unsigned int gpio_gt_watch_poweroff;

	node = of_find_compatible_node(NULL, NULL, "hisilicon,dfx_gt_poweroff");
	if (node == NULL) {
		BB_PRINT_ERR("%s[%d]: ap_ready no compatible node found\n",
				__func__, __LINE__);
		return ret;
	}
	ret = of_property_read_u32(node, "gpio", &gpio_gt_watch_poweroff);
	if (ret || !gpio_gt_watch_poweroff) {
		BB_PRINT_ERR("%s[%d]: gpio not found, not support poweroff\n",
				__func__, __LINE__);
		return ret;
	}
	ret = gpio_request(gpio_gt_watch_poweroff, "dfx_gt_poweroff");
	if (ret < 0) {
		BB_PRINT_ERR("%s[%u]: request gpio %u fail!\n",
				__func__, __LINE__, gpio_gt_watch_poweroff);
		return ret;
	}
	ret = gpio_direction_output(gpio_gt_watch_poweroff, GPIO_PULL_DOWN);
	if (ret < 0) {
		BB_PRINT_ERR("%s[%d]: pull down poweroff fail\n", __func__, __LINE__);
		gpio_free(gpio_gt_watch_poweroff);
		return ret;
	}
	gpio_free(gpio_gt_watch_poweroff);
	BB_PRINT_ERR("pull down poweroff to notice mcu poweroff\n");
	return ret;
}

static unsigned char dfx_pm_get_gt_watch_reset_type(const char *cmd)
{
	unsigned char reset_type;

	if (cmd == NULL || *cmd == '\0' ||
		!strcmp("userrequested", cmd) ||
		!strcmp("huawei_reboot", cmd) ||
		!strcmp("adb", cmd)) {
		BB_PRINT_ERR("%s whole machine reset\n", __func__);
		reset_type = GT_WATCH_RESET;
	} else {
		BB_PRINT_ERR("%s cmd is %s\n", __func__, cmd);
		reset_type = GT_WATCH_AP_RESET;
	}
	return reset_type;
}
static int dfx_pm_notify_gt_watch(unsigned char power_status)
{
	struct command send_gt_watch = {0};
	int ret;

	send_gt_watch.service_id = AP_MCU_SERVICE_ID;
	send_gt_watch.command_id = AP_NOTIFY_MCU_COMMAND_ID;
	send_gt_watch.send_buffer = &power_status;
	send_gt_watch.send_buffer_len = 1;
	ret = send_command(MNTN_CHANNEL, &send_gt_watch, false, NULL);
	if (ret < 0)
		BB_PRINT_ERR("[%s], send mcu msg fail!\n", __func__);
	BB_PRINT_ERR("[%s], power_status = %u!\n", __func__, power_status);
	return ret;
}
void dfx_pm_gt_watch_reset_process(const char *cmd)
{
	unsigned char reset_type;
	unsigned int count = 0;
	int ret_notify = -1;
	int ret_pull_down = -1;

	reset_type = dfx_pm_get_gt_watch_reset_type(cmd);
	while (count < AP_MAX_NOTIFY_COUNT) {
		ret_notify = dfx_pm_notify_gt_watch(reset_type);
		if (ret_notify >= 0)
			break;
		mdelay(100);
		count++;
	}
	if (g_gt_watch_type == GT_WATCH_A) {
		if (ret_notify >= 0) {
			mdelay(1000);
			ret_pull_down = dfx_pm_ap_pull_down_state();
		}
		/* wait mcu Power off and then power on when whole reset */
		if (ret_notify >= 0 && ret_pull_down >= 0 &&
			reset_type == GT_WATCH_RESET) {
			set_reboot_reason(AP_S_COLDBOOT);
			BB_PRINT_ERR("[%s],notify mcu whole reset\n", __func__);
			while (1);
		}
	}
	BB_PRINT_ERR("[%s],reset reset_type[%u]\n", __func__, reset_type);
}
void dfx_pm_gt_watch_poweroff_process(void)
{
	int ret_notify = -1;
	int ret_pull_down = -1;
	unsigned int count = 0;

	while (count < AP_MAX_NOTIFY_COUNT) {
		ret_notify = dfx_pm_notify_gt_watch(GT_WATCH_POWER_OFF);
		if (ret_notify >= 0)
			break;
		mdelay(100);
		count++;
	}
	if (g_gt_watch_type == GT_WATCH_A) {
		if (ret_notify < 0) {
			BB_PRINT_ERR("[%s], noctify mcu poweroff fail notify[%d]\n",
					__func__, ret_notify);
			/* pull down gpio notify mcu whole shutdown */
			if (!dfx_pm_pull_down_poweroff())
				set_reboot_reason(AP_S_COLDBOOT);
		} else {
			mdelay(1000);
			ret_pull_down = dfx_pm_ap_pull_down_state();
			if (ret_pull_down < 0)
				BB_PRINT_ERR("[%s], pull down fail\n", __func__);
			set_reboot_reason(AP_S_COLDBOOT);
			BB_PRINT_ERR("[%s], notify mcu poweroff end\n", __func__);
		}
		while (1);
	}
	BB_PRINT_ERR("[%s], dfx_pm_gt_watch_poweroff_process\n", __func__);
}
static int dfx_pm_gt_watch_reboot_handler(struct notifier_block *this,
						unsigned long mode, void *cmd)
{
	if (cmd != NULL)
		BB_PRINT_ERR("mode = %lu, cmd = %s\n", mode, cmd);
	if (mode == SYS_RESTART)
		dfx_pm_gt_watch_reset_process((const char *)cmd);
	else if (mode == SYS_POWER_OFF)
		dfx_pm_gt_watch_poweroff_process();
	else
		BB_PRINT_ERR("[%s], mode is valid!\n", __func__);
	return 0;
}

static struct notifier_block dfx_pm_gt_watch_reboot_nb = {
	.notifier_call = dfx_pm_gt_watch_reboot_handler,
	.priority = INT_MAX,
};
void dfx_pm_get_gt_watch_support_state(void)
{
	const char *gt_watch_type = NULL;
	struct device_node *np = NULL;
	int ret;
	np = of_find_compatible_node(NULL, NULL, g_dts_powerkey_only_status_name);
	if (!np) {
		BB_PRINT_ERR("%s: dts node(powerhold) not found\n", __func__);
		return;
	}
	ret = of_property_read_string(np, "gt_watch_type", &gt_watch_type);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find gt watch in dts!\n", __func__);
		goto exit;
	}
	if (gt_watch_type == NULL)
		goto exit;

	if (strncmp("GT_WATCH_A", gt_watch_type, sizeof("GT_WATCH_A")) == 0)
		g_gt_watch_type = GT_WATCH_A;
	else if (strncmp("GT_WATCH_B", gt_watch_type, sizeof("GT_WATCH_B")) == 0)
		g_gt_watch_type = GT_WATCH_B;
	else
		g_gt_watch_type = GT_WATCH_DISABLE;

	if (g_gt_watch_type !=  GT_WATCH_DISABLE) {
		ret = register_reboot_notifier(&dfx_pm_gt_watch_reboot_nb);
		if (ret)
			BB_PRINT_ERR("[%s] register_restart_handler fail!\n", __func__);
	}
	BB_PRINT_ERR("[%s],get support watch ok g_gt_watch_type:%d\n",
				__func__, g_gt_watch_type);
exit:
	of_node_put(np);
}
#endif
void dfx_pmic_powerkey_only_status_get(void)
{
	const char *press_to_fastboot_status = NULL;
	struct device_node *np = NULL;
	int ret;

	np = of_find_compatible_node(NULL, NULL, g_dts_powerkey_only_status_name);
	if (!np) {
		BB_PRINT_ERR("%s: dts node(powerhold) not found\n", __func__);
		return;
	}
	ret = of_property_read_string(np, "press_6s_to_fastboot", &press_to_fastboot_status);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find powerkey_only_product in dts!\n", __func__);
		g_powerkey_only_status = 0;
		goto exit;
	}
	if (strncmp("ok", press_to_fastboot_status, sizeof("ok")) == 0) {
		g_powerkey_only_status = POWERKEY_ONLY_PRODUCT;
	} else {
		g_powerkey_only_status = 0;
		BB_PRINT_ERR("[%s], press_6s_to_fastboot status is not ok!\n", __func__);
	}
exit:
	of_node_put(np);
}

int rdr_press_key_to_fastboot(struct notifier_block *nb, unsigned long event, void *buf)
{
	if (dfx_pmic_powerkey_only_flag() == POWERKEY_ONLY_PRODUCT) {
		if (event != PRESS_KEY_6S)
			return -1;
	} else {
		if (event != PRESS_KEY_UP)
			return -1;
	}
	if (check_mntn_switch(MNTN_PRESS_KEY_TO_FASTBOOT)) {
#ifdef CONFIG_KEYBOARD_GPIO_VOLUME_KEY
		if ((VOL_UPDOWN_PRESS & (unsigned int)gpio_key_vol_updown_press_get())
				== VOL_UPDOWN_PRESS) {
			gpio_key_vol_updown_press_set_zero();
			if (is_gpio_key_vol_updown_pressed()) {
				BB_PRINT_PN("[%s]Powerkey+VolUp_key+VolDn_key\n", __func__);
#ifdef CONFIG_DFX_BB
				rdr_syserr_process_for_ap(MODID_AP_S_COMBINATIONKEY, 0, 0);
#endif
			}
		}
#endif

#ifdef CONFIG_KEYBOARD_PMIC_VOLUME_KEY
		if ((VOL_UPDOWN_PRESS & (unsigned int)pmic_gpio_key_vol_updown_press_get())
				== VOL_UPDOWN_PRESS) {
			pmic_gpio_key_vol_updown_press_set_zero();
			if (is_pmic_gpio_key_vol_updown_pressed()) {
				BB_PRINT_PN("[%s]pmic Powerkey+VolUp_key+VolDn_key\n", __func__);
#ifdef CONFIG_DFX_BB
				rdr_syserr_process_for_ap(MODID_AP_S_COMBINATIONKEY, 0, 0);
#endif
			}
		}
#endif
		if (event == PRESS_KEY_6S) {
			BB_PRINT_PN("[%s]pmic Powerkey 6s\n", __func__);
#ifdef CONFIG_DFX_BB
			rdr_syserr_process_for_ap(MODID_AP_S_COMBINATIONKEY, 0, 0);
			return 0;
#endif
		}
	}
#ifdef CONFIG_DFX_BB
	if (get_reboot_reason() == AP_S_PRESS6S)
		set_reboot_reason(reboot_reason_flag);
#endif
	return 0;
}

void rdr_long_press_powerkey(void)
{
	u32 reboot_reason = get_reboot_reason();

	set_reboot_reason(AP_S_PRESS6S);
	if (get_boot_keypoint() != STAGE_BOOTUP_END) {
		BB_PRINT_PN("press6s in boot\n");
#ifdef CONFIG_BOOT_DETECTOR
		set_valid_long_press_flag();
#endif
		save_log_to_dfx_tempbuffer(AP_S_PRESS6S);
		SYS_SYNC();
	} else {
		reboot_reason_flag = reboot_reason;
		BB_PRINT_PN("press6s:reboot_reason_flag=%u\n", reboot_reason_flag);
	}
}
