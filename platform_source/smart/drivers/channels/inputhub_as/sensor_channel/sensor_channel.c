/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: sensor channel implement.
 * Create: 2019/11/05
 */

#include "sensor_channel.h"

#include <securec.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/suspend.h>
#include <linux/reboot.h>

#include "common/common.h"
#include "inputhub_wrapper/inputhub_wrapper.h"
#include "device_manager.h"
#include "device_interface.h"
#include "iomcu_route.h"
#include "plat_func.h"
#include "iomcu_ipc.h"
#include "platform_include/smart/linux/iomcu_dump.h"
#include "iomcu_log.h"

#define DISABLE_SLEEP_TIME 50

#define AP_FLUSH_SLEEP_TIME 60

#define MS_TO_NS 1000000

#define ADDITIONAL_EVENT_LEN 12

// g_sensorlist[0] is array len, g_sensorlist[1]...g_sensorlist[g_sensorlist[0]] is array element
u16 g_sensorlist[SENSOR_LIST_NUM];

struct sensor_status g_sensor_status;

static bool g_is_customized_tag[TAG_END];
u8 g_tag_to_hal_sensor_type[TAG_SENSOR_END];
static u8 g_hal_sensor_type_to_tag[SENSORHUB_TYPE_END];
static DEFINE_MUTEX(g_mutex_status);

struct inputhub_route_table g_shb_route_table = {
	"sensor channel", {NULL, 0}, {NULL, 0}, {NULL, 0},
	__WAIT_QUEUE_HEAD_INITIALIZER(g_shb_route_table.read_wait)
};

static s64 g_sensors_tm[TAG_SENSOR_END];

struct sensors_cmd_map {
	int hal_sensor_type;
	int tag;
	bool is_customized; // true will use customize interface to enable, set delay and etc...
	struct customized_interface *customize_interface;
};

/*
 * map of senoshrub type - {sensorhub type, tag, is_customized, customized interface}
 * will use in init_hash_tables
 * defalut is_customized is false, customzied_interface is null
 * sensorhub type which regsitered by register_customized_interface, its is_customized is true.
 */
static struct sensors_cmd_map g_sensors_cmd_map_tab[SENSORHUB_TYPE_END] = {
	[SENSORHUB_TYPE_ACCELEROMETER] = {SENSORHUB_TYPE_ACCELEROMETER, TAG_ACCEL},
	[SENSORHUB_TYPE_GYROSCOPE] = {SENSORHUB_TYPE_GYROSCOPE, TAG_GYRO},
	[SENSORHUB_TYPE_CAP_PROX] = {SENSORHUB_TYPE_CAP_PROX, TAG_CAP_PROX},
	[SENSORHUB_TYPE_DUMMY_DS1] = {SENSORHUB_TYPE_DUMMY_DS1, TAG_DUMMY_DS1},
	[SENSORHUB_TYPE_DUMMY_AS1] = {SENSORHUB_TYPE_DUMMY_AS1, TAG_DUMMY_AS1},
	[SENSORHUB_TYPE_DUMMY_AS2] = {SENSORHUB_TYPE_DUMMY_AS2, TAG_DUMMY_AS2},
	[SENSORHUB_TYPE_DUMMY_AS3] = {SENSORHUB_TYPE_DUMMY_AS3, TAG_DUMMY_AS3},
	[SENSORHUB_TYPE_META_DATA] = {SENSORHUB_TYPE_META_DATA, TAG_FLUSH_META},
};

static bool is_customized_tag(int tag)
{
	if (tag < TAG_BEGIN || tag >= TAG_END) {
		ctxhub_warn("%s, tag is valid %d\n", __func__, tag);
		return false;
	}
	return g_is_customized_tag[tag];
}

int register_customized_interface(int shb_type, int tag, const struct customized_interface *interface)
{
	if (shb_type < SENSORHUB_TYPE_BEGIN || shb_type >= SENSORHUB_TYPE_END) {
		ctxhub_err("%s, sensorhub type error!\n", __func__);
		return RET_FAIL;
	}

	if (tag < TAG_BEGIN || tag >= TAG_END) {
		ctxhub_err("%s, tag is invalid\n", __func__);
		return RET_FAIL;
	}

	g_sensors_cmd_map_tab[shb_type].hal_sensor_type = shb_type;
	g_sensors_cmd_map_tab[shb_type].tag = tag;
	g_sensors_cmd_map_tab[shb_type].is_customized = true;
	g_sensors_cmd_map_tab[shb_type].customize_interface = (struct customized_interface *)interface;
	g_hal_sensor_type_to_tag[shb_type] = tag;
	g_is_customized_tag[tag] = true;
	return RET_SUCC;
}

