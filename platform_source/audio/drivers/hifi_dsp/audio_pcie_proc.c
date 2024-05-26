#include <linux/of_platform.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <../fs/internal.h>
#include <linux/hrtimer.h>

#include <linux/ioctl.h>
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

#include "dsp_om.h"
#include "audio_pcie_proc.h"
#include "dsp_misc.h"
#include "voice_proxy/voice_proxy.h"

#define PCIE_IOCTL_SET_READ_CB (0x7F001001)
#define PCIE_IOCTL_GET_RD_BUFF (0x7F001011)
#define PCIE_IOCTL_RETURN_BUFF (0x7F001012)
#define PCIE_IOCTL_RELLOC_WRITE_BUFF (0x7F001013)
#define PCIE_IOCTL_WRITE_ASYNC (0x7F001010)
#define PCIE_IOCTL_SET_WRITE_CB (0x7F001000)
#define PCIE_IOCTL_SEND_EVT (0x7F001005)

#define NV_BUFFER_SIZE (256 * 1024)

#define PCIE_INIT_THREAD_NAME ("pcie init thread")
#define OM_DEV ("/dev/pcdev_agent_om")
#define NV_DEV ("/dev/pcdev_agent_nv")
#define MSG_DEV ("/dev/pcdev_agent_msg")
#define PCIE_ACCESS_WAIT_CYCLE (1000)
#define PCIE_REGISTER_MAX_WAITE_COUNT (120)

#define DONE 0
#define UNDONE 1

typedef struct
{
	char *vir_addr;
	char *phy_addr;
	uint32_t len;
	void *drv_priv;
}HIFI_WR_ASYNC_INFO_STRU;

typedef struct
{
	uint32_t buffer_len;
}HIFI_READ_BUFF_INFO;

typedef struct
{
	char device_name[128];
	struct file *dev_obj;
}VOICE_AGENT_DEV_INFO_STRU;

enum PCIE_DEV_ENUM
{
	PCIE_DEV_NV = 0,
	PCIE_DEV_OM,
	PCIE_DEV_MSG,
	PCIE_DEV_BUTT,
};
enum PCIE_EVENT_ENUM
{
	PCIE_EVT_DEV_SUSPEND = 0,
	PCIE_EVT_DEV_READY,
	PCIE_EVT_DEV_BOTTOM
};

#define HIFI_PCIE_MSG_MAIGC_WORD        0x5a5b5c5d
typedef struct
{
	uint32_t sendercpuid;
	uint32_t senderpid;
	uint32_t receivercpuid;
	uint32_t receiverpid;
	uint32_t length;
	uint8_t  aucdata[4];
}HIFI_AGENT_MSG_OSA_STRU;
typedef struct
{
	uint16_t msg_id;
	uint16_t reserved;
}HIFI_AGENT_MSG_BODY_STRU;

typedef struct
{
	uint32_t hifimsgmagic;
	HIFI_AGENT_MSG_OSA_STRU stmsg;
}HIFI_MSG_PCIE_STRU;

extern int32_t voice_proxy_mailbox_send_msg_cb(uint32_t mailcode, uint16_t msg_id, const void *buf, uint32_t size);

VOICE_AGENT_DEV_INFO_STRU g_pcie_dev[PCIE_DEV_BUTT] =
{
	{{0}, NULL},
	{{0}, NULL},
	{{0}, NULL}
};
struct task_struct* pcie_init_thread = NULL;
bool g_first_time_nv_send_flag = true;
static void *_bsp_pcdev_open(char *file_name)
{
	struct file* filp;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	filp = filp_open(file_name, O_RDWR, 0);
	set_fs(old_fs);

	if (IS_ERR_OR_NULL(filp)) {
		return NULL;
	}

	return filp;
}

