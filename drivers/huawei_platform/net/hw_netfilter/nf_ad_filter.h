/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2020. All rights reserved.
 * Description: Netfilter head file.
 * Author: chenzhongxian@huawei.com
 * Create: 2016-05-28
 */

#ifndef _NF_AD_FILTER
#define _NF_AD_FILTER
#define AD 0
#define DELTA 1
#define CLOUD_IP_NUM 10
#define IPV4_AND_PORT_MAX_LEN 24
#define IPV4_MAX_LEN 16
#define TCP_PORT_MAX_LEN 5
#define TCP_MAX_PORT_NUM 65535
#define DECIMAL 10
struct list_info {
	struct list_head head[MAX_HASH];
	spinlock_t lock[MAX_HASH];
	int type;
	u64 lastrepot;
};

struct ip_rule {
	unsigned int ipaddr;
	unsigned int port;
	unsigned int valid;
};

extern struct list_info g_adlist_info;
extern struct list_info g_deltalist_info;

void uninit_ad(void);
void init_ad(void);

void add_ad_rule(struct list_info *listinfo, const char *rule, bool reset);
void output_ad_rule(struct list_info *listinfo);
void clear_ad_rule(struct list_info *listinfo, int opt, const char *data);
bool match_ad_uid(struct list_info *listinfo, unsigned int uid);
bool match_ad_url(struct list_info *listinfo, unsigned int uid,
	const char *data, int datalen);
bool match_url_string(const char *url, const char *filter, int len);
bool is_droplist(const char *url, int len);
void add_to_droplist(const char *url, int len);
bool is_quickapp_ip_and_port(const unsigned int ipaddr, const unsigned short port);
#endif /* _NF_AD_FILTER */
