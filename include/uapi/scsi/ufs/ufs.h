#ifndef UAPI_UFS_H_
#define UAPI_UFS_H_

#define TURBO_ZONE_INFO_SIZE (sizeof(struct tz_status))

struct tz_status {
	uint8_t enable;         /* 0: disable; 1: enable; */
	uint8_t lu_number;      /* LU number */
	uint16_t max_ec;        /* max EC of Turbo Zone */
	uint16_t avg_ec;        /* average EC of turbo zone */
	uint8_t return_en;      /* return to tlc enable */
	uint8_t return_back_en; /* return back to slc enable */
	uint32_t max_lba;       /* max LBA(4KB unit) */
	uint32_t total_vpc;     /* Total VPC, Valid when TurboZone Enable */
	uint8_t close_qcnt;     /* blocks to return */
	uint8_t forbidden;      /* already used once, cannot enable again, reserved for 2.0 */
	uint16_t tz_close_cnt;  /* for 2.0 */
};

struct tz_cap_info {
	uint32_t marked_slc_blks;  /* turbo zone marked slc blks, unit: 4K */
	uint32_t marked_tlc_blks;  /* turbo zone marked tlc blks, unit: 4K */
	uint32_t remain_slc_blks;  /* remained slc blks, unit: 4K */
	uint32_t slc_exist_status; /* slc exist flag */
};

struct tz_blk_info {
	uint32_t addr;  /* query blk info lba */
	uint16_t len;   /* query blk info length */
	uint8_t *flags; /* the return info of query lba */
	uint16_t buf_len; /* the flags buffer length */
};

enum {
	TZ_INVALID = 0,
	TZ_VER_1_0 = 1,
	TZ_VER_2_0 = 2,
};

enum {
	TZ_READ_DESC	= 1,
	TZ_STATUS	= 2,
	TZ_CAP_INFO	= 3,
	TZ_BLK_INFO	= 4,
};

enum tz_desc_id {
	TZ_RETURN_FLAG      = 0x01,
#ifdef CONFIG_DFX_DEBUG_FS
	TZ_RETURN_BACK_FLAG = 0x04,
#endif
	TZ_FORCE_CLOSE_FLAG = 0x08,
#ifdef CONFIG_DFX_DEBUG_FS
	TZ_FORCE_OPEN_FLAG  = 0x10,
#endif
	TZ_DESC_MAX,
};

enum vendor_ctrl_desc_id {
	VENDOR_IDN_TT_UNIT_RD		= 0x05,
	DEVICE_CAPABILITY_FLAG		= 0x06,
	VENDOR_IDN_TT_UNIT_WT		= 0x08,
	SET_TZ_STREAM_ID		= 0x0C,
	VENDOR_CTRL_MAX,
};

#ifndef CONFIG_DFX_DEBUG_FS
void delete_ufs_product_name(char *cmdline);
#endif

#if defined(CONFIG_DFX_DEBUG_FS) && defined(CONFIG_HUFS_HC)
void ufs_change_vol_pre(void);
void ufs_change_vol_post(void);
#else
static inline void ufs_change_vol_pre(void)
{
}
static inline void ufs_change_vol_post(void)
{
}
#endif

#endif /* UAPI_UFS_H_ */