static int32_t _bsp_pcdev_ioctl(void *handle, u32 cmd, void *para)
{
	int status = 0;
	mm_segment_t old_fs;
	struct file* filp = (struct file*)handle;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	status = vfs_ioctl(filp, (unsigned int)cmd, (unsigned long)para);
#else
	status = do_vfs_ioctl(filp, 0, (unsigned int)cmd, (unsigned long)para);
#endif
	set_fs(old_fs);

	return status;
}

void _bsp_pcdev_close(void *handle)
{
	mm_segment_t old_fs;
	struct file* filp = (struct file*)handle;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	filp_close(filp, NULL);
	set_fs(old_fs);
}

static void _pcie_read_nv_data(void)
{
	uint8_t ret;
	unsigned char	*hifi_vir_addr;
	HIFI_WR_ASYNC_INFO_STRU ctrl_param;
	uint8_t *msgbuffer = NULL;
	HIFI_AGENT_MSG_BODY_STRU msgToHifi;

	memset(&ctrl_param, 0, sizeof(HIFI_WR_ASYNC_INFO_STRU));/* unsafe_function_ignore: memset */

	ret = _bsp_pcdev_ioctl(g_pcie_dev[PCIE_DEV_NV].dev_obj, PCIE_IOCTL_GET_RD_BUFF, &ctrl_param);
	if (ret) {
		loge("ABLOG_KEY:pcie get read buffer failed, ret %d\n", ret);
		return;
	}

	if (!ctrl_param.vir_addr) {
		loge("ABLOG_KEY:pcie read buffer is null\n");
		goto exit;
	}
	if (AP_MODEM_NV_SIZE < ctrl_param.len) {
		loge("ABLOG_KEY:pcie read data size is too long, data size %d\n", ctrl_param.len);
		goto exit;
	}

	hifi_vir_addr = get_dsp_viradr();
	if (!hifi_vir_addr) {
		loge("ABLOG_KEY:read nv,hifi vir addr is null\n");
		goto exit;
	}
	memcpy(hifi_vir_addr + (AP_MODEM_NV_ADDR - DSP_UNSEC_BASE_ADDR), ctrl_param.vir_addr, ctrl_param.len);/* unsafe_function_ignore: memcpy */

		/* Notify HIFI to refresh NV */
	if (!g_first_time_nv_send_flag) {
		memset(&msgToHifi, 0, sizeof(HIFI_AGENT_MSG_BODY_STRU));/* unsafe_function_ignore: memset */
		msgToHifi.msg_id = ID_AP_HIFI_NV_REFRESH_IND;
		ret = voice_proxy_mailbox_send_msg_cb(MAILBOX_MAILCODE_ACPU_TO_HIFI_VOICE,
				msgToHifi.msg_id,
				&msgToHifi,
				sizeof(HIFI_AGENT_MSG_BODY_STRU));
		logi("ABLOG_KEY:mailbox_send_msg ID :0x%x\n", msgToHifi.msg_id);
		if (ret) {
			loge("ABLOG_KEY:mailbox_send_msg fail:%d\n", ret);
		}
	} else {
		g_first_time_nv_send_flag = false;
	}

	exit:
		ret = _bsp_pcdev_ioctl(g_pcie_dev[PCIE_DEV_NV].dev_obj, PCIE_IOCTL_RETURN_BUFF, &ctrl_param);
		if (ret ) {
			loge("pcie release buffer failed, ret %d\n", ret);
		}
		if (msgbuffer) {
			kfree(msgbuffer);
		}
		return;
}
static void _pcie_read_msg_data(void)
{
	uint8_t ret;
	HIFI_WR_ASYNC_INFO_STRU ctrl_param;
	uint8_t *msgbuffer = NULL;
	HIFI_MSG_PCIE_STRU * pPcieData = NULL;

	memset(&ctrl_param, 0, sizeof(HIFI_WR_ASYNC_INFO_STRU));/* unsafe_function_ignore: memset */

	ret = _bsp_pcdev_ioctl(g_pcie_dev[PCIE_DEV_MSG].dev_obj, PCIE_IOCTL_GET_RD_BUFF, &ctrl_param);
	if (ret) {
		loge("ABLOG_KEY:pcie get read buffer failed, ret %d\n", ret);
		return;
	}

	if (!ctrl_param.vir_addr) {
		loge("ABLOG_KEY:pcie read buffer is null\n");
		goto exit;
	}
	if (AP_MODEM_MSG_SIZE < ctrl_param.len) {
		loge("ABLOG_KEY:pcie read data size is too long, data size %d\n", ctrl_param.len);
		goto exit;
	}
	if (HIFI_PCIE_MSG_MAIGC_WORD == *((uint32_t*)ctrl_param.vir_addr)) {
		pPcieData = (HIFI_MSG_PCIE_STRU*)ctrl_param.vir_addr;
		msgbuffer = kzalloc(pPcieData->stmsg.length, GFP_ATOMIC);
		if (!msgbuffer) {
			loge("ABLOG_KEY:malloc msgbuffer failed\n");
			return;
		}
		memset(msgbuffer, 0, pPcieData->stmsg.length);/* unsafe_function_ignore: memset */
		memcpy(msgbuffer, pPcieData->stmsg.aucdata, pPcieData->stmsg.length);/* unsafe_function_ignore: memcpy */
		ret = voice_proxy_mailbox_send_msg_cb(MAILBOX_MAILCODE_ACPU_TO_HIFI_VOICE,
					*((uint16_t*)msgbuffer),
					msgbuffer,
					pPcieData->stmsg.length);
		logi("ABLOG_KEY:mailbox_send_msg ID :0x%x\n", *((uint16_t*)msgbuffer));
		goto exit;
	} else {
		loge("ABLOG_KEY:_pcie_read_msg_data ERROR MAGIC NUM %d\n",*((uint32_t*)ctrl_param.vir_addr));
	}

	exit:
		ret = _bsp_pcdev_ioctl(g_pcie_dev[PCIE_DEV_MSG].dev_obj, PCIE_IOCTL_RETURN_BUFF, &ctrl_param);
		if (ret ) {
			loge("pcie release buffer failed, ret %d\n", ret);
		}
		if (msgbuffer) {
			kfree(msgbuffer);
		}
		return;
}

