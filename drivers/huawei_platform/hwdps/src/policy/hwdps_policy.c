/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
 * Description: Hwdps Policy Engin.
 * Create: 2020-06-16
 */

#include "inc/policy/hwdps_policy.h"
#include <linux/hashtable.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <securec.h>
#include "inc/base/hwdps_utils.h"
#include "inc/data/hwdps_data.h"

#define MAX_POLICY_BUF_LEN 256

static bool parse_ruleset_jduge(const struct ruleset_cache_entry_t *parsed_rule)
{
	if (!parsed_rule || !parsed_rule->path_node ||
		(parsed_rule->path_node_len == 0))
		return false;
	if ((parsed_rule->path_node[0] != '/') ||
		strstr(parsed_rule->path_node, ".."))
		return false;
	if (parsed_rule->path_node[parsed_rule->path_node_len - 1] != '/')
		return false;
	return true;
}

/*
 * This function parse rule to jduge path node is valid.
 * @return true for success
 */
static bool parse_ruleset(const s8 *ruleset_json_buffer,
	struct ruleset_cache_entry_t *parsed_rule)
{
	if (memset_s(parsed_rule, sizeof(struct ruleset_cache_entry_t), 0,
		sizeof(struct ruleset_cache_entry_t)) != EOK)
		return false;
	parsed_rule->path_node = hwdps_utils_get_json_str(ruleset_json_buffer,
		"path", &parsed_rule->path_node_len);
	if (parse_ruleset_jduge(parsed_rule))
		return true;

	kfree(parsed_rule->path_node);
	if (memset_s(parsed_rule, sizeof(struct ruleset_cache_entry_t), 0,
		sizeof(struct ruleset_cache_entry_t)) != EOK)
		hwdps_pr_err("memset_s fail, should never happen\n");
	return false;
}

s32 hwdps_analys_policy_ruleset(struct ruleset_cache_entry_t *ruleset,
	struct hwdps_package_info_t *pinfo)
{
	if (!ruleset || !pinfo)
		return -EINVAL;
	return (parse_ruleset(pinfo->app_policy, ruleset)) ? 0 : -EINVAL;
}

static bool calcu_user_id(const s8 *filename, u32 *cnt)
{
	const s8 *p = NULL;
	p = filename;
	while (*p != '\0' && (*p <= '9' && *p >= '0')) {
		(*cnt)++;
		p++;
	}
	if (*p != '/')
		return false;
	return true;
}

static bool do_path_node_matches_rule(const s8 *filename, u32 filename_len,
	const s8 *path_node, u32 path_node_len)
{
	/*
	 * Maybe path_node is data/user/XX/XXX
	 * filename is /user/XX/XXX
	 * For example path_node:data/user/0/XXX, filename:/user/10/XXX
	 */
	u32 sub_cnt = 0;
	u32 primary_cnt = 0;
	u32 sub_user_file_path_len = strlen(SUB_USER_FILE_PATH);
	u32 primary_path_node_len = strlen(PATH_NODE);
	const s8 *p = NULL;

	if (strlen(filename) < sub_user_file_path_len)
		return false;
	p = filename + sub_user_file_path_len;
	if (!calcu_user_id(p, &sub_cnt))
		return false;

	p = path_node + primary_path_node_len;
	if (!calcu_user_id(p, &primary_cnt))
		return false;

	if ((path_node_len - (primary_path_node_len + primary_cnt + 1)) >
		(filename_len - (sub_user_file_path_len + sub_cnt + 1)))
		return false;

	if (strncmp(path_node + primary_path_node_len + primary_cnt + 1,
		filename + sub_user_file_path_len + sub_cnt + 1,
		path_node_len -
		(primary_path_node_len + primary_cnt + 1)) != 0) {
		hwdps_pr_err("filename %s not match pathnode %s\n",
			filename, path_node);
		return false;
	}
	return true;
}

static bool do_path_node_matches_rule_for_primary(const s8 *filename,
	u32 filename_len, const s8 *path_node, u32 path_node_len)
{
	/*
	 * Maybe path_node is data/user/10/XXX,
	 * filename is /data/XXX.
	 */
	u32 count_num = 0;
	u32 primary_user_file_path_len = strlen(PRIMARY_USER_FILE_PATH);
	u32 base_path_node_len = strlen(PATH_NODE);
	const s8 *p = NULL;

	if (strlen(filename) < primary_user_file_path_len)
		return false;
	p = path_node + base_path_node_len;
	if (!calcu_user_id(p, &count_num))
		return false;
	if ((path_node_len - base_path_node_len - count_num - 1) >
		(filename_len - primary_user_file_path_len))
		return false;
	if (strncmp(path_node + base_path_node_len + count_num + 1, filename +
		primary_user_file_path_len,
		path_node_len - base_path_node_len - count_num - 1) != 0) {
		hwdps_pr_err("filename %s not match pathnode %s\n",
			filename, path_node);
		return false;
	}
	return true;
}

