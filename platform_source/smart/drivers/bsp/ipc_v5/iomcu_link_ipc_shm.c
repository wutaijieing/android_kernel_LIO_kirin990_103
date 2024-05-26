/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Contexthub ipc link shm.
 * Create: 2021-10-14
 */
#include "common/common.h"
#include <platform_include/smart/linux/iomcu_dump.h>
#include <platform_include/smart/linux/iomcu_ipc.h>
#include <platform_include/smart/linux/iomcu_priv.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
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
#include <iomcu_ddr_map.h>


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
	bool is_inited;
};

struct ipcshm_mng_head {
	struct list_head head_list;
	u64 alloc_time;
	uintptr_t addr;
	unsigned int size;
};

struct link_ipc_shmem_data_hdr {
	unsigned int buf_size;
	unsigned int offset;
	unsigned int magic_word;
	unsigned int data_free;
	unsigned int context;
};

static struct shmem g_shmem_gov = {0};
static spinlock_t g_ipcshm_lock;
static unsigned int g_shm_alloc_cnt = 0;
static struct gen_pool *g_ipcshm_pool = NULL;
static LIST_HEAD(g_ipcshm_alloc_list);

void *ipc_shm_get_recv_buf(const unsigned int offset)
{
	void __iomem *recv_addr = g_shmem_gov.recv_addr_base + offset;
	struct link_ipc_shmem_data_hdr *shm_data_hdr = (struct link_ipc_shmem_data_hdr *)recv_addr;
	void *data;
	int ret;

	if (shm_data_hdr == NULL || g_shmem_gov.recv_addr_base == NULL) {
		pr_err("[%s] invalid para %x, %x\n", __func__, shm_data_hdr, g_shmem_gov.recv_addr_base);
		return NULL;
	}

	if (offset != shm_data_hdr->offset) {
		pr_err("[%s] offset mismatch %x, %x\n", __func__, shm_data_hdr->offset, offset);
		return NULL;
	}

	if (shm_data_hdr->offset > DDR_SHMEM_CH_SEND_SIZE - sizeof(struct link_ipc_shmem_data_hdr)) {
		pr_err("[%s] data invalid; offset %x\n", __func__, shm_data_hdr->offset);
		return NULL;
	}

	if (shm_data_hdr->buf_size > DDR_SHMEM_CH_SEND_SIZE - sizeof(struct link_ipc_shmem_data_hdr)) {
		pr_err("[%s] invalid buf size %x\n", __func__, shm_data_hdr->buf_size);
		return NULL;
	}

	if ((shm_data_hdr->offset + shm_data_hdr->buf_size) >
	    DDR_SHMEM_CH_SEND_SIZE - sizeof(struct link_ipc_shmem_data_hdr)) {
		pr_err("[%s] data invalid; offset %x\n", __func__, shm_data_hdr->offset);
		return NULL;
	}

	IOMCU_SHM_PR_DEBUG("[%s]recv offset[%u]\n", __func__, shm_data_hdr->offset);

	if (shm_data_hdr->magic_word != IPC_SHM_MAGIC) {
		pr_err("[%s] magic_word [0x%x]err\n", __func__, shm_data_hdr->magic_word);
		return NULL;
	}

	data = kzalloc(shm_data_hdr->buf_size, GFP_ATOMIC);
	if (data == NULL) {
		pr_err("[%s] kzalloc failed %u\n", __func__, shm_data_hdr->buf_size);
		return NULL;
	}

	ret = memcpy_s(data, shm_data_hdr->buf_size, (void *)(&shm_data_hdr[1]), shm_data_hdr->buf_size);
	if (ret != EOK)
		pr_err("[%s] memcpy_s fail[%d]\n", __func__, ret);

	shm_data_hdr->data_free = IPC_SHM_FREE;

	return data;
}

unsigned int ipc_shm_get_capacity(void)
{
	unsigned int limit_phymem = (unsigned int)DDR_SHMEM_AP_SEND_SIZE - sizeof(struct link_ipc_shmem_data_hdr);
	unsigned int limit_protocol = (unsigned int)UINT16_MAX;
	unsigned int capacity = limit_phymem < limit_protocol ? limit_phymem : limit_protocol;
	IOMCU_SHM_PR_DEBUG("[%s] limit_phymem %u,limit_protocol %u, capacity %u\n", __func__,
		limit_phymem, limit_protocol, capacity);
	return capacity;
}