static void _pcie_write_om_data_callback(char *viraddr, char *phyaddr, int size)
{
	if (viraddr == NULL) {
		loge("pcie callback vir buffer is null\n");
		return;
	}
	kfree(viraddr);
	if (size < 0) {
		loge("pcie callback write fail\n");
	}
}

int8_t pcie_write_om_data_init(void)
{
	int8_t ret = UNDONE;
	struct file *dev;
	int32_t om_dev_status;

	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		om_dev_status = ksys_access(OM_DEV, O_RDWR);
	#else
		om_dev_status = sys_access(OM_DEV, O_RDWR);
	#endif
	if (om_dev_status) {
		return ret;
	}
	if (g_pcie_dev[PCIE_DEV_OM].dev_obj) {
		return ret;
	}
	logi("ABLOG_KEY:pcie om data init\n");
	memset(&g_pcie_dev[PCIE_DEV_OM], 0, sizeof(VOICE_AGENT_DEV_INFO_STRU));/* unsafe_function_ignore: memset */
	strncpy(g_pcie_dev[PCIE_DEV_OM].device_name, OM_DEV, sizeof(OM_DEV));/* unsafe_function_ignore: strncpy */
	dev = _bsp_pcdev_open(g_pcie_dev[PCIE_DEV_OM].device_name);
	if (!dev) {
		loge("ABLOG_KEY:write_om_bsp_pcdev_open fail");
		return ret;
	}

	g_pcie_dev[PCIE_DEV_OM].dev_obj = dev;
	ret = _bsp_pcdev_ioctl(g_pcie_dev[PCIE_DEV_OM].dev_obj, PCIE_IOCTL_SET_WRITE_CB, _pcie_write_om_data_callback);
	if (ret) {
		loge("ABLOG_KEY:pcie set data read callback for om data write failed, ret %d\n", ret);
		return ret;
	}

	return ret;
}

