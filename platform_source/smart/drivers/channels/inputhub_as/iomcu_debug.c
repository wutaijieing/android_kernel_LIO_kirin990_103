/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 * Description:dft channel.c
 * Create:2019.09.22
 */

#include "iomcu_debug.h"

#include <securec.h>

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/slab.h>

#include "dft_channel.h"
#include "common/common.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "iomcu_ipc.h"
#include "platform_include/smart/linux/iomcu_dump.h"
#include "platform_include/smart/linux/iomcu_pm.h"
#include "iomcu_logbuff.h"
#include "iomcu_log.h"
#include "plat_func.h"

static char g_show_str[MAX_STR_SIZE];
static struct class *g_contexthub_debug_class;

static struct t_sensor_debug_operations_list g_sensor_debug_operations_list = {
	.mlock = __MUTEX_INITIALIZER(g_sensor_debug_operations_list.mlock),
	.head = LIST_HEAD_INIT(g_sensor_debug_operations_list.head),
};

static char *g_iomcu_app_id_str[] = {
	[TAG_STEP_COUNTER] = "TAG_STEP_COUNTER",
	[TAG_SIGNIFICANT_MOTION] = "TAG_SIGNIFICANT_MOTION",
	[TAG_STEP_DETECTOR] = "TAG_STEP_DETECTOR",
	[TAG_AR] = "TAG_ACTIVITY",
	[TAG_ORIENTATION] = "TAG_ORIENTATION",
	[TAG_LINEAR_ACCEL] = "TAG_LINEAR_ACCEL",
	[TAG_GRAVITY] = "TAG_GRAVITY",
	[TAG_ROTATION_VECTORS] = "TAG_ROTATION_VECTORS",
	[TAG_GEOMAGNETIC_RV] = "TAG_GEOMAGNETIC_RV",
	[TAG_MOTION] = "TAG_MOTION",
	[TAG_ACCEL] = "TAG_ACCEL",
	[TAG_GYRO] = "TAG_GYRO",
	[TAG_MAG] = "TAG_MAG",
	[TAG_ALS] = "TAG_ALS",
	[TAG_PS] = "TAG_PS",
	[TAG_PRESSURE] = "TAG_PRESSURE",
	[TAG_PDR] = "TAG_PDR",
	[TAG_AR] = "TAG_AR",
	[TAG_FINGERSENSE] = "TAG_FINGERSENSE",
	[TAG_PHONECALL] = "TAG_PHONECALL",
	[TAG_CONNECTIVITY] = "TAG_CONNECTIVITY",
	[TAG_MAG_UNCALIBRATED] = "TAG_MAG_UNCALIBRATED",
	[TAG_GYRO_UNCALIBRATED] = "TAG_GYRO_UNCALIBRATED",
	[TAG_HANDPRESS] = "TAG_HANDPRESS",
	[TAG_CA] = "TAG_CA",
	[TAG_OIS] = "TAG_OIS",
	[TAG_FP] = "TAG_FP",
	[TAG_CAP_PROX] = "TAG_CAP_PROX",
	[TAG_KEY] = "TAG_KEY",
	[TAG_AOD] = "TAG_AOD",
	[TAG_MAGN_BRACKET] = "TAG_MAGN_BRACKET",
	[TAG_CONNECTIVITY_AGENT] = "TAG_CONNECTIVITY_AGENT",
	[TAG_FLP] = "TAG_FLP",
	[TAG_TILT_DETECTOR] = "TAG_TILT_DETECTOR",
	[TAG_RPC] = "TAG_RPC",
	[TAG_FP_UD] = "TAG_FP_UD",
	[TAG_ACCEL_UNCALIBRATED] = "TAG_ACCEL_UNCALIBRATED",
	[TAG_HW_PRIVATE_APP_END] = "TAG_HW_PRIVATE_APP_END",
};

/* to find tag by str */
static const struct sensor_debug_tag_map g_tag_map_tab[] = {
	{ "accel", TAG_ACCEL },
	{ "magnitic", TAG_MAG },
	{ "gyro", TAG_GYRO },
	{ "als_light", TAG_ALS },
	{ "ps_promixy", TAG_PS },
	{ "linear_accel", TAG_LINEAR_ACCEL },
	{ "gravity", TAG_GRAVITY },
	{ "orientation", TAG_ORIENTATION },
	{ "rotationvector", TAG_ROTATION_VECTORS },
	{ "maguncalibrated", TAG_MAG_UNCALIBRATED },
	{ "gamerv", TAG_GAME_RV },
	{ "gyrouncalibrated", TAG_GYRO_UNCALIBRATED },
	{ "significantmotion", TAG_SIGNIFICANT_MOTION },
	{ "stepdetector", TAG_STEP_DETECTOR },
	{ "stepcounter", TAG_STEP_COUNTER },
	{ "geomagnetic", TAG_GEOMAGNETIC_RV },
	{ "airpress", TAG_PRESSURE },
	{ "handpress", TAG_HANDPRESS },
	{ "cap_prox", TAG_CAP_PROX },
	{ "hall", TAG_HALL },
	{ "fault", TAG_FAULT },
	{ "ar", TAG_AR },
	{ "fingersense", TAG_FINGERSENSE },
	{ "fingerprint", TAG_FP },
	{ "key", TAG_KEY },
	{ "aod", TAG_AOD },
	{ "magn_bracket", TAG_MAGN_BRACKET },
	{ "tiltdetector", TAG_TILT_DETECTOR },
	{ "environment", TAG_ENVIRONMENT },
	{ "fingerprint_ud", TAG_FP_UD },
	{ "acceluncalibrated", TAG_ACCEL_UNCALIBRATED },
};


