/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: device manager implement.
 * Create: 2019-11-05
 */

#include "device_manager.h"

#include <securec.h>

#include <linux/module.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include "iomcu_dmd.h"
#include "iomcu_route.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "platform_include/smart/linux/iomcu_pm.h"
#include "platform_include/smart/linux/iomcu_dump.h"
#include "platform_include/smart/linux/iomcu_boot.h"
#include "platform_include/smart/linux/iomcu_priv.h"
#include "iomcu_ipc.h"
#include "iomcu_log.h"
#include "common/common.h"


struct device_manager_list_node {
	struct list_head head;
	struct mutex lock;
};

#define ADAPT_SENSOR_LIST_NUM 20
#define WIA_NUM 6
#define DATA_FLAG_LEN 4
#define DYLOAD_END_FLG 2

static struct device_manager_list_node g_device_manager_list;
static int g_max_dyn_file_id_num;
static int g_max_aux_file_id_num;
#define AUX_ELE_NUM 2

static struct sensor_redetect_state g_redetect_state;
static struct wakeup_source g_sensor_rd;
static struct work_struct g_redetect_work;

static char g_pkg_buf[MAX_PKT_LENGTH];
static char g_aux_buf[MAX_PKT_LENGTH];
/* dyload file load,  data structure is [file_cout, file_id1, file_id2...]*/
static pkt_sys_dynload_req_t *g_dyn_req = (pkt_sys_dynload_req_t *)g_pkg_buf;

/*
 * aux file load
 * data structure is [file_cout, file_id1, tag1, file_id2, tag2...]
 */
static pkt_sys_dynload_req_t *g_aux_req = (pkt_sys_dynload_req_t *)g_aux_buf;

/*
 * Function    : get_adapt_file_id_for_dyn_load
 * Description : Get adapt file id from dts,
 *               which not read from specific sensors dts.
 * Input       : none
 * Output      : none
 * Return      : 0 is ok, otherwise failed.
 */
static int get_adapt_file_id_for_dyn_load(void)
{
	u32 wia[ADAPT_SENSOR_LIST_NUM] = {0};
	struct property *prop = NULL;
	unsigned int i;
	unsigned int j;
	unsigned int len;
	struct device_node *sensorhub_node = NULL;
	const char *name = "adapt_file_id";

	sensorhub_node =
		of_find_compatible_node(NULL, NULL, "hisilicon,sensorhub");
	if (!sensorhub_node) {
		ctxhub_err("%s, can't find node sensorhub\n", __func__);
		return RET_FAIL;
	}

	prop = of_find_property(sensorhub_node, name, NULL);
	if (!prop) {
		ctxhub_err("%s! prop is NULL\n", __func__);
		return RET_FAIL;
	}
	if (!prop->value) {
		ctxhub_err("%s! prop->value is NULL\n", __func__);
		return RET_FAIL;
	}
	len = prop->length / sizeof(unsigned int);

	if (len > ADAPT_SENSOR_LIST_NUM) {
		ctxhub_err("%s, dts adapt file id num greater than %d\n", __func__,
			ADAPT_SENSOR_LIST_NUM);
		return RET_FAIL;
	}
	if (of_property_read_u32_array(sensorhub_node, name, wia, len)) {
		ctxhub_err("%s:read adapt_file_id from dts fail\n", name);
		return RET_FAIL;
	}
	// duplicate file id will not join
	for (i = 0; i < len; i++) {
		for (j = 0; j < g_dyn_req->file_count; j++) {
			if (g_dyn_req->file_list[j] == wia[i])
				break;
		}

		if (j == g_dyn_req->file_count) {
			g_dyn_req->file_list[g_dyn_req->file_count] = wia[i];
			g_dyn_req->file_count++;
		}
	}
	return RET_SUCC;
}

/*
 * Function    : send_dynload_fileid_to_mcu
 * Description : Packaged dynamic load pack and send to iomcu
 * Input       : [buf] buf of file id that send to iomcu
 *             : [len] buf len
 * Output      : none
 * Return      : 0 is ok, otherwise failed.
 */