static void ipcshm_free_by_mng(struct ipcshm_mng_head *mng_head)
{
	struct link_ipc_shmem_data_hdr *shm_data_hdr;

	shm_data_hdr = (struct link_ipc_shmem_data_hdr *)mng_head->addr;
	gen_pool_free(g_ipcshm_pool, mng_head->addr, mng_head->size);
	list_del(&mng_head->head_list);
	kfree(mng_head);
}

static void pool_status_show(void)
{
	struct link_ipc_shmem_data_hdr *shm_data_hdr = NULL;
	struct ipcshm_mng_head *shm_mng_node = NULL;
	unsigned int used_size = 0;

	pr_info("%s\n", __func__);

	list_for_each_entry(shm_mng_node, &g_ipcshm_alloc_list, head_list) {
		pr_info("alloc_time[%llx]addr[%pK]size[%u]\n",
			shm_mng_node->alloc_time, (void *)shm_mng_node->addr, shm_mng_node->size);
		used_size += shm_mng_node->size;
		shm_data_hdr = (struct link_ipc_shmem_data_hdr *)shm_mng_node->addr;
		if (shm_data_hdr != NULL)
			pr_info("magic[%x]free[%x]ctx[%x]\n",
				shm_data_hdr->magic_word, shm_data_hdr->data_free, shm_data_hdr->context);
	}

	pr_info("used_size %u\n", used_size);
}

uintptr_t ipc_shm_alloc(size_t size, unsigned int context, unsigned int *offset)
{
	uintptr_t addr;
	int shm_reclaimed = 0;
	struct link_ipc_shmem_data_hdr *shm_data_hdr = NULL;
	struct ipcshm_mng_head *shm_mng_node = NULL;
	struct ipcshm_mng_head *shm_mng_node_next = NULL;
	struct ipcshm_mng_head *alloc_node = NULL;
	u64 diff_msecs;
	size_t alloc_size;

	if (g_shmem_gov.is_inited != true) {
		pr_err("%s: ipc shm not inited.\n", __func__);
		return 0;
	}

	if (size >= DDR_SHMEM_CH_SEND_SIZE) {
		pr_err("%s: too large size %x.\n", __func__, size);
		return 0;
	}

	spin_lock(&g_ipcshm_lock);

	if ((g_shm_alloc_cnt & (IPC_SHM_GC_THR - 1)) == (IPC_SHM_GC_THR - 1)) {
try_reclaim:
		list_for_each_entry_safe(shm_mng_node, shm_mng_node_next, &g_ipcshm_alloc_list, head_list) {
			shm_data_hdr = (struct link_ipc_shmem_data_hdr *)shm_mng_node->addr;

			if (shm_data_hdr == NULL)
				continue;

			if (shm_data_hdr->magic_word != IPC_SHM_MAGIC)
				continue;

			if (shm_data_hdr->data_free == IPC_SHM_FREE) {
				ipcshm_free_by_mng(shm_mng_node);
				continue;
			}

			diff_msecs = jiffies64_to_nsecs(get_jiffies_64() - shm_mng_node->alloc_time)  / NS_TO_MS;
			if (diff_msecs > 10000) { /* 10s */
				pr_err("%s No Mem!, context[0x%x] len[%u] latency %llums\n", __func__,
					shm_data_hdr->context, shm_data_hdr->buf_size, diff_msecs);
				ipcshm_free_by_mng(shm_mng_node);
				continue;
			}
		}
		shm_reclaimed = 1;
	}

	alloc_size = size + sizeof(struct link_ipc_shmem_data_hdr);
	addr = gen_pool_alloc(g_ipcshm_pool, alloc_size);
	if (addr == 0) {
		if (shm_reclaimed != 0) {
			pr_err("[%s] gen_pool_alloc failed\n", __func__);
			pool_status_show();
			spin_unlock(&g_ipcshm_lock);
			return 0;
		}

		goto try_reclaim;
	}

	(void)memset_s((void *)addr, alloc_size, 0, alloc_size);

	shm_data_hdr = (struct link_ipc_shmem_data_hdr *)addr;
	shm_data_hdr->magic_word = IPC_SHM_MAGIC;
	shm_data_hdr->data_free = IPC_SHM_BUSY;
	shm_data_hdr->buf_size = (unsigned int)size;
	shm_data_hdr->context = context;
	shm_data_hdr->offset = (unsigned int)(addr - SHM_SEND_BASE_ADDR);

	shm_mng_node = (struct ipcshm_mng_head *)kzalloc(sizeof(struct ipcshm_mng_head), GFP_KERNEL);
	if (shm_mng_node == NULL) {
		pr_err("[%s] alloc ipcshm_mng_head failed\n", __func__);
		gen_pool_free(g_ipcshm_pool, addr, size);
		return 0;
	}

	shm_mng_node->alloc_time = get_jiffies_64();

	shm_mng_node->addr = addr;
	shm_mng_node->size = (unsigned int)size;
	list_add_tail(&shm_mng_node->head_list, &g_ipcshm_alloc_list);

	g_shm_alloc_cnt++;
	g_shm_alloc_cnt &= (IPC_SHM_GC_THR - 1);

	spin_unlock(&g_ipcshm_lock);

	if (offset != NULL)
		*offset = (addr - SHM_SEND_BASE_ADDR);

	pr_info("ipc_shm_alloc: offset %x, %x\n", *offset, shm_data_hdr->offset);

	return (uintptr_t)&shm_data_hdr[1];
}


static void ipc_shm_clean_pool(void)
{
	struct ipcshm_mng_head *shm_mng_node = NULL;
	struct ipcshm_mng_head *shm_mng_node_next = NULL;

	if (g_ipcshm_pool != NULL) {
		gen_pool_destroy(g_ipcshm_pool);
		g_ipcshm_pool = NULL;
	}

	list_for_each_entry_safe(shm_mng_node, shm_mng_node_next, &g_ipcshm_alloc_list, head_list) {
		ipcshm_free_by_mng(shm_mng_node);
	}
}

static int ipc_shm_init_pool(void)
{
	int ret = 0;

	ipc_shm_clean_pool();

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

void ipc_shm_free(uintptr_t addr)
{
	struct link_ipc_shmem_data_hdr *shm_head = NULL;
	struct ipcshm_mng_head *mng_head = NULL;
	struct ipcshm_mng_head *tmp = NULL;

	if (addr < sizeof(struct link_ipc_shmem_data_hdr))
		return;

	shm_head = (struct link_ipc_shmem_data_hdr *)((unsigned long)addr - sizeof(struct link_ipc_shmem_data_hdr));
	if (shm_head->magic_word != IPC_SHM_MAGIC)
		return;

	spin_lock(&g_ipcshm_lock);

	list_for_each_entry_safe(mng_head, tmp,	&g_ipcshm_alloc_list, head_list) {
		if (mng_head->addr == (uintptr_t)shm_head) {
			ipcshm_free_by_mng(mng_head);
			break;
		}
	}

	spin_unlock(&g_ipcshm_lock);
}

static int iomcu_link_ipc_shmem_init(void)
{
	int ret;

	ret = get_contexthub_dts_status();
	if (ret != 0)
		return ret;

	if (g_shmem_gov.is_inited == true) {
		pr_err("[%s] already inited.\n", __func__);
		return -EINVAL;
	}

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
	ret = ipc_shm_init_pool();
	if (ret == 0)
		g_shmem_gov.is_inited = true;

	pr_info("[%s] done, ret %x\n", __func__, ret);
	return ret;
}

static void __exit iomcu_link_ipc_shmem_exit(void)
{
	ipc_shm_clean_pool();
	if (g_shmem_gov.send_addr_base != NULL) {
		iounmap(g_shmem_gov.send_addr_base);
		g_shmem_gov.send_addr_base = NULL;
	}
	if (g_shmem_gov.recv_addr_base != NULL) {
		iounmap(g_shmem_gov.recv_addr_base);
		g_shmem_gov.recv_addr_base = NULL;
	}

	g_shmem_gov.is_inited = false;
	pr_info("[%s] done\n", __func__);
}

device_initcall(iomcu_link_ipc_shmem_init);
module_exit(iomcu_link_ipc_shmem_exit);

MODULE_ALIAS("ipcshm");
MODULE_LICENSE("GPL v2");
