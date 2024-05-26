/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: ar adapt implement.
 * Create: 2020/03/05
 */

#include "ar.h"
#include <securec.h>

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>

#include "iomcu_log.h"
#include "device_interface.h"
#include "iomcu_route.h"
#include "device_manager.h"
#include "platform_include/smart/linux/iomcu_dump.h"
#include "common/common.h"
#include "sensor_channel.h"
#include "inputhub_wrapper/inputhub_wrapper.h"
#include "device_common.h"


#define CARE_ENTER 1
#define CARE_ENTER_AND_EXIT 3
#define WAKEUP 0
#define NON_WAKEUP 1
#define ENTER_VALUE 1
#define DEFAULT_REPORT_LATENCY 0
#define SUPPORT_TIMEOUT true
#define NOT_SUPPORT_TIMEOUT false
static struct customized_interface *g_ar_interface;
static DEFINE_MUTEX(g_mutex_ar_status);

struct sensorhub_type_map_ar_type {
	int ar_shb_type;
	int ar_type;
};

static struct sensorhub_type_map_ar_type g_shb2ar_map[] = {
	{SENSORHUB_TYPE_AR_MOTION, AR_MOTION},
	{SENSORHUB_TYPE_AR_WALKING, AR_WALKING},
	{SENSORHUB_TYPE_AR_RUNNING, AR_RUNNING},
	{SENSORHUB_TYPE_AR_WRIST_DOWN, AR_WRIST_DOWN},
	{SENSORHUB_TYPE_AR_STATIONARY, AR_STATIONARY},
	{SENSORHUB_TYPE_AR_OFFBODY, AR_OFFBODY}
};

static int g_enable_cnt;
static struct ar_status g_ar_status[] = {
	{SENSORHUB_TYPE_AR_MOTION, NOT_SUPPORT_TIMEOUT, false, DEFAULT_REPORT_LATENCY, DEFAULT_REPORT_LATENCY,
	 WAKEUP, CARE_ENTER, SENSOR_FLAG_ONE_SHOT_MODE + SENSOR_WAKEUP_FLAG, "ar_motion"},
	{SENSORHUB_TYPE_AR_WALKING, SUPPORT_TIMEOUT, false, DEFAULT_REPORT_LATENCY, DEFAULT_REPORT_LATENCY,
	 WAKEUP, CARE_ENTER_AND_EXIT, SENSOR_FLAG_ON_CHANGE_MODE + SENSOR_WAKEUP_FLAG, "ar_walking"},
	{SENSORHUB_TYPE_AR_RUNNING, SUPPORT_TIMEOUT, false, DEFAULT_REPORT_LATENCY, DEFAULT_REPORT_LATENCY,
	 WAKEUP, CARE_ENTER_AND_EXIT, SENSOR_FLAG_ON_CHANGE_MODE + SENSOR_WAKEUP_FLAG, "ar_running"},
	{SENSORHUB_TYPE_AR_WRIST_DOWN, NOT_SUPPORT_TIMEOUT, false, DEFAULT_REPORT_LATENCY, DEFAULT_REPORT_LATENCY,
	 NON_WAKEUP, CARE_ENTER, SENSOR_FLAG_ONE_SHOT_MODE, "ar_wrist_down"},
	{SENSORHUB_TYPE_AR_STATIONARY, NOT_SUPPORT_TIMEOUT, false, DEFAULT_REPORT_LATENCY, DEFAULT_REPORT_LATENCY,
	 WAKEUP, CARE_ENTER, SENSOR_FLAG_ONE_SHOT_MODE + SENSOR_WAKEUP_FLAG, "ar_stationary"},
	{SENSORHUB_TYPE_AR_OFFBODY, NOT_SUPPORT_TIMEOUT, false, DEFAULT_REPORT_LATENCY, DEFAULT_REPORT_LATENCY,
	 NON_WAKEUP, CARE_ENTER_AND_EXIT, SENSOR_FLAG_SPECIAL_MODE, "ar_offbody"},
};

static int check_param(int shb_type)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_shb2ar_map); i++) {
		if (g_shb2ar_map[i].ar_shb_type == shb_type)
			return RET_SUCC;
	}
	return RET_FAIL;
}

struct ar_status *get_ar_status(int shb_type)
{
	int ret;
	u32 i;

