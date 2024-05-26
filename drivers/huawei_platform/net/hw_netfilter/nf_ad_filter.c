/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2020. All rights reserved.
 * Description: This file is used by the netfilter hook for AD filter.
 * Author: chenzhongxian@huawei.com
 * Create: 2016-05-28
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h> /* add for log */
#include <linux/ctype.h> /* add for tolower */
#include <linux/slab.h>
#include <linux/inet.h>
#include <linux/jhash.h>
#include <securec.h>

#include "nf_hw_common.h"
#include "nf_ad_filter.h"
struct ad_url {
	struct list_head list;
	char *url;
	int len;
};

struct ad_rule {
	struct list_head list;
	unsigned int uid;
	struct ad_url urllist;
};
struct list_info g_adlist_info ;
struct list_info g_deltalist_info;

#define MAX_RECENT_DROP 128

static spinlock_t g_droplock;
static struct ad_url g_droparr[MAX_RECENT_DROP];
static unsigned int g_curcount;
static struct ip_rule g_quick_app_iprule[CLOUD_IP_NUM];
static seqlock_t g_ip_rule_lock;

bool is_droplist(const char *url, int len)
{
	int i;

	if (!url)
		return false;
	spin_lock_bh(&g_droplock);
	for (i = 0; i < MAX_RECENT_DROP; i++) {
		if (g_droparr[i].url && g_droparr[i].len == len) {
			if (memcmp(g_droparr[i].url, url, len) == 0) {
				spin_unlock_bh(&g_droplock);
				return true;
			}
		}
	}
	spin_unlock_bh(&g_droplock);
	return false;
}

void add_to_droplist(const char *url, int len)
{
	unsigned int idx;
	char *temp = NULL;

	if (!url)
		return;
	if (is_droplist(url, len))
		return;
	if (len < 0 || len > MAX_URL_LENGTH)
		return;
	temp = kmalloc(len + 1, GFP_ATOMIC);
	if (temp == NULL)
		return;
	temp[len] = 0;
	memcpy(temp, url, len);
	spin_lock_bh(&g_droplock);
	idx = g_curcount++;
	if (g_curcount >= MAX_RECENT_DROP)
		g_curcount = 0;
	kfree(g_droparr[idx].url);
	g_droparr[idx].url = temp;
	g_droparr[idx].len = len;
	spin_unlock_bh(&g_droplock);
}

void add_url_to_node(struct ad_rule *node, const char *url, int len)
{
	char *urlstr = NULL;
	struct ad_url *nd = NULL;
	int urllen = len / DOUBLE_TIME;

	if (node == NULL || url == NULL)
		return;
	urlstr = kmalloc(urllen + ULR_LEN_OFFSET_2, GFP_ATOMIC); /* new 101 */
	if (urlstr == NULL)
		return;
	hex_to_byte(url, len, urlstr, urllen + ULR_LEN_OFFSET_2);
	urlstr[urllen] = 0;
	list_for_each_entry(nd, &node->urllist.list, list) {
		if (urllen == nd->len) {
			if (memcmp(nd->url, urlstr, urllen) == 0) {
				pr_info("hwad: already in list url=%s", url);
				kfree(urlstr); /* kfree 101 */
				return;
			}
		}
	}
	nd = kmalloc(sizeof(struct ad_url), GFP_ATOMIC); /* new 102 */
	if (nd == NULL) {
		kfree(urlstr); /* kfree 101 */
		return;
	}
	nd->url = urlstr;
	nd->len = urllen;
	list_add(&nd->list, &node->urllist.list);
}

void add_ad_rule_to_list(struct list_info *listinfo, unsigned int uid,
	const char *url, int len)
{
	struct ad_rule *node = NULL;
	unsigned int hid = get_hash_id(uid);

	if (listinfo == NULL || url == NULL)
		return;
	if (hid >= MAX_HASH) {
		pr_info("hwad:url to list>=MAX_HASH id=%u\n", hid);
		return;
	}
	spin_lock_bh(&listinfo->lock[hid]);
	list_for_each_entry(node, &listinfo->head[hid], list) {
		if (node->uid == uid) {
			add_url_to_node(node, url, len);
			spin_unlock_bh(&listinfo->lock[hid]);
			return;
		}
	}
	node = kmalloc(sizeof(struct ad_rule), GFP_ATOMIC); /* new 103 */
	if (node == NULL) {
		spin_unlock_bh(&listinfo->lock[hid]);
		return;
	}
	pr_info("hwad:add_%s_rule_to_list newuid=%u url=<%s>\n",
		listinfo->type == AD ? "ad" : "delta", uid, url);
	INIT_LIST_HEAD(&node->urllist.list);
	node->uid = uid;
	add_url_to_node(node, url, len);
	list_add(&node->list, &listinfo->head[hid]);
	spin_unlock_bh(&listinfo->lock[hid]);
}

