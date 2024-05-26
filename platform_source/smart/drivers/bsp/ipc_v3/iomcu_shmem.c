/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: Contexthub activity recognition driver.
 * Create: 2017-03-31
 */

#include <platform_include/smart/linux/iomcu_shmem.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "common/common.h"
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/pm_wakeup.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/string.h>
#include <linux/jiffies.h>
#include <securec.h>


#ifdef __LLT_UT__
#define STATIC
#else
#define STATIC static
#endif

#define IOMCU_SHM_PR_DEBUG pr_debug
#define SHM_SEND_BASE_ADDR ((uintptr_t)g_shmem_gov.send_addr_base)
#define SHM_RECV_BASE_ADDR ((uintptr_t)g_shmem_gov.recv_addr_base)
#define IPC_SHM_GC_THR 64
#define IPC_SHM_BLOCK_SHIFT 7
#define IPC_SHM_BLOCK_SIZE (1 << IPC_SHM_BLOCK_SHIFT)
#define UINT16_MAX 0xFFFF
#define NS_TO_MS 1000000

struct shmem {
	void __iomem *recv_addr_base;
	void __iomem *send_addr_base;
};

struct ipcshm_mng_head {
	struct list_head head_list;
	u64 alloc_time;
	uintptr_t addr;
	unsigned int size;
};

static struct shmem g_shmem_gov;
static spinlock_t g_ipcshm_lock;
static unsigned int g_shm_alloc_cnt = 0;
static struct gen_pool *g_ipcshm_pool = NULL;
static struct ipcshm_mng_head *g_ipcshm_mng_nodes = NULL;
static LIST_HEAD(g_ipcshm_busy_list);
static LIST_HEAD(g_ipcshm_free_list);


void *ipcshm_get_data(const char *buf)
{
	int ret;
	void __iomem *recv_addr = NULL;
	struct ipcshm_data_hdr *shm_hdr = NULL;
	struct shmem_ipc_ctrl_package *ctrl =
		(struct shmem_ipc_ctrl_package *)buf;
	void *head = NULL;
	unsigned int usr_len;

	if (ctrl == NULL || g_shmem_gov.recv_addr_base == NULL)
		return NULL;

	if (ctrl->sh_hdr.offset > DDR_SHMEM_CH_SEND_SIZE - sizeof(struct ipcshm_data_hdr)) {
		pr_err("[%s] data invalid; offset %x\n", __func__,
			   ctrl->sh_hdr.offset);
		return NULL;
	}

	IOMCU_SHM_PR_DEBUG("[%s]recv offset[%u]\n",
		__func__, ctrl->sh_hdr.offset);

	recv_addr = g_shmem_gov.recv_addr_base + ctrl->sh_hdr.offset;
	shm_hdr	= (struct ipcshm_data_hdr *)recv_addr;
	if (shm_hdr->magic_word != IPC_SHM_MAGIC) {
		pr_err("[%s] magic_word [0x%x]err\n",
			__func__, shm_hdr->magic_word);
		return NULL;
	}

	usr_len = shm_hdr->pkt.length + sizeof(struct pkt_header);
	if (usr_len > DDR_SHMEM_CH_SEND_SIZE - ctrl->sh_hdr.offset) {
		pr_err("[%s] len too big %u\n", __func__, shm_hdr->pkt.length);
		return NULL;
	}

	head = kzalloc(usr_len, GFP_ATOMIC);
	if (head == NULL) {
		pr_err("[%s] kzalloc failed %u\n", __func__, usr_len);
		return NULL;
	}

	ret = memcpy_s(head, usr_len, &shm_hdr->pkt, usr_len);
	if (ret != EOK)
		pr_err("[%s] memcpy_s fail[%d]\n", __func__, ret);

	shm_hdr->data_free = IPC_SHM_FREE;
	return head;
}

/*lint -e446*/
int shmem_send(enum obj_tag module_id, const void *usr_buf, unsigned int usr_buf_size)
{
	struct write_info wr;

	if (usr_buf == NULL) {
		pr_err("[%s] usr_buf null err\n", __func__);
		return -1;
	}

	(void)memset_s(&wr, sizeof(wr), 0, sizeof(wr));
	wr.tag = module_id;
	wr.wr_buf = usr_buf;
	wr.wr_len = (int)usr_buf_size;
	return write_customize_cmd(&wr, NULL, false);
}