	ret = check_param(shb_type);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, check param failed!input type is %d\n",
			   __func__, shb_type);
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(g_ar_status); i++) {
		if (g_ar_status[i].ar_shb_type == shb_type)
			return &g_ar_status[i];
	}
	return NULL;
}

struct sensorlist_info *get_ar_sensor_list_info(int shb_type)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_ar_status); i++) {
		if (g_ar_status[i].ar_shb_type == shb_type)
			return &g_ar_status[i].sensor_info;
	}
	ctxhub_warn("%s, %d is not ar type\n", __func__, shb_type);
	return NULL;
}

static uint8_t get_ar_type(int shb_type)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_shb2ar_map); i++) {
		if (g_shb2ar_map[i].ar_shb_type == shb_type)
			return (uint8_t)g_shb2ar_map[i].ar_type;
	}
	ctxhub_warn("%s, %d is not ar type\n", __func__, shb_type);
	return 0xFF;
}

static int get_ar_shb_type(int ar_type)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_shb2ar_map); i++) {
		if (g_shb2ar_map[i].ar_type == ar_type)
			return g_shb2ar_map[i].ar_shb_type;
	}
	return RET_FAIL;
}

static void update_ar_status(bool enable, int shb_type)
{
	u32 i;
	int delta = enable ? 1 : -1;

	for (i = 0; i < ARRAY_SIZE(g_ar_status); i++) {
		if (g_ar_status[i].ar_shb_type == shb_type) {
			g_enable_cnt += delta;
			g_ar_status[i].enable = enable;
			break;
		}
	}
}

#define AR_TIMER 40
/*
 * Function    : ar_enable_internal
 * Description : Enable ar activity.If no activity enabled before, you need open TAG_AR APP,
 *             : set interval (sensorhub will start a timer) and add cared ar activty(send config packet).
 *             : When you send the packet of cared ar activity, your ar activity don't have
 *             : report latency, now we use default latency, which can config in dts of product.
 * Input       : [shb_type] sensorhub type of ar
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
static int ar_enable_internal(int shb_type)
{
	int ret;
	struct ar_config_packet pack;
	interval_param_t interval_param;
	struct ar_status *p_ar_status = NULL;

	ctxhub_info("%s, enable %d enter!current enable cnt is %d\n",
		__func__, shb_type, g_enable_cnt);

	mutex_lock(&g_mutex_ar_status);
	if (g_enable_cnt == 0) {
		ret = inputhub_device_enable(TAG_AR, true);
		if (ret != RET_SUCC) {
			mutex_unlock(&g_mutex_ar_status);
			ctxhub_err("%s, enable failed!\n", __func__);
			return ret;
		}

		interval_param.period = AR_TIMER;
		ret = inputhub_device_setdelay(TAG_AR, &interval_param);
		if (ret != RET_SUCC) {
			mutex_unlock(&g_mutex_ar_status);
			ctxhub_err("%s, set interval failed!\n", __func__);
			return ret;
		}
	}

	p_ar_status = get_ar_status(shb_type);
	pack.sub_cmd = SUB_CMD_SET_PARAMET_REQ;
	pack.config_data.core = 0; // 0 is core ap
	pack.config_data.oper = AR_ADD_EVENT;
	pack.config_data.para.activity = get_ar_type(shb_type);
	pack.config_data.para.event_type = p_ar_status->care_event;
	pack.config_data.para.non_wakeup = p_ar_status->non_wakeup;
	pack.config_data.para.report_latency = p_ar_status->default_report_latency;
	ret = inputhub_wrapper_send_cmd(TAG_AR, CMD_CMN_CONFIG_REQ, (const void *)&pack,
					sizeof(pack), NULL);
	if (ret != RET_SUCC) {
		mutex_unlock(&g_mutex_ar_status);
		ctxhub_err("%s, config cmd send failed!ret is %d\n", __func__, ret);
		return RET_FAIL;
	}

	// only send succ will save status
	update_ar_status(true, shb_type);
	p_ar_status->report_latency = pack.config_data.para.report_latency;
	mutex_unlock(&g_mutex_ar_status);
	return RET_SUCC;
}

/*
 * Function    : ar_disable_internal
 * Description : Disable ar activity.When func come in here, the status of ar activity must be active now(status
 *             : has been checked by caller).
 *             : After close this activity success, if cnt of enabled ar activity become 0,
 *             : you need close TAG_AR APP.
 * Input       : [shb_type] sensorhub type of ar
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
static int ar_disable_internal(int shb_type)
{
	int ret;
	struct ar_config_packet pack;
	struct ar_status *p_ar_status = NULL;

	ctxhub_info("%s, disable %d enter!\n", __func__, shb_type);

	p_ar_status = get_ar_status(shb_type);
	pack.sub_cmd = SUB_CMD_SET_PARAMET_REQ;
	pack.config_data.core = 0; // 0 is core ap
	pack.config_data.oper = AR_DEL_EVENT;
	pack.config_data.para.activity = get_ar_type(shb_type);
	pack.config_data.para.event_type = p_ar_status->care_event;
	ret = inputhub_wrapper_send_cmd(TAG_AR, CMD_CMN_CONFIG_REQ, (const void *)&pack,
					sizeof(pack), NULL);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, config cmd send failed!ret is %d\n", __func__, ret);
		return RET_FAIL;
	}
	ctxhub_info("%s, current enable cnt %d\n", __func__, g_enable_cnt);
	mutex_lock(&g_mutex_ar_status);
	p_ar_status->report_latency = 0;
	// disable succ will change status
	update_ar_status(false, shb_type);
	// if all ar event close, close ar app
	if (g_enable_cnt == 0) {
		ret = inputhub_device_enable(TAG_AR, false);
		if (ret != RET_SUCC) {
			mutex_unlock(&g_mutex_ar_status);
			ctxhub_err("%s, disable failed!\n", __func__);
			return ret;
		}
	}
	mutex_unlock(&g_mutex_ar_status);
	return RET_SUCC;
}

static bool ar_switch_sensor(int shb_type, bool enable)
{
	u32 i;
	int ret;
	bool result = false;

	ret = check_param(shb_type);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, invalid shb type %d\n", __func__, shb_type);
		return false;
	}

	/* if request enable is different with current enable status, return true.
	 * return false otherwise.
	 */
	mutex_lock(&g_mutex_ar_status);
	for (i = 0; i < ARRAY_SIZE(g_ar_status); i++) {
		if (g_ar_status[i].ar_shb_type == shb_type) {
			result = (g_ar_status[i].enable != enable);
			break;
		}
	}
	mutex_unlock(&g_mutex_ar_status);
	return result;
}

