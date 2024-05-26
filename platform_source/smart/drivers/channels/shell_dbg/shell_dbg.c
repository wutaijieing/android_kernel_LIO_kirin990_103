/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: Contexthub shell debug driver. Test only, not enable in USER build.
 * Create: 2016-06-14
 */

#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <global_ddr_map.h>
#include "inputhub_api/inputhub_api.h"
#include "common/common.h"
#include <securec.h>

#define MODULE_NAME "shell_dbg"
#define CHAR_LR 0xA
#define CHAR_CR 0xD
#define SHELL_DBG_STRING_SIZE ((MAX_PKT_LENGTH - (int)sizeof(struct pkt_header)) - 4)
#define SHELL_DBG_FILE_WRITE_MODE        (0666)
enum {
	pro_resp_none,
	pro_resp_ipc,
	pro_resp_wait,
};

struct shell_dbg_resp {
	struct pkt_header hd;
	uint32_t log_addr;
	uint32_t log_len;
};

struct sh_dbg_pkt {
	struct pkt_header hd;
	unsigned char data[SHELL_DBG_STRING_SIZE];
};

static struct completion wait_resp;
static uint32_t get_resp = pro_resp_none;
static uint32_t log_address, log_length;
static char *str_ipc_tmout = "wait:";
static uint32_t ipc_tmout = 6000;

/*
 * SHELL dbg 响应操作
 */
static int shell_dbg_resp_ope(const struct pkt_header *head)
{
	struct shell_dbg_resp *p;

	p = (struct shell_dbg_resp *)head;

	if (p != NULL) {
		pr_info("get resp: tag:%x, addr:%x, len:%x;\n", p->hd.tag, p->log_addr, p->log_len);
		log_address = p->log_addr;
		log_length = p->log_len;
	}
	complete(&wait_resp);
	return 0;
}

/*
 * SHELL dbg 信息发送给contexthub
 */
int shell_dbg_send(const void *buf, unsigned int len)
{
	struct sh_dbg_pkt pkt;
	errno_t ret;
#ifdef CONFIG_INPUTHUB_20
	struct write_info winfo;
#endif

	(void)memset_s(pkt.data, sizeof(pkt.data), 0, sizeof(pkt.data));
	ret = memcpy_s(pkt.data, sizeof(pkt.data), buf, (size_t)len);
	if (ret != EOK) {
		pr_err("%s memcpy buf fail, ret[%d]\n", __func__, ret);
		return -EFAULT;
	}
#ifdef CONFIG_INPUTHUB_20
	winfo.tag = TAG_SHELL_DBG;
	winfo.cmd = CMD_SHELL_DBG_REQ;
	winfo.wr_buf = pkt.data;
	winfo.wr_len = len;
	return write_customize_cmd(&winfo, NULL, true);
#else
	pkt.hd.tag = TAG_SHELL_DBG;
	pkt.hd.cmd = CMD_SHELL_DBG_REQ;
	pkt.hd.resp = 0;
	pkt.hd.length = (uint16_t)len;
	return inputhub_mcu_write_cmd_adapter(&pkt, len + offsetof(struct sh_dbg_pkt, data), NULL);
#endif
}

/*
 * 从contexthub读取 SHELL dbg 回复
 */
static ssize_t shell_dbg_rd(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	ssize_t wr_len = 0;
	char *p_save = NULL;
	char s_buf[0x40] = {0};
	int ret;

	pr_info("shell_dbg_rd:%d\n", file->f_flags); /* for clear warning */
	if (get_resp == pro_resp_ipc) {
		/* get ipc message */
		p_save = (char *)ioremap_wc((size_t)log_address, (size_t)count);
		wr_len = simple_read_from_buffer(buf, count, pos, p_save, (size_t)log_length);
		iounmap(p_save);
	} else if (get_resp == pro_resp_wait) {
		/* return for wait cmd */
		ret = snprintf_s(s_buf, sizeof(s_buf), sizeof(s_buf) - 1, "\nset wait %d(ms) success;\n\n", ipc_tmout);
		if (ret < 0) {
		    pr_err("buffer length is enough\n");
		}
		wr_len = simple_read_from_buffer(buf, count, pos, s_buf, strlen(s_buf));
	} else {
		/* nothing feedback */
		ret = snprintf_s(s_buf, sizeof(s_buf), sizeof(s_buf) - 1, "\nerror: no msg get!\n\n");
		if (ret < 0) {
		    pr_err("buffer length is enough\n");
		}
		wr_len = simple_read_from_buffer(buf, count, pos, s_buf, strlen(s_buf));
	}
	return wr_len;
}

/*
 * 发送信息给contexthub并读取 SHELL dbg 回复
 */