static int send_dynload_fileid_to_mcu(const void *buf, int len)
{
	struct write_info pkg_ap;
	struct read_info pkg_mcu;
	int ret, iom3_power_state, iom3_state;

	(void)memset_s(&pkg_ap, sizeof(pkg_ap), 0, sizeof(pkg_ap));
	(void)memset_s(&pkg_mcu, sizeof(pkg_mcu), 0, sizeof(pkg_mcu));

	iom3_power_state = get_iomcu_power_state();
	iom3_state = get_iom3_state();

	pkg_ap.tag = TAG_SYS;
	pkg_ap.cmd = CMD_SYS_DYNLOAD_REQ;
	pkg_ap.wr_buf = buf;
	pkg_ap.wr_len = len;

	if (iom3_state == IOM3_ST_RECOVERY || iom3_power_state == ST_SLEEP)
		ret = write_customize_cmd(&pkg_ap, NULL, false);
	else
		ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);

	if (ret != 0) {
		ctxhub_err("send file id to mcu fail,ret=%d\n", ret);
		return RET_FAIL;
	}

	if (pkg_mcu.errno != 0) {
		ctxhub_err("file id set fail\n");
		return RET_FAIL;
	}

	return 0;
}

/*
 * Function    : send_fileid_to_mcu
 * Description : When mcu ready stage of boot, recovery or resume, called.
 *               Send normal file id first, then send aux file id.
 *               Use file flg to distinguish different file id send.
 *               0 is normal, 1 is aux, 2 is end.
 * Input       : none
 * Output      : none
 * Return      : 0 is ok, otherwise failed.
 */
int send_fileid_to_mcu(void)
{
	int i;
	pkt_sys_dynload_req_t dynload_req;

	if (g_dyn_req->file_count != 0) {
		ctxhub_info("sensorhub after check, get dynload file id number = %d, fild id",
			 (int)g_dyn_req->file_count);
		for (i = 0; i < g_dyn_req->file_count; i++)
			ctxhub_info("%d\n", (int)g_dyn_req->file_list[i]);
		g_dyn_req->file_flg = 0;
		if (send_dynload_fileid_to_mcu(&g_dyn_req->file_flg,
					g_dyn_req->file_count *
				sizeof(g_dyn_req->file_list[0]) +
				sizeof(g_dyn_req->file_flg) +
				sizeof(g_dyn_req->file_count)))
			ctxhub_err("%s send file_id to mcu failed\n", __func__);
	} else {
		ctxhub_warn("%s file_count = 0, not send file_id to mcu\n",
			__func__);
	}

	if (g_aux_req->file_count) {
		ctxhub_info("sensorhub after check, get aux file id number = %d, aux file id and tag ",
			 (int)g_aux_req->file_count);
		for (i = 0; i < g_aux_req->file_count; i++)
			ctxhub_info("--%d, %d",
				 (int)g_aux_req->file_list[AUX_ELE_NUM * i],
				(int)g_aux_req->file_list[AUX_ELE_NUM * i + 1]);

		g_aux_req->file_flg = 1;
		if (send_dynload_fileid_to_mcu(&g_aux_req->file_flg,
					g_aux_req->file_count *
				sizeof(g_aux_req->file_list[0]) * AUX_ELE_NUM +
				sizeof(g_aux_req->file_flg) +
				sizeof(g_aux_req->file_count)))
			ctxhub_err("%s send aux file_id to mcu failed\n",
				__func__);
	} else {
		ctxhub_warn("%s aux count=0,not send file_id to mcu\n", __func__);
	}
	(void)memset_s(&dynload_req, sizeof(pkt_sys_dynload_req_t), 0,
		sizeof(pkt_sys_dynload_req_t));
	dynload_req.file_flg = DYLOAD_END_FLG;

	return send_dynload_fileid_to_mcu(&dynload_req.file_flg,
			sizeof(pkt_sys_dynload_req_t) -
			sizeof(struct pkt_header));
}

/*
 * Function    : add_dynamic_file_id
 * Description : Add normal dynamic file id.
 *               Will check max file count.
 *               Internal function.
 * Input       : [file_id] normal file id
 * Output      : none
 * Return      : none
 */
static void add_dynamic_file_id(uint16_t file_id)
{
	if (g_dyn_req->file_count >= g_max_dyn_file_id_num) {
		ctxhub_err("[%s], dynamic file id array is full\n", __func__);
		return;
	}

	g_dyn_req->file_list[g_dyn_req->file_count] = file_id;
	g_dyn_req->file_count++;
}