/*
 * Function    : ar_enable
 * Description : enable ar activity.
 * Input       : [shb_type] sensorhub type of ar
 *             : [enable]] enable or disable
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
static int ar_enable(int shb_type, bool enable)
{
	int ret;

	ret = check_param(shb_type);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, invalid shb type %d\n", __func__, shb_type);
		return RET_FAIL;
	}

	if (ar_switch_sensor(shb_type, enable)) {
		if (enable)
			return ar_enable_internal(shb_type);
		else
			return ar_disable_internal(shb_type);
	}
	return RET_SUCC;
}

static int report_flush_event(struct inputhub_route_table *route_item, int shb_type)
{
	struct sensor_data event;
	int ret;

	event.type = SENSORHUB_TYPE_META_DATA;
	event.length = sizeof(int);
	event.value[0] = shb_type;
	ret = inputhub_route_write_batch(route_item, (char *)&event,
					 event.length + offsetofend(struct sensor_data, length), 0);
	if (ret <= 0) {
		ctxhub_err("%s, write flush event failed!\n", __func__);
		return RET_FAIL;
	}
	return RET_SUCC;
}

static check_ar_head(const struct pkt_header *head)
{
	// have check null before
	s32 pkt_len = head->length;
	s32 expect_len = 0;
	struct ar_report_data *p = (struct ar_report_data*)head;

	/*
	 * ar_report_data is {
	 *    struct pkt_header hd;
	 *    uint16_t event_flag;
	 *    uint16_t num;
	 *    struct ar_report_state ar_data[];
	 * }
	 * hd.length must greater than sizeof(event_flag) + sizeof(num) at least, which record as x.
	 * Then, recordding num * sizeof(struct ar_report_state) as y.
	 * Last, x + y need equal hd.length.
	 * Because of packking 1 byte, it will not have align problem.
	 */
	expect_len += offsetofend(struct ar_report_data, num) - offsetofend(struct ar_report_data, hd);
	if (pkt_len < expect_len) {
		ctxhub_err("%s, head len %d is too short\n", __func__, pkt_len);
		return RET_FAIL;
	}

	expect_len += (s32)p->num * sizeof(struct ar_report_state);
	if (expect_len != pkt_len) {
		ctxhub_err("%s, head len %d is not equal %d\n", __func__, pkt_len, expect_len);
		return RET_FAIL;
	}
	return RET_SUCC;
}