int unregister_customized_interface(int shb_type)
{
	int tag;

	if (shb_type < SENSORHUB_TYPE_BEGIN || shb_type >= SENSORHUB_TYPE_END) {
		ctxhub_err("%s, sensorhub type error!\n", __func__);
		return RET_FAIL;
	}

	tag = g_sensors_cmd_map_tab[shb_type].tag;
	if (tag < TAG_BEGIN || tag >= TAG_END) {
		ctxhub_err("%s, tag is invalid\n", __func__);
		return RET_FAIL;
	}

	g_sensors_cmd_map_tab[shb_type].customize_interface = NULL;
	g_sensors_cmd_map_tab[shb_type].hal_sensor_type = 0;
	g_sensors_cmd_map_tab[shb_type].tag = 0;
	g_sensors_cmd_map_tab[shb_type].is_customized = true;
	g_hal_sensor_type_to_tag[shb_type] = 0;
	g_is_customized_tag[tag] = false;
	return RET_SUCC;
}

static int ap_sensor_flush(int tag)
{
	struct sensor_data event;

	if (work_on_ap(tag)) {
		ap_device_enable(tag, true);
		msleep(AP_FLUSH_SLEEP_TIME);
		event.type = g_tag_to_hal_sensor_type[TAG_FLUSH_META];
		event.length = 4; /* 4 : sensor data length */
		event.value[0] = g_tag_to_hal_sensor_type[tag];

		return inputhub_route_write_batch(&g_shb_route_table,
			(char *)&event, event.length +
			offsetofend(struct sensor_data, length), 0);
	}
	return 0;
}

void sensor_get_data(struct sensor_data *data)
{
	struct t_sensor_get_data *get_data = NULL;

	if (!data || (!((data->type >= SENSORHUB_TYPE_BEGIN) &&
			(data->type < SENSORHUB_TYPE_END)))) {
		ctxhub_err("%s, para err\n", __func__);
		return;
	}

	get_data = &g_sensor_status.get_data[data->type];
	if (atomic_cmpxchg(&get_data->reading, 1, 0)) {
		(void)memcpy_s(&get_data->data, sizeof(struct sensor_data),
			data, sizeof(struct sensor_data));
		complete(&get_data->complete);
	}
}

/*
 * Function    : report_sensor_event
 * Description : write sensor event to kernel buffer
 * Input       : [shb_type] key, identify an app of sensor
 *             : [value] sensor data
 *             : [length] sensor data length
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int report_sensor_event(int shb_type, const int value[], unsigned int length)
{
	struct sensor_data event;
	int ret;

	if ((!(shb_type >= SENSORHUB_TYPE_BEGIN && shb_type < SENSORHUB_TYPE_END)) ||
	    (length > sizeof(event.value))) {
		ctxhub_err("para error (shb_type : %d), (length : %d) in %s\n",
			shb_type, length, __func__);
		return -EINVAL;
	}

	event.type = shb_type;
	event.length = length;
	ret = memcpy_s(event.value, sizeof(event.value), value, length);
	if (ret != EOK) {
		ctxhub_err("[%s], memcpy failed!\n", __func__);
		return -EINVAL;
	}
	sensor_get_data(&event);

	return inputhub_route_write(&g_shb_route_table, (char *)&event,
		event.length + offsetofend(struct sensor_data, length));
}

static int report_sensor_event_batch(int tag, const int value[], int length,
				     uint64_t timestamp)
{
	struct sensor_data event;
	s64 ltimestamp;
	u8 event_value_tag;
	int ret;

	if (tag < TAG_FLUSH_META || tag >= TAG_SENSOR_END) {
		ctxhub_err("para error (tag : %d), in %s\n",
			tag, __func__);
		return -EINVAL;
	}

	if (tag != TAG_FLUSH_META) {
		ltimestamp = timestamp;
		event.type = g_tag_to_hal_sensor_type[tag];
		event.length = length;
		if (memcpy_s(event.value, sizeof(event.value),
			     (char *)value, length) != EOK) {
			ctxhub_err("[%s], memcpy failed!\n", __func__);
			return RET_FAIL;
		}

		sensor_get_data(&event);
	} else {
		ltimestamp = 0;
		event_value_tag = ((struct pkt_header *)value)->app_tag;
		event.type = g_tag_to_hal_sensor_type[tag];
		event.length = 4; /* 4 : sensor data length */
		if (event_value_tag < TAG_SENSOR_END)
			event.value[0] =
				g_tag_to_hal_sensor_type[event_value_tag];
	}

	ret = inputhub_route_write_batch(&g_shb_route_table, (char *)&event,
		event.length + offsetofend(struct sensor_data, length),
		ltimestamp);
	if (ret <= 0)
		return RET_FAIL;

	return RET_SUCC;
}

