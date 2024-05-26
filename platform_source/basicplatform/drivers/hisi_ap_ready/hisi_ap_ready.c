#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/of_gpio.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

static void hisi_ap_ready_notice(void)
{
	int ret;
	const struct device_node *node = NULL;
	unsigned int gpio_ap_ready = 0;

	node = of_find_compatible_node(NULL, NULL, "hisilicon,ap_ready");
	if (node == NULL) {
		pr_info("%s[%d]: hisi_ap_ready no compatible node found\n",
				__func__, __LINE__);
		return;
	}

	ret = of_property_read_u32(node, "gpio", &gpio_ap_ready);
	if (ret || !gpio_ap_ready) {
		pr_info("%s[%d]: gpio not found, not support ap ready notice\n",
				__func__, __LINE__);
		return;
	}

	ret = gpio_request(gpio_ap_ready, "ap_ready");
	if (ret < 0) {
		pr_err("%s[%d]: request gpio %u fail!\n",
				__func__, __LINE__, gpio_ap_ready);
		return;
	}

	ret = gpio_direction_output(gpio_ap_ready, 1);
	if (ret < 0) {
		pr_info("%s[%d]: pull up ap ready fail\n", __func__, __LINE__);
		gpio_free(gpio_ap_ready);
		return;
	}

	gpio_free(gpio_ap_ready);

	pr_err("success pull up ap ready to notice kernel init end\n");

	return;
}

static char hisi_ap_ready_flag = false;

static ssize_t hisi_ap_ready_proc_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	if (!hisi_ap_ready_flag) {
		hisi_ap_ready_notice();
		hisi_ap_ready_flag = true;
	}

	return count;
}

static const struct file_operations hisi_ap_ready_proc_fops = {
	.write      = hisi_ap_ready_proc_write,
};

static int __init hisi_ap_ready_init(void)
{
	if (!proc_create("ap_ready", 0220, NULL, &hisi_ap_ready_proc_fops))
		pr_err("Faild to create ap ready node\n");
	return 0;
}

fs_initcall(hisi_ap_ready_init);