/*
 * Function    : add_aux_file_id
 * Description : Add aux file id and tag.
 *               Will check max file count.
 *               Aux file id use for multiple device such as two acc,
 *               so need trans device tag to distinguish device.
 *               Internal function.
 * Input       : [tag] device tag
 *             : [file_id] aux file id
 * Output      : none
 * Return      : none
 */
static void add_aux_file_id(u16 tag, uint16_t file_id)
{
	if (g_aux_req->file_count >= g_max_aux_file_id_num) {
		ctxhub_err("[%s], aux file id array is full\n", __func__);
		return;
	}

	g_aux_req->file_list[g_aux_req->file_count * AUX_ELE_NUM] = file_id;
	g_aux_req->file_list[g_aux_req->file_count * AUX_ELE_NUM + 1] = tag;
	g_aux_req->file_count++;
}

/*
 * Function    : get_file_id_for_dyn_load
 * Description : get file id from device manager node
 *               which get file id that read from dts when detect stage
 *               Internal function.
 * Input       : none
 * Output      : none
 * Return      : 0 is ok, otherwise is error.always success
 */
static int get_file_id_for_dyn_load(void)
{
	struct device_manager_node *node = NULL;
	struct device_manager_node *n = NULL;

	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(node, n, &g_device_manager_list.head, entry) {
		if (node->dyn_file_id != INVALID_FILE_ID)
			add_dynamic_file_id(node->dyn_file_id);

		if (node->aux_file_id != INVALID_FILE_ID)
			add_aux_file_id((uint16_t)node->tag, node->aux_file_id);
	}
	mutex_unlock(&g_device_manager_list.lock);
	return RET_SUCC;
}

/*
 * Function    : get_adapt_id_and_send
 * Description : Get dyn file id and aux file id, then send to iomcu
 *               Internal function.
 * Input       : none
 * Output      : none
 * Return      : 0 is ok, otherwise is error
 */
static int get_adapt_id_and_send(void)
{
	int ret;

	ret = get_adapt_file_id_for_dyn_load();
	if (ret != RET_SUCC)
		ctxhub_err("get adapt file id for dyn load failed!\n");

	ret = get_file_id_for_dyn_load();
	if (ret != RET_SUCC)
		ctxhub_err("get file id for dyn load failed!\n");

	ctxhub_info("get file id number = %d\n", g_dyn_req->file_count);

	return send_fileid_to_mcu();
}

/*
 * Function    : get_device_manager_node
 * Description : According device name , get device_manager_node pointer
 *               Internal function.
 * Input       : [name_buf] device name
 *             : [len] device name len
 * Output      : none
 * Return      : if node exist, return node, otherwise return NULL
 */
static struct device_manager_node *get_device_manager_node(const char *name_buf,
							   int len)
{
	struct device_manager_node *node = NULL;
	struct device_manager_node *n = NULL;

	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(node, n, &g_device_manager_list.head, entry) {
		if (len == strlen(node->device_name_str) &&
			!strncmp(name_buf, node->device_name_str, len)) {
			goto out;
		}
	}
	// not exist
	node = NULL;
out:
	mutex_unlock(&g_device_manager_list.lock);
	return node;
}

/*
 * Function    : get_device_manager_node_by_tag
 * Description : according tag, get device_manager_node pointer
 * Input       : [tag] the key for device manager node
 * Output      : none
 * Return      : if node exist, return node, otherwise return NULL
 */
struct device_manager_node *get_device_manager_node_by_tag(int tag)
{
	struct device_manager_node *node = NULL;
	struct device_manager_node *n = NULL;

	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(node, n, &g_device_manager_list.head, entry) {
		if (node->tag == tag)
			goto out;
	}
	node = NULL;
out:
	mutex_unlock(&g_device_manager_list.lock);
	return node;
}

#ifdef CONFIG_HUAWEI_DSM