/*
 * Function    : ar_report_event
 * Description : report ar event
 * Input       : [route_item] event will write to route item
 *             : [head] report packet from sensorhub
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int ar_report_event(struct inputhub_route_table *route_item, const struct pkt_header *head)
{
	struct ar_report_data *p_head = (struct ar_report_data *)head;
	struct sensor_data event;
	u32 i;
	int ret;
	int type = RET_FAIL;

	if (!route_item || !head) {
		ctxhub_err("%s, input is null!\n", __func__);
		return RET_FAIL;
	}

	if (check_ar_head(head) != RET_SUCC) {
		ctxhub_err("%s, report head error\n", __func__);
		return RET_FAIL;
	}

	for (i = 0; i < p_head->num; i++) {
		type = get_ar_shb_type(p_head->ar_data[i].activity);
		if (type == RET_FAIL) {
			ctxhub_warn("%s, %d is not ar type\n", __func__, p_head->ar_data[i].activity);
			continue;
		}
		event.type = type;
		event.value[0] = (p_head->ar_data[i].event_type == ENTER_VALUE) ? 1 : 0;
		if (type == SENSORHUB_TYPE_AR_OFFBODY)
			/*
			 * for offbody, sensorhub give 1, is enter offbody, else is exit offbody
			 * for android standard 0.0 is off-body, 1.0 is on-body
			 * so need special process
			 */
			event.value[0] = (p_head->ar_data[i].event_type == ENTER_VALUE) ? 0 : 1;
		event.length = sizeof(float);

		sensor_get_data(&event);

		ret = inputhub_route_write_batch(route_item, (char *)&event,
						 event.length + offsetofend(struct sensor_data, length),
						 p_head->ar_data[i].timestamp);
		if (ret <= 0)
			ctxhub_err("%s, ar event write failed!\n", __func__);
	}

	if (p_head->num != 0 && (p_head->event_flag & FLUSH_END) && type != RET_FAIL) {
		/*
		 * For flush, sensorhub will only return one event type.
		 * If sensorhub send multiple event, behavior of flush will error.
		 */
		ret = report_flush_event(route_item, event.type);
		if (ret != RET_SUCC) {
			ctxhub_err("%s, report flush event failed\n", __func__);
			return RET_FAIL;
		}
	}
	return RET_SUCC;
}

/*
 * Function    : ar_setdelay
 * Description : Set report latency for ar activity, period for ar has no effect, some
 *             : of ar activity will use timeout(which can config in dts of product).If
 *             : the activity don't support timeout, will use default latency.If support timeout
 *             : and is_batch, timeout will be used.
 * Input       : [shb_type] sensorhub type of ar
 *             : [period] reserved, ar setdelay not use
 *             : [timeout] max report latency, not all ar activity support.
 *             : [is_batch] true, timeout is valid; false, timeout is invalid.
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
static int ar_setdelay(int shb_type, int period, int timeout, bool is_batch)
{
	int ret;
	struct ar_config_packet pack;
	struct ar_status *p_ar_status = NULL;

	ctxhub_info("%s, ar %d set delay %d timeout %d is_batch %d enter!\n",
		    __func__, shb_type, period, timeout, is_batch);
	ret = check_param(shb_type);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, invalid shb type %d\n", __func__, shb_type);
		return RET_FAIL;
	}

	p_ar_status = get_ar_status(shb_type);
	pack.sub_cmd = SUB_CMD_SET_PARAMET_REQ;
	pack.config_data.core = 0; // 0 is core ap
	pack.config_data.oper = AR_ADD_EVENT;
	pack.config_data.para.activity = get_ar_type(shb_type);
	pack.config_data.para.event_type = p_ar_status->care_event;
	// if support use timeout which passed in, else use default
	if (p_ar_status->support_timeout && is_batch)
		pack.config_data.para.report_latency = timeout;
	else
		pack.config_data.para.report_latency = p_ar_status->default_report_latency;
	pack.config_data.para.non_wakeup = p_ar_status->non_wakeup;

	ret = inputhub_wrapper_send_cmd(TAG_AR, CMD_CMN_CONFIG_REQ, (const void *)&pack,
					sizeof(pack), NULL);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, config cmd send failed!ret is %d\n", __func__, ret);
		return RET_FAIL;
	}
	mutex_lock(&g_mutex_ar_status);
	p_ar_status->report_latency = pack.config_data.para.report_latency;
	mutex_unlock(&g_mutex_ar_status);
	ctxhub_info("%s, set delay %d succ!\n", __func__, shb_type);
	return RET_SUCC;
}

/*
 * Function    : ar_flush
 * Description : Flush specified ar activity, for disabled ar activity, flush cmd will
 *             : be intercepted by param check in sensor hal.
 * Input       : [shb_type] sensorhub type of ar
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int ar_flush(int shb_type)
{
	struct ar_flush_data flush;
	int ret;

	ret = check_param(shb_type);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, invalid shb type %d\n", __func__, shb_type);
		return RET_FAIL;
	}

	flush.subcmd = SUB_CMD_FLUSH_REQ;
	flush.flush_data.core = 0; // 0 is ap
	flush.flush_data.activity = get_ar_type(shb_type);

	ctxhub_info("flush sensor %u\n", flush.flush_data.activity);
	ret = inputhub_wrapper_send_cmd(TAG_AR, CMD_CMN_CONFIG_REQ, (const void *)&flush,
					sizeof(flush), NULL);
	if (ret != 0) {
		ctxhub_err("%s, ar flush failed!ret is %d\n", __func__, ret);
		return RET_FAIL;
	}

	return RET_SUCC;
}

static void set_disable_flag()
{
	u32 i;

	g_enable_cnt = 0;
	for (i = 0; i < ARRAY_SIZE(g_ar_status); i++)
		g_ar_status[i].enable = false;
}

/*
 * Function    : enable_ar_when_recovery_iom3
 * Description : recovery ar app and cared activity.
 * Input       : none
 * Output      : none
 * Return      : none
 */