static int64_t get_timestamp(void)
{
	struct timespec ts;

	get_monotonic_boottime(&ts);
	/* timevalToNano */
	return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static int64_t get_sensor_syscounter_timestamp(
	pkt_batch_data_req_t *sensor_event)
{
	s64 timestamp;

	timestamp = get_timestamp();

	if (sensor_event->data_hd.data_flag & DATA_FLAG_VALID_TIMESTAMP)
		timestamp = sensor_event->data_hd.timestamp;

	ctxhub_dbg("sensor %d origin tick %lld transfer timestamp %lld.\n",
		  inputhub_wrapper_get_app_tag((struct pkt_header *)sensor_event),
		sensor_event->data_hd.timestamp, timestamp);
	return timestamp;
}

static void report_meta_event_for_additional_info(int data_type, int app_tag,
						  int serial, s64 timestamp)
{
	struct sensor_data event;
	int ret;

	/* app_tag range check outside */
	if (data_type != AINFO_BEGIN && data_type != AINFO_END)
		return;

	event.type = SENSORHUB_TYPE_ADDITIONAL_INFO;
	event.data_type = data_type;
	event.sensor_type = g_tag_to_hal_sensor_type[app_tag];
	event.serial = serial;
	event.length = ADDITIONAL_EVENT_LEN;
	ret = inputhub_route_write_batch(&g_shb_route_table, (char *)&event,
			event.length + offsetofend(struct sensor_data, length),
			timestamp);
	if (ret <= 0)
		ctxhub_warn("%s, write event failed!\n", __func__);

	ctxhub_info("***report sensor_type %d %s addition info event!***\n",
		 event.sensor_type, data_type == AINFO_BEGIN ? "start" : "end");
}

int inputhub_process_additional_info_report(const struct pkt_header *head)
{
	s64 timestamp;
	pkt_additional_info_req_t *addi_info = NULL;
	struct sensor_data event;
	unsigned int info_len;
	int app_tag = inputhub_wrapper_get_app_tag(head);
	int serial;
	int ret;

	addi_info = (pkt_additional_info_req_t *)head;
	timestamp = get_timestamp();
	if (app_tag < TAG_SENSOR_BEGIN || app_tag >= TAG_SENSOR_END) {
		ctxhub_err("%s head->app_tag = %d\n", __func__, app_tag);
		return -EINVAL;
	}
	if (head->length < (offsetof(pkt_additional_info_req_t, data_int32) - offsetofend(pkt_additional_info_req_t, hd))) {
		ctxhub_err("%s, head length %d too small\n", __func__, (int)head->length);
		return -EINVAL;
	}

	if (addi_info->serial == 1) /* create a begin event */
		report_meta_event_for_additional_info(AINFO_BEGIN, app_tag, 0, timestamp);

	info_len = head->length - (offsetof(pkt_additional_info_req_t, data_int32) - offsetofend(pkt_additional_info_req_t, hd));
	event.type = SENSORHUB_TYPE_ADDITIONAL_INFO;
	event.serial = addi_info->serial;
	event.sensor_type = g_tag_to_hal_sensor_type[app_tag];
	event.data_type = addi_info->type;
	event.length = info_len + ADDITIONAL_EVENT_LEN;
	if (info_len > sizeof(addi_info->data_int32)) {
		ctxhub_err("[%s], copy len %d bigger than src len %lu\n", __func__,
			info_len, sizeof(addi_info->data_int32));
		return -EINVAL;
	}
	if (memcpy_s(event.info, sizeof(event.info), addi_info->data_int32,
		     info_len) != EOK) {
		ctxhub_err("[%s], memcpy_s error\n", __func__);
		return -EINVAL;
	}

	ret = inputhub_route_write_batch(&g_shb_route_table, (char *)&event,
			event.length + offsetofend(struct sensor_data, length),
			timestamp);
	if (ret <= 0)
		ctxhub_warn("%s, write additional info failed!\n", __func__);
	ctxhub_info("report sensor type %d addition info: %d !\n",
		 event.sensor_type, event.info[0]);

	if (addi_info->end == 1) {
		serial = ++addi_info->serial;
		report_meta_event_for_additional_info(AINFO_END, app_tag, serial, timestamp);
	}

	return 0;
}

static int param_check(const struct pkt_header *head)
{
	int app_tag;

	if (!head) {
		ctxhub_err("%s, header is null\n", __func__);
		return RET_FAIL;
	}

	app_tag = inputhub_wrapper_get_app_tag(head);
	// tag is uint8, no need to check >= TAG_FLUSH_META, which macro is 0
	if (app_tag >= TAG_SENSOR_END && !is_customized_tag(app_tag)) {
		ctxhub_err("%s, error head tag %u\n", __func__, (uint32_t)app_tag);
		return RET_FAIL;
	}
	return RET_SUCC;
}

static struct customized_interface *get_customized_interface(int tag)
{
	int i;

	for (i = SENSORHUB_TYPE_BEGIN; i < SENSORHUB_TYPE_END; i++) {
		if (g_sensors_cmd_map_tab[i].tag == tag)
			return g_sensors_cmd_map_tab[i].customize_interface;
	}
	return NULL;
}

static int inputhub_process_universal_sensor_report(const struct pkt_header *head)
{
	int ret = RET_SUCC;
	u64 delta;
	u64 i;
	s64 timestamp;
	s64 head_timestamp;
	pkt_batch_data_req_t *sensor_event = (pkt_batch_data_req_t *)head;
	int app_tag = inputhub_wrapper_get_app_tag(head);

	timestamp = get_sensor_syscounter_timestamp(sensor_event);
	if (timestamp <= g_sensors_tm[app_tag])
		timestamp = g_sensors_tm[app_tag] + 1;

	if (sensor_event->data_hd.cnt < 1) {
		goto flush_event;
	} else if (sensor_event->data_hd.cnt > 1) {
		delta = (uint64_t)(sensor_event->data_hd.sample_rate) *
			MS_TO_NS;
		head_timestamp = timestamp - (sensor_event->data_hd.cnt - 1) *
			(int64_t)delta;
		if (head_timestamp <= g_sensors_tm[app_tag]) {
			delta = (timestamp - g_sensors_tm[app_tag]) /
				sensor_event->data_hd.cnt;
			timestamp = g_sensors_tm[app_tag] + delta;
		} else {
			timestamp = head_timestamp;
		}

		for (i = 0; i < sensor_event->data_hd.cnt; i++) {
			report_sensor_event_batch(app_tag,
				(int *)((char *)head +
				offsetof(pkt_batch_data_req_t, xyz) +
				i * sensor_event->data_hd.len_element),
				sensor_event->data_hd.len_element, timestamp);
			timestamp += delta;
		}
		timestamp -= delta;
		g_sensors_tm[app_tag] = timestamp;
		goto flush_event;
	}

	/* There was a data process before, can move up to sensor hal */
	ret = report_sensor_event_batch(app_tag, (int *)(sensor_event->xyz),
			sensor_event->data_hd.len_element, timestamp);
	g_sensors_tm[app_tag] = timestamp;

flush_event:
	if (sensor_event->data_hd.data_flag & FLUSH_END)
		ret = report_sensor_event_batch(TAG_FLUSH_META, (int *)head,
					  sizeof(struct pkt_header), 0);

	return ret;
}

static int check_universal_sensor_head(const struct pkt_header *head)
{
	// have check null before
	s32 pkt_len = head->length;
	s32 expect_len = 0;
	pkt_common_data_t *p = (pkt_common_data_t*)head;

	/*
	 * standard sensor packet use struct pkt_batch_data_req_t which is
	 * {
	 *    pkt_common_data_t data_hd;
	 *    struct sensor_data_xyz xyz[];
	 * }
	 * struct pkt_common_data_t is
	 * {
	 *    struct pkt_header hd;
	 *    uint16_t data_flag;
	 *    uint16_t cnt;
	 *    uint16_t len_element;
	 *    uint16_t sample_rate;
	 *    uint64_t timestamp;
	 * }
	 * cnt is len of xyz.
	 * First, length in hd must greater than (sizeof(pkt_common_data_t) - offsetofend(pkt_common_data_t, hd)) which record as x.
	 * Then, x value is ok, so we can access cnt.We calculate dynamic array xyz by cnt * sizeof(struct sensor_data_xyz), which record as y.
	 * Last, hd and xyz maybe not align.(offsetof(pkt_batch_data_req_t, xyz) - offsetofend(pkt_batch_data_req_t, data_hd)) record as z.
	 * x + y + z need equal head->length. params in packet are all uint16, we use int32 will not overflow.
	 */
	expect_len += (sizeof(pkt_common_data_t) - offsetofend(pkt_common_data_t, hd));
	if (head->length < expect_len) {
		ctxhub_err("%s, head length is too short\n", __func__);
		return RET_FAIL;
	}

	expect_len += (s32)p->cnt * p->len_element;
	if (p->cnt != 0)
		expect_len += (offsetof(pkt_batch_data_req_t, xyz) - offsetofend(pkt_batch_data_req_t, data_hd));

	if (expect_len != pkt_len) {
		ctxhub_err("%s, head length %d is not equal %d\n", __func__, pkt_len, expect_len);
		return RET_FAIL;
	}
	return RET_SUCC;
}

int inputhub_process_sensor_report(const struct pkt_header *head)
{
	int app_tag = inputhub_wrapper_get_app_tag(head);
	struct customized_interface *customize_interface = NULL;

	if (param_check(head) != RET_SUCC) {
		ctxhub_err("[%s], param check failed!\n", __func__);
		return RET_FAIL;
	}

	if (is_customized_tag(app_tag)) {
		customize_interface = get_customized_interface(app_tag);
		if (customize_interface && customize_interface->report_event)
			return customize_interface->report_event(&g_shb_route_table, head);
		return RET_FAIL;
	}

	if (check_universal_sensor_head(head) != RET_SUCC) {
		ctxhub_err("[%s], check head failed!\n", __func__);
		return RET_FAIL;
	}
	return inputhub_process_universal_sensor_report(head);
}

/*
 * Function    : get_status_from_dts
 * Description : get hifi status, g_sensorlist
 * Input       : none
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
static int get_status_from_dts(void)
{
	unsigned int i;
	unsigned int len;
	u32 sensorlists[SENSOR_LIST_NUM];
	struct device_node *sensorhub_node = NULL;
	struct property *prop = NULL;
	const char *sensor_list_name = "adapt_sensor_list_id";

	sensorhub_node =
		of_find_compatible_node(NULL, NULL, "hisilicon,sensorhub");
	if (!sensorhub_node) {
		ctxhub_err("%s, can't find node sensorhub\n", __func__);
		return RET_FAIL;
	}

	prop = of_find_property(sensorhub_node, sensor_list_name, NULL);
	if (!prop) {
		ctxhub_err("%s! prop is NULL\n", __func__);
		return -EINVAL;
	}

	if (!prop->value) {
		ctxhub_err("%s! prop->value is NULL\n", __func__);
		return -ENODATA;
	}

	len = prop->length / sizeof(unsigned int);
	if (len > SENSOR_LIST_NUM) {
		ctxhub_err("%s! sensor list len %u from dts is too long, capacity is %d\n",
			__func__, len, SENSOR_LIST_NUM);
		return -EINVAL;
	}

	if (of_property_read_u32_array(sensorhub_node, sensor_list_name,
				       sensorlists, len)) {
		ctxhub_err("%s:read adapt_sensor_list_id from dts fail\n",
			sensor_list_name);
		return -EINVAL;
	}

	if ((uint32_t)g_sensorlist[0] + len > SENSOR_LIST_NUM - 1) {
		ctxhub_err("%s:sensor list is too much %u, greater than %d\n",
			__func__, (uint32_t)g_sensorlist[0] + len,
			SENSOR_LIST_NUM - 1);
		return -EINVAL;
	}
	for (i = 0; i < len; i++) {
		g_sensorlist[g_sensorlist[0] + 1] = (uint16_t)sensorlists[i];
		g_sensorlist[0]++;
	}
	return 0;
}


static void init_hash_tables(void)
{
	unsigned int i;

	for (i = SENSORHUB_TYPE_BEGIN; i < SENSORHUB_TYPE_END; ++i) {
		/*
		 * If corresponding tag of sensorhub type is customized tag, hash table will maintain in customized
		 * modules.Sensor channel only maintain acc, gyro and etc.
		 */
		if (!is_customized_tag(g_sensors_cmd_map_tab[i].tag)) {
			g_tag_to_hal_sensor_type[g_sensors_cmd_map_tab[i].tag] =
				g_sensors_cmd_map_tab[i].hal_sensor_type;
			g_hal_sensor_type_to_tag[g_sensors_cmd_map_tab[i].hal_sensor_type] =
				g_sensors_cmd_map_tab[i].tag;
		}
	}
}