static void update_detectic_client_info(void)
{
	char device_name[DSM_MAX_IC_NAME_LEN] = { 0 };
	struct device_manager_node *pnode = NULL;
	struct device_manager_node *n = NULL;
	struct dsm_dev *dsm_sensorhub = get_shb_dsm_dev();
	int ret;

	if (!dsm_sensorhub) {
		ctxhub_err("[%s], get sensorhub dsm dev failed!\n", __func__);
		return;
	}

	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(pnode, n, &g_device_manager_list.head, entry) {
		if (pnode->detect_result == DET_FAIL) {
			ret = strcat_s(device_name, DSM_MAX_IC_NAME_LEN, pnode->device_name_str);
			if (ret != EOK) {
				mutex_unlock(&g_device_manager_list.lock);
				ctxhub_err("[%s], strcat failed!\n", __func__);
				return;
			}
		}
	}
	mutex_unlock(&g_device_manager_list.lock);

	device_name[DSM_MAX_IC_NAME_LEN - 1] = '\0';
	ctxhub_info("%s %s.\n", __func__, device_name);
	dsm_sensorhub->ic_name = device_name;
	if (dsm_update_client_vendor_info(dsm_sensorhub))
		ctxhub_info("dsm_update_client_vendor_info failed\n");
}

static void exception_report(enum detect_mode mode, char *detect_result, uint32_t len)
{
	struct dsm_client *shb_dclient = NULL;

	// param check outside
	if (strlen(detect_result) >= len)
		return;

	if (mode == BOOT_DETECT_END) {
		shb_dclient = get_shb_dclient();
		if (!shb_dclient) {
			ctxhub_err("[%s], get shb dsm client failed!\n",
				__func__);
			return;
		}
		if (!dsm_client_ocuppy(shb_dclient)) {
			update_detectic_client_info();
			dsm_client_record(shb_dclient, "[%s]%s",
					  __func__, detect_result);
			dsm_client_notify(shb_dclient,
					  DSM_SHB_ERR_IOM7_DETECT_FAIL);
		} else {
			ctxhub_info("%s:dsm_client_ocuppy fail\n",
				 __func__);
			dsm_client_unocuppy(shb_dclient);
			if (!dsm_client_ocuppy(shb_dclient)) {
				update_detectic_client_info();
				dsm_client_record(shb_dclient, "[%s]%s",
						  __func__, detect_result);
				dsm_client_notify(shb_dclient,
						  DSM_SHB_ERR_IOM7_DETECT_FAIL);
			}
		}
	}
}
#endif

/*
 * Function    : check_detect_result
 * Description : when detect finish, check detect result and generate error
 *               report which will report to dsm.
 *               And maintain some variable for later redetect.(If all device
 *               detected success, later redetect will not happen)
 * Input       : [mode] which stage for detect
 * Output      : none
 * Return      : detect failed device num
 */
static uint32_t check_detect_result(enum detect_mode mode)
{
	u32 detect_fail_num = 0;
	struct device_manager_node *pnode = NULL;
	struct device_manager_node *n = NULL;
	char detect_result[MAX_STR_SIZE] = {0};
	const char *detect_failed = "detect fail!";
	int ret;


	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(pnode, n, &g_device_manager_list.head, entry) {
		if (pnode->detect_result == DET_FAIL) {
			detect_fail_num++;
			ret = strcat_s(detect_result, MAX_STR_SIZE,
				       pnode->device_name_str);
			if (ret != EOK)
				ctxhub_err("[%s], strcat failed!\n", __func__);

			ret = strcat_s(detect_result, MAX_STR_SIZE, " ");
			if (ret != EOK)
				ctxhub_err("[%s], strcat failed!\n", __func__);

			ctxhub_info("%s: %s detect fail\n", __func__,
				 pnode->device_name_str);
		} else if (pnode->detect_result == DET_SUCC) {
			ctxhub_info("%s: %s detect success\n", __func__,
				 pnode->device_name_str);
		}
	}
	mutex_unlock(&g_device_manager_list.lock);

	if (detect_fail_num > 0) {
		g_redetect_state.need_redetect_sensor = 1;
		ret = strcat_s(detect_result, MAX_STR_SIZE, detect_failed);
		if (ret != EOK) {
			ctxhub_err("[%s], strcat failed!\n", __func__);
			goto out;
		}
#ifdef CONFIG_HUAWEI_DSM
		exception_report(mode, detect_result, MAX_STR_SIZE);
#endif
	} else {
		g_redetect_state.need_redetect_sensor = 0;
	}

out:
	if ((detect_fail_num < g_redetect_state.detect_fail_num) &&
	    (mode == REDETECT_LATER)) {
		g_redetect_state.need_recovery = 1;
		ctxhub_info("%s : %u sensors detect success after redetect\n",
			 __func__, g_redetect_state.detect_fail_num -
			detect_fail_num);
	}
	g_redetect_state.detect_fail_num = detect_fail_num;
	return detect_fail_num;
}