static const char * const g_fault_type_table[] = {
	"hardfault",
	"busfault",
	"memfault",
	"usagefault",
	"rdrdump",
};

void tell_screen_status_to_mcu(uint8_t status)
{
	struct write_info pkg_ap;
	struct read_info pkg_mcu;
	int ret, iom3_power_state, iom3_state;
	pkt_parameter_req_t spkt;
	struct pkt_header *shd = (struct pkt_header *)&spkt;

	(void)memset_s(&pkg_ap, sizeof(pkg_ap), 0, sizeof(pkg_ap));
	(void)memset_s(&pkg_mcu, sizeof(pkg_mcu), 0, sizeof(pkg_mcu));
	(void)memset_s(&spkt, sizeof(spkt), 0, sizeof(spkt));

	iom3_power_state = get_iomcu_power_state();
	iom3_state = get_iom3_state();

	spkt.subcmd = SUB_CMD_SCREEN_STATUS;
	pkg_ap.tag = TAG_POSTURE;
	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.wr_buf = &shd[1]; // buf data from shd buf[1]
	pkg_ap.wr_len = min(sizeof(spkt.para), sizeof(status)) + SUBCMD_LEN;
	ret = memcpy_s(spkt.para, sizeof(spkt.para),
			&status, min(sizeof(spkt.para), sizeof(status)));
	if (ret != EOK) {
		ctxhub_err("[%s], memcpy failed!\n", __func__);
		return;
	}

	ctxhub_info("%s, get screen status = %d\n", __func__, (int)status);
	if ((iom3_state == IOM3_ST_RECOVERY) ||
		(iom3_power_state == ST_SLEEP))
		ret = write_customize_cmd(&pkg_ap, NULL, false);
	else
		ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);

	if (ret) {
		ctxhub_err("send screen status to mcu fail,ret=%d\n", ret);
		return;
	}
	if (pkg_mcu.errno != 0)
		ctxhub_err("screen status send fail\n");
}


#define DFT_ARGC_NUM 2
static int dft_test(int tag, int argv[], int argc)
{
	uint32_t event_id, fush_flag;
	uint64_t fetch_data = 0;

	ctxhub_info("%s: start\n", __func__);

	if (argc != DFT_ARGC_NUM) {
		ctxhub_err("%s argc %d not two\n", __func__, argc);
		return -EINVAL;
	}

	event_id = argv[0];
	fush_flag = argv[1];

	if (fush_flag == 0) {
		iomcu_dft_flush(event_id);
		ctxhub_info("%s iomcu_dft_flush event_id %u flush success\n", __func__, event_id);
	} else {
		(void)iomcu_dft_data_fetch(event_id, &fetch_data, sizeof(fetch_data));
		ctxhub_info("dft test fetch type = %u, res = hi: %u , low: %u\n",
				event_id, (uint32_t)(fetch_data >> 32), (uint32_t)(fetch_data));
	}
	return 0;
}

static int set_fault_type(int tag, int argv[], int argc)
{
	struct write_info pkg_ap;
	uint8_t fault_type;
	int ret;

	if (argc == 0)
		return RET_FAIL;

	fault_type = (uint8_t)argv[0] & 0xFF;
	if (fault_type >= (ARRAY_SIZE(g_fault_type_table))) {
		ctxhub_err("unsupported fault_type : %d\n", fault_type);
		return RET_FAIL;
	}

	(void)memset_s(&pkg_ap, sizeof(struct write_info), 0, sizeof(struct write_info));

	pkg_ap.tag = TAG_FAULT;
	pkg_ap.cmd = CMD_SET_FAULT_TYPE_REQ;
	pkg_ap.wr_buf = &fault_type;
	pkg_ap.wr_len = sizeof(fault_type);

	ctxhub_info("%s, %s, fault type:%s\n",
		__func__, get_tag_str(TAG_FAULT), g_fault_type_table[fault_type]);
	ret = write_customize_cmd(&pkg_ap, NULL, true);
	if (ret != 0) {
		ctxhub_err("set fault type %s failed, ret = %d in %s\n",
			g_fault_type_table[fault_type], ret, __func__);
		return RET_FAIL;
	}
	ctxhub_info("set fault type %s success\n", g_fault_type_table[fault_type]);
	return 0;
}