static bool decode_ip_and_port(const char *ipstr, int len, int num)
{
	if (ipstr == NULL || len >= IPV4_AND_PORT_MAX_LEN || num >= CLOUD_IP_NUM)
		return false;
	char ip_port_str[IPV4_AND_PORT_MAX_LEN + 1] = {0};
	char ip_str[IPV4_MAX_LEN + 1] = {0};
	char port_str[TCP_PORT_MAX_LEN + 1] = {0};
	unsigned int tmpaddr = 0;
	if (memcpy_s(ip_port_str, IPV4_AND_PORT_MAX_LEN, ipstr, len) != EOK)
		return false;
	ip_port_str[len] = '\0';
	char *port = strchr(ip_port_str, ':');
	if (port == NULL)
		return false;
	int ip_len = port - ip_port_str;
	port++;
	int port_len = ip_port_str + len - port;
	if (ip_len >= IPV4_MAX_LEN || port_len > TCP_PORT_MAX_LEN || ip_len <= 0 || port_len <= 0)
		return false;
	if (memcpy_s(ip_str, IPV4_MAX_LEN, ip_port_str, ip_len) != EOK)
		return false;
	ip_str[ip_len] = '\0';
	if (memcpy_s(port_str, TCP_PORT_MAX_LEN, port, port_len) != EOK)
		return false;
	port_str[port_len] = '\0';
	if (in4_pton(ip_str, ip_len, (unsigned char *)&tmpaddr, -1, NULL)) {
		int port_num;
		int ret = kstrtoint(port_str, DECIMAL, &port_num);
		if (ret != 0 || port_num > TCP_MAX_PORT_NUM || port_num < 0)
			return false;
		write_seqlock(&g_ip_rule_lock);
		g_quick_app_iprule[num].ipaddr = tmpaddr;
		g_quick_app_iprule[num].port = port_num;
		g_quick_app_iprule[num].valid = 1;
		write_sequnlock(&g_ip_rule_lock);
		return true;
	}
	return false;
}

static void trans_to_ip_and_port_str(const char *buf, int len)
{
	if (buf == NULL)
		return;
	const char *p = buf;
	int tmplen = 0;
	int i = p - buf;
	int ip_rule_num = 0;
	while (i < len) {
		if (buf[i] == ',') {
			tmplen = buf + i - p;
			if (tmplen > 0 && decode_ip_and_port(p, tmplen, ip_rule_num))
				ip_rule_num++;
			p = buf + i + 1;
		}
		i++;
	}
	if (p - buf < len) {
		tmplen = buf + len - p;
		if (tmplen > 0 && decode_ip_and_port(p, tmplen, ip_rule_num))
			ip_rule_num++;
	}
}

static void add_cloud_ip_to_local(const char *src, int len)
{
	if (src ==  NULL || len <= 0 || len % 2 != 0)
		return;
	char buf[CLOUD_IP_NUM * IPV4_MAX_LEN + 1] = {0};
	int count = len >> 1;
	if (count >= CLOUD_IP_NUM * IPV4_MAX_LEN)
		return;
	if (hex2bin((unsigned char *)buf, src, count) != 0)
		return;
	buf[count] = '\0';
	if (strchr(buf, '.') == NULL || strchr(buf, ':') == NULL)
		return;
	write_seqlock(&g_ip_rule_lock);
	(void)memset_s(g_quick_app_iprule, sizeof(struct ip_rule) * CLOUD_IP_NUM, 0, sizeof(struct ip_rule) * CLOUD_IP_NUM);
	write_sequnlock(&g_ip_rule_lock);
	trans_to_ip_and_port_str(buf, count);
}