static void device_detect_process(struct device_node *dn,
				  struct device_manager_node *node)
{
	int ret;

	if (node->detect_result != DET_SUCC) {
		if (!node->detect) {
			ctxhub_err("%s, for device %s, detect function is null\n",
				__func__, node->device_name_str);
			return;
		}

		ret = node->detect(dn);
		if (ret != RET_SUCC) {
			ctxhub_err("%s : device %s is detect failed\n", __func__,
				node->device_name_str);
				node->detect_result = DET_FAIL;
			return;
		}
		ctxhub_info("[%s], %s detect succ!\n", __func__,
			 node->device_name_str);
		node->detect_result = DET_SUCC;
	}
}

/*
 * Function    : device_detect
 * Description : Internal function.Read param from dts and according to
 *               sensor_type to find device_manager_node.Call the detect func
 *               that device imply.
 * Input       : none
 * Output      : none
 * Return      : none
 */
static void device_detect(void)
{
	struct device_node *dn = NULL;
	char *sensor_ty = NULL;
	char *sensor_st = NULL;
	const char *st = "disabled";
	struct device_manager_node *node = NULL;
	int ret;

	for_each_node_with_property(dn, "sensor_type") {
		/* sensor type */
		ret = of_property_read_string(dn, "sensor_type",
					      (const char **)&sensor_ty);
		if (ret != 0) {
			ctxhub_err("get sensor type fail ret=%d\n", ret);
			continue;
		}

		node = get_device_manager_node(sensor_ty, strlen(sensor_ty));
		if (!node) {
			ctxhub_warn("For %s, not regitser callback for detect\n",
				sensor_ty);
			continue;
		}

		/* sensor status:ok or disabled */
		ret = of_property_read_string(dn, "status",
					      (const char **)&sensor_st);
		if (ret != 0) {
			ctxhub_err("get sensor %s status fail ret=%d\n", sensor_ty, ret);
			continue;
		}

		ret = strcmp(st, sensor_st);
		if (ret == 0) {
			ctxhub_info("%s : sensor %s status is %s\n", __func__,
				 sensor_ty, sensor_st);
			continue;
		}

		device_detect_process(dn, node);
	}
}

static void redetect_failed_devices(enum detect_mode mode)
{
	device_detect();
	check_detect_result(mode);
}

/*
 * Function    : device_detect_exception_process
 * Description : detect failed process, will try twice redetect.
 *               If redetect also fail, will try to redetec when screen off.
 * Input       : [failed_num] detect failed num
 * Output      : none
 * Return      : none
 */
static void device_detect_exception_process(uint32_t failed_num)
{
	int i;

	if (failed_num > 0) {
		for (i = 0; i < SENSOR_DETECT_RETRY; i++) {
			ctxhub_info("%s : %d redect device after detect all device, fail device num  %u\n",
				 __func__,
				i, g_redetect_state.detect_fail_num);
			if (g_redetect_state.detect_fail_num > 0)
				redetect_failed_devices(DETECT_RETRY + i);
		}
	}
}

/*
 * Function    : init_devices_cfg_data_from_dts
 * Description : When boot in mini ready stage will call this func.
 *               Device detect and send filed id.
 * Input       : none
 * Output      : none
 * Return      : none
 */
static int init_devices_cfg_data_from_dts(void)
{
	u32 detect_failed_num;

	device_detect();

	detect_failed_num = check_detect_result(BOOT_DETECT);

	/*
	 * when iomcu boot, will try to redetect twice if had detect-failed
	 * device
	 */
	device_detect_exception_process(detect_failed_num);

	/* get file id and send */
	if (get_adapt_id_and_send())
		return -EINVAL;

	return RET_SUCC;
}