static int set_fault_addr(int tag, int argv[], int argc)
{
	struct write_info pkg_ap;
	int ret;
	pkt_fault_addr_req_t cpkt;
	struct pkt_header *hd = (struct pkt_header *)&cpkt;

	if (argc == 0)
		return RET_FAIL;

	(void)memset_s(&pkg_ap, sizeof(struct write_info), 0, sizeof(struct write_info));

	pkg_ap.tag = TAG_FAULT;
	pkg_ap.cmd = CMD_SET_FAULT_ADDR_REQ;
	cpkt.wr = (unsigned int)argv[0] & 0xFF;
	cpkt.fault_addr = argv[1];
	pkg_ap.wr_buf = &hd[1];
	pkg_ap.wr_len = 5; /* length is 5 */

	ret = write_customize_cmd(&pkg_ap, NULL, true);
	if (ret != 0) {
		ctxhub_err("set fault addr, read/write: %d, 0x%x failed, ret = %d in %s\n",
			argv[0], argv[1], ret, __func__);
		return RET_FAIL;
	}
	ctxhub_info("set fault addr,  read/write: %d, fault addr: 0x%x success\n",
		argv[0], argv[1]);
	return 0;
}

#define IOMCU_DBG_MAX_ARGC    7
#define IOMCU_DBG_MIN_ARGC    1

static int iomcu_dbg(int tag, int argv[], int argc)
{
	struct write_info pkg_ap;
	uint32_t *send = NULL;
	uint32_t send_len;
	uint32_t i;
	int ret;

	if ((uint32_t)argc > IOMCU_DBG_MAX_ARGC || argc < IOMCU_DBG_MIN_ARGC) {
		ctxhub_err("%s : invalid arg number %x.\n", __func__, argc);
		return RET_FAIL;
	}

	send_len = sizeof(uint32_t) * (argc - 1);
	send = kzalloc(send_len, GFP_ATOMIC);
	if (send == NULL) {
		ctxhub_err("%s: malloc send buffer failed. [%x]\n", __func__, ret);
		return RET_FAIL;
	}

	for (i = 0; i < (argc - 1); i++)
		send[i] = argv[i + 1];

	(void)memset_s(&pkg_ap, sizeof(struct write_info), 0, sizeof(struct write_info));

	pkg_ap.tag = TAG_DBG;
	pkg_ap.cmd = argv[0];
	pkg_ap.wr_buf = send;
	pkg_ap.wr_len = send_len;

	ctxhub_info("%s: argc %x\n", __func__, argc);
	ret = write_customize_cmd(&pkg_ap, NULL, true);
	if (ret != 0) {
		ctxhub_err("%s: failed [%x]\n", __func__, ret);
		kfree(send);
		return RET_FAIL;
	}

	kfree(send);
	ctxhub_info("%s: success\n", __func__);
	return 0;
}