static void init_have_send_calibrate_data(void)
{
	int i;

	for (i = TAG_SENSOR_BEGIN; i < TAG_SENSOR_END; ++i)
		g_sensor_status.have_send_calibrate_data[i] = false;
}

static int inputhub_sensor_channel_flush(int shb_type, int tag)
{
	u32 subcmd;
	struct customized_interface *customize_interface = NULL;

	ap_sensor_flush(tag);
	subcmd = SUB_CMD_FLUSH_REQ;
	ctxhub_info("flush sensor %s tag:%d, shb type:%d\n", get_tag_str(tag), tag, shb_type);
	if (!is_customized_tag(tag))
		return inputhub_wrapper_send_cmd(tag, CMD_CMN_CONFIG_REQ, (const void *)&subcmd,
					SUBCMD_LEN, NULL);

	// else is customize flush
	customize_interface = get_customized_interface(tag);
	if (customize_interface && customize_interface->flush)
		return customize_interface->flush(shb_type);

	ctxhub_err("%s, customize interface is null\n", __func__);
	return RET_FAIL;
}

bool is_one_shot(u32 flags)
{
	return ((flags & REPORTING_MODE_MASK) == SENSOR_FLAG_ONE_SHOT_MODE);
}

static int send_sensor_batch_flush_cmd(unsigned int cmd,
				       struct ioctl_para *para, int tag)
{
	if (cmd == SHB_IOCTL_APP_SENSOR_BATCH)
		return inputhub_sensor_channel_setdelay(tag, para, true);
	else if (cmd == SHB_IOCTL_APP_SENSOR_FLUSH)
		return inputhub_sensor_channel_flush(para->shbtype, tag);
	return 0;
}