/*
 * Function    : device_set_cfg_data
 * Description : when boot on mcu ready stage will be called.
 *               send device cfg data to iomcu, complete device init
 * Input       : none
 * Output      : none
 * Return      : none
 */
void device_set_cfg_data(void)
{
	int ret;
	struct device_manager_node *pnode = NULL;
	struct device_manager_node *n = NULL;

	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(pnode, n, &g_device_manager_list.head, entry) {
		if (pnode->cfg) {
			ret = pnode->cfg();
			if (ret != RET_SUCC) {
				ctxhub_err("%s: for device %s, config failed\n",
					__func__, pnode->device_name_str);
				continue;
			}
		} else {
			ctxhub_info("%s: for device %s, config func is null\n",
				 __func__, pnode->device_name_str);
		}
	}
	mutex_unlock(&g_device_manager_list.lock);

}

static void redetect_device_work_handler(struct work_struct *wk)
{
	__pm_stay_awake(&g_sensor_rd);
	redetect_failed_devices(REDETECT_LATER);

	if (g_redetect_state.need_recovery == 1) {
		g_redetect_state.need_recovery = 0;
		ctxhub_info("%s: some sensor detect success after %u redetect, begin recovery\n",
			 __func__,
			(uint32_t)g_redetect_state.redetect_num);
		/* this interface always return 0 */
		iom3_need_recovery(SENSORHUB_USER_MODID, SH_FAULT_REDETECT);
	} else {
		ctxhub_info("%s: no sensor redetect success\n", __func__);
	}
	__pm_relax(&g_sensor_rd);
}

/*
 * Function    : register_contexthub_device
 * Description : register contexthub device, add node in list
 *               this func only can be called in device_initcall_sync,
 *               otherwise maybe have problem
 * Input       : [node] device_manager_node
 * Output      : none
 * Return      : 0 OK, other error
 */
int register_contexthub_device(struct device_manager_node *node)
{
	struct device_manager_node *pnode = NULL;
	struct device_manager_node *n = NULL;
	int ret = RET_SUCC;

	if (!node || !node->device_name_str) {
		ctxhub_err("input node or device name is null\n");
		return -EINVAL;
	}

	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(pnode, n, &g_device_manager_list.head, entry) {
		if (pnode->tag == node->tag) {
			ctxhub_warn("tag = %d has already registered in %s\n",
				 node->tag, __func__);
			ret = RET_SUCC;
			goto out;
		}
	}

	list_add(&node->entry, &g_device_manager_list.head);
out:
	mutex_unlock(&g_device_manager_list.lock);
	return ret;
}

/*
 * Function    : unregister_contexthub_device
 * Description : unregister contexthub device, delete node from list
 *               this func only can be called in device_initcall_sync,
 *               otherwise maybe have problem
 * Input       : [node] device_manager_node
 * Output      : none
 * Return      : 0 OK, other error
 */
int unregister_contexthub_device(struct device_manager_node *node)
{
	struct device_manager_node *pnode = NULL;
	struct device_manager_node *n = NULL;

	if (!node) {
		ctxhub_warn("input node is null\n");
		return -EINVAL;
	}

	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(pnode, n, &g_device_manager_list.head, entry) {
		if (pnode->tag == node->tag) {
			list_del(&pnode->entry);
			break;
		}
	}
	mutex_unlock(&g_device_manager_list.lock);
	return 0;
}

/*
 * Function    : device_manager_node_alloc
 * Description : alloc device manager node and init
 * Input       : none
 * Output      : none
 * Return      : pointer of alloc space
 */
struct device_manager_node *device_manager_node_alloc()
{
	struct device_manager_node *device = kzalloc(sizeof(struct device_manager_node), GFP_ATOMIC);

	if (!device)
		return NULL;

	device->detect_result = DET_INIT;
	device->tag = INVALID_VALUE;
	device->sensorlist_info.version = INVALID_VALUE;
	device->sensorlist_info.max_range = INVALID_VALUE;
	device->sensorlist_info.resolution = INVALID_VALUE;
	device->sensorlist_info.power = INVALID_VALUE;
	device->sensorlist_info.min_delay = INVALID_VALUE;
	device->sensorlist_info.max_delay = INVALID_VALUE;
	device->sensorlist_info.fifo_reserved_event_count = INVALID_VALUE;
	device->sensorlist_info.fifo_max_event_count = INVALID_VALUE;
	device->sensorlist_info.flags = INVALID_VALUE;
	device->dyn_file_id = INVALID_FILE_ID;
	device->aux_file_id = INVALID_FILE_ID;
	return device;
}