static int register_sensorhub_debug_op_and_opstr(const char *func_name,
	sensor_debug_pfunc op)
{
	struct sensor_debug_cmd *node = NULL, *n = NULL;
	int ret = 0;

	if (!func_name || !op) {
		ctxhub_err("error in %s\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_sensor_debug_operations_list.mlock);
	list_for_each_entry_safe(node, n,
		&g_sensor_debug_operations_list.head, entry) {
		if (op == node->operation) {
			ctxhub_warn("%s has already registed in %s\n!",
				func_name, __func__);
			goto out; /* return when already registed */
		}
	}
	node = kzalloc(sizeof(*node), GFP_ATOMIC);
	if (node == NULL) {
		ctxhub_err("alloc failed in %s\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	node->str = func_name;
	node->operation = op;
	list_add_tail(&node->entry, &g_sensor_debug_operations_list.head);
out:
	mutex_unlock(&g_sensor_debug_operations_list.mlock);
	return ret;
}

static int unregister_sensorhub_debug_op_and_opstr(sensor_debug_pfunc op)
{
	struct sensor_debug_cmd *pos = NULL, *n = NULL;

	if (!op) {
		ctxhub_err("error in %s\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_sensor_debug_operations_list.mlock);
	list_for_each_entry_safe(pos, n,
		&g_sensor_debug_operations_list.head, entry) {
		if (op == pos->operation) {
			list_del(&pos->entry);
			kfree(pos);
			break;
		}
	}
	mutex_unlock(&g_sensor_debug_operations_list.mlock);
	return 0;
}

static void register_debug_operations(void)
{
	register_sensorhub_debug_operation(set_fault_type);
	register_sensorhub_debug_operation(set_fault_addr);
	register_sensorhub_debug_operation(set_log_level);
	register_sensorhub_debug_operation(dft_test);
	register_sensorhub_debug_operation(iomcu_dbg);
}

static void unregister_debug_operations(void)
{
	unregister_sensorhub_debug_operation(set_fault_type);
	unregister_sensorhub_debug_operation(set_fault_addr);
	unregister_sensorhub_debug_operation(set_log_level);
	unregister_sensorhub_debug_operation(dft_test);
	unregister_sensorhub_debug_operation(iomcu_dbg);
}

/*fuzzy matching*/
static bool str_fuzzy_match(const char *cmd_buf, const char *target)
{
	if (!cmd_buf || !target)
		return false;

	for (; !is_space_ch(*cmd_buf) && !end_of_string(*cmd_buf) && *target;
		++target) {
		if (*cmd_buf == *target)
			++cmd_buf;
	}

	return is_space_ch(*cmd_buf) || end_of_string(*cmd_buf);
}

static sensor_debug_pfunc get_operation(const char *str)
{
	sensor_debug_pfunc op = NULL;
	struct sensor_debug_cmd *node = NULL, *n = NULL;

	mutex_lock(&g_sensor_debug_operations_list.mlock);
	list_for_each_entry_safe(node, n,
		&g_sensor_debug_operations_list.head, entry) {
		if (str_fuzzy_match(str, node->str)) {
			op = node->operation;
			break;
		}
	}
	mutex_unlock(&g_sensor_debug_operations_list.mlock);
	return op;
}

static int get_sensor_tag(const char *str)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_tag_map_tab); ++i) {
		if (str_fuzzy_match(str, g_tag_map_tab[i].str))
			return g_tag_map_tab[i].tag;
	}
	return RET_FAIL;
}

static void parse_str(const char *cmd_buf)
{
	sensor_debug_pfunc operation = NULL;
	int tag = -1;
	int arg = -1;
	int argv[MAX_CMD_BUF_ARGC] = { 0 };
	int argc = 0;

	for (; (cmd_buf = get_str_begin(cmd_buf)) != NULL;
		cmd_buf = get_str_end(cmd_buf)) {
		if (!operation)
			operation = get_operation(cmd_buf);

		if (tag == -1)
			tag = get_sensor_tag(cmd_buf);

		if (get_arg(cmd_buf, &arg)) {
			if (argc < MAX_CMD_BUF_ARGC)
				argv[argc++] = arg;
			else
				ctxhub_err("too many args, %d will be ignored\n",
					arg);
		}
	}

	if (operation != NULL)
		operation(tag, argv, argc);
}

static ssize_t cls_attr_debug_show_func(struct class *cls,
	struct class_attribute *attr, char *buf)
{
	unsigned int i;
	unsigned int offset = 0;
	int ret;
	struct sensor_debug_cmd *node = NULL, *n = NULL;

	ret = sprintf_s(buf + offset, PAGE_SIZE - offset,
		"operations format: echo operation tag optarg > %s\n"
		"for example:\n"
		"    to open accel:           echo open_sensor accel > %s\n"
		"    to setdelay accel 100ms: echo set_delay accel 100 > %s\n"
		"\noperations supported as follow:\n",
		attr->attr.name, attr->attr.name, attr->attr.name);
	if (ret < 0) {
		ctxhub_err("[%s], sprintf_s failed!\n", __func__);
		return 0;
	}
	offset += ret;

	mutex_lock(&g_sensor_debug_operations_list.mlock);
	list_for_each_entry_safe(node, n,
		&g_sensor_debug_operations_list.head, entry) {
		ret = sprintf_s(buf + offset, PAGE_SIZE - offset, "%s\n", node->str);
		if (ret < 0) {
			ctxhub_err("[%s], sprintf_s failed!\n", __func__);
			mutex_unlock(&g_sensor_debug_operations_list.mlock);
			return 0;
		}
		offset += ret;
	}
	mutex_unlock(&g_sensor_debug_operations_list.mlock);

	ret = sprintf_s(buf + offset, PAGE_SIZE - offset,
		"\ntags supported as follow:\n");
	if (ret < 0) {
		ctxhub_err("[%s], sprintf_s failed!\n", __func__);
		return 0;
	}
	offset += ret;

	for (i = 0; i < ARRAY_SIZE(g_tag_map_tab); ++i) {
		ret = sprintf_s(buf + offset, PAGE_SIZE - offset, "%s\n",
			g_tag_map_tab[i].str);
		if (ret < 0) {
			ctxhub_err("[%s], sprintf_s failed!\n", __func__);
			return 0;
		}
		offset += ret;
	}

	return offset;
}

static ssize_t cls_attr_debug_store_func(struct class *cls,
	struct class_attribute *attr, const char *buf, size_t size)
{
	parse_str(buf);
	return size;
}

static struct class_attribute g_class_attr_sensorhub_dbg =
	__ATTR(sensorhub_dbg, 0660, cls_attr_debug_show_func,
	cls_attr_debug_store_func); // access permission

enum dbg_channel_stat {
	NO_TEST,
	TESTING,
	TEST_FINI_SUC,
	TEST_FINI_ERR,
	TEST_END,
};

static const char *const g_dbg_channel_test_stat_str[] = {
	"not in test",
	"testing",
	"test finish with success",
	"test finish with fail",
	"test not available",
};

enum user_cmd_para {
	USER_CMD_TAG,
	USER_CMD_APP_TAG,
	USER_CMD_CMD,
	USER_CMD_SUBCMD,
	USER_CMD_WRITE_LEN,
	USER_CMD_RESP,
};

struct user_cmd_t {
	struct write_info wr;
	uint32_t subcmd;
	int resp;
	enum dbg_channel_stat test_state;
	struct read_info rd;
};

#define MAX_CMD_NUM 6
#define MAX_DATA_BUF_SIZE 136
struct dbg_channel_wbuf {
	uint32_t subcmd;
	uint8_t buf[MAX_DATA_BUF_SIZE];
};
static DEFINE_MUTEX(g_dbg_channel_mutex);

static struct dbg_channel_wbuf g_dbg_channel_wbuf;
static struct user_cmd_t g_dbg_channel_usercmd;

#define dbg_chn_attr_show(ret, buf, offset, fmt, ...) \
do { \
	ret = sprintf_s(buf + offset, PAGE_SIZE - offset, fmt, ##__VA_ARGS__); \
	if (ret < 0) \
		return 0; \
	offset += ret; \
} while (0)

static ssize_t dbg_channel_show_func(struct class *cls,
	struct class_attribute *attr, char *buf)
{
	int ret, i;
	unsigned int offset = 0;
	struct user_cmd_t *ucmd = (struct user_cmd_t *)&g_dbg_channel_usercmd;
	struct write_info *wr = &ucmd->wr;
	int test_stat;

	dbg_chn_attr_show(ret, buf, offset,
		"help: format: echo tag app_tag cmd subcmd len resp > %s\n",
		attr->attr.name);

	mutex_lock(&g_dbg_channel_mutex);
	test_stat = ucmd->test_state;
	if (test_stat < NO_TEST || test_stat >= TEST_END)
		test_stat = TEST_END;

	dbg_chn_attr_show(ret, buf, offset,
		"test state: %s\n", g_dbg_channel_test_stat_str[test_stat]);

	if (test_stat != NO_TEST && test_stat != TEST_END) {
		dbg_chn_attr_show(ret, buf, offset,
			"test param: tag=%d, app_tag=%d, cmd=%d, subcmd=%u buflen=%u, resp=%d\n",
			wr->tag, wr->app_tag, wr->cmd, ucmd->subcmd,
			wr->wr_len - sizeof(g_dbg_channel_wbuf.subcmd), ucmd->resp);

		if (ucmd->rd.data_length <= 0 || ucmd->rd.data_length >= MAX_PKT_LENGTH)
			goto OUT;

		dbg_chn_attr_show(ret, buf, offset, "get data:\n");

		for (i = 0; i < ucmd->rd.data_length; i++) {
			dbg_chn_attr_show(ret, buf, offset, "%x ", ucmd->rd.data[i]);
			if (i % 10 == 0)
				dbg_chn_attr_show(ret, buf, offset,  "\n");
		}
		dbg_chn_attr_show(ret, buf, offset,  "\n");
	}

OUT:
	mutex_unlock(&g_dbg_channel_mutex);
	return offset;
}

// ret 0 suc: -1 err
static int parse_cmd(const char *buf, size_t size, struct user_cmd_t *ucmd)
{
	int arg = -1;
	int argc = 0;
	int argv[MAX_CMD_NUM] = {0};

	for (; (buf = get_str_begin(buf)) != NULL; buf = get_str_end(buf)) {
		if (get_arg(buf, &arg)) {
			argv[argc++] = arg;
			if (argc == MAX_CMD_NUM)
				break;
		}
	}

	if (argc < MAX_CMD_NUM)
		return -1;

	ucmd->wr.tag = argv[USER_CMD_TAG];
	ucmd->wr.app_tag = argv[USER_CMD_APP_TAG];
	ucmd->wr.cmd = argv[USER_CMD_CMD];
	ucmd->subcmd = argv[USER_CMD_SUBCMD];
	ucmd->wr.wr_len = argv[USER_CMD_WRITE_LEN];
	if (ucmd->wr.wr_len  < 0)
		ucmd->wr.wr_len = 0;
	if (ucmd->wr.wr_len >= MAX_DATA_BUF_SIZE)
		ucmd->wr.wr_len = MAX_DATA_BUF_SIZE;
	ucmd->resp = argv[USER_CMD_RESP];

	return 0;
}

static ssize_t dbg_channel_store_func(struct class *cls,
	struct class_attribute *attr, const char *buf, size_t size)
{
	struct user_cmd_t *ucmd = &g_dbg_channel_usercmd;
	struct write_info *wr = &ucmd->wr;
	struct read_info *p_rd = NULL;
	int i;
	int test_result = 0;

	if (get_sensor_mcu_mode() != 1) {
		ctxhub_err("mcu not ready\n");
		return size;
	}

	mutex_lock(&g_dbg_channel_mutex);

	(void)memset_s(&g_dbg_channel_usercmd, sizeof(struct user_cmd_t),
			0, sizeof(struct user_cmd_t));
	(void)memset_s(&g_dbg_channel_wbuf, sizeof(struct dbg_channel_wbuf),
			0, sizeof(struct dbg_channel_wbuf));
	// get use cmd
	if (parse_cmd(buf, size, ucmd)) {
		ctxhub_err("param err: should be tag, app_tag, cmd, subcmd, resp, len\n");
		test_result = 1;
		goto OUT;
	}

	ctxhub_info("%s: write cmd: tag=%d, app_tag=%d, cmd=%d, subcmd=%u buflen=%d, resp=%d\n",
		    __func__, wr->tag, wr->app_tag, wr->cmd, ucmd->subcmd, wr->wr_len, ucmd->resp);

	ucmd->test_state = TESTING;

	// fill config buff, 0, 1, 2...
	if (wr->wr_len > 0) {
		for (i = 0; i < wr->wr_len; i++)
			g_dbg_channel_wbuf.buf[i] = i;
	}

	g_dbg_channel_wbuf.subcmd = ucmd->subcmd;
	wr->wr_len += sizeof(g_dbg_channel_wbuf.subcmd);
	wr->wr_buf = &g_dbg_channel_wbuf;

	if (ucmd->resp)
		p_rd = &ucmd->rd;

	// send cmd
	if (write_customize_cmd(wr, p_rd, false) != 0) {
		ctxhub_err("%s write cmd failed\n", __func__);
		test_result = 1;
		goto OUT;
	}

OUT:
	ucmd->test_state = TEST_FINI_SUC;
	if (test_result != 0 || (p_rd && p_rd->errno != 0))
		ucmd->test_state = TEST_FINI_ERR;
	mutex_unlock(&g_dbg_channel_mutex);
	ctxhub_info("%s finished\n", __func__);

	return size;
}


static struct class_attribute g_class_attr_dbg_channel =
	__ATTR(dbg_channel, 0660, dbg_channel_show_func,
	dbg_channel_store_func);

static ssize_t cls_attr_dump_show_func(struct class *cls,
	struct class_attribute *attr, char *buf)
{
	ctxhub_info("read sensorhub_dump node, IOM7 will restart\n");
	iom3_need_recovery(SENSORHUB_USER_MODID, SH_FAULT_USER_DUMP);
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1,
		"read sensorhub_dump node, IOM7 will restart\n");
}

static struct class_attribute g_class_attr_sensorhub_dump =
	__ATTR(sensorhub_dump, 0660, cls_attr_dump_show_func, NULL);

static ssize_t cls_attr_tell_mcu_streen_store_func(struct class *cls,
	struct class_attribute *attr, const char *buf, size_t size)
{
	long screen_status;

	if (buf == NULL)
		return size;

	if (kstrtol(buf, 10, &screen_status) < 0) {
		ctxhub_warn("%s strtol failed\n", __func__);
		return size;
	}

	ctxhub_info("get screen info = %ld\n", screen_status);
	tell_screen_status_to_mcu((uint8_t)screen_status);
	return size;
}

static struct class_attribute g_class_attr_dual_screen_status =
	__ATTR(dual_screen_status, 0660, NULL,
	cls_attr_tell_mcu_streen_store_func);

static ssize_t show_cpu_usage(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t fetch_data;
	int ret;

	if (buf == NULL) {
		ctxhub_err("%s: buf is NULL\n", __func__);
		return -EINVAL;
	}

	ret = iomcu_dft_data_fetch(DFT_EVENT_CPU_USE, &fetch_data,
		sizeof(uint32_t));
	if (ret != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1,
			"dft date fetch fail\n");

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%u\n",
		fetch_data);
}
static DEVICE_ATTR(cpu_usage, 0660, show_cpu_usage, NULL);

#define MEMRY_INFO_LEN 5
static ssize_t show_memory_usage(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t data[MEMRY_INFO_LEN];
	int ret;

	if (buf == NULL) {
		ctxhub_err("%s: buf is NULL\n", __func__);
		return -EINVAL;
	}

	ret = iomcu_dft_data_fetch(DFT_EVENT_MEM_USE, data, sizeof(data));
	if (ret != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1,
			"dft date fetch fail\n");

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1,
		"totalSize %u usedSize %u freeSize %u allocCount %u freeCount %u\n",
		data[0], data[1], data[2], data[3], data[4]);
}
static DEVICE_ATTR(memory_usage, 0660, show_memory_usage, NULL);


static const char *get_iomcu_power_status(void)
{
	int ret;
	struct iomcu_power_status status;
	(void)memset_s(g_show_str, MAX_STR_SIZE, 0, MAX_STR_SIZE);

	atomic_get_iomcu_power_status(&status);

	switch (status.power_status) {
	case SUB_POWER_ON:
		ret = snprintf_s(g_show_str, MAX_STR_SIZE, MAX_STR_SIZE - 1,
			"%s", "SUB_POWER_ON");
		if (ret < 0) {
			ctxhub_err("[%s], snprintf failed\n", __func__);
			return "error";
		}
		break;
	case SUB_POWER_OFF:
		ret = snprintf_s(g_show_str, MAX_STR_SIZE, MAX_STR_SIZE - 1,
			"%s", "SUB_POWER_OFF");
		if (ret < 0) {
			ctxhub_err("[%s], snprintf failed\n", __func__);
			return "error";
		}
		break;
	default:
		ret = snprintf_s(g_show_str, MAX_STR_SIZE, MAX_STR_SIZE - 1,
			"%s", "unknown status");
		if (ret < 0) {
			ctxhub_err("[%s], snprintf failed\n", __func__);
			return "error";
		}
		break;
	}
	return g_show_str;
}

static const char *get_iomcu_current_opened_app(void)
{
	int i;
	char buf[SINGLE_STR_LENGTH_MAX] = {0};
	int copy_length;
	char *tag_str = NULL;
	int ret;
	struct iomcu_power_status status;
	(void)memset_s(g_show_str, MAX_STR_SIZE, 0, MAX_STR_SIZE);

	atomic_get_iomcu_power_status(&status);

	for (i = 0; i < TAG_END; i++) {
		(void)memset_s(buf, SINGLE_STR_LENGTH_MAX, 0,
			SINGLE_STR_LENGTH_MAX);

		/* for opened apps */
		if (status.app_status[i]) {
			tag_str = get_tag_str(i);
			if (tag_str != NULL) {
				copy_length = strlen(tag_str);
				ret = strncpy_s(buf, SINGLE_STR_LENGTH_MAX,
					tag_str, copy_length);
				if (ret != EOK) {
					ctxhub_err("[%s] strcpy %s failed\n", __func__, tag_str);
					continue;
				}
			} else {
				copy_length = 3;
				ret = snprintf_s(buf, SINGLE_STR_LENGTH_MAX, 3, "%3d", i);
				if (ret < 0) {
					ctxhub_err("[%s] snprintf %d failed\n", __func__, i);
					continue;
				}
			}

			buf[copy_length] = '\n';
			ret = strcat_s(g_show_str, MAX_STR_SIZE, buf);
			if (ret != EOK) {
				ctxhub_err("[%s] strcat failed\n", __func__);
				g_show_str[MAX_STR_SIZE - 1] = 'X';
				break;
			}
		}
	}
	return g_show_str;
}

static int get_iomcu_idle_time(void)
{
	struct iomcu_power_status status;

	atomic_get_iomcu_power_status(&status);

	return status.idle_time;
}

static const char *get_iomcu_active_app_during_suspend(void)
{
	int i;
	char buf[SINGLE_STR_LENGTH_MAX] = {0};
	int tf;
	uint64_t bit_map;
	int copy_length;
	int ret;
	struct iomcu_power_status status;

	(void)memset_s(g_show_str, MAX_STR_SIZE, 0, MAX_STR_SIZE);

	atomic_get_iomcu_power_status(&status);
	bit_map = status.active_app_during_suspend;

	for (i = 0; i < TAG_HW_PRIVATE_APP_END; i++) {
		(void)memset_s(buf, SINGLE_STR_LENGTH_MAX, 0, SINGLE_STR_LENGTH_MAX);
		tf = (bit_map >> i) & 0x01;
		if (tf) {
			if (g_iomcu_app_id_str[i] != NULL) {
				copy_length = strlen(g_iomcu_app_id_str[i]);
				ret = strncpy_s(buf, SINGLE_STR_LENGTH_MAX,
					g_iomcu_app_id_str[i], copy_length);
				if (ret != EOK) {
					ctxhub_err("[%s] strcpy failed\n", __func__);
					continue;
				}
			} else {
				copy_length = 3;
				ret = snprintf_s(buf, SINGLE_STR_LENGTH_MAX, 3, "%3d", i);
				if (ret < 0) {
					ctxhub_err("[%s] snprintf failed\n", __func__);
					continue;
				}
			}
			buf[copy_length] = '\n';
			ret = strcat_s(g_show_str, MAX_STR_SIZE, buf);
			if (ret != EOK) {
				ctxhub_err("[%s] store to g_show_str failed\n", __func__);
				g_show_str[MAX_STR_SIZE - 1] = 'X';
				break;
			}
		}
	}
	return g_show_str;
}

static ssize_t show_power_status(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%s\n",
		get_iomcu_power_status());
}