static bool is_ip_and_port_match(const unsigned int ipaddr, const unsigned short port)
{
	for (int i = 0; i < CLOUD_IP_NUM; i++) {
		if (!g_quick_app_iprule[i].valid)
			break;
		if (ipaddr == g_quick_app_iprule[i].ipaddr && port == g_quick_app_iprule[i].port)
			return true;
	}
	return false;
}

bool is_quickapp_ip_and_port(const unsigned int ipaddr, const unsigned short port)
{
	unsigned int seq;
	bool result;
	do {
		seq = read_seqbegin(&g_ip_rule_lock);
		result = is_ip_and_port_match(ipaddr, port);
	} while (read_seqretry(&g_ip_rule_lock, seq));
	return result;
}

void add_ad_uid_rule(struct list_info *listinfo, const char *rule, int len)
{
	/* 1001:url9,url10, 15 */
	const char *p = NULL;
	unsigned int uid;
	int temlen;
	int i;
	const char *ipstr = "69703A"; /* find ASCII "ip:" */
	int ipstrlen = strlen(ipstr);
	if (listinfo == NULL || rule == NULL)
		return;
	p = strstr(rule, ":");
	if (p == NULL)
		return;
	uid = simple_strtoull(rule, NULL, DECIMAL_BASE);
	p++;
	i = p - rule;
	while (i < len) {
		if (rule[i] == ',') {
			temlen = rule + i - p;
			if (listinfo->type == DELTA && temlen > ipstrlen && strncmp(p, ipstr, ipstrlen) == 0) {
				temlen = temlen - ipstrlen;
				add_cloud_ip_to_local(p + ipstrlen, temlen);
			} else if (temlen > 0) {
				add_ad_rule_to_list(listinfo, uid, p, temlen);
			}
			p = rule + i + 1;
		}
		i++;
	}
	if (p - rule < len) {
		temlen = rule + len - p;
		if (listinfo->type == DELTA && temlen > ipstrlen && strncmp(p, ipstr, ipstrlen) == 0) {
			temlen = temlen - ipstrlen;
			add_cloud_ip_to_local(p + ipstrlen, temlen);
		} else {
			add_ad_rule_to_list(listinfo, uid, p, temlen);
		}
	}
}

void add_ad_rule(struct list_info *listinfo, const char *rule, bool reset)
{
	/* 1001:url9,url10;10004:url6,url7,url8; */
	const char *p = rule;
	const char *temp = NULL;
	int len;

	if (listinfo == NULL || rule == NULL)
		return;
	len = strlen(rule);
	if (reset)
		clear_ad_rule(listinfo, 1, "");
	while (*p != 0) {
		temp = strstr(p, ";");
		if (!temp)
			break;
		if (temp - p > 0) {
			add_ad_uid_rule(listinfo, p, temp - p);
			if (temp - rule >= len)
				break;
			p = temp + 1;
			if (*p == 0)
				break;
		} else {
			break;
		}
	}
}

void clear_node_ad_rule(struct ad_rule *node)
{
	struct ad_url *temp = NULL;
	struct ad_url *next = NULL;

	if (node == NULL)
		return;
	list_for_each_entry_safe(temp, next, &node->urllist.list, list) {
		list_del(&temp->list);
		temp->len = 0;
		kfree(temp->url); /* kfree 101 */
		kfree(temp); /* kfree 102 */
	}
	list_del(&node->list);
	kfree(node); /* kfree 103 */
}

void clear_uid_ad_rule(struct list_info *listinfo, int uid)
{
	struct ad_rule *node = NULL;
	struct ad_rule *next = NULL;
	unsigned int hid = get_hash_id(uid);

	if (listinfo == NULL)
		return;
	if (hid >= MAX_HASH)
		return;
	spin_lock_bh(&listinfo->lock[hid]);
	list_for_each_entry_safe(node, next, &listinfo->head[hid], list) {
		if (uid == node->uid)
			clear_node_ad_rule(node);
	}
	spin_unlock_bh(&listinfo->lock[hid]);
}