/*
 * Function    : device_manager_node_free
 * Description : free device_manager_node
 * Input       : none
 * Output      : none
 * Return      : none
 */
void device_manager_node_free(struct device_manager_node *device)
{
	kfree(device);
}


/*
 * Function    : reset_calibrate_data_to_mcu
 * Description : reset all device calibrate data to mcu
 *               use no lock ipc send.Always called when recovery
 * Input       : none
 * Output      : none
 * Return      : none
 */
void reset_calibrate_data_to_mcu(void)
{
	struct device_manager_node *pnode = NULL;
	struct device_manager_node *n = NULL;
	int ret;

	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(pnode, n, &g_device_manager_list.head, entry) {
		if (pnode->send_calibrate_data) {
			ret = pnode->send_calibrate_data(false);
			if (ret != RET_SUCC)
				ctxhub_err("for device tag %d, send calibrate data func return failed!\n",
					pnode->tag);
		}
	}
	mutex_unlock(&g_device_manager_list.lock);
}

/*
 * Function    : send_calibrate_data_to_iomcu
 * Description : send calibrate data to iomcu.
 *               when iomcu normal, use lock
 *               when iomcu recovery, use no lock
 * Input       : [tag] sensor tag
 *             : [is_lock] use lock or not
 * Output      : none
 * Return      : none
 */
void send_calibrate_data_to_iomcu(int tag, bool is_lock)
{
	struct device_manager_node *pnode = NULL;
	struct device_manager_node *n = NULL;
	int ret;

	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(pnode, n, &g_device_manager_list.head, entry) {
		if (pnode->tag == tag && pnode->send_calibrate_data) {
			ret = pnode->send_calibrate_data(is_lock);
			if (ret != RET_SUCC)
				ctxhub_err("for device tag %d, send calibrate data func return failed!\n",
					tag);
			break;
		}
	}
	mutex_unlock(&g_device_manager_list.lock);
}

static void show_last_detect_fail_sensor(void)
{
	struct device_manager_node *pnode = NULL;
	struct device_manager_node *n = NULL;

	mutex_lock(&g_device_manager_list.lock);
	list_for_each_entry_safe(pnode, n, &g_device_manager_list.head, entry) {
		if (pnode) {
			if (pnode->detect_result == DET_FAIL)
				pr_err("last detect fail device: %s\n",
				       pnode->device_name_str);
		}
	}
	mutex_unlock(&g_device_manager_list.lock);
}

/*
 * Function    : device_redetect_enter
 * Description : when screen off, will be called.
 *               Called by pm currently.
 *               If having detect-failed sensor, will redetect
 * Input       : none
 * Output      : none
 * Return      : none
 */
void device_redetect_enter(void)
{
	int iom3_state = get_iom3_state();

	if (iom3_state == IOM3_ST_NORMAL) {
		if (g_redetect_state.need_redetect_sensor == 1) {
			if (g_redetect_state.redetect_num < MAX_REDETECT_NUM) {
				queue_work(system_power_efficient_wq,
					   &g_redetect_work);
				g_redetect_state.redetect_num++;
			} else {
				ctxhub_info("%s: some sensors detect fail, but the max redetect num is over flow\n",
					 __func__);
				show_last_detect_fail_sensor();
			}
		}
	}
}

/*
 * Function    : mcu_sys_ready_callback
 * Description : when sensorhub boot, ap process
 * Input       : [head] packet from sensorhub
 * Output      : none
 * Return      : always 0
 */