unsigned int shmem_get_capacity(void)
{
	unsigned int limit_phymem = (unsigned int)DDR_SHMEM_AP_SEND_SIZE - sizeof(struct ipcshm_data_hdr);
	unsigned int limit_protocol = (unsigned int)UINT16_MAX;
	unsigned int capacity = limit_phymem < limit_protocol ? limit_phymem : limit_protocol;
	IOMCU_SHM_PR_DEBUG("[%s] limit_phymem %u,limit_protocol %u, capacity %u\n", __func__,
		limit_phymem, limit_protocol, capacity);
	return capacity;
}


static void ipcshm_free_by_mng(struct ipcshm_mng_head *mng_head)
{
	struct ipcshm_data_hdr *shm_head;

	shm_head = (struct ipcshm_data_hdr *)mng_head->addr;
	gen_pool_free(g_ipcshm_pool, mng_head->addr, mng_head->size);
	list_del(&mng_head->head_list);

	shm_head->data_free = IPC_SHM_FREE;
	list_add_tail(&mng_head->head_list, &g_ipcshm_free_list);
}

static void ipc_shm_ddr_ctrl(uintptr_t addr, size_t size)
{
	struct ipcshm_data_hdr *shm_head = NULL;
	struct ipcshm_mng_head *busy_flag_entry = NULL;

	shm_head = (struct ipcshm_data_hdr *)addr;
	shm_head->magic_word = IPC_SHM_MAGIC;
	shm_head->data_free = IPC_SHM_BUSY;

	busy_flag_entry = list_first_entry(&g_ipcshm_free_list,
		struct ipcshm_mng_head, head_list);
	list_del(&busy_flag_entry->head_list);

	busy_flag_entry->alloc_time = get_jiffies_64();
	busy_flag_entry->addr = addr;
	busy_flag_entry->size = size;
	list_add_tail(&busy_flag_entry->head_list, &g_ipcshm_busy_list);

	g_shm_alloc_cnt++;
	g_shm_alloc_cnt &= (IPC_SHM_GC_THR - 1);
}

static void pool_status_show(void)
{
	struct ipcshm_data_hdr *shm_head = NULL;
	struct ipcshm_mng_head *busy_flag_entry = NULL;
	unsigned int used_size = 0;
	pr_info("%s\n", __func__);

	if (list_empty(&g_ipcshm_free_list))
		return;

	list_for_each_entry(busy_flag_entry, &g_ipcshm_busy_list, head_list) {
		pr_info("alloc_time[%llx]addr[%pK]size[%u]\n", busy_flag_entry->alloc_time,
		(void *)busy_flag_entry->addr, busy_flag_entry->size);
		used_size += busy_flag_entry->size;
		shm_head = (struct ipcshm_data_hdr *)busy_flag_entry->addr;
		if (shm_head != NULL)
			pr_info("magic[%x]f[%x]tag[%x]cmd[%x]tranid[%x]\n", shm_head->magic_word, shm_head->data_free,
			shm_head->pkt.tag, shm_head->pkt.cmd, shm_head->pkt.tranid);
	}

	pr_info("used_size %u\n", used_size);
}

static unsigned long ipc_shm_alloc(size_t size)
{
	uintptr_t addr;
	int shm_reclaimed = 0;
	struct ipcshm_data_hdr *shm_head = NULL;
	struct ipcshm_mng_head *busy_flag_entry = NULL, *next = NULL;
	u64 diff_msecs;

	spin_lock(&g_ipcshm_lock);

	if (list_empty(&g_ipcshm_free_list)) {
		spin_unlock(&g_ipcshm_lock);
		return 0;
	}

	if ((g_shm_alloc_cnt & (IPC_SHM_GC_THR - 1)) == (IPC_SHM_GC_THR - 1)) {
try_reclaim:
		list_for_each_entry_safe(busy_flag_entry, next,
		&g_ipcshm_busy_list, head_list) {
			shm_head =
				(struct ipcshm_data_hdr *)busy_flag_entry->addr;

			if (shm_head == NULL)
				continue;

			if (shm_head->magic_word != IPC_SHM_MAGIC)
				continue;

			if (shm_head->data_free == IPC_SHM_FREE) {
				ipcshm_free_by_mng(busy_flag_entry);
				continue;
			}

			diff_msecs = jiffies64_to_nsecs(get_jiffies_64() - busy_flag_entry->alloc_time)  / NS_TO_MS;
			if (diff_msecs > 10000) { /* 10s */
				pr_err("%s No Mem!,tag[0x%x]cmd[0x%x]len[%u]latency%llums\n", __func__,
					shm_head->pkt.tag, shm_head->pkt.cmd,
					shm_head->pkt.length, diff_msecs);
				ipcshm_free_by_mng(busy_flag_entry);
				continue;
			}
		}
		shm_reclaimed = 1;
	}

	addr = gen_pool_alloc(g_ipcshm_pool, size);
	if (addr == 0) {
		if (shm_reclaimed) {
			pr_err("[%s]gen_pool_alloc failed\n", __func__);
			pool_status_show();
			spin_unlock(&g_ipcshm_lock);
			return 0;
		}

		goto try_reclaim;
	}

	(void)memset_s((void *)addr, size, 0, size);

	ipc_shm_ddr_ctrl(addr, size);
	spin_unlock(&g_ipcshm_lock);
	return addr;
}