void enable_ar_when_recovery_iom3(void)
{
	struct ar_config_packet pack;
	int ret;
	u32 i;
	interval_param_t interval_param;

	mutex_lock(&g_mutex_ar_status);
	if (g_enable_cnt == 0) {
		mutex_unlock(&g_mutex_ar_status);
		ctxhub_warn("%s, don't care any ar event, don't open ar\n", __func__);
		return;
	}

	ret = inputhub_device_enable(TAG_AR, true);
	if (ret != RET_SUCC) {
		set_disable_flag();
		mutex_unlock(&g_mutex_ar_status);
		ctxhub_err("%s, enable failed!\n", __func__);
		return;
	}

	interval_param.period = AR_TIMER;
	ret = inputhub_device_setdelay(TAG_AR, &interval_param);
	if (ret != RET_SUCC) {
		set_disable_flag();
		mutex_unlock(&g_mutex_ar_status);
		ctxhub_err("%s, recovery ar start timer failed!\n", __func__);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(g_ar_status); i++) {
		if (g_ar_status[i].enable) {
			pack.sub_cmd = SUB_CMD_SET_PARAMET_REQ;
			pack.config_data.core = 0; // 0 is core ap
			pack.config_data.oper = AR_ADD_EVENT;
			pack.config_data.para.activity = get_ar_type(g_ar_status[i].ar_shb_type);
			pack.config_data.para.event_type = g_ar_status[i].care_event;
			pack.config_data.para.report_latency = g_ar_status[i].report_latency;
			pack.config_data.para.non_wakeup = g_ar_status[i].non_wakeup;

			ret = inputhub_wrapper_send_cmd(TAG_AR, CMD_CMN_CONFIG_REQ, (const void *)&pack,
							sizeof(pack), NULL);
			if (ret != RET_SUCC) {
				// if recovery failed, set false flag
				g_ar_status[i].enable = false;
				g_enable_cnt--;
				ctxhub_err("%s, recovery activity %d failed!ret is %d\n",
					__func__, get_ar_type(i), ret);
			}
		}
	}
	mutex_unlock(&g_mutex_ar_status);
}

static int ar_recovery_notifier(struct notifier_block *nb,
				unsigned long foo, void *bar)
{
	ctxhub_info("%s %lu +\n", __func__, foo);
	switch (foo) {
	case IOM3_RECOVERY_IDLE:
	case IOM3_RECOVERY_START:
	case IOM3_RECOVERY_MINISYS:
	case IOM3_RECOVERY_3RD_DOING:
	case IOM3_RECOVERY_FAILED:
		break;
	case IOM3_RECOVERY_DOING:
		enable_ar_when_recovery_iom3();
		break;
	default:
		ctxhub_err("%s -unknown state %ld\n", __func__, foo);
		break;
	}
	ctxhub_info("%s -\n", __func__);
	return 0;
}