static int mcu_sys_ready_callback(const struct pkt_header *head)
{
	int ret;

	if (((pkt_sys_statuschange_req_t *)head)->status == ST_MINSYSREADY) {
		ret = sensor_power_check();
		if (ret != RET_SUCC)
			ctxhub_err("[%s], sensor power check failed ,ret=%d\n",
				__func__, ret);
		/* mini ready, will detect device and send file id */
		ret = init_devices_cfg_data_from_dts();
		if (ret != RET_SUCC)
			ctxhub_err("get sensors cfg data from dts fail,ret=%d, use default config data!\n",
				ret);
		else
			ctxhub_info("get sensors cfg data from dts success!\n");
	} else if (((pkt_sys_statuschange_req_t *)head)->status == ST_MCUREADY) {
		ctxhub_info("mcu all ready!\n");
		/* mcu ready, send cfg data to mcu for device init */
		device_set_cfg_data();
		unregister_mcu_event_notifier(TAG_SYS,
					      CMD_SYS_STATUSCHANGE_REQ,
					      mcu_sys_ready_callback);
		atomic_set(&iom3_rec_state, IOM3_RECOVERY_IDLE);
	} else {
		ctxhub_info("other status\n");
	}
	return 0;
}

/*
 * Function    : iom3_rec_sys_callback
 * Description : when sensorhub recovery, ap process
 * Input       : [head] packet from sensorhub
 * Output      : none
 * Return      : always 0
 */
int iom3_rec_sys_callback(const struct pkt_header *head)
{
	int ret;

	iomcu_minready_done();
	if (atomic_read(&iom3_rec_state) == IOM3_RECOVERY_MINISYS) {
		if (((pkt_sys_statuschange_req_t *)head)->status ==
			ST_MINSYSREADY) {
			ctxhub_info("REC sys ready mini!\n");
			/* mini ready, need send file id for dynamic load */
			ret = send_fileid_to_mcu();
			if (ret)
				ctxhub_err("REC get sensors cfg data from dts fail,ret=%d, use default config data!\n",
					ret);
			else
				ctxhub_info("REC get sensors cfg data from dts success!\n");
		} else if (ST_MCUREADY ==
			((pkt_sys_statuschange_req_t *)head)->status) {
			ctxhub_info("REC mcu all ready!\n");
			/* mcu ready, send cfg data to mcu for device init */
			device_set_cfg_data();

			/* notify dump that device cfg data had send */
			notify_recovery_done();
		}
	}

	return 0;
}

void device_detect_init(void)
{
	int ret;

	ret = register_mcu_event_notifier(TAG_SYS, CMD_SYS_STATUSCHANGE_REQ,
					  mcu_sys_ready_callback);
	if (ret != 0)
		ctxhub_err("[%s], register mcu sys callback failed\n", __func__);

	ret = register_mcu_event_notifier(TAG_SYS, CMD_SYS_STATUSCHANGE_REQ,
					  iom3_rec_sys_callback);
	if (ret != 0)
		ctxhub_err("[%s], register rec sys callback failed\n", __func__);

	(void)memset_s(&g_redetect_state, sizeof(g_redetect_state), 0,
		sizeof(g_redetect_state));
	INIT_WORK(&g_redetect_work, redetect_device_work_handler);
}

static int contexthub_device_mgr_init(void)
{
	if (get_contexthub_dts_status())
		return -EINVAL;
	INIT_LIST_HEAD(&g_device_manager_list.head);
	mutex_init(&g_device_manager_list.lock);

	/* init dyn req param */
	g_dyn_req->file_count = 0;
	g_max_dyn_file_id_num =
		(MAX_PKT_LENGTH -
		offsetof(pkt_sys_dynload_req_t, file_list)) / sizeof(uint16_t);
	if (g_max_dyn_file_id_num < 0)
		g_max_dyn_file_id_num = 0;

	/* init aux req param */
	g_aux_req->file_count = 0;
	g_max_aux_file_id_num =
		(MAX_PKT_LENGTH -
		offsetof(pkt_sys_dynload_req_t, file_list)) / sizeof(uint16_t) / AUX_ELE_NUM;
	if (g_max_aux_file_id_num < 0)
		g_max_aux_file_id_num = 0;

	wakeup_source_init(&g_sensor_rd, "sensorhub_redetect");
	return 0;
}

static void __exit contexthub_device_mgr_exit(void)
{
	wakeup_source_trash(&g_sensor_rd);
}

device_initcall(contexthub_device_mgr_init);
module_exit(contexthub_device_mgr_exit);

MODULE_AUTHOR("Smart");
MODULE_DESCRIPTION("device mgr driver");
MODULE_LICENSE("GPL");