int8_t pcie_read_nv_data_init(void)
{
	int8_t ret = UNDONE;
	struct file *dev;
	HIFI_READ_BUFF_INFO buff_info;
	int32_t nv_dev_status;
	unsigned char *hifi_vir_addr;
	uint32_t pcie_event = PCIE_EVT_DEV_READY;

	hifi_vir_addr = get_dsp_viradr();
	memset(hifi_vir_addr + (AP_MODEM_NV_ADDR - DSP_UNSEC_BASE_ADDR), 0, AP_MODEM_NV_SIZE);/* unsafe_function_ignore: memset */
	logi("ABLOG_KEY:hifi_vir_addr magicnum :0x%x\n", *((uint32_t *)(hifi_vir_addr + (AP_MODEM_NV_ADDR - DSP_UNSEC_BASE_ADDR))));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	nv_dev_status = ksys_access(NV_DEV, O_RDWR);
#else
	nv_dev_status = sys_access(NV_DEV, O_RDWR);
#endif
	if (nv_dev_status) {
		return ret;
	}

	if (g_pcie_dev[PCIE_DEV_NV].dev_obj) {
		return ret;
	}
	logi("ABLOG_KEY:pcie nv read init\n");
	memset(&g_pcie_dev[PCIE_DEV_NV], 0, sizeof(VOICE_AGENT_DEV_INFO_STRU));/* unsafe_function_ignore: memset */
	strncpy(g_pcie_dev[PCIE_DEV_NV].device_name, NV_DEV, sizeof(NV_DEV));/* unsafe_function_ignore: strncpy */

    dev = _bsp_pcdev_open(g_pcie_dev[PCIE_DEV_NV].device_name);
	if (!dev) {
		loge("ABLOG_KEY:read_nv_bsp_pcdev_open fail");
		return ret;
	}

	g_pcie_dev[PCIE_DEV_NV].dev_obj = dev;
	buff_info.buffer_len = NV_BUFFER_SIZE;
	ret = _bsp_pcdev_ioctl(g_pcie_dev[PCIE_DEV_NV].dev_obj, PCIE_IOCTL_RELLOC_WRITE_BUFF, &buff_info);
	if (ret) {
		loge("ABLOG_KEY:pcie nv read realloc read buffer failed, ret %d\n", ret);
		return ret;
	}

	ret = _bsp_pcdev_ioctl(g_pcie_dev[PCIE_DEV_NV].dev_obj, PCIE_IOCTL_SET_READ_CB, _pcie_read_nv_data);
	if (ret) {
		loge("ABLOG_KEY:pcie set nv read callback failed, ret %d\n", ret);
		return ret;
	}

	ret = _bsp_pcdev_ioctl(g_pcie_dev[PCIE_DEV_NV].dev_obj, PCIE_IOCTL_SEND_EVT, &pcie_event);
	if (ret) {
		loge("ABLOG_KEY:pcie set send event failed, ret %d\n", ret);
		return ret;
	}


	return ret;
}