void ipcshm_clean(void)
{
	if (g_ipcshm_pool != NULL) {
		gen_pool_destroy(g_ipcshm_pool);
		g_ipcshm_pool = NULL;
	}

	if (g_ipcshm_mng_nodes != NULL) {
		INIT_LIST_HEAD(&g_ipcshm_free_list);
		INIT_LIST_HEAD(&g_ipcshm_busy_list);
		kfree((void *)g_ipcshm_mng_nodes);
		g_ipcshm_mng_nodes = NULL;
	}
}

int ipcshm_init(void)
{
	int loop;
	unsigned int mng_nodes_num;
	int ret = 0;

	ipcshm_clean();

	// block size is 128byte (1 << 7)
	g_ipcshm_pool = gen_pool_create(IPC_SHM_BLOCK_SHIFT, -1);
	if (g_ipcshm_pool == NULL) {
		pr_err("Cannot allocate memory pool for ipc share memory\n");
		return -ENOMEM;
	}

	g_ipcshm_pool->name = kstrdup_const("ipcshm", GFP_KERNEL);
	if (g_ipcshm_pool->name == NULL)
		goto IPCSHM_INIT_ERR;

	ret = gen_pool_add_virt(g_ipcshm_pool, SHM_SEND_BASE_ADDR,
		DDR_SHMEM_AP_SEND_ADDR_AP, DDR_SHMEM_AP_SEND_SIZE, -1);
	if (ret != 0)
		goto IPCSHM_INIT_ERR;

	mng_nodes_num = DDR_SHMEM_AP_SEND_SIZE / IPC_SHM_BLOCK_SIZE *
		   sizeof(struct ipcshm_mng_head);
	g_ipcshm_mng_nodes =
		(struct ipcshm_mng_head *)kzalloc(mng_nodes_num, GFP_KERNEL);
	if (g_ipcshm_mng_nodes == NULL)
		goto IPCSHM_INIT_ERR;

	for (loop = 0;
		loop < DDR_SHMEM_AP_SEND_SIZE / IPC_SHM_BLOCK_SIZE; loop++)
		list_add_tail(&g_ipcshm_mng_nodes[loop].head_list,
		&g_ipcshm_free_list);

	pr_info("[%s]fin\n", __func__);
	return 0;

IPCSHM_INIT_ERR:
	pr_err("[%s]err out\n", __func__);
	if (g_ipcshm_pool != NULL) {
		gen_pool_destroy(g_ipcshm_pool);
		g_ipcshm_pool = NULL;
	}
	return ret;
}

static void ipcshm_free(uintptr_t addr)
{
	struct ipcshm_data_hdr *shm_head = NULL;
	struct ipcshm_mng_head *mng_head = NULL;
	struct ipcshm_mng_head *tmp = NULL;

	if (addr == 0)
		return;

	shm_head = (struct ipcshm_data_hdr *)addr;
	if (shm_head->magic_word != IPC_SHM_MAGIC)
		return;

	spin_lock(&g_ipcshm_lock);

	list_for_each_entry_safe(mng_head, tmp,
		&g_ipcshm_busy_list, head_list) {
		if (mng_head->addr == addr) {
			ipcshm_free_by_mng(mng_head);
			break;
		}
	}

	spin_unlock(&g_ipcshm_lock);
}


int ipcshm_send(struct write_info *wr, struct pkt_header *pkt,
	struct read_info *rd)
{
	uintptr_t shm_addr;
	int ret;
	struct ipcshm_data_hdr *shm_head = NULL;
	struct ipc_shm_ctrl_hdr *shm_ctrl_hdr = NULL;

	if (wr == NULL || pkt == NULL || g_shmem_gov.send_addr_base == NULL || g_ipcshm_pool == NULL)
		return -EINVAL;