static bool path_node_matches_rule(const s8 *filename, u32 filename_len,
	const s8 *path_node, u32 path_node_len)
{
	u32 primary_user_file_path_len = strlen(PRIMARY_USER_FILE_PATH);
	u32 sub_user_file_path_len = strlen(SUB_USER_FILE_PATH);
	u32 base_path_node_len = strlen(PATH_NODE);
	/*
	 * sub-user filename is /user/10/XXX, path_node is /data/user/10/XXX;
	 * primary-user filename is /data/XXX, path_node is /data/user/0/XXX.
	 */
	bool sub_check_filename = (filename_len > sub_user_file_path_len) &&
		(strncmp(SUB_USER_FILE_PATH,
			filename, sub_user_file_path_len) == 0);
	bool check_path = (path_node_len > base_path_node_len) &&
		(strncmp(PATH_NODE, path_node,
			base_path_node_len) == 0);
	bool primary_check_filename =
		(filename_len > primary_user_file_path_len) &&
		(strncmp(PRIMARY_USER_FILE_PATH, filename,
			primary_user_file_path_len) == 0);

	if (sub_check_filename && check_path) {
		if (strncmp(path_node + base_path_node_len, filename +
			sub_user_file_path_len,
			path_node_len - base_path_node_len) == 0) {
			return true;
		}
		return do_path_node_matches_rule(filename, filename_len,
			path_node, path_node_len);
	}

	if (primary_check_filename && check_path) {
		return do_path_node_matches_rule_for_primary(filename,
			filename_len, path_node, path_node_len);
	}
	return false;
}

static bool jduge_filename_valid(const s8 *fsname, const s8 *filename)
{
	if ((strcmp("f2fs", fsname) != 0) && (strcmp("hmfs", fsname) != 0)) {
		hwdps_pr_err("fsname err [%s]\n", fsname);
		return false;
	}
	if (strstr(filename, "../"))
		return false; /* make sure we are not under filepath attact */
	return true;
}

static bool file_matches_rule_path_node(
	const struct ruleset_cache_entry_t *rule,
	const s8 *filename_part, size_t filename_len, const s8 *filename)
{
	/* Check that the filename has a '/' in it. */
	if (filename_part) {
		/* Check that the initial path matches the filename path. */
		if (path_node_matches_rule(filename, filename_len,
			rule->path_node, rule->path_node_len) != true) {
			hwdps_pr_err("failed filename %s path_node %s\n",
				filename, rule->path_node);
			return false;
		}
	} else {
		hwdps_pr_err("file_matches_rule filename_part empty\n");
		return false;
	}
	return true;
}

/*
 * Json rule tags match.
 *
 * "rule" - match on the pathname
 * "fsname" - exact match on the filesystem identification
 * "filename" - exact match on the file name
 */
static bool file_matches_rule(const struct ruleset_cache_entry_t *rule,
	const s8 *fsname, const s8 *filename)
{
	size_t filename_len;
	const s8 *filename_part = NULL;

	if (!rule || !fsname || !filename) {
		hwdps_pr_err("%s bad arg\n", __func__);
		return false;
	}
	if (!jduge_filename_valid(fsname, filename))
		return false;
	filename_part = strrchr(filename, '/');
	filename_len = strlen(filename);
	if (rule->path_node) {
		if (!file_matches_rule_path_node(rule, filename_part,
			filename_len, filename))
			return false;
	}
	return true;
}

void hwdps_utils_free_policy_ruleset(
	struct ruleset_cache_entry_t *ruleset)
{
	if (!ruleset)
		return;
	kfree(ruleset->path_node);
	ruleset->path_node = NULL;
}

bool hwdps_evaluate_policies(struct package_info_listnode_t *pkg_info_node,
	const u8 *fsname, const u8 *filename)
{
	bool matched = false;

	if (!pkg_info_node)
		return false;
	if (file_matches_rule(pkg_info_node->ruleset_cache, fsname, filename)) {
		matched = true;
		hwdps_pr_info("%s match\n", filename);
	}

	return matched;
}