void clear_ad_rule_string(struct list_info *listinfo, const char *rule)
{
	/* 1001;10004;10064; */
	const char *p = NULL;
	const char *temp = NULL;
	int uid;

	if (listinfo == NULL || rule == NULL)
		return;
	temp = rule;
	p = rule;
	while (p && *p != 0) {
		p = strstr(temp, ";");
		if (p) {
			uid = simple_strtoull(temp, NULL, DECIMAL_BASE);
			clear_uid_ad_rule(listinfo, uid);
			temp = p + 1;
		}
	}
}

void clear_ad_rule(struct list_info *listinfo, int opt, const char *data)
{
	int i;
	struct ad_rule *node = NULL;
	struct ad_rule *next = NULL;

	if (listinfo == NULL || data == NULL)
		return;
	if (opt == 0) {
		clear_ad_rule_string(listinfo, data);
		return;
	}
	pr_info("hwad:clear_%s_rule\n", listinfo->type == AD ? "ad" : "delta");
	if (listinfo->type == DELTA) {
		write_seqlock(&g_ip_rule_lock);
		(void)memset_s(g_quick_app_iprule, sizeof(struct ip_rule) * CLOUD_IP_NUM, 0, sizeof(struct ip_rule) * CLOUD_IP_NUM);
		write_sequnlock(&g_ip_rule_lock);
	}
	for (i = 0; i < MAX_HASH; i++) {
		spin_lock_bh(&listinfo->lock[i]);
		list_for_each_entry_safe(node, next, &listinfo->head[i], list) {
			clear_node_ad_rule(node);
		}
		spin_unlock_bh(&listinfo->lock[i]);
	}
}

bool match_ad_uid(struct list_info *listinfo, unsigned int uid)
{
	struct ad_rule *node = NULL;
	unsigned int hid = get_hash_id(uid);

	if (listinfo == NULL)
		return false;
	if (hid >= MAX_HASH) {
		pr_info("hwad:match_%s_uid>=MAX_hid=%uuid=%u\n",
			listinfo->type == AD ? "ad" : "delta", hid, uid);
		return false;
	}
	spin_lock_bh(&listinfo->lock[hid]);
	list_for_each_entry(node, &listinfo->head[hid], list) {
		if (node->uid == uid) {
			spin_unlock_bh(&listinfo->lock[hid]);
			return true;
		}
	}
	spin_unlock_bh(&listinfo->lock[hid]);
	return false;
}

bool match_ab_string(const char *str, const char *a,
	int al, const char *b, int bl)
{
	const char *ps = NULL;

	ps = strfinds2(str, a, al);
	if (ps == NULL)
		return false;
	ps += al;
	return match_url_string(ps, b, bl);
}

/*
 * Description
 * 1. xxx
 *  1.1 abcd bc
 *  1.2 abcde abcde
 * 2. *xxx
 *  2.1 abcdef *bc
 *  2.2 abcdef *cdef
 *  2.3 abcdef *abcdef
 * 3. xxx*
 *  3.1 abcdef bc*
 *  3.2 abcdef abc*
 *  3.3 abcdef abcdef*
 * 4. xxx*yyy
 *  4.1 abcdef ab*f
 *  4.2 abcdefg bc*de
 *  4.3 abcdef ab*def
 *  4.4 abcdef abc*def
 * 5. xxx*yyy*xxx*zzz
 *  5.1 abcdefghigk ab*f*igk
 *  5.2 abcdefghigk *ab*f*
 *  5.3 abcdefghigk a*c*e*g*i*k
 *  5.4 abcdefghigk *a*c*e*g*i*k*
 * http://xxx/adhhhh.html
 */
bool match_url_string(const char *url, const char *filter, int len)
{
	const char *ps = NULL;

	if (url == NULL || filter == NULL)
		return false;
	ps = strfindpos(filter, "*", len);

	if (ps == NULL) {
		/* 1. xxx */
		if (strfinds2(url, filter, len))
			return true;
		return false;
	}
	if (filter == ps) {
		/* 2. *xxx */
		return match_url_string(url, filter + 1, len - 1);
	} else if ((filter + len - 1) == ps) {
		/* 3. xxx* */
		if (strfinds2(url, filter, len - 1))
			return true;
		else
			return false;
	} else {
		/* 4. xxx*yyy */
		return match_ab_string(url, filter, ps - filter,
			ps + 1, filter + len - ps - 1);
	}
}