/*
 * Function    : inputhub_sensor_channel_enable
 * Description : enable command send to sensorhub
 * Input       : [shb_type] sensor identify, use in sensor hal
 *             : [tag] sensor identify, use in sensor kernel
 *             : [enable] enable or disable
 * Output      : none
 * Return      : 0 is ok, otherwise fail
 */
int inputhub_sensor_channel_enable(int shb_type, int tag, bool enable)
{
	struct customized_interface *customize_interface = NULL;

	ctxhub_info("%s, shb_type %d, tag %d, enable %d enter!\n",
		__func__, shb_type, tag, enable);
	if (is_customized_tag(tag)) {
		customize_interface = get_customized_interface(tag);
		if (customize_interface && customize_interface->enable)
			return customize_interface->enable(shb_type, enable);
		ctxhub_err("%s, customize enable interface is null\n", __func__);
		return RET_FAIL;
	}
	return inputhub_device_enable(tag, enable);
}

/*
 * Function    : inputhub_sensor_channel_setdelay
 * Description : batch param send to sensorhub
 * Input       : [tag] sensor identify, use in sensor kernel
 *             : [para] include period, timeout, shb_type; set delay will not use timeout
 *             : [is_batch] true is batch, false is set delay
 * Output      : none
 * Return      : 0 is ok, otherwise fail
 */