int8_t pcie_read_msg_data_init(void)
{
	int8_t ret = UNDONE;
	struct file *dev;
	int32_t msg_dev_status;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	msg_dev_status = ksys_access(MSG_DEV, O_RDWR);
#else
	msg_dev_status = sys_access(MSG_DEV, O_RDWR);
#endif
	if (msg_dev_status) {
		return ret;
	}
	if (g_pcie_dev[PCIE_DEV_MSG].dev_obj) {
		return ret;
	}
	logi("ABLOG_KEY:pcie msg read init\n");
	memset(&g_pcie_dev[PCIE_DEV_MSG], 0, sizeof(VOICE_AGENT_DEV_INFO_STRU));/* unsafe_function_ignore: memset */
	strncpy(g_pcie_dev[PCIE_DEV_MSG].device_name, MSG_DEV, sizeof(MSG_DEV));/* unsafe_function_ignore: strncpy */

    dev = _bsp_pcdev_open(g_pcie_dev[PCIE_DEV_MSG].device_name);
	if (!dev) {
		loge("ABLOG_KEY:read_msg_bsp_pcdev_open fail");
		return ret;
	}

	g_pcie_dev[PCIE_DEV_MSG].dev_obj = dev;

	ret = _bsp_pcdev_ioctl(g_pcie_dev[PCIE_DEV_MSG].dev_obj, PCIE_IOCTL_SET_READ_CB, _pcie_read_msg_data);
	if (ret) {
		loge("ABLOG_KEY:pcie set msg read callback failed, ret %d\n", ret);
		return ret;
	}

	return ret;
}
static bool _pcie_all_ready(int8_t status1, int8_t status2, int8_t status3)
{
	if ((status1 == DONE) && (status2 == DONE) && (status3 == DONE)) {
		return true;
	}
	return false;
}
static int _pcie_init_loop(void *p)
{
	uint32_t wait_count = 0;
	int8_t ret_om = UNDONE;
	int8_t ret_nv = UNDONE;
	int8_t ret_msg = UNDONE;

	while (!kthread_should_stop()) {

		if (ret_om != DONE) {
			ret_om = pcie_write_om_data_init();
		}

		if (ret_nv != DONE) {
			ret_nv = pcie_read_nv_data_init();
		}

		if (ret_msg != DONE) {
			ret_msg = pcie_read_msg_data_init();
		}
		if (_pcie_all_ready(ret_msg, ret_om, ret_nv)) {
			break;
		}

		msleep(PCIE_ACCESS_WAIT_CYCLE);
		wait_count++;
		logi("ABLOG_KEY:pcie register wait count %d\n", wait_count);
		if (PCIE_REGISTER_MAX_WAITE_COUNT < wait_count) {
			loge("ABLOG_KEY:pcie register wait too long, wait count %d\n", wait_count);
			break;
		}
	}

	return 0;
}

void start_pcie_thread(void)
{
	pcie_init_thread = kthread_create(_pcie_init_loop, 0, PCIE_INIT_THREAD_NAME);
	if (IS_ERR(pcie_init_thread)) {
		loge("ABLOG_KEY:creat hifi pcie register thread fail\n");
		return;
	}

	wake_up_process(pcie_init_thread);
	logi("ABLOG_KEY:start pcie init thread\n");
}

int32_t write_om_data_async(uint8_t *addr, uint32_t len)
{
	int32_t ret;
	HIFI_WR_ASYNC_INFO_STRU ctrl_param;

	if (!g_pcie_dev[PCIE_DEV_OM].dev_obj) {
		loge("ABLOG_KEY:pcie invalid device\n");
		return -EINVAL;
	}

	ctrl_param.vir_addr = (char *)addr;
	ctrl_param.phy_addr = NULL;
	ctrl_param.len	= len;
	ctrl_param.drv_priv = NULL;

	ret = _bsp_pcdev_ioctl(g_pcie_dev[PCIE_DEV_OM].dev_obj, PCIE_IOCTL_WRITE_ASYNC, &ctrl_param);
	if (ret) {
		loge("ABLOG_KEY:pcie write data failed, ret %d\n", ret);
		return ret;
	}
	return ret;
}

void pcie_deinit(void)
{
	uint32_t index;

	for (index = 0; index < PCIE_DEV_BUTT; index++) {
		if (NULL == g_pcie_dev[index].dev_obj) {
			loge("ABLOG_KEY:pcie dev is null, dev id %d\n", index);
			continue;
		}
		_bsp_pcdev_close(g_pcie_dev[index].dev_obj);
		g_pcie_dev[index].dev_obj = NULL;
	}

	if (!pcie_init_thread) {
		kthread_stop(pcie_init_thread);
		pcie_init_thread = NULL;
	}
	logi("ABLOG_KEY:pcie_deinit end\n");
	printk("ABLOG_KEY:clear pcie info\n");
}