bool match_uid_url(struct ad_rule *node, const char *url)
{
	struct ad_url *temp_node = NULL;
	bool bret = false;
	if (node == NULL || url == NULL)
		return false;

	list_for_each_entry(temp_node, &node->urllist.list, list) {
		if (temp_node->len != 0) {
			if (match_url_string(url, temp_node->url,
					temp_node->len)) {
				pr_info("hwad: match_url_string true rule=%s\n",
					temp_node->url);
				bret = true;
				break;
			}
		}
	}
	return bret;
}

bool match_ad_url(struct list_info* listinfo, unsigned int uid,
	const char *data, int datalen)
{
	struct ad_rule *node = NULL;
	unsigned int hid = get_hash_id(uid);
	bool bret = false;
	char *tempurl = NULL;

	if (listinfo == NULL || data == NULL)
		return false;
	if (uid == 0xffffffff)
		return false;
	if (hid >= MAX_HASH)
		return bret;

	tempurl = get_url_form_data(data, datalen);

	if (tempurl == NULL) {
		if (datalen < 0 || datalen > MAX_URL_LENGTH)
			return bret;
		tempurl = kmalloc(datalen + 1, GFP_ATOMIC);
		if (tempurl == NULL)
			return bret;
		memcpy(tempurl, data, datalen);
		*(tempurl + datalen) = 0;
		pr_info("hwad:match_%s_url multi part\n",
			listinfo->type == AD ? "ad" : "delta");
	}
	spin_lock_bh(&listinfo->lock[hid]);
	list_for_each_entry(node, &listinfo->head[hid], list) {
		if (node->uid == uid) {
			bret = match_uid_url(node, tempurl);
			break;
		}
	}
	spin_unlock_bh(&listinfo->lock[hid]);
	kfree(tempurl);
	return bret;
}

void output_ad_rule(struct list_info* listinfo)
{
	int i;
	struct ad_rule *node = NULL;
	struct ad_url *temp = NULL;

	if (listinfo == NULL)
		return;
	pr_info("hwad:output_%s_rule start\n",
		listinfo->type == AD ? "ad" : "delta");
	for (i = 0; i < MAX_HASH; i++) {
		spin_lock_bh(&listinfo->lock[i]);
		list_for_each_entry(node, &listinfo->head[i], list) {
			pr_info("hwad:output_%s_rule uid=%d\n",
				listinfo->type == AD ? "ad" : "delta", node->uid);
			list_for_each_entry(temp, &node->urllist.list, list) {
				pr_info("hwad:output_%s_rule=%s len=%d\n",
					listinfo->type == AD ? "ad" : "delta",
					temp->url, temp->len);
			}
			pr_info("\n");
		}
		spin_unlock_bh(&listinfo->lock[i]);
	}
	pr_info("output_%s_rule end\n", listinfo->type == AD ? "ad" : "delta");
}

void init_ad(void)
{
	int i;
	g_curcount = 0;
	g_adlist_info.type = AD;
	g_deltalist_info.type = DELTA;
	g_adlist_info.lastrepot = 0;
	g_deltalist_info.lastrepot = 0;

	for (i = 0; i < MAX_HASH; i++) {
		INIT_LIST_HEAD(&g_adlist_info.head[i]);
		spin_lock_init(&g_adlist_info.lock[i]);
	}
	for (i = 0; i < MAX_HASH; i++) {
		INIT_LIST_HEAD(&g_deltalist_info.head[i]);
		spin_lock_init(&g_deltalist_info.lock[i]);
	}
	for (i = 0; i < MAX_RECENT_DROP; i++) {
		g_droparr[i].url = NULL;
		g_droparr[i].len = 0;
	}
	spin_lock_init(&g_droplock);
	seqlock_init(&g_ip_rule_lock);
}

void uninit_ad(void)
{
	int i;

	clear_ad_rule(&g_adlist_info, 1, "");
	clear_ad_rule(&g_deltalist_info, 1, "");
	for (i = 0; i < MAX_RECENT_DROP; i++) {
		kfree(g_droparr[i].url);
		g_droparr[i].url = NULL;
		g_droparr[i].len = 0;
	}
}