int inputhub_sensor_channel_setdelay(int tag, const struct ioctl_para *para, bool is_batch)
{
	struct customized_interface *customize_interface = NULL;
	interval_param_t batch_param;

	if (!para) {
		ctxhub_err("%s, ioctl param is null\n", __func__);
		return RET_FAIL;
	}

	if (para->period_ms < 0 || (is_batch && para->timeout_ms < 0)) {
		ctxhub_err("%s, period %d, timeout %d invalid\n", __func__,
			para->period_ms, para->timeout_ms);
		return RET_FAIL;
	}

	ctxhub_info("%s, shb_type %d, tag %d, is_batch %d enter!\n",
		__func__, para->shbtype, tag, is_batch);
	if (is_customized_tag(tag)) {
		customize_interface = get_customized_interface(tag);
		if (customize_interface && customize_interface->setdelay)
			return customize_interface->setdelay(para->shbtype, para->period_ms, para->timeout_ms,
							     is_batch);
		ctxhub_err("%s, customize setdelay interface is null\n", __func__);
		return RET_FAIL;
	}

	batch_param.mode = AUTO_MODE;
	batch_param.period = para->period_ms;
	batch_param.batch_count = 1;

	if (is_batch && para->period_ms != 0)
		batch_param.batch_count =
			(para->timeout_ms > para->period_ms) ?
			(para->timeout_ms / para->period_ms) : 1;
	ctxhub_info("%s batch period=%d, count=%d\n", __func__,
		    para->period_ms, batch_param.batch_count);
	return inputhub_device_setdelay(tag, &batch_param);
}

/************************************************************
 *functions for debug
 *************************************************************/
ssize_t get_sensor_list_info(char *buf)
{
	int i;
	int ret;

	if (buf == NULL) {
		ctxhub_err("[%s], buffer is NULL!\n", __func__);
		return 0;
	}

	ctxhub_info("sensor list: ");
	for (i = 0; i <= g_sensorlist[0]; i++)
		ctxhub_info(" %d  ", g_sensorlist[i]);
	ctxhub_info("\n");
	ret = memcpy_s(buf, PAGE_SIZE, g_sensorlist,
		((g_sensorlist[0] + 1) * sizeof(uint16_t)));
	if (ret != EOK) {
		ctxhub_err("[%s], memcpy failed!\n", __func__);
		return 0;
	}
	return (g_sensorlist[0] + 1) * sizeof(uint16_t);
}

struct t_sensor_get_data* get_sensors_status_data(int shb_type)
{

	if ((shb_type < SENSORHUB_TYPE_BEGIN) || (shb_type >= SENSORHUB_TYPE_END)) {
		ctxhub_err("%s, shb_type err\n", __func__);
		return NULL;
	}