static ssize_t show_app_status(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%s\n",
		get_iomcu_current_opened_app());
}
static ssize_t show_idle_time(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d\n",
		get_iomcu_idle_time());
}

static ssize_t show_active_app_during_suspend(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%s\n",
		get_iomcu_active_app_during_suspend());
}

static DEVICE_ATTR(power_status, 0660, show_power_status, NULL);
static DEVICE_ATTR(current_app, 0660, show_app_status, NULL);
static DEVICE_ATTR(idle_time, 0660, show_idle_time, NULL);
static DEVICE_ATTR(active_app_during_suspend, 0660,
	show_active_app_during_suspend, NULL);

static struct attribute *g_power_info_attrs[] = {
	&dev_attr_power_status.attr,
	&dev_attr_current_app.attr,
	&dev_attr_idle_time.attr,
	&dev_attr_active_app_during_suspend.attr,
	&dev_attr_cpu_usage.attr,
	&dev_attr_memory_usage.attr,
	NULL,
};

static const struct attribute_group g_power_info_attrs_grp = {
	.attrs = g_power_info_attrs,
};

static struct power_dbg g_power_info = {
	 .name = "power_info",
	 .attrs_group = &g_power_info_attrs_grp,
};

static void create_cls_files(void)
{
	int ret;

	ret = class_create_file(g_contexthub_debug_class,
				&g_class_attr_sensorhub_dbg);
	if (ret != 0)
		ctxhub_err(" %s creat cls file sensorhub dbg fail\n",
								__func__);

	ret = class_create_file(g_contexthub_debug_class,
				&g_class_attr_dbg_channel);
	if (ret != 0)
		ctxhub_err(" %s creat cls file dbg chl fail\n", __func__);

	ret = class_create_file(g_contexthub_debug_class,
				&g_class_attr_sensorhub_dump);
	if (ret != 0)
		ctxhub_err(" %s creat cls file sensorhub dump fail\n",
								__func__);

	ret = class_create_file(g_contexthub_debug_class,
				&g_class_attr_dual_screen_status);
	if (ret != 0)
		ctxhub_err(" %s creat cls file dual screen status fail\n",
								__func__);
}

