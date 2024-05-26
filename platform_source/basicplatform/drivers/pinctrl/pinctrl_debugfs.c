#include <linux/compiler.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/reboot.h>
#include <linux/syscalls.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <securec.h>
#include "core.h"
#include "devicetree.h"
#include "pinconf.h"
#include "pinmux.h"

#define DRIVER_NAME "pinctrl-debug"

struct pinctrl_dt_map {
	struct list_head node;
	struct pinctrl_dev *pctldev;
	struct pinctrl_map *map;
	unsigned num_maps;
};

struct pinctrl_debug_dev {
	struct device *dev;
	struct pinctrl *pinctrl;
	struct pinctrl_state            *pins_default;
	struct pinctrl_state            *pins_idle;
	struct dentry *device_root;
};
struct gpio_debug {
	long gpioid;
	uint32_t irq;
	uint32_t flags;
	uint32_t counter;
};

#ifdef CONFIG_PINCTRL_V500
#define PAD_NAME "pinctrl-v500,pad"
#define MUX_NAME "pinctrl-v500,mux"
#else
#define PAD_NAME "pinctrl-single,pins"
#endif

#define PARAM_MIN  4
#define PARAM_MAX  9
#define GPIO_OFFSET 5

static struct gpio_debug g_gpio_debug;
static void free_gpio_irq(void);
static void pinctrl_init_device_debugfs(struct pinctrl_debug_dev *debug_dev);
static void pinctrl_remove_device_debugfs(struct pinctrl_debug_dev *debug_dev);

static int pinctrl_debug_probe(struct platform_device *pdev)
{
	struct pinctrl_debug_dev *debug_dev = NULL;
	struct device *dev = &pdev->dev;

	debug_dev = devm_kzalloc(dev, sizeof(struct pinctrl_debug_dev), GFP_KERNEL);
	if (!debug_dev)
		return -ENOMEM;
	dev_set_drvdata(dev, debug_dev);
	debug_dev->dev = &pdev->dev;
	debug_dev->pinctrl = pinctrl_get(dev);
	debug_dev->pins_default = pinctrl_lookup_state(debug_dev->pinctrl,
			PINCTRL_STATE_DEFAULT);
	debug_dev->pins_idle = pinctrl_lookup_state(debug_dev->pinctrl, PINCTRL_STATE_IDLE);

	pinctrl_init_device_debugfs(debug_dev);
	return 0;
}
static int pinctrl_debug_remove(struct platform_device *pdev)
{
	struct pinctrl_debug_dev *debug_dev = NULL;

	debug_dev = dev_get_drvdata(&pdev->dev);
	if (!debug_dev)
		return -1;

	free_gpio_irq();
	pinctrl_remove_device_debugfs(debug_dev);
	dev_set_drvdata(&pdev->dev, NULL);
	return 0;
}