	if (wr->wr_len <= (int)IPC_BUF_USR_SIZE || wr->wr_len > (int)shmem_get_capacity()) {
		pr_err("[%s]tag[0x%x]orLen[0x%x]err\n", __func__, wr->tag, wr->wr_len);
		return -EINVAL;
	}

	shm_addr = (uintptr_t)ipc_shm_alloc(wr->wr_len + sizeof(struct ipcshm_data_hdr));
	if (shm_addr == 0)
		return -ENOMEM;

	shm_head = (struct ipcshm_data_hdr *)shm_addr;

	ret = memcpy_s((void *)(shm_head + 1), wr->wr_len, wr->wr_buf, wr->wr_len);
	if (ret != EOK)
		pr_err("[%s] memcpy_s fail[%d]\n", __func__, ret);

	shm_head->pkt.tag = wr->tag;
	shm_head->pkt.app_tag = wr->app_tag;
	shm_head->pkt.cmd = wr->cmd;
	shm_head->pkt.tranid = pkt->tranid;
	shm_head->pkt.length = wr->wr_len;
	shm_head->pkt.resp = (rd == NULL) ? NO_RESP : RESP;
	shm_head->pkt.hw_trans_mode = IOMCU_TRANS_SHMEM_TYPE;
	pkt->tag = wr->tag;
	pkt->app_tag = (unsigned int)(wr->app_tag);
	pkt->cmd = (unsigned char)(wr->wr_len > (int)MID_PKT_LEN ? CMD_CMN_IPCSHM_REQ : wr->cmd);
	pkt->resp = NO_RESP;
	pkt->hw_trans_mode = IOMCU_TRANS_SHMEM_TYPE;
	pkt->length = (unsigned short int)sizeof(struct ipc_shm_ctrl_hdr);

	shm_ctrl_hdr = (struct ipc_shm_ctrl_hdr *)(pkt + 1);
	shm_ctrl_hdr->offset = (unsigned int)(shm_addr - SHM_SEND_BASE_ADDR);
	shm_ctrl_hdr->buf_size = (unsigned int)(wr->wr_len) + sizeof(struct ipcshm_data_hdr);

	IOMCU_SHM_PR_DEBUG("[%s]tag[0x%x]cmd[0x%x]pktlen[%u]"
		"offset[%u]tranid[0x%x]\n",
		__func__, pkt->tag, pkt->cmd, pkt->length,
		shm_ctrl_hdr->offset, shm_head->pkt.tranid);

	ret = iomcu_write_ipc_msg(pkt, pkt->length + sizeof(struct pkt_header), rd, wr->cmd);
	if (ret != 0)
		ipcshm_free(shm_addr);

	return ret;
}

void iomcu_shm_exit(void)
{
	ipcshm_clean();
	if (g_shmem_gov.send_addr_base != NULL) {
		iounmap(g_shmem_gov.send_addr_base);
		g_shmem_gov.send_addr_base = NULL;
	}
	if (g_shmem_gov.recv_addr_base != NULL) {
		iounmap(g_shmem_gov.recv_addr_base);
		g_shmem_gov.recv_addr_base = NULL;
	}
}

/*lint -e86*/
int contexthub_shmem_init(void)
{
	int ret;

	ret = get_contexthub_dts_status();
	if (ret != 0)
		return ret;

	g_shmem_gov.recv_addr_base = ioremap_wc((ssize_t)DDR_SHMEM_CH_SEND_ADDR_AP,
		(unsigned long)DDR_SHMEM_CH_SEND_SIZE);
	if (g_shmem_gov.recv_addr_base == NULL) {
		pr_err("[%s] recv_addr_base err\n", __func__);
		return -ENOMEM;
	}

	g_shmem_gov.send_addr_base = ioremap_wc((ssize_t)DDR_SHMEM_AP_SEND_ADDR_AP,
		(unsigned long)DDR_SHMEM_AP_SEND_SIZE);
	if (g_shmem_gov.send_addr_base == NULL) {
		iounmap(g_shmem_gov.recv_addr_base);
		g_shmem_gov.recv_addr_base = NULL;
		pr_err("[%s] send_addr_base err\n", __func__);
		return -ENOMEM;
	}

	spin_lock_init(&g_ipcshm_lock);
	ret = ipcshm_init();
	return ret;
}

/*lint +e446 +e86*/
MODULE_ALIAS("ipcshm");
MODULE_LICENSE("GPL v2");