	return &g_sensor_status.get_data[shb_type];
}
/*******************************************************************/

static int send_sensor_cmd(unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	int tag, ret;
	struct ioctl_para para;

	if (copy_from_user(&para, argp, sizeof(para))) {
		ctxhub_err("%s, copy from user error\n", __func__);
		return -EFAULT;
	}

	if (!(para.shbtype >= SENSORHUB_TYPE_BEGIN &&
	      para.shbtype < SENSORHUB_TYPE_END)) {
		ctxhub_err("error shbtype %d in %s\n", para.shbtype, __func__);
		return -EINVAL;
	}

	tag = g_hal_sensor_type_to_tag[para.shbtype];
	switch (cmd) {
	case SHB_IOCTL_APP_ENABLE_SENSOR:
		return inputhub_sensor_channel_enable(para.shbtype, tag, true);
	case SHB_IOCTL_APP_DISABLE_SENSOR:
		ret = inputhub_sensor_channel_enable(para.shbtype, tag, false);
		if (ret != RET_SUCC) {
			ctxhub_err("%s, failed to disable device %d,\n",
				__func__, para.shbtype);
			return RET_FAIL;
		}
		break;
	case SHB_IOCTL_APP_DELAY_SENSOR:
		ret = inputhub_sensor_channel_enable(para.shbtype, tag, true);
		if (ret != RET_SUCC) {
			ctxhub_err("%s, failed to enable device %d,\n",
				__func__, para.shbtype);
			return RET_FAIL;
		}
		return inputhub_sensor_channel_setdelay(tag, &para, false);
	case SHB_IOCTL_APP_SENSOR_BATCH:
		ret = inputhub_sensor_channel_enable(para.shbtype, tag, true);
		if (ret != RET_SUCC) {
			ctxhub_err("%s, failed to enable device %d,\n",
				__func__, para.shbtype);
			return RET_FAIL;
		}
		// fall through
	case SHB_IOCTL_APP_SENSOR_FLUSH:
		return send_sensor_batch_flush_cmd(cmd, &para, tag);
	default:
		ctxhub_err("unknown shb_hal_cmd %d in %s\n", cmd, __func__);
		return -EINVAL;
	}

	return 0;
}

static ssize_t shb_read(struct file *file, char __user *buf, size_t count,
			loff_t *pos)
{
	return inputhub_route_read(&g_shb_route_table, buf, count);
}

static ssize_t shb_write(struct file *file, const char __user *data,
			 size_t len, loff_t *ppos)
{
	ctxhub_info("%s need to do\n", __func__);
	return len;
}

static void send_sensor_calib_data(unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	int tag;
	struct ioctl_para para;

	if (copy_from_user(&para, argp, sizeof(para))) {
		ctxhub_err("error copy_from_user arg %ld in %s\n", arg, __func__);
		return;
	}

	if (!(para.shbtype >= SENSORHUB_TYPE_BEGIN &&
	      para.shbtype < SENSORHUB_TYPE_END)) {
		ctxhub_err("error shbtype %d in %s\n", para.shbtype, __func__);
		return;
	}

	tag = g_hal_sensor_type_to_tag[para.shbtype];
	if (tag >= TAG_SENSOR_BEGIN && tag < TAG_SENSOR_END) {
		if (g_sensor_status.have_send_calibrate_data[tag] == false) {
			g_sensor_status.have_send_calibrate_data[tag] = true;
			send_calibrate_data_to_iomcu(tag, true);
		}
	}
}

static void init_sensors_get_data(void)
{
	int shb_type;

	for (shb_type = SENSORHUB_TYPE_BEGIN; shb_type < SENSORHUB_TYPE_END; ++shb_type) {
		atomic_set(&g_sensor_status.get_data[shb_type].reading, 0);
		init_completion(&g_sensor_status.get_data[shb_type].complete);
	}
}

static long shb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	int sensor_mcu_mode;

	if (!argp) {
		ctxhub_err("%s, user pointer is null\n", __func__);
		return -EINVAL;
	}
	switch (cmd) {
	case SHB_IOCTL_APP_ENABLE_SENSOR:
	case SHB_IOCTL_APP_DISABLE_SENSOR:
	case SHB_IOCTL_APP_DELAY_SENSOR:
		break; /* fall through */
	case SHB_IOCTL_APP_GET_SENSOR_MCU_MODE:
		sensor_mcu_mode = get_sensor_mcu_mode();
		ctxhub_info("is_sensor_mcu_mode [%d]\n", sensor_mcu_mode);
		if (copy_to_user(argp, &sensor_mcu_mode,
				 sizeof(sensor_mcu_mode)))
			return -EFAULT;
		return 0;
	case SHB_IOCTL_APP_SENSOR_BATCH:
		ctxhub_info("%s cmd : batch flush SHB_IOCTL_APP_SENSOR_BATCH\n", __func__);
		break;
	case SHB_IOCTL_APP_SENSOR_FLUSH:
		ctxhub_info("%s cmd : batch flush SHB_IOCTL_APP_SENSOR_FLUSH\n", __func__);
		break;
	default:
		ctxhub_err("%s unknown cmd : %d\n", __func__, cmd);
		return -ENOTTY;
	}
	send_sensor_calib_data(arg);
	return send_sensor_cmd(cmd, arg);
}