static ssize_t pinctrl_dbg_write(struct file *file, const char __user *buff,
				size_t count,loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct pinctrl_debug_dev *vendor_dev = s->private;
	char buf[2];
	int status;

	(void)memset_s(buf, sizeof(buf), 0, sizeof(buf));
	if (copy_from_user(&buf, buff, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;
	if (!strncmp(buf, "0", 1)) {
		status = pinctrl_select_state(
			vendor_dev->pinctrl, vendor_dev->pins_default);
		pr_err("pinctrl set default %d \n", status);
	} else if (!strncmp(buf, "1", 1)) {
		status = pinctrl_select_state(
			vendor_dev->pinctrl, vendor_dev->pins_idle);
		pr_err("pinctrl set idle %d \n", status);
	}

	return count;
}

static int pinctrl_dbg_show(struct seq_file *s, void *unused)
{
	struct pinctrl_debug_dev *vendor_dev = s->private;
	struct pinctrl_dt_map *dt_map = NULL;
	struct device_node *np_config = NULL;
	struct resource *res = NULL;
	void __iomem *remap_addr = NULL;
	struct of_phandle_args pinctrl_spec;
	unsigned long addr;

	if (!vendor_dev->pinctrl->state) {
		seq_puts(s, "current state is null\n");
		return 0;
	}
	list_for_each_entry(dt_map, &vendor_dev->pinctrl->dt_maps, node) {
		np_config = of_find_node_by_name(NULL, dt_map->map->data.mux.group);
		if (!np_config) {
			seq_printf(s, "get %s node failed\n", dt_map->map->data.mux.group);
			return 0;
		}
		if (pinctrl_parse_index_with_args(np_config, PAD_NAME, 0, &pinctrl_spec)) {
			seq_printf(s, "get %s info failed\n", dt_map->map->data.mux.group);
			return 0;
		}
		res = platform_get_resource(to_platform_device(dt_map->pctldev->dev), IORESOURCE_MEM, 0);
		if (!res) {
			seq_printf(s, "could not get resource\n");
			return -ENODEV;
		}
		addr = res->start + pinctrl_spec.args[0];
		remap_addr = ioremap(addr, 4);
		if (!remap_addr) {
			seq_printf(s, "remap_addr failed\n");
			return 0;
		}
		seq_printf(s, "%s: addr[0x%x] value[0x%x]\n",dt_map->map->data.mux.group,
			addr, readl(remap_addr));
		iounmap(remap_addr);
#ifdef CONFIG_PINCTRL_V500
		if (!pinctrl_parse_index_with_args(np_config, MUX_NAME, 0, &pinctrl_spec)) {
			addr = pinctrl_spec.args[0];
			remap_addr = ioremap(addr, 4);
			if (!remap_addr) {
				seq_printf(s, "remap_addr failed\n");
				return 0;
			}
			seq_printf(s, "mux: addr[0x%x] value[0x%x]\n", addr, readl(remap_addr));
			iounmap(remap_addr);
		}
#endif
	}
	return 0;
}

static irqreturn_t irq_dbg_handle(int irq, void *data)
{
	pr_err("%s:%d ", __func__, g_gpio_debug.gpioid);
	g_gpio_debug.counter++;
	return IRQ_HANDLED;
}

static void trigger_gpio_irq()
{
	if ((g_gpio_debug.flags & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH) {
		gpio_direction_output(g_gpio_debug.gpioid, 0);
		msleep(10);
		gpio_direction_output(g_gpio_debug.gpioid, 1);
		msleep(10);
		gpio_direction_output(g_gpio_debug.gpioid, 0);
	} else if (g_gpio_debug.flags & IRQ_TYPE_EDGE_RISING) {
		gpio_direction_output(g_gpio_debug.gpioid, 0);
		msleep(10);
		gpio_direction_output(g_gpio_debug.gpioid, 1);
	} else if (g_gpio_debug.flags & IRQ_TYPE_EDGE_FALLING) {
		gpio_direction_output(g_gpio_debug.gpioid, 1);
		msleep(10);
		gpio_direction_output(g_gpio_debug.gpioid, 0);
	}
}

static void free_gpio_irq()
{
	if (g_gpio_debug.irq >= 0)
		free_irq(g_gpio_debug.irq, NULL);
	if (g_gpio_debug.gpioid >= 0)
		gpio_free(g_gpio_debug.gpioid);
	(void)memset_s(&g_gpio_debug, sizeof(struct gpio_debug), 0, sizeof(struct gpio_debug));
	pr_err("%s:gpio is free\n", __func__);
}

static int set_irq(char *buf, size_t count)
{
	int ret = 0;
	if (strncmp(buf, "free", 4) && strncmp(buf, "trigger", 7) &&
		g_gpio_debug.gpioid) {
		pr_err("%s:gpio already set\n", __func__);
		return -EFAULT;
	}

	if (!strncmp(buf, "fall", 4)) {
		g_gpio_debug.flags = IRQ_TYPE_EDGE_FALLING;
	} else if (!strncmp(buf, "rise", 4)) {
		g_gpio_debug.flags = IRQ_TYPE_EDGE_RISING;
	} else if (!strncmp(buf, "both", 4)) {
		g_gpio_debug.flags = IRQ_TYPE_EDGE_BOTH;
	} else if (!strncmp(buf, "trigger", 7)) {
		trigger_gpio_irq();
		ret = count;
	} else if (!strncmp(buf, "free", 4)) {
		free_gpio_irq();
		ret = count;
	} else {
		ret = -EFAULT;
	}

	return ret;
}
static ssize_t gpio_dbg_write(struct file *file, const char __user *buff,
				size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct pinctrl_debug_dev *vendor_dev = s->private;
	char buf[64];
	struct gpio_desc *desc = NULL;
	int	status;

	if (count < PARAM_MIN || count > PARAM_MAX)
		return -EFAULT;

	(void)memset_s(buf, sizeof(buf), 0, sizeof(buf));
	if (copy_from_user(&buf, buff, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	status = set_irq(buf, count);
	if (status)
		return status;

	status = kstrtol(buf + GPIO_OFFSET, 0, &g_gpio_debug.gpioid);
	if (status < 0)
		return -EFAULT;

	if (gpio_request_one(g_gpio_debug.gpioid, GPIOF_DIR_IN, "gpiodbg"))
		goto free_gpio_irq;

	g_gpio_debug.irq = gpio_to_irq(g_gpio_debug.gpioid);
	if (g_gpio_debug.irq < 0)
		goto free_gpio_irq;

	status = request_threaded_irq(g_gpio_debug.irq, irq_dbg_handle, NULL,
		g_gpio_debug.flags, "gpiodbg", NULL);
	if(status)
		goto free_gpio_irq;

	status = gpio_direction_output(g_gpio_debug.gpioid, 0);
	if(status)
		goto free_gpio_irq;

	g_gpio_debug.counter = 0;
	return count;

free_gpio_irq:
	free_gpio_irq();
	return -EFAULT;
}
static int gpio_dbg_show(struct seq_file *s, void *unused)
{
	char *irqtype =NULL;
	if (g_gpio_debug.gpioid == 0) {
		seq_printf(s,"please setup irq\n"
			"irqtye: both/fall/rise gpioid: 1-gpio_max\n"
			"trigger or free\n");
		return 0;
	}
	if ((g_gpio_debug.flags & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH) {
		seq_printf(s, "irqtype is edge both,will trigger 2 times\n");
		irqtype = "both";
	} else if (g_gpio_debug.flags & IRQ_TYPE_EDGE_FALLING) {
		irqtype = "fall";
	} else if (g_gpio_debug.flags & IRQ_TYPE_EDGE_RISING) {
		irqtype = "rise";
	} else {
		irqtype = "null";
	}
	seq_printf(s, "gpio:%d type:%s count:%d\n", g_gpio_debug.gpioid, irqtype,
		g_gpio_debug.counter);
	return 0;
}
static int pinctrl_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, pinctrl_dbg_show, inode->i_private);
}
static int gpio_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, gpio_dbg_show, inode->i_private);
}
static const struct file_operations pinctrl_dbg_ops = {
	.open		= pinctrl_dbg_open,
	.read		= seq_read,
	.write		= pinctrl_dbg_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
static const struct file_operations gpio_dbg_ops = {
	.open		= gpio_dbg_open,
	.read		= seq_read,
	.write		= gpio_dbg_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
static void pinctrl_init_device_debugfs(struct pinctrl_debug_dev *vendor_dev)
{
	struct dentry *device_root = NULL;

	device_root = debugfs_create_dir("pinctrl-debug", NULL);
	vendor_dev->device_root = device_root;
	if (IS_ERR(device_root) || !device_root) {
		pr_err("failed to create debugfs directory for pinctrl-debug\n");
		return;
	}
	debugfs_create_file("pinctrl_dbg", S_IFREG | S_IRUGO,
			    device_root, vendor_dev, &pinctrl_dbg_ops);
	debugfs_create_file("gpio_irq_dbg", S_IFREG | S_IRUGO,
			    device_root, vendor_dev, &gpio_dbg_ops);
}
static void pinctrl_remove_device_debugfs(struct pinctrl_debug_dev *vendor_dev)
{
	debugfs_remove_recursive(vendor_dev->device_root);
}

static const struct of_device_id pinctrl_debug_of_match[] = {
	{ .compatible = "pinctrl-debug" },
	{ },
};
MODULE_DEVICE_TABLE(of, pinctrl_debug_of_match);

static struct platform_driver pinctrl_debug_driver = {
	.probe = pinctrl_debug_probe,
	.remove = pinctrl_debug_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table	= pinctrl_debug_of_match,
	},
};

static int __init pinctrl_debug_init(void)
{
	return platform_driver_register(&pinctrl_debug_driver);
}
static void __exit pinctrl_debug_exit(void)
{
	platform_driver_unregister(&pinctrl_debug_driver);
}
device_initcall(pinctrl_debug_init);
module_exit(pinctrl_debug_exit);