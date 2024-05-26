#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/pm_wakeup.h>
#include <linux/errno.h>
#include <linux/of_address.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/completion.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
#include <linux/delay.h>

#include <asm/memory.h>
/*lint -e451*/
#include <asm/types.h>
/*lint +e451*/
#include <asm/io.h>

#include "modem_strategy_acore.h"
#include "audio_pcie_proc.h"
#include "dsp_misc.h"

static void proxy_write_om_buffer(struct work_struct *work);
static void pcie_reopen(struct work_struct *work);
static void pcie_close(struct work_struct *work);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static struct socdsp_voice_om_work_info work_info_proxy[] = {
#else
static struct socdsp_om_work_info work_info_proxy[] = {
#endif
	{HIFI_OM_WORK_PROXY, "hifi_om_work_proxy", proxy_write_om_buffer, {0}},
	{HIFI_PCIE_REOPEN, "hifi_pcie_close", pcie_reopen, {0}},
	{HIFI_PCIE_CLOSE, "hifi_pcie_reopen", pcie_close, {0}},
};

static void pcie_close(struct work_struct *work)
{
	logi("pcie_close\n");
	pcie_deinit();
}
static void pcie_reopen(struct work_struct *work)
{
	logi("pcie_reopen\n");
	start_pcie_thread();
}
static void proxy_write_om_buffer(struct work_struct *work)
{
	int work_id = HIFI_OM_WORK_PROXY;
	struct socdsp_om_work *om_work = NULL;
	unsigned char data[MAIL_LEN_MAX] = {'\0'};
	unsigned int data_len = 0;
	int32_t ret;
	uint8_t *om_buffer;
	unsigned char *hifi_vir_addr;
	struct hifi_ap_om_data_notify *om_data;
	spin_lock_bh(&work_info_proxy[work_id].ctl.lock);
	if (!list_empty(&work_info_proxy[work_id].ctl.list)) {
		om_work = list_entry(work_info_proxy[work_id].ctl.list.next, struct socdsp_om_work, om_node);
		data_len = om_work->data_len;
		memcpy(data, om_work->data, om_work->data_len);/* unsafe_function_ignore: memcpy */
		list_del(&om_work->om_node);
		kfree(om_work);
	}
	spin_unlock_bh(&work_info_proxy[work_id].ctl.lock);

	if (data_len < sizeof(struct hifi_ap_om_data_notify)) {
		loge("ABLOG_KEY: recv msg length error, data_len:%d\n", data_len);
		return;
	}

	om_data = (struct hifi_ap_om_data_notify *)data;
	if ((OM_CHUNK_NUM <= om_data->chunk_index) || (OM_CHUNK_SIZE < om_data->len)) {
		loge("ABLOG_KEY:om buffer chunk info error, chunk_index :%d, chunk_len:%d\n",  om_data->chunk_index, om_data->len);
		return;
	}

	om_buffer = kzalloc(OM_CHUNK_SIZE, GFP_ATOMIC);
	if (!om_buffer) {
		loge("ABLOG_KEY:malloc om buffer failed\n");
		return;
	}
	memset(om_buffer, 0, OM_CHUNK_SIZE);/* unsafe_function_ignore: memset */

	hifi_vir_addr = get_dsp_viradr();
	if (!hifi_vir_addr) {
		loge("ABLOG_KEY:write om, hifi vir addr is null\n");
		kfree(om_buffer);
		om_buffer = NULL;
		return;
	}
	memcpy(om_buffer, hifi_vir_addr + (AP_OM_BUFFER_ADDR - DSP_UNSEC_BASE_ADDR + om_data->chunk_index * OM_CHUNK_SIZE),  om_data->len);/*lint !e679 *//* unsafe_function_ignore: memcpy */

	ret = write_om_data_async((uint8_t *)om_buffer, om_data->len);
	if (ret) {
		loge("ABLOG_KEY:write data error, ret %d\n", ret);
		kfree(om_buffer);
		om_buffer = NULL;
		return;
	}

	return;
}

void modem_om_acore_init(void)
{
	int i;
	start_pcie_thread();
	logi("ABLOG_KEY: modem_om_acore_init\n");

	for (i = 0; i < ARRAY_SIZE(work_info_proxy); i++) {
	work_info_proxy[i].ctl.wq = create_singlethread_workqueue(work_info_proxy[i].work_name);
	if (!work_info_proxy[i].ctl.wq) {
		pr_err("%s(%u):workqueue create failed!\n", __FUNCTION__, __LINE__);
	} else {
		INIT_WORK(&work_info_proxy[i].ctl.work, work_info_proxy[i].func);
		spin_lock_init(&work_info_proxy[i].ctl.lock);
		INIT_LIST_HEAD(&work_info_proxy[i].ctl.list);
	}
	}
	return;
}

void modem_om_acore_deinit(void)
{
	int i;
	pcie_deinit();
	logi("ABLOG_KEY: modem_om_acore_deinit\n");
	for (i = 0; i < ARRAY_SIZE(work_info_proxy); i++) {
	if (work_info_proxy[i].ctl.wq) {
		flush_workqueue(work_info_proxy[i].ctl.wq);
		destroy_workqueue(work_info_proxy[i].ctl.wq);
		work_info_proxy[i].ctl.wq = NULL;
	}
	}
	return;
}

void proxy_rev_data_handle(enum socdsp_om_work_id work_id, const unsigned char *addr, unsigned int len)
{
	struct socdsp_om_work *work = NULL;

	if (!addr || 0 == len || len > MAIL_LEN_MAX) {
		loge("addr is null or len is invaled, len: %u", len);
		return;
	}

	work = kzalloc(sizeof(*work) + len, GFP_ATOMIC);
	if (!work) {
		loge("malloc size %zu failed\n", sizeof(*work) + len);
		return;
	}
	memcpy(work->data, addr, len);/* unsafe_function_ignore: memcpy */
	work->data_len = len;

	spin_lock_bh(&work_info_proxy[work_id].ctl.lock);
	list_add_tail(&work->om_node, &work_info_proxy[work_id].ctl.list);
	spin_unlock_bh(&work_info_proxy[work_id].ctl.lock);

	if (!queue_work(work_info_proxy[work_id].ctl.wq, &work_info_proxy[work_id].ctl.work)) {
		loge("work_id: %d This work was already on the queue\n", work_id);
	}

	return;
}