/* shb_open: do nothing now */
static int shb_open(struct inode *inode, struct file *file)
{
	ctxhub_info("%s ok\n", __func__);
	return 0;
}

/* shb_release: do nothing now */
static int shb_release(struct inode *inode, struct file *file)
{
	ctxhub_info("%s ok\n", __func__);
	return 0;
}

static const struct file_operations g_shb_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = shb_read,
	.write = shb_write,
	.unlocked_ioctl = shb_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = shb_ioctl,
#endif
	.open = shb_open,
	.release = shb_release,
};

static struct miscdevice g_senorhub_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sensorhub",
	.fops = &g_shb_fops,
};

void add_sensorlist(s32 tag, uint16_t sensor_list_info_id)
{
	ctxhub_info("%s, current sensor count is %d, tag is %d, index is %d\n",
		__func__, g_sensorlist[0], tag, sensor_list_info_id);
	if (g_sensorlist[0] + 1 >= SENSOR_LIST_NUM) {
		ctxhub_err("[%s], tag %d, sensor list array is full, add failed\n",
			__func__, tag);
		return;
	}
	g_sensorlist[++g_sensorlist[0]] = sensor_list_info_id;
}

static int match_sensor_data_report(const struct pkt_header *head)
{
	int tag = inputhub_wrapper_get_app_tag(head);
	int cmd = head->cmd;

	if ((tag >= TAG_SENSOR_BEGIN && tag < TAG_SENSOR_END) && cmd == CMD_DATA_REQ)
		return 1;

	// for customized tag, any cmd will process in customized interface
	if (is_customized_tag(tag))
		return 1;

	return 0;
}

static int match_additional_info_report(const struct pkt_header *head)
{
	return (head->cmd == CMD_CMN_CONFIG_REQ) &&
		(((pkt_subcmd_req_t *)head)->subcmd == SUB_CMD_ADDITIONAL_INFO);
}

static int __init sensorhub_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;

	ret = get_status_from_dts();
	if (ret < 0)
		ctxhub_err("get_status_from_dts() failed!\n");

	init_sensors_get_data();
	init_hash_tables();
	init_have_send_calibrate_data();
	ret = inputhub_route_open(&g_shb_route_table);
	if (ret != 0) {
		ctxhub_err("cannot open inputhub route err=%d\n", ret);
		return ret;
	}

	ret = inputhub_wrapper_register_event_notifier_ex(match_sensor_data_report,
							  inputhub_process_sensor_report);
	if (ret != 0) {
		ctxhub_err("register event notifier ret = %d\n", ret);
		goto out_route_table_close;
	}

	ret = inputhub_wrapper_register_event_notifier_ex(match_additional_info_report,
							  inputhub_process_additional_info_report);
	if (ret != 0) {
		ctxhub_err("register additional event notifier ret = %d\n", ret);
		goto out_unregister_sensor_data;
	}

	ret = misc_register(&g_senorhub_miscdev);
	if (ret != 0) {
		ctxhub_err("cannot register miscdev err=%d\n", ret);
		goto out_unregister_additional_data;
	}

	ctxhub_info("%s ok\n", __func__);
	return 0;

out_unregister_additional_data:
	(void)inputhub_wrapper_unregister_event_notifier_ex(match_additional_info_report,
						      inputhub_process_additional_info_report);

out_unregister_sensor_data:
	(void)inputhub_wrapper_unregister_event_notifier_ex(match_sensor_data_report,
						      inputhub_process_sensor_report);

out_route_table_close:
	inputhub_route_close(&g_shb_route_table);
	return ret;
}

static void __exit sensorhub_exit(void)
{
	(void)inputhub_wrapper_unregister_event_notifier_ex(match_sensor_data_report,
						      inputhub_process_sensor_report);

	(void)inputhub_wrapper_unregister_event_notifier_ex(match_additional_info_report,
						      inputhub_process_additional_info_report);

	inputhub_route_close(&g_shb_route_table);
	misc_deregister(&g_senorhub_miscdev);
}

late_initcall_sync(sensorhub_init);
module_exit(sensorhub_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SensorHub driver");
MODULE_AUTHOR("Smart");