static ssize_t shell_dbg_wr(struct file *file, const char __user *userbuf,
			  size_t bytes, loff_t *off)
{
	char *kn_buf = NULL;
	ssize_t byte_writen;
	int i;
	long val = 0;

	pr_info("shell_dbg_wr:%d\n", file->f_flags); /* for clear warning */
	get_resp = pro_resp_none;	/* clear resp flag first */

	if (bytes >= SHELL_DBG_STRING_SIZE) {
		pr_err("Invalide buffer length\n");
		return -EINVAL;
	}
	kn_buf = kzalloc((ssize_t)SHELL_DBG_STRING_SIZE, GFP_KERNEL);
	if (kn_buf == NULL) {
		pr_err("kn_buf is null\n");
		return -EFAULT;
	}

	byte_writen = simple_write_to_buffer(kn_buf,
					(unsigned long)SHELL_DBG_STRING_SIZE, off,
					userbuf, bytes);
	if (byte_writen <= 0) {
		pr_err("Invalide buffer data\n");
		goto END;
	}

	/* process wait cmd: */
	if (0 == strncmp(str_ipc_tmout, kn_buf, strlen(str_ipc_tmout))) {
		if ((strict_strtol(kn_buf + strlen(str_ipc_tmout), 10, &val) < 0) || (val < 0)) {
			byte_writen = -EINVAL;
		} else {
			get_resp = pro_resp_wait;
			ipc_tmout = (uint32_t)val;
			pr_err("ipc_tmout is %d;\n", ipc_tmout);
		}
		goto END;
	}

	/* process normal cmd: */
	for (i = 0; i < SHELL_DBG_STRING_SIZE; i++) {
		if (kn_buf[i] == CHAR_LR || kn_buf[i] == CHAR_CR) {
			kn_buf[i] = '\0';
STRING_END:
			byte_writen = (ssize_t)(i + 1); /*lint !e776*/
			break;
		}
		if (kn_buf[i] == '\0')
			goto STRING_END;
	}
	kn_buf[SHELL_DBG_STRING_SIZE - 1] = '\0';

	pr_info("shell dgb send str:%s;\n", kn_buf);

	/* process string */
	shell_dbg_send(kn_buf,
	(unsigned int)(strnlen(kn_buf, SHELL_DBG_STRING_SIZE-1) + 1));

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	INIT_COMPLETION(wait_resp);
#else
	reinit_completion(&wait_resp);
#endif
	barrier();

	if (!!wait_for_completion_timeout
	    (&wait_resp, msecs_to_jiffies(ipc_tmout))) {
		get_resp = pro_resp_ipc;
	}

END:
	kfree(kn_buf);
	return byte_writen;
}

static const struct file_operations shell_dbg_fops = {
	.write = shell_dbg_wr,
	.read = shell_dbg_rd,
}; /*lint !e785*/

/*
 * SHELL dbg 初始化
 */
static int __init shell_dbg_init(void)
{
	struct dentry *ch_root = NULL;
	int ret = -1;
	struct dentry *shell_dbg_file_dentry = NULL;
#ifdef CONFIG_DFX_DEBUG_FS
	ret = get_contexthub_dts_status();
	if (ret) {
		return ret;
	}

	init_completion(&wait_resp);

	/*creat this dir first, because multi-platform will use this dir */
	ch_root = debugfs_lookup("contexthub", NULL);
	if (IS_ERR_OR_NULL(ch_root))
		ch_root = debugfs_create_dir("contexthub", NULL);
	else
		pr_info("%s dir contexthub contexthub is already exist\n", __func__);

	if (!IS_ERR_OR_NULL(ch_root)) {
		shell_dbg_file_dentry =
		    debugfs_create_file("shell_dbg", SHELL_DBG_FILE_WRITE_MODE, ch_root, NULL,
					&shell_dbg_fops);
		if ((shell_dbg_file_dentry == NULL)
		    || IS_ERR(shell_dbg_file_dentry)) {
			debugfs_remove(ch_root);
			pr_err("contexthub shell dbg creat failed!\n");
			goto FUNC_END;
		} else {
			ret = register_mcu_event_notifier(TAG_SHELL_DBG,
				CMD_SHELL_DBG_RESP, shell_dbg_resp_ope);
			if (ret) {
				pr_err("[%s] register_mcu_event_notifier err\n", __func__);
			} else {
				pr_info("contexthub shell dbg creat successfully!\n");
			}
		}
	}
#endif
FUNC_END:
	return ret;
}

/*lint -e528 -e753*/
late_initcall_sync(shell_dbg_init);
MODULE_ALIAS("platform:contexthub"MODULE_NAME);
MODULE_LICENSE("GPL v2");