#define AR_RECOVERY_PRIORITY (-2)
static struct notifier_block g_ar_recovery_notify = {
	.notifier_call = ar_recovery_notifier,
	.priority = AR_RECOVERY_PRIORITY, // smaller than true sensor is better
};

/*
 * get ar sensor info from dts
 * read sensor list info, timeout support and report latency
 */
static void get_ar_sensor_info(void)
{
	u32 i;
	struct device_node *node = NULL;
	int temp;
	bool one_shot = false;
	bool wakeup = false;

	for (i = 0; i < ARRAY_SIZE(g_ar_status); i++) {
		node = of_find_compatible_node(NULL, NULL, g_ar_status[i].dts_name);
		if (node) {
			// read sensor list info
			read_sensorlist_info(node, &g_ar_status[i].sensor_info, TAG_AR);
			if (g_ar_status[i].sensor_info.flags != INVALID_VALUE) {
				wakeup = (SENSOR_WAKEUP_FLAG & g_ar_status[i].sensor_info.flags);
				g_ar_status[i].non_wakeup = !wakeup ? NON_WAKEUP : WAKEUP;
				g_ar_status[i].flags = g_ar_status[i].sensor_info.flags;
				// one shot will only care enter
				one_shot = is_one_shot(g_ar_status[i].flags);
				g_ar_status[i].care_event = one_shot ? CARE_ENTER : CARE_ENTER_AND_EXIT;
			}
			// read timeout support
			if (of_property_read_u32(node, "supportTimeout", &temp) == 0) {
				g_ar_status[i].support_timeout = (bool)temp;
				ctxhub_info("ar type %d support timeout %d\n", g_ar_status[i].ar_shb_type, temp);
			}
			// read default latency
			if (of_property_read_u32(node, "defaultLatency", &temp) == 0) {
				g_ar_status[i].default_report_latency = temp;
				ctxhub_info("ar type %d default latency %d\n", g_ar_status[i].ar_shb_type, temp);
			}
		}
	}
}

static int __init ar_init(void)
{
	int ret;
	u32 i;

	ctxhub_info("%s, enter\n", __func__);
	if (get_contexthub_dts_status())
		return RET_FAIL;

	get_ar_sensor_info();

	g_ar_interface = (struct customized_interface *)vzalloc(sizeof(struct customized_interface));
	if (!g_ar_interface) {
		ctxhub_err("%s, alloc failed", __func__);
		return RET_FAIL;
	}

	ret = register_iom3_recovery_notifier(&g_ar_recovery_notify);
	if (ret != 0) {
		ctxhub_err("%s, register ar recovery notifer failed!\n", __func__);
		ret = RET_FAIL;
		goto free;
	}
	g_ar_interface->enable = ar_enable;
	g_ar_interface->report_event = ar_report_event;
	g_ar_interface->setdelay = ar_setdelay;
	g_ar_interface->flush = ar_flush;

	for (i = SENSORHUB_TYPE_AR_START; i < SENSORHUB_TYPE_AR_END; i++) {
		ret = register_customized_interface(i, TAG_AR, g_ar_interface);
		if (ret != RET_SUCC)
			ctxhub_warn("%s, register customized for AR activity %d failed!\n",
				    __func__, i);
	}

	ctxhub_info("%s, exit\n", __func__);
	return ret;

free:
	vfree(g_ar_interface);
	g_ar_interface = NULL;
	return ret;
}

static void __exit ar_exit(void)
{
	u32 i;
	int ret;

	if (get_contexthub_dts_status())
		return;

	for (i = SENSORHUB_TYPE_AR_START; i < SENSORHUB_TYPE_AR_END; i++) {
		ret = unregister_customized_interface(i);
		if (ret != RET_SUCC)
			ctxhub_err("%s, unregister customized for AR failed!\n", __func__);
	}

	if (g_ar_interface)
		vfree(g_ar_interface);
	g_ar_interface = NULL;
}


late_initcall(ar_init);
module_exit(ar_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("adapt for ar use sensor channel pass data or cmd");
MODULE_AUTHOR("Smart");