static int iomcu_debug_init(void)
{
	g_contexthub_debug_class = class_create(THIS_MODULE, "contexthub_debug");
	if (IS_ERR(g_contexthub_debug_class)) {
		ctxhub_err(" %s class creat fail\n", __func__);
		return RET_FAIL;
	}

	create_cls_files();
	g_power_info.dev = device_create(g_contexthub_debug_class, NULL, 0,
		&g_power_info, g_power_info.name);
	if (!(g_power_info.dev)) {
		ctxhub_err(" %s creat dev fail\n", __func__);
		class_destroy(g_contexthub_debug_class);
		return RET_FAIL;
	}

	if (g_power_info.attrs_group) {
		if (sysfs_create_group(&g_power_info.dev->kobj,
			g_power_info.attrs_group)) {
			ctxhub_err("create files failed in %s\n", __func__);
		} else {
			ctxhub_info("%s ok\n", __func__);
			return 0;
		}
	} else {
		ctxhub_err("power_info.attrs_group is null\n");
	}

	device_destroy(g_contexthub_debug_class, 0);
	class_destroy(g_contexthub_debug_class);
	g_contexthub_debug_class = NULL;
	return RET_FAIL;
}

static void iomcu_debug_exit(void)
{
	device_destroy(g_contexthub_debug_class, 0);
	class_destroy(g_contexthub_debug_class);
	g_contexthub_debug_class = NULL;
}

static int contexthub_debug_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return RET_FAIL;
	register_debug_operations();
	ret = iomcu_debug_init();
	if (ret != 0) {
		ctxhub_err("iomcu debug init failed!\n");
		return RET_FAIL;
	}
	return 0;
}

static void contexthub_debug_exit(void)
{
	unregister_debug_operations();
	iomcu_debug_exit();
}

late_initcall_sync(contexthub_debug_init);
module_exit(contexthub_debug_exit);


MODULE_AUTHOR("SensorHub");
MODULE_DESCRIPTION("SensorHub debug driver");
MODULE_ALIAS("platform:contexthub"MODULE_NAME);
MODULE_LICENSE("GPL v2");
