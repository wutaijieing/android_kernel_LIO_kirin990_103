/*
 * Copyright (c) CompanyNameMagicTag. 2019-2019. All rights reserved.
 * Description: hdmi driver edid module main source file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */
#include <linux/slab.h>
#include <linux/types.h>

#include <securec.h>
#include "dkmd_log.h"

#include "hdmitx_common.h"
#include "hdmitx_modes.h"
#include "hdmitx_connector.h"
#include "hdmitx_edid.h"

typedef void detailed_cb(const struct detailed_timing *timing, void *closure);

#define version_comparison(edid, maj, min) \
	(((edid)->version > (maj)) ||  \
		((edid)->version == (maj) && (edid)->revision > (min)))

#define for_each_cea_db(cea, i, start, end) \
	for ((i) = (start); (i) < (end) && (i) + cea_db_payload_len(&(cea)[(i)]) < (end); \
		 (i) += cea_db_payload_len(&(cea)[(i)]) + 1)

#define EDID_EST_TIMING_NUM	  16
#define EDID_STD_TIMING_NUM	  8
#define EDID_DETAILED_TIMING_NUM 4

#define EDID_VEND_YEAR_BASE	   1990
#define EDID_DEFAULT_DVI_MAX_TMDS 225000

#define EDID_AUDIO_TAG			 0x01
#define EDID_VIDEO_TAG			 0x02
#define EDID_VENDOR_TAG			0x03
#define EDID_SPEAKER_TAG		   0x04
#define USE_EXTENDED_TAG		   0x07
#define EXT_VIDEO_CAPABILITY_TAG   0x00
#define EXT_VENDOR_TAG			 0x01
#define EXT_COLORIMETRY_TAG		0x05
#define EXT_HDR_STATIC_TAG		 0x06
#define EXT_HDR_DYNAMIC_TAG		0x07
#define EXT_VIDEO_DATA_TAG_420	 0x0E
#define EXT_VIDEO_CAP_TAG_Y420CMDB 0x0F
#define EDID_BASIC_AUDIO			 (1 << 6)
#define EDID_CEA_YCRCB444			(1 << 5)
#define EDID_CEA_YCRCB422			(1 << 4)
#define EDID_CEA_VCDB_QS			 (1 << 6)
#define EDID_CEA_VCDB_QY			 (1 << 7)

/*
 * EDID blocks out in the wild have a variety of bugs, try to collect
 * them here (note that userspace may work around broken monitors first,
 * but fixes should make their way here so that the kernel "just works"
 * on as many displays as possible).
 */
/* The first detailed mode is wrong, use the maximum 60Hz mode */
#define EDID_QUIRK_PREFER_LARGEST_60 (1 << 0)
/* The reported 135MHz pixel clock is too high and needs to be adjusted */
#define EDID_QUIRK_TOO_HIGH_135_CLOCK (1 << 1)
/* Preferred 75 Hz maximum mode */
#define EDID_QUIRK_PREFER_LARGEST_75 (1 << 2)
/* The detailed timing unit is cm, not mm */
#define EDID_QUIRK_DETAILED_IN_CM (1 << 3)
/* The detailed timing descriptor has a false size value,
 * so only the maximum size is used.
 */
#define EDID_QUIRK_DETAILED_USE_MAXIMUM_SIZE (1 << 4)
/* use +hsync +vsync for detailed mode */
#define EDID_QUIRK_DETAILED_SYNC_PP (1 << 6)
/* Force reduction of blank time in detailed mode */
#define EDID_QUIRK_FORCE_REDUCED_BLANKING (1 << 7)
/* Force 8bpc mode */
#define EDID_QUIRK_FORCE_8BPC (1 << 8)
/* Force 12bpc mode */
#define EDID_QUIRK_FORCE_12BPC (1 << 9)
/* Force 6bpc mode */
#define EDID_QUIRK_FORCE_6BPC (1 << 10)
/* Force 10bpc mode */
#define EDID_QUIRK_FORCE_10BPC (1 << 11)
/* Non-desktop display */
#define EDID_QUIRK_NON_DESKTOP (1 << 12)

struct detailed_mode_closure {
	struct hdmitx_connector *connector;
	const struct edid *edid;
	bool preferred;
	u32 modes;
};

/* use for edid parse cea audio */
struct edid_ext_info {
	u32 sad_count;
	u8 fmt_code;
	u8 ext_code;
	u8 byte0;
	u8 byte1;
	u8 byte2;
};

#define LEVEL_DMT  0
#define LEVEL_GTF  1
#define LEVEL_GTF2 2
#define LEVEL_CVT  3

struct stereo_mandatory_mode {
	s32 width, height, vrefresh;
	bool progressive;
	u32 flags;
};

static const struct stereo_mandatory_mode g_stereo_mandatory_modes[] = {
	{ 1920, 1080, 24, true,  HDMITX_3D_BZ_TOP_AND_BOTTOM },
	{ 1920, 1080, 24, true,  HDMITX_3D_BZ_FRAME_PACKING },
	{ 1920, 1080, 50, false, HDMITX_3D_BZ_SIDE_BY_SIDE_HALF },
	{ 1920, 1080, 60, false, HDMITX_3D_BZ_SIDE_BY_SIDE_HALF },
	{ 1280, 720,  50, true,  HDMITX_3D_BZ_TOP_AND_BOTTOM },
	{ 1280, 720,  50, true,  HDMITX_3D_BZ_FRAME_PACKING },
	{ 1280, 720,  60, true,  HDMITX_3D_BZ_TOP_AND_BOTTOM },
	{ 1280, 720,  60, true,  HDMITX_3D_BZ_FRAME_PACKING }
};

#define EDID_HEADER_MAX_ALLOW_ERR_NUM	  0x2
/* DDC fetch and block validation */
static const u8 g_edid_header[] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00
};


/*
 * 0 is reserved.  The spec says 0x01 fill for unused timings.	Some old
 * monitors fill with ascii space (0x20) instead.
 */
static s32 bad_std_timing(u8 a, u8 b)
{
	return (a == 0x00 && b == 0x00) ||
		   (a == 0x01 && b == 0x01) ||
		   (a == 0x20 && b == 0x20);
}

/*
 * Search EDID for CEA extension block.
 */
static u8 *find_edid_extension(const struct edid *edid, u32 ext_id)
{
	u8 *edid_ext = NULL;
	u32 i;

	/* No EDID or EDID extensions */
	if (edid->extensions == 0)
		return NULL;

	/* Find CEA extension */
	for (i = 0; i < edid->extensions; i++) {
		edid_ext = (u8 *)edid + EDID_LENGTH * (i + 1);
		if (edid_ext[0] == ext_id)
			break;
	}

	if (i == edid->extensions)
		return NULL;

	return edid_ext;
}

static bool cea_vic_is_valid(u8 vic)
{
	/* 128-192  Forbidden ;220-255  Reserved for the Future by CTA-861-G */
	return (vic > 0 && vic < 128) || (vic > 192 && vic < 220);
}

static bool hdmi_vic_is_valid(u8 hdmi_vic)
{
	/* hdmi vic is in the range 0~5. */
	return hdmi_vic > 0 && hdmi_vic < 5;
}

static u8 svd_to_vic(u8 svd)
{
	/* 0-6 bit vic, 7th bit native mode indicator. 1-64 is not native mode, 129-192 is native mode. */
	if ((svd >= 1 && svd <= 64) || (svd >= 129 && svd <= 192))
		return svd & 127; /* vic is svd & 127.  */

	return svd;
}

static bool stereo_match_mandatory(const struct hdmitx_display_mode *mode,
	const struct stereo_mandatory_mode *stereo_mode)
{
	return mode->detail.h_active == stereo_mode->width &&
		   mode->detail.v_active == stereo_mode->height &&
		   mode->detail.progressive == stereo_mode->progressive &&
		   drv_hdmitx_modes_get_vrefresh(&mode->detail) == stereo_mode->vrefresh;
}

static void cea_for_each_detailed_block(const u8 *ext, detailed_cb *cb, void *closure)
{
	u32 i, n;
	u8 d = ext[0x02]; /* extend block's 2th byte. */
	const u8 *det_base = ext + d;

	if (d > 127) /* confirm d value not greater than 127. */
		return;

	n = (127 - d) / 18; /* all dtbs size is 127 - d, dtb nums is all dtbs size divide by 18. */
	for (i = 0; i < n; i++)
		cb ((struct detailed_timing *)(det_base + 18 * i), closure); /* every dtb size is 18bytes. */
}

static void vtb_for_each_detailed_block(const u8 *ext, detailed_cb *cb, void *closure)
{
	u32 i;
	u32 n = min((s32)ext[0x02], 6); /* The minimum value between extend block's 2th byte and 6. */
	const u8 *det_base = ext + 5; /* need add 5. */

	if (ext[0x01] != 1) /* 01H != 0xff, unknown version */
		return;

	for (i = 0; i < n; i++)
		cb ((struct detailed_timing *)(det_base + 18 * i), closure); /* every dtb size is 18bytes. */
}

static u32 cea_db_payload_len(const u8 *db)
{
	return db[0] & 0x1f; /* the lower 5 bits(&0x1f) of db's 0th byte. */
}

static u32 cea_db_extended_tag(const u8 *db)
{
	return db[1]; /* the db's 1th byte. */
}

static u32 cea_db_tag(const u8 *db)
{
	return db[0] >> 5; /* the upper 3 bits(>>5) db's 0th byte. */
}

static u32 cea_revision(const u8 *cea)
{
	return cea[1]; /* the db's 1th byte. */
}

static s32 cea_db_offsets(const u8 *cea, u32 *start, u32 *end)
{
	/* Data block offset in CEA extension block */
	*start = 4; /* db offset start is 4. */
	*end = cea[2]; /* db offset end is cea[2]. */
	if (*end == 0)
		*end = 127; /* db offset end is 127. */

	if (*end < 4 || *end > 127) /* db offset end <4 or >127 is invalid. */
		return -1;

	return 0;
}

static bool cea_db_is_hdmi_vsdb(const u8 *db)
{
	u32 oui;

	if (cea_db_tag(db) != EDID_VENDOR_TAG)
		return -1;

	if (cea_db_payload_len(db) < 5) /* vsdb's min size is 5. */
		return -1;

	oui = db[1] | (db[2] << 8) | (db[3] << 16); /* oui is db[1]|(db[2] << 8)|(db[3] << 16). */

	return oui == HDMITX_IEEE_OUI;
}

static bool cea_db_is_hdmi_forum_vsdb(const u8 *db)
{
	u32 oui;

	if (cea_db_tag(db) != EDID_VENDOR_TAG)
		return -1;

	if (cea_db_payload_len(db) < 7) /* hf-vsdb's min size is 7. */
		return -1;

	oui = (db[3] << 16) | (db[2] << 8) | db[1]; /* oui is (db[3] << 16)|(db[2] << 8)| b[1]. */

	return oui == HDMITX_FORUM_IEEE_OUI;
}

static bool cea_db_is_dolbyvision(const u8 *db)
{
	u32 oui;

	if (cea_db_tag(db) != USE_EXTENDED_TAG)
		return -1;

	if (cea_db_payload_len(db) < 0xb)  /* hf-vsdb's min size is 0xb. */
		return -1;

	if (cea_db_extended_tag(db) != EXT_VENDOR_TAG)
		return -1;

	oui = (db[4] << 16) | (db[3] << 8) | db[2]; /* oui is db[4] << 16|db[3] << 8|db[2]. */

	return oui == HDMITX_DOLBY_IEEE_OUI;
}

static bool cea_db_is_cuva_hdr_db(const u8 *db)
{
	u32 oui;

	if (cea_db_tag(db) != USE_EXTENDED_TAG)
		return -1;

	if (cea_db_payload_len(db) < 0x5) /* cuva-vsvdb's min size is 0x5. */
		return -1;

	if (cea_db_extended_tag(db) != EXT_VENDOR_TAG)
		return -1;

	oui = (db[4] << 16) | (db[3] << 8) | db[2]; /* oui is db[4] << 16|db[3] << 8|db[2]. */

	return oui == HDMITX_CUVA_IEEE_OUI;
}

static bool cea_db_is_vcdb(const u8 *db)
{
	if (cea_db_tag(db) != USE_EXTENDED_TAG)
		return -1;

	if (cea_db_payload_len(db) != 2)  /* vcdb's size is 2. */
		return -1;

	if (cea_db_extended_tag(db) != EXT_VIDEO_CAPABILITY_TAG)
		return -1;

	return true;
}

static bool cea_db_is_colorimetry_db(const u8 *db)
{
	if (cea_db_tag(db) != USE_EXTENDED_TAG)
		return -1;

	if (cea_db_payload_len(db) != 3)  /* colorimetry_db's size is 3. */
		return -1;

	if (cea_db_extended_tag(db) != EXT_COLORIMETRY_TAG)
		return -1;

	return true;
}

static bool cea_db_is_hdr_static_db(const u8 *db)
{
	if (cea_db_tag(db) != USE_EXTENDED_TAG)
		return -1;

	if (cea_db_payload_len(db) < 2)  /* hdr_static_db's min size is 2. */
		return -1;

	if (cea_db_extended_tag(db) != EXT_HDR_STATIC_TAG)
		return -1;

	return true;
}

static bool cea_db_is_hdr_dynamic_db(const u8 *db)
{
	if (cea_db_tag(db) != USE_EXTENDED_TAG)
		return -1;

	if (cea_db_payload_len(db) < 2)  /* hdr_dynamic_db's min size is 2. */
		return -1;

	if (cea_db_extended_tag(db) != EXT_HDR_DYNAMIC_TAG)
		return -1;

	return true;
}

static bool cea_db_is_y420cmdb(const u8 *db)
{
	if (cea_db_tag(db) != USE_EXTENDED_TAG)
		return -1;

	if (!cea_db_payload_len(db))
		return -1;

	if (cea_db_extended_tag(db) != EXT_VIDEO_CAP_TAG_Y420CMDB)
		return -1;

	return true;
}

static bool cea_db_is_y420vdb(const u8 *db)
{
	if (cea_db_tag(db) != USE_EXTENDED_TAG)
		return -1;

	if (!cea_db_payload_len(db))
		return -1;

	if (cea_db_extended_tag(db) != EXT_VIDEO_DATA_TAG_420)
		return -1;

	return true;
}

static void edid_do_interlace_quirk(struct hdmi_detail *detail, bool interlaced)
{
	if (interlaced) {
		detail->v_active = (u32)(2 * detail->v_active); /* need multiply by 2. */
		detail->v_total = (u32)(2 * detail->v_total); /* need multiply by 2. */
		detail->v_total |= 1;
	}
}

static u32 edid_get_header_err_nums(const u8 *raw_edid)
{
	u32 i;
	u32 err = 0;

	for (i = 0; i < sizeof(g_edid_header); i++) {
		if (raw_edid[i] != g_edid_header[i])
			err++;
	}
	return err;
}

static u32 edid_get_block_checksum(const u8 *raw_edid)
{
	u32 i;
	u8 csum = 0;

	for (i = 0; i < EDID_LENGTH; i++)
		csum += raw_edid[i];

	return csum;
}

static bool edid_data_is_zero(const u8 *in_edid, u32 length)
{
	u32 i;
	for (i = 0; i < length; i++) {
		if (in_edid[i] != 0)
			return false;
	}

	return true;
}

static void edid_print_raw_data(const u8 *raw_edid, u32 length)
{
	u32 i;

	if (edid_data_is_zero(raw_edid, EDID_LENGTH)) {
		dpu_pr_err("EDID block is all zeroes\n");
	} else {
		dpu_pr_err("Raw EDID:\n");
		for (i = 0; i < length; i++) {
			if (((i % 16) == 0) && (i != 0)) /* If the value is greater than 16, the line breaks. */
				dpu_pr_err("\n");

			dpu_pr_err("0x%x\t", raw_edid[i]);
		}
	}
}

static bool edid_block_is_valid(u8 *raw_edid, u32 block, bool print_bad_edid)
{
	u8 csum;
	u32 err;
	struct edid *edid = NULL;

	edid = (struct edid *)raw_edid;

	if (block == 0) {
		err = edid_get_header_err_nums(raw_edid);
		if (err == 0) {
			dpu_pr_info("edid header is good.\n");
		} else if (err <= EDID_HEADER_MAX_ALLOW_ERR_NUM) {
			dpu_pr_info("Fixing EDID header, your hardware may be failing\n");
			if (memcpy_s(raw_edid, sizeof(struct edid), g_edid_header, sizeof(g_edid_header)) != EOK) {
				dpu_pr_err("memcpy_s err\n");
				goto bad;
			}
		} else {
			goto bad;
		}
	}

	csum = edid_get_block_checksum(raw_edid);
	if (csum) {
		if (print_bad_edid)
			dpu_pr_err("EDID checksum is invalid, remainder is %d\n", csum);
	}
	/* per-block-type checks */
	switch (raw_edid[0]) {
	case 0: /* base block 0 */
		if (edid->version != 1) {
			dpu_pr_err("EDID has major version %d, instead of 1\n", edid->version);
			goto bad;
		}

		if (edid->revision > 4)  /* revision > 4 is assuming backward compatibility. */
			dpu_pr_info("EDID minor > 4, assuming backward compatibility\n");
		break;

	default:
		break;
	}

	return true;

bad:
	if (print_bad_edid)
		edid_print_raw_data(raw_edid, EDID_LENGTH);
	return -1;
}

static bool edid_data_is_valid(const struct edid *edid)
{
	u32 i;
	u8 *raw = (u8 *)edid;

	for (i = 0; i <= edid->extensions; i++) {
		if (!edid_block_is_valid(raw + i * EDID_LENGTH, i, true))
			return -1;
	}
	return true;
}

static void edid_for_each_detailed_block(const u8 *raw_edid, detailed_cb *cb, void *closure)
{
	u32 i;
	const u8 *ext = NULL;
	struct edid *edid = (struct edid *)raw_edid;

	for (i = 0; i < EDID_DETAILED_TIMING_NUM; i++)
		cb(&(edid->detailed_timings[i]), closure);

	for (i = 1; i <= raw_edid[0x7e]; i++) { /* extend block's 0x7eth byte. */
		ext = raw_edid + (i * EDID_LENGTH);

		switch (*ext) {
		case CEA_EXTEND:
			cea_for_each_detailed_block(ext, cb, closure);
			break;
		case VTB_EXTEND:
			vtb_for_each_detailed_block(ext, cb, closure);
			break;
		default:
			break;
		}
	}
}

static struct hdmitx_display_mode *edid_create_mode_by_std(const struct hdmitx_connector *connector,
														   const struct edid *edid,
														   const struct std_timing *t)
{
	struct hdmitx_display_mode *m = NULL;
	struct hdmitx_display_mode *mode = NULL;
	u32 hsize, vsize;
	u32 vrefresh_rate;
	u32 aspect_ratio = (t->vfreq_aspect & EDID_TIMING_ASPECT_RATIO_MASK) >> EDID_TIMING_ASPECT_RATIO_SHIFT;
	u32 vfreq = (t->vfreq_aspect & EDID_TIMING_VF_REQ_MASK) >> EDID_TIMING_VF_REQ_SHIFT;
	u64 temp;
	bool flag = false;

	if (bad_std_timing(t->hsize, t->vfreq_aspect))
		return NULL;

	/* According to the EDID spec, the hdisplay = hsize * 8 + 248 */
	temp = t->hsize * 8 + 248;
	hsize = (u32)temp;
	/* According to the EDID spec, the vrefresh_rate = vfreq + 60 */
	vrefresh_rate = vfreq + 60;
	/* the vdisplay is calculated based on the aspect ratio */
	if (aspect_ratio == 0) {
		if (edid->revision < 3) { /* revision less than 3. */
			vsize = hsize;
		} else {
			/* According to the EDID spec, the vsize = (hsize * 10) / 16 */
			vsize = (hsize * 10) / 16;
		}
	} else if (aspect_ratio == 1) {
		/* According to the EDID spec, the vsize = (hsize * 3) / 4 */
		vsize = (hsize * 3) / 4;
	} else if (aspect_ratio == 2) { /* aspect_ratio is 2. */
		/* According to the EDID spec, the vsize = (hsize * 4) / 5; */
		vsize = (hsize * 4) / 5;
	} else {
		/* According to the EDID spec, the vsize = (hsize * 9) / 16 */
		vsize = (hsize * 9) / 16;
	}

	/* HDTV hack, part 1 */
	flag = vrefresh_rate == 60 &&  /* vrefresh_rate is 60. */
		((hsize == 1360 && vsize == 765) || /* hsize is 1360, vsize is 765. */
		 (hsize == 1368 && vsize == 769)); /* hsize is 1368, vsize is 769. */
	if (flag) {
		hsize = 1366; /* hsize is 1366 */
		vsize = 768; /* hsize is 768 */
	}

	/*
	 * If this connector already has a mode for this size and refresh
	 * rate (because it came from detailed or CVT info), use that
	 * instead.  This way we don't have to guess at interlace or
	 * reduced blanking.
	 */
	list_for_each_entry(m, &connector->probed_modes, head) {
		flag = m->detail.h_active == hsize && m->detail.v_active == vsize &&
			drv_hdmitx_modes_get_vrefresh(&m->detail) == vrefresh_rate;
		if (flag)
			return NULL;
	}

	mode = drv_hdmitx_modes_create_mode_by_std_timing(hsize, vsize, vrefresh_rate);
	return mode;
}

static struct hdmitx_display_mode *edid_create_detailed_mode(struct hdmitx_connector *connector,
	const struct hdmi_detail *detail)
{
	struct hdmitx_display_mode *mode = NULL;

	mode = drv_hdmitx_modes_create_mode();
	if (mode == NULL)
		return NULL;

	if (memcpy_s(&mode->detail, sizeof(mode->detail), detail, sizeof(*detail)) != EOK) {
		dpu_pr_err("memcpy_s fail.\n");
		drv_hdmitx_modes_destroy_mode(mode);
		return NULL;
	}

	mode->vic = ++connector->detail_vic_base;

	return mode;
}

static s32 edid_get_detail_info(const struct detailed_timing *timing, struct hdmi_detail *detail)
{
	const struct detailed_pixel_timing *pt = &timing->data.pixel_data;
	u32 hactive = ((pt->hactive_hblank_hi & 0xf0) << 4) | pt->hactive_lo; /* need left shift 4bits. */
	u32 vactive = ((pt->vactive_vblank_hi & 0xf0) << 4) | pt->vactive_lo; /* need left shift 4bits. */
	u32 hblank = ((pt->hactive_hblank_hi & 0xf) << 8) | pt->hblank_lo; /* need left shift 8bits. */
	u32 vblank = ((pt->vactive_vblank_hi & 0xf) << 8) | pt->vblank_lo; /* need left shift 8bits. */
	u32 hsync_offset = ((pt->hsync_vsync_offset_pulse_width_hi & 0xc0) << 2) | /* need left shift 2bits. */
							pt->hsync_offset_lo;
	u32 hsync_pulse_width = ((pt->hsync_vsync_offset_pulse_width_hi & 0x30) << 4) |  /* need left shift 4bits. */
								pt->hsync_pulse_width_lo;
	u32 vsync_offset = ((pt->hsync_vsync_offset_pulse_width_hi & 0xc) << 2) |  /* need left shift 2bits. */
						   (pt->vsync_offset_pulse_width_lo >> 4); /* need right shift 4bits. */
	u32 vsync_pulse_width = ((pt->hsync_vsync_offset_pulse_width_hi & 0x3) << 4) | /* need left shift 4bits. */
								(pt->vsync_offset_pulse_width_lo & 0xf);
	u32 pixel_clk = (u32)(le16_to_cpu(timing->pixel_clock) * 10); /* need multiply by 10. */

	/* ignore tiny modes */
	if (hactive < 64 || vactive < 64) /* if hactive < 64 and vactive < 64, is invalid mode. */
		return -1;

	if (pt->misc & HDMITX_EDID_PT_STEREO) {
		dpu_pr_debug("stereo mode not supported\n");
		return -1;
	}

	/* it is incorrect if hsync/vsync width is zero */
	if (!hsync_pulse_width || !vsync_pulse_width) {
		dpu_pr_debug("Incorrect Detailed timing. Wrong Hsync/Vsync pulse width\n");
		return -1;
	}

	detail->pixel_clock = pixel_clk;
	detail->h_active = hactive;
	detail->h_total = hactive + hblank;
	detail->h_blank = hblank;
	detail->h_front = hsync_offset - pt->hborder;
	detail->h_sync = hsync_pulse_width;
	detail->h_back = hblank - detail->h_sync - detail->h_front;
	detail->v_active = vactive;
	detail->v_total = vactive + vblank;
	detail->v_blank = vblank;
	detail->v_front = vsync_offset - pt->vborder;
	detail->v_sync = vsync_pulse_width;
	detail->v_back = vblank - detail->v_sync - detail->v_front;
	detail->h_pol = (pt->misc & HDMITX_EDID_PT_HSYNC_POSITIVE) ? true : false;
	detail->v_pol = (pt->misc & HDMITX_EDID_PT_VSYNC_POSITIVE) ? true : false;
	detail->progressive = (pt->misc & HDMITX_EDID_PT_INTERLACED) ? false : true;
	edid_do_interlace_quirk(detail, !detail->progressive);
	detail->field_rate = drv_hdmitx_modes_get_vrefresh(detail);
	return 0;
}

static struct hdmitx_display_mode *edid_create_mode_by_detailed(struct hdmitx_connector *connector,
	const struct detailed_timing *timing)
{
	struct hdmi_detail detail;
	struct hdmitx_display_mode *mode = NULL;

	if (memset_s(&detail, sizeof(detail), 0, sizeof(detail)) != 0) {
		dpu_pr_err("memset_s fail.\n");
		return NULL;
	}

	if (edid_get_detail_info(timing, &detail) != 0)
		return NULL;

	mode = drv_hdmitx_modes_create_mode_by_detail_timing(&detail);
	if (mode != NULL)
		return mode;

	mode = edid_create_detailed_mode(connector, &detail);

	return mode;
}

static u32 edid_add_established_modes(struct hdmitx_connector *connector, const struct edid *edid)
{
	u32 est_bits = edid->established_timings.t1 |
					  (edid->established_timings.t2 << 8) | /* need left shift 8 bits. */
					  ((edid->established_timings.mfg_rsvd & 0x80) << 9); /* need left shift 9 bits. */
	u32 i;
	u32 modes = 0;
	struct hdmitx_display_mode *newmode = NULL;

	for (i = 0; i <= EDID_EST_TIMING_NUM; i++) {
		if (!(est_bits & (1 << i)))
			continue;

		newmode = drv_hdmitx_modes_create_mode_by_vic(VIC_VESA_800X600P60 + i);
		if (newmode != NULL) {
			newmode->parse_type |= MODE_TYPE_ESTABLISHED_TIMINGE;
			drv_hdmitx_modes_add_probed_mode(connector, newmode);
			modes++;
		}
	}

	return modes;
}

static void edid_do_standard_modes(const struct detailed_timing *timing, void *c)
{
	const struct std_timing *std = NULL;
	struct hdmitx_display_mode *newmode = NULL;
	struct detailed_mode_closure *closure = c;
	const struct detailed_non_pixel *data = &timing->data.other_data;
	struct hdmitx_connector *connector = closure->connector;
	const struct edid *edid = closure->edid;
	u32 i;

	if (data->type != EDID_DETAILED_STD_MODES)
		return;

	for (i = 0; i < 6; i++) { /* std timing max size is 6. */
		std = &data->data.timings[i];
		newmode = edid_create_mode_by_std(connector, edid, std);
		if (newmode != NULL) {
			newmode->parse_type |= MODE_TYPE_STD_TIMINGE;
			drv_hdmitx_modes_add_probed_mode(connector, newmode);
			closure->modes++;
		}
	}
}

static u32 edid_add_standard_modes(struct hdmitx_connector *connector, const struct edid *edid)
{
	u32 i;
	u32 modes = 0;
	struct hdmitx_display_mode *newmode = NULL;
	struct detailed_mode_closure closure = {
		.connector = connector,
		.edid = edid,
	};

	for (i = 0; i < EDID_STD_TIMING_NUM; i++) {
		newmode = edid_create_mode_by_std(connector, edid, &edid->standard_timings[i]);
		if (newmode != NULL) {
			newmode->parse_type |= MODE_TYPE_STD_TIMINGE;
			drv_hdmitx_modes_add_probed_mode(connector, newmode);
			modes++;
		}
	}

	if (version_comparison(edid, 1, 0))
		edid_for_each_detailed_block((u8 *)edid, edid_do_standard_modes, &closure);

	return modes + closure.modes;
}

static void edid_do_detailed_mode(const struct detailed_timing *timing, void *c)
{
	struct detailed_mode_closure *closure = c;
	struct hdmitx_display_mode *newmode = NULL;

	if (!timing->pixel_clock)
		return;

	newmode = edid_create_mode_by_detailed(closure->connector, timing);
	if (newmode == NULL)
		return;

	if (closure->preferred)
		newmode->type |= HDMITX_MODE_TYPE_PREFERRED;

	/*
	 * Detailed modes are limited to 10kHz pixel clock resolution,
	 * so fix up anything that looks like CEA/HDMITX mode, but the clock
	 * is just slightly off.
	 */
	newmode->parse_type |= MODE_TYPE_DETAILED_TIMINGE;
	drv_hdmitx_modes_add_probed_mode(closure->connector, newmode);
	closure->modes++;
	closure->preferred = false;
}

static u32 edid_add_detailed_modes(struct hdmitx_connector *connector, const struct edid *edid)
{
	struct detailed_mode_closure closure = {
		.connector = connector,
		.edid = edid,
		.preferred = true,
	};

	if (closure.preferred && !version_comparison(edid, 1, 3)) /* version min is 3 */
		closure.preferred = (edid->features & HDMITX_EDID_FEATURE_PREFERRED_TIMING);

	edid_for_each_detailed_block((u8 *)edid, edid_do_detailed_mode, &closure);

	return closure.modes;
}

static u8 *edid_find_cea_extension(const struct edid *edid)
{
	return find_edid_extension(edid, CEA_EXTEND);
}

static struct hdmitx_display_mode *edid_create_mode_by_vic(const u8 *video_db,
														   u8 video_len,
														   u8 video_index)
{
	struct hdmitx_display_mode *newmode = NULL;
	bool native_mode = false;
	u8 vic, svd;

	if (video_index >= video_len)
		return NULL;

	/* CEA modes are numbered 1..127 or 192..219 */
	svd = video_db[video_index];
	if (svd >= 129 && svd <= 192)  /* svd is 129~192, is native mode. */
		native_mode = true;

	vic = svd_to_vic(svd);
	if (!cea_vic_is_valid(vic))
		return NULL;

	newmode = drv_hdmitx_modes_create_mode_by_vic(vic);
	if (newmode == NULL)
		return NULL;

	newmode->native_mode = native_mode;

	return newmode;
}

static u32 edid_do_y420vdb_modes(struct hdmitx_connector *connector,
	const u8 *svds, u8 svds_len)
{
	u32 modes = 0;
	u32 i;
	u8 vic;
	struct hdmitx_display_mode *newmode = NULL;
	struct color_property *color = &connector->color;

	for (i = 0; i < svds_len; i++) {
		vic = svd_to_vic(svds[i]);
		if (!cea_vic_is_valid(vic))
			continue;

		newmode = drv_hdmitx_modes_create_mode_by_vic(vic);
		if (newmode == NULL)
			break;

		newmode->parse_type |= MODE_TYPE_Y420VDB;
		drv_hdmitx_modes_add_probed_mode(connector, newmode);
		modes++;
	}

	if (modes > 0) {
		color->format.ycbcr420 = true;
		color->depth.y420_24 = true;
	}
	return modes;
}

static u32 edid_do_cea_modes(struct hdmitx_connector *connector, const u8 *db, u8 len)
{
	u32 i;
	u32 modes = 0;
	struct drm_hdmitx_info *hdmi = &connector->display_info.hdmi;
	struct hdmitx_display_mode *mode = NULL;

	for (i = 0; i < len; i++) {
		mode = edid_create_mode_by_vic(db, len, i);
		if (mode != NULL) {
			mode->parse_type |= MODE_TYPE_VDB;
			/*
			 * The YCBCR420 functional block contains a bitmap, which
			 * provides the index of the CEA mode in the CEA VDB,YCBCR
			 * 420 sampling outputs (except for RGB/YCBCR444, etc.) can
			 * also be supported. For example, if bit 0 in the bitmap
			 * is set,The first mode in the VDB can also support YCBCR420
			 * output.The YCBCR420 mode is added only when the receiver
			 * has the HDMITX 2.0 function.
			 */
			if ((i < 64) && (hdmi->y420_cmdb_map & (1ULL << i)))  /* max size is 64. */
				mode->parse_type |= MODE_TYPE_VDB_Y420CMDB;

			/*
			 * Mark the first mode in the video data block of EDID.
			 */
			if (i == 0)
				mode->first_mode = true;

			drv_hdmitx_modes_add_probed_mode(connector, mode);
			modes++;
		}
	}

	return modes;
}

static u32 edid_add_mandatory_stereo_modes(const struct hdmitx_connector *connector)
{
	struct hdmitx_display_mode *mode = NULL;
	u32 i;
	const struct stereo_mandatory_mode *mandatory = NULL;

	list_for_each_entry(mode, &connector->probed_modes, head) {
		for (i = 0; i < hdmitx_array_size(g_stereo_mandatory_modes); i++) {
			if (!stereo_match_mandatory(mode, &g_stereo_mandatory_modes[i]))
				continue;

			mandatory = &g_stereo_mandatory_modes[i];
			mode->detail.mode_3d |= mandatory->flags;
		}
	}

	return 0;
}

static u32 edid_add_hdmi_mode(struct hdmitx_connector *connector, u8 hdmi_vic)
{
	struct hdmitx_display_mode *newmode = NULL;
	u32 vic;

	if (!hdmi_vic_is_valid(hdmi_vic)) {
		dpu_pr_err("Unknown HDMITX VIC: %d\n", hdmi_vic);
		return 0;
	}

	/* hdmi_vic to vic,according to HDMITX2.1 spec Table 10-2 */
	switch (hdmi_vic) {
	case 1: /* hdmi_vic is 1. */
		vic = VIC_3840X2160P30_16_9;
		break;
	case 2: /* hdmi_vic is 2. */
		vic = VIC_3840X2160P25_16_9;
		break;
	case 3: /* hdmi_vic is 3. */
		vic = VIC_3840X2160P24_16_9;
		break;
	case 4: /* hdmi_vic is 4. */
		vic = VIC_4096X2160P24_256_135;
		break;
	default:
		return 0;
	}

	newmode = drv_hdmitx_modes_create_mode_by_vic(vic);
	if (newmode == NULL)
		return 0;

	newmode->parse_type |= MODE_TYPE_VSDB_4K;
	drv_hdmitx_modes_add_probed_mode(connector, newmode);

	return 1;
}

static u32 edid_add_3d_struct_modes(struct hdmitx_connector *connector, u16 structure,
	const u8 *video_db, u8 video_len, u8 video_index)
{
	struct hdmitx_display_mode *newmode = NULL;
	u32 modes = 0;

	if (structure & (1 << 0)) {
		newmode = edid_create_mode_by_vic(video_db, video_len, video_index);
		if (newmode != NULL) {
			newmode->detail.mode_3d |= HDMITX_3D_BZ_FRAME_PACKING;
			drv_hdmitx_modes_add_probed_mode(connector, newmode);
			modes++;
		}
	}
	if (structure & (1 << 6)) { /* check 6th bit is not 0. */
		newmode = edid_create_mode_by_vic(video_db, video_len, video_index);
		if (newmode != NULL) {
			newmode->detail.mode_3d |= HDMITX_3D_BZ_TOP_AND_BOTTOM;
			drv_hdmitx_modes_add_probed_mode(connector, newmode);
			modes++;
		}
	}
	if (structure & (1 << 8)) { /* check 8th bit is not 0. */
		newmode = edid_create_mode_by_vic(video_db, video_len, video_index);
		if (newmode != NULL) {
			newmode->detail.mode_3d |= HDMITX_3D_BZ_SIDE_BY_SIDE_HALF;
			newmode->parse_type |= MODE_TYPE_VSDB_3D;
			drv_hdmitx_modes_add_probed_mode(connector, newmode);
			modes++;
		}
	}

	return modes;
}

static u32 edid_vsdb_get_multi_len(u32 multi_present)
{
	u32 multi_len;

	if (multi_present == 1)  /* present is 1, length is 2. */
		multi_len = 2;
	else if (multi_present == 2)  /* present is 2, length is 4. */
		multi_len = 4;
	else
		multi_len = 0;

	return multi_len;
}

static s32 edid_vsdb_check_fields(u8 len, const u8 *db, u32 *offset)
{
	s32 ret = -1;

	if (len < 8)  /* if length is less than 8, vsdb payload is null. */
		goto out;

	/* no HDMITX_Video_Present */
	if (!(db[8] & (1 << 5)))  /* check 5th bit of db's 8th byte is 0. */
		goto out;

	/* Latency_Fields_Present */
	if (db[8] & (1 << 7))  /* check 7th bit of db's 8th byte is 0. */
		*offset += 2; /* offset need add 2. */

	/* I_Latency_Fields_Present */
	if (db[8] & (1 << 6))  /* check 6th bit of db's 8th byte is 0. */
		*offset += 2; /* offset need add 2. */

	/*
	 * The declared length is not long enough for the 2 first bytes
	 * of additional video format capabilities
	 */
	if (len < (8 + *offset + 2)) /* if length is less than 8 +  offset + 2, vsdb payload is null. */
		goto out;

	ret = 0;

out:
	return ret;
}

static void edid_vsdb_detail_present(struct hdmitx_connector *connector, const u8 *db, u32 *modes,
	const u8 *video_db, struct edid_vsdb_info vsdb_info)
{
	struct hdmitx_display_mode *newmode = NULL;
	s32 vic_index;
	u32 i;
	u32 newflag = 0;
	bool detail_present;

	for (i = 0; i < vsdb_info.diff_len; i++) {
		/* According to the EDID spec, the detail_present = ((db[8 + offset + i] & 0x0f) > 7). */
		detail_present = ((db[8 + vsdb_info.offset + i] & 0x0f) > 7);

		if (detail_present && (i + 1 == vsdb_info.diff_len))
			break;

		/* According to the EDID spec, the vic_index = db[8 + offset + i] >> 4. */
		vic_index = db[8 + vsdb_info.offset + i] >> 4;

		/* According to the EDID spec, the 3D_Structure_X = db[8 + offset + i] & 0x0f. */
		switch (db[8 + vsdb_info.offset + i] & 0x0f) {
		case 0: /* 3D_Structure_X is 0. */
			newflag = HDMITX_3D_BZ_FRAME_PACKING;
			break;
		case 6: /* 3D_Structure_X is 6. */
			newflag = HDMITX_3D_BZ_TOP_AND_BOTTOM;
			break;
		case 8: /* 3D_Structure_X is 8. */
			/* According to the EDID spec, the 3D_Detail_X = db[9 + offset + i] >> 4. */
			if ((db[9 + vsdb_info.offset + i] >> 4) == 1)
				newflag = HDMITX_3D_BZ_SIDE_BY_SIDE_HALF;
			break;
		default:
			break;
		}
		if (newflag != 0) {
			newmode = edid_create_mode_by_vic(video_db, vsdb_info.video_len, vic_index);
			if (newmode != NULL) {
				newmode->detail.mode_3d |= newflag;
				newmode->parse_type |= MODE_TYPE_VSDB_3D;
				drv_hdmitx_modes_add_probed_mode(connector, newmode);
				*modes += 1;
			}
		}

		if (detail_present)
			i++;
	}
}

static u16 edid_vsdb_get_mask(u32 offset, const u8 *db, u32 multi_present)
{
	u16 mask;

	/* check if 3D_MASK is present, if multi_present is 2, has 3D_MASK. */
	if (multi_present == 2) {
		/* According to the EDID spec, the mask = (db[10 + offset] << 8) | db[11 + offset]. */
		mask = (db[10 + offset] << 8) | db[11 + offset];
	} else {
		mask = 0xffff;
	}

	return mask;
}

static u32 edid_vsdb_get_multi_present(const struct hdmitx_connector *connector, u32 *offset,
	const u8 *db, u32 *modes)
{
	u32 multi_present = 0;

	/* 3D_Present */
	*offset += 1;

	if (db[8 + *offset] & (1 << 7)) { /* check 7th bit of db's (8+offset)th byte is 0. */
		*modes += edid_add_mandatory_stereo_modes(connector);
		/* According to the EDID spec, the multi_present = (db[8 + offset] & 0x60) >> 5. */
		multi_present = (db[8 + *offset] & 0x60) >> 5;
	}

	return multi_present;
}

static u32 edid_vsdb_get_hdmi_3d_len(struct hdmitx_connector *connector, u32 *offset, const u8 *db,
	u8 len, u32 *modes)
{
	u8 vic, vic_len;
	u32 i, hdmi_3d_len;

	*offset += 1;

	/* According to the EDID spec, the vic_len = db[8 + offset] >> 5. */
	vic_len = db[8 + *offset] >> 5;
	/* According to the EDID spec, the vic_len = db[8 + offset] & 0x1f. */
	hdmi_3d_len = db[8 + *offset] & 0x1f;

	for (i = 0; i < vic_len && len >= (9 + *offset + i); i++) { /* base offset is 9. */
		/* According to the EDID spec, the vic = db[9 + offset + i]. */
		vic = db[9 + *offset + i];
		*modes += edid_add_hdmi_mode(connector, vic);
	}
	*offset += 1 + vic_len;

	return hdmi_3d_len;
}

static u32 edid_do_hdmi_vsdb_modes(struct hdmitx_connector *connector, const u8 *db, u8 len,
	const u8 *video_db, u8 video_len)
{
	struct hdmitx_display_info *info = &connector->display_info;
	u16 mask, structure_all;
	u32 modes = 0;
	u32 multi_present, multi_len, hdmi_3d_len, i;
	s32 ret;
	struct edid_vsdb_info vsdb_info;

	if (memset_s(&vsdb_info, sizeof(vsdb_info), 0x00, sizeof(vsdb_info)) != EOK) {
		dpu_pr_err("edid_vsdb_info memset_s fail.\n");
		goto out;
	}

	vsdb_info.video_len = video_len;

	ret = edid_vsdb_check_fields(len, db, &vsdb_info.offset);
	if (ret != 0)
		goto out;

	multi_present = edid_vsdb_get_multi_present(connector, &vsdb_info.offset, db, &modes);
	hdmi_3d_len = edid_vsdb_get_hdmi_3d_len(connector, &vsdb_info.offset, db, len, &modes);
	multi_len = edid_vsdb_get_multi_len(multi_present);

	/* if length is less than (8 + offset + hdmi_3d_len - 1), no 3D struct mode. */
	if (len < (8 + vsdb_info.offset + hdmi_3d_len - 1))
		goto out;

	if (hdmi_3d_len < multi_len)
		goto out;

	/* if multi_present is 1 or 2, has structure_all. */
	if (multi_present == 1 || multi_present == 2) {
		/* According to the EDID spec, the structure_all = (db[8 + offset] << 8) | db[9 + offset]. */
		structure_all = (db[8 + vsdb_info.offset] << 8) | db[9 + vsdb_info.offset];
		mask = edid_vsdb_get_mask(vsdb_info.offset, db, multi_present);

		for (i = 0; i < 16; i++) { /* max is 16. */
			if (mask & (1 << i))
				modes += edid_add_3d_struct_modes(connector, structure_all, video_db, vsdb_info.video_len, i);
		}
	}

	vsdb_info.offset += multi_len;
	vsdb_info.diff_len = hdmi_3d_len - multi_len;

	edid_vsdb_detail_present(connector, db, &modes, video_db, vsdb_info);

out:
	if (modes > 0)
		info->has_hdmi_infoframe = true;

	return modes;
}

static void edid_parse_y420cmdb_bitmap(struct hdmitx_connector *connector,
	const u8 *db)
{
	struct hdmitx_display_info *info = &connector->display_info;
	struct color_property *color = &connector->color;
	struct drm_hdmitx_info *hdmi = &info->hdmi;
	u8 map_len;
	u8 count;
	u64 map = 0;

	map_len = cea_db_payload_len(db) - 1;
	if (map_len == 0) {
		/* All CEA modes support ycbcr420 sampling also. */
		hdmi->y420_cmdb_map = (u64)~0ULL;
		color->format.ycbcr420 = true;
		color->depth.y420_24 = true;
		return;
	}

	/*
	 * This map shows which VDBs in the existing CEA block mode
	 * can also support YCBCR420 output. If bit = 0, the first
	 * VDB mode can also support YCBCR420 output. Before parsing
	 * the VDB itself, we will parse and preserve this mapping to
	 * avoid passing through the same piece again and again.
	 * The maximum possible size of this block is not known, and the
	 * maximum bitmap block size is limited to 8 bytes. Each byte
	 * can solve eight CEA modes. In this way, the map can solve
	 * the first 64 (8 x 8) SVDs.
	 */
	if (map_len > 8)  /* map_len max value is 8. */
		map_len = 8; /* set map_len to 8, when map_len greater than 8. */

	for (count = 0; count < map_len; count++)
		map |= (u64)db[2 + count] << (8 * count); /* db's 2th byte need left shift 8 bits */

	if (map) {
		color->format.ycbcr420 = true;
		color->depth.y420_24 = true;
	}

	hdmi->y420_cmdb_map = map;
}

static u32 edid_add_cea_modes(struct hdmitx_connector *connector, const struct edid *edid)
{
	const u8 *cea = edid_find_cea_extension(edid);
	const u8 *db = NULL;
	const u8 *hdmi = NULL;
	const u8 *video = NULL;
	u8 dbl;
	u8 hdmi_len;
	u8 video_len = 0;
	u32 modes = 0;
	u32 i, start, end;
	const u8 *vdb420 = NULL;

	if (cea == NULL || cea_revision(cea) < 3)  /* the min cea revision is 3. */
		return modes;

	if (cea_db_offsets(cea, &start, &end))
		return modes;

	for_each_cea_db(cea, i, start, end) {
		db = &cea[i];
		dbl = cea_db_payload_len(db);

		if (cea_db_tag(db) == EDID_VIDEO_TAG) {
			video = db + 1;
			video_len = dbl;
			modes += edid_do_cea_modes(connector, video, dbl);
		} else if (cea_db_is_hdmi_vsdb(db)) {
			hdmi = db;
			hdmi_len = dbl;
		} else if (cea_db_is_y420vdb(db)) {
			vdb420 = &db[2]; /* vdb420 is db's 2th byte. */

			/* Add 4:2:0(only) modes present in EDID */
			modes += edid_do_y420vdb_modes(connector, vdb420, dbl - 1);
		}
	}

	/*
	 * We parse the HDMITX VSDB after having added the cea modes as we will
	 * be patching their flags when the sink supports stereo 3D.
	 */
	if (hdmi != NULL)
		modes += edid_do_hdmi_vsdb_modes(connector, hdmi, hdmi_len, video, video_len);

	return modes;
}

static void edid_parse_hdmi_vsdb_audio(struct hdmitx_connector *connector, const u8 *db)
{
	u8 len;

	len = cea_db_payload_len(db);
	/*
	 * if I_Latency_Fields_Present flag == 1(byte8 [6]) then this field only indicates the latency while
	 * receiving progressive video formats, otherwise this field indicates the latency when
	 * receiving any video format. Value is number of (milliseconds / 2) + 1 with a maximum
	 * allowed value of 251 (indicating 500 millisecond duration). A value of 0 indicates that the
	 * field is not valid or that the latency is un known. A value of 255 indicates that no video is
	 * supported in this device or downstream.
	 */
	if (len >= 10) { /* if db's length is less than 10, no p_latency. */
		connector->latency.latency_present = (db[8] & (1 << 7)) && /* check the bit7 of db's 8th byte. */
											 (!db[9] && (db[9] <= 251)) && /* check the db's 9th byte is in 0~251. */
											 (!db[10] && (db[10] <= 251)); /* check the db's 10th byte is in 0~251. */

		if (!db[9] && (db[9] <= 251)) /* check the db's 9th byte is in 0~251. */
			/* According to the EDID spec, the p_video = (db[9] - 1) * 2. */
			connector->latency.p_video = (db[9] - 1) * 2;

		if (!db[10] && (db[10] <= 251))  /* check the db's 10th byte is in 0~251. */
			/* According to the EDID spec, the p_audio = (db[10] - 1) * 2. */
			connector->latency.p_audio = (db[10] - 1) * 2;

		connector->latency.i_video = connector->latency.p_video;
		connector->latency.i_audio = connector->latency.p_audio;
	}

	/*
	 * I_Latency_Fields_Present shall  be zero if Latency_Fields_Present(byte8 [7]) is zero.
	 * spec no mention about value is number of (milliseconds / 2) + 1 with a maximum.
	 */
	if ((len >= 12) && /* if db's length is less than 12, no i_latency. */
		(db[8] & (1 << 7)) && /* check the the bit7 of db's 8th byte. */
		(db[8] & (1 << 6))) { /* check the the bit6 of db's 8th byte. */
		if (!db[11] && (db[11] <= 251)) /* check the db's 11th byte is in 0~251. */
			/* According to the EDID spec, the i_video = (db[11] - 1) * 2. */
			connector->latency.i_video = (db[11] - 1) * 2;

		if (!db[12] && (db[12] <= 251)) /* check the db's 12th byte is in 0~251. */
			/* According to the EDID spec, the i_audio = (db[12] - 1) * 2. */
			connector->latency.i_audio = (db[12] - 1) * 2;
	}
}

static void edid_parse_vcdb(struct hdmitx_connector *connector, const u8 *db)
{
	struct color_property *color = &connector->color;

	/* According to the EDID spec, the rgb_qs_selecable = !!(db[2] & EDID_CEA_VCDB_QS). */
	color->quantization.rgb_qs_selecable = !!(db[2] & EDID_CEA_VCDB_QS);
	/* According to the EDID spec, the ycc_qy_selecable = !!(db[2] & EDID_CEA_VCDB_QY) */
	color->quantization.ycc_qy_selecable = !!(db[2] & EDID_CEA_VCDB_QY);
}

static void edid_parse_colorimetry(struct hdmitx_connector *connector, const u8 *db)
{
	struct color_property *color = &connector->color;

	/* According to the EDID spec, the xvycc601 = !!(db[2] & (1 << 0)). */
	color->colorimetry.xvycc601 = !!(db[2] & (1 << 0));
	/* According to the EDID spec, the xvycc709 = !!(db[2] & (1 << 1)). */
	color->colorimetry.xvycc709 = !!(db[2] & (1 << 1));
	/* According to the EDID spec, the sycc601 = !!(db[2] & (1 << 2)). */
	color->colorimetry.sycc601 = !!(db[2] & (1 << 2));
	/* According to the EDID spec, the adobe_ycc601 = !!(db[2] & (1 << 3)). */
	color->colorimetry.adobe_ycc601 = !!(db[2] & (1 << 3));
	/* According to the EDID spec, the adobe_rgb = !!(db[2] & (1 << 4)). */
	color->colorimetry.adobe_rgb = !!(db[2] & (1 << 4));
	/* According to the EDID spec, the bt2020_cycc = !!(db[2] & (1 << 5)). */
	color->colorimetry.bt2020_cycc = !!(db[2] & (1 << 5));
	/* According to the EDID spec, the bt2020_ycc = !!(db[2] & (1 << 6)). */
	color->colorimetry.bt2020_ycc = !!(db[2] & (1 << 6));
	/* According to the EDID spec, the bt2020_rgb = !!(db[2] & (1 << 7)). */
	color->colorimetry.bt2020_rgb = !!(db[2] & (1 << 7));
	/* According to the EDID spec, the dci_p3 = !!(db[3] & (1 << 7)). */
	color->colorimetry.dci_p3 = !!(db[3] & (1 << 7));
}

static void edid_parse_hdr_static(struct hdmitx_connector *connector, const u8 *db)
{
	u32 db_len;
	struct hdr_property *hdr = &connector->hdr;

	db_len = cea_db_payload_len(db);
	if (db_len < 2) /* if db_len is less than 2, no static hdr description. */
		return;

	/* According to the EDID spec, the traditional_sdr = !!(db[2] & (1 << 0)). */
	hdr->eotf.traditional_sdr = !!(db[2] & (1 << 0));
	/* According to the EDID spec, the traditional_hdr = !!(db[2] & (1 << 1)). */
	hdr->eotf.traditional_hdr = !!(db[2] & (1 << 1));
	/* According to the EDID spec, the st2084_hdr = !!(db[2] & (1 << 2)). */
	hdr->eotf.st2084_hdr = !!(db[2] & (1 << 2));
	/* According to the EDID spec, the hlg = !!(db[2] & (1 << 3)). */
	hdr->eotf.hlg = !!(db[2] & (1 << 3));

	/* According to the EDID spec, the s_type1 = db_len > 2 ? (!!(db[3] & (1 << 0))) : false. */
	hdr->st_metadata.s_type1 = db_len > 2 ? (!!(db[3] & (1 << 0))) : false;
	/* According to the EDID spec, the max_lum_cv = db_len > 3 ? db[4] : 0. */
	hdr->st_metadata.max_lum_cv = db_len > 3 ? db[4] : 0;
	/* According to the EDID spec, the aver_lum_cv = db_len > 4 ? db[5] : 0. */
	hdr->st_metadata.aver_lum_cv = db_len > 4 ? db[5] : 0;
	/* According to the EDID spec, the min_lum_cv = db_len > 5 ? db[6] : 0. */
	hdr->st_metadata.min_lum_cv = db_len > 5 ? db[6] : 0;
}

static void edid_parse_hdr_dynamic_type(struct hdmitx_connector *connector, u16 type, u8 version)
{
	struct hdr_property *hdr = &connector->hdr;

	if (type == 0x0001) {
		hdr->dy_metadata.d_type1_support = true;
		hdr->dy_metadata.d_type1_version = version;
	} else if (type == 0x0002) {
		hdr->dy_metadata.d_type2_support = true;
		hdr->dy_metadata.d_type2_version = version;
	} else if (type == 0x0003) {
		hdr->dy_metadata.d_type3_support = true;
	} else if (type == 0x0004) {
		hdr->dy_metadata.d_type4_support = true;
		hdr->dy_metadata.d_type4_version = version;
	} else {
		dpu_pr_debug("un-known dynamic hdr type:%d,version:%d\n", type, version);
	}
}

static void edid_parse_hdr_dynamic(struct hdmitx_connector *connector, const u8 *db)
{
	u32 db_len;
	u32 type_len = 0;
	u32 len;
	u16 dynamic_metadata_type;
	u8 dynamic_version;

	db_len = cea_db_payload_len(db);

	for (len = 1; db_len - len < 3; len += type_len) { /* length is less than 3. */
		len++;
		type_len = db[len];
		if (type_len >= 2) { /* length is less than 2. */
			len++;
			dynamic_metadata_type = db[len] << 8; /* left shift 8 bits. */
			len++;
			dynamic_metadata_type |= db[len];
			type_len -= 2; /* length is less than 2. */
		} else {
			dpu_pr_info("error dynamic !\n");
			break;
		}

		if (type_len) {
			len++;
			dynamic_version = db[len] & 0xf;
			type_len--;
		} else {
			dynamic_version = 0;
		}

		edid_parse_hdr_dynamic_type(connector, dynamic_metadata_type, dynamic_version);
	}
}

static void edid_parse_dolbyvision_v0(struct dolby_v0 *v0, const u8 *db)
{
	/* According to the EDID spec, the y422_36bit = !!(db[5] & BIT0_MASK). */
	v0->y422_36bit = !!(db[5] & BIT0_MASK);
	/* According to the EDID spec, the is_2160p60 = !!(db[5] & BIT1_MASK). */
	v0->is_2160p60 = !!(db[5] & BIT1_MASK);
	/* According to the EDID spec, the global_dimming = !!(db[5] & BIT2_MASK). */
	v0->global_dimming = !!(db[5] & BIT2_MASK);
	/* According to the EDID spec, the white_x = (db[16] << 4) | ((db[15] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->white_x = (db[16] << 4) | ((db[15] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the white_y = (db[17] << 4) | (db[15] & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->white_y = (db[17] << 4) | (db[15] & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the dm_minor_ver = (db[21] & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->dm_minor_ver = (db[21] & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the dm_major_ver = ((db[21] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->dm_major_ver = ((db[21] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the target_min_pq = (db[19] << 4)|((db[18] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->target_min_pq = (db[19] << 4) | ((db[18] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the target_max_pq = (db[20] << 4) | (db[18] & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->target_max_pq = (db[20] << 4) | (db[18] & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the red_x = (db[7] << 4) | ((db[6] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->red_x = (db[7] << 4) | ((db[6] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the red_y = (db[8] << 4) | ((db[6]) & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->red_y = (db[8] << 4) | ((db[6]) & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the green_x = (db[10] << 4) | ((db[9] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->green_x = (db[10] << 4) | ((db[9] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the green_y = (db[11] << 4) | (db[9] & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->green_y = (db[11] << 4) | (db[9] & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the blue_x = (db[13] << 4) | ((db[12] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->blue_x = (db[13] << 4) | ((db[12] >> 4) & EDID_DOLBY_LOWER_NIBBLE_MASK);
	/* According to the EDID spec, the blue_y = (db[14] << 4) | (db[12] & EDID_DOLBY_LOWER_NIBBLE_MASK). */
	v0->blue_y = (db[14] << 4) | (db[12] & EDID_DOLBY_LOWER_NIBBLE_MASK);
}

static void edid_parse_dolbyvision_v1(struct dolby_v1 *v1, const u8 *db)
{
	u32 db_len = cea_db_payload_len(db);
	if (db_len == EDID_DOLBY_VSVDB_VERSION_1_LEN) { /* V1 15Bytes (standard) */
		v1->y422_36bit = !!(db[5] & BIT0_MASK); /* the bit 0 of db's 5th byte. */
		v1->is_2160p60 = !!(db[5] & BIT1_MASK); /* the bit 1 of db's 5th byte. */
		v1->global_dimming = (db[6] & BIT0_MASK); /* the bit 0 of db's 6th byte. */
		v1->colorimetry = (db[7] & BIT0_MASK); /* the bit 0 of db's 7th byte. */
		v1->dm_version = ((db[5] >> 2) & EDID_DOLBY_LOWER_3BIT_MASK); /* db's 5th byte right shift 2 bits. */
		v1->low_latency = 0;
		v1->target_min_lum = ((db[6] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK); /* db's 6th byte right shift 1 bits. */
		v1->target_max_lum = ((db[7] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK); /* db's 7th byte right shift 1 bits. */
		v1->red_x = db[9]; /* db's 9th byte. */
		v1->red_y = db[10]; /* db's 10th byte. */
		v1->green_x = db[11]; /* db's 11th byte. */
		v1->green_y = db[12]; /* db's 12th byte. */
		v1->blue_x = db[13]; /* db's 13th byte. */
		v1->blue_y = db[14]; /* db's 14th byte. */
	} else { /* V1 12Bytes (low-latency) */
		v1->y422_36bit = !!(db[5] & BIT0_MASK); /* the bit 0 of db's 5th byte. */
		v1->is_2160p60 = !!(db[5] & BIT1_MASK); /* the bit 1 of db's 5th byte. */
		v1->global_dimming = (db[6] & BIT0_MASK); /* the bit 0 of db's 6th byte. */
		v1->dm_version = ((db[5] >> 2) & EDID_DOLBY_LOWER_3BIT_MASK); /* db's 5th byte right shift 2 bits. */
		v1->colorimetry = (db[7] & BIT0_MASK); /* the bit 0 of db's 7th byte. */
		v1->low_latency = db[8] & EDID_DOLBY_LOWER_2BIT_MASK; /* the lower 2 bit of db's 8th byte. */
		v1->target_min_lum = ((db[6] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK); /* db's 6th byte right shift 1 bits. */
		v1->target_max_lum = ((db[7] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK); /* db's 7th byte right shift 1 bits. */
		/* According to the EDID spec, the red_x = 0xa0 | ((db[11] >> 3) & EDID_DOLBY_LOWER_5BIT_MASK). */
		v1->red_x = 0xa0 | ((db[11] >> 3) & EDID_DOLBY_LOWER_5BIT_MASK);

		v1->red_y = 0x40 | /* 0x40 */
					((db[9] & BIT0_MASK) | /* the bit 0 of db's 9th byte */
					((db[10] & BIT0_MASK) << 1) | /* the bit 0 of db's 10th byte */
					((db[11] & EDID_DOLBY_LOWER_3BIT_MASK) << 2)); /* db's 11th byte left shift 2 bits. */
		/* According to the EDID spec, the green_x = 0x00 | ((db[9] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK). */
		v1->green_x = 0x00 | ((db[9] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK);
		/* According to the EDID spec, the green_y = 0x80 | ((db[10] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK). */
		v1->green_y = 0x80 | ((db[10] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK);
		/* According to the EDID spec, the blue_x = 0x20 | ((db[8] >> 5) & EDID_DOLBY_LOWER_3BIT_MASK). */
		v1->blue_x = 0x20 | ((db[8] >> 5) & EDID_DOLBY_LOWER_3BIT_MASK);
		/* According to the EDID spec, the blue_y = 0x08 | ((db[8] >> 2) & EDID_DOLBY_LOWER_3BIT_MASK). */
		v1->blue_y = 0x08 | ((db[8] >> 2) & EDID_DOLBY_LOWER_3BIT_MASK);
	}
}

static void edid_parse_dolbyvision_v2(struct dolby_v2 *v2, const u8 *db)
{
	v2->y422_36bit = !!(db[3] & BIT0_MASK); /* the bit 0 of db's 3th byte. */
	v2->back_light_ctrl = !!(db[3] & BIT1_MASK); /* the bit 1 of db's 3th byte. */
	v2->global_dimming = !!(db[4] & BIT2_MASK); /* the bit 2 of db's 4th byte. */
	v2->dm_version = ((db[3] >> 2) & EDID_DOLBY_LOWER_3BIT_MASK); /* db's 3th byte right shift 2 bits. */
	v2->back_lt_min_lum = db[4] & EDID_DOLBY_LOWER_2BIT_MASK; /* the lower 2 bits of db's 4th byte. */
	v2->interface = db[5] & EDID_DOLBY_LOWER_2BIT_MASK; /* the lower 2 bits of db's 5th byte. */
	/* According to the EDID spec, the y444_rgb_30b36b = ((db[6] & BIT0_MASK) << 1) | (db[7] & BIT0_MASK). */
	v2->y444_rgb_30b36b = ((db[6] & BIT0_MASK) << 1) | (db[7] & BIT0_MASK);
	v2->target_min_pq_v2 = (db[4] >> 3) & EDID_DOLBY_LOWER_5BIT_MASK; /* db's 4th byte right shift 3 bits. */
	v2->target_max_pq_v2 = (db[5] >> 3) & EDID_DOLBY_LOWER_5BIT_MASK; /* db's 5th byte right shift 3 bits. */
	/* According to the EDID spec, the red_x = 0xa0 | ((db[8] >> 3) & EDID_DOLBY_LOWER_5BIT_MASK). */
	v2->red_x = 0xa0 | ((db[8] >> 3) & EDID_DOLBY_LOWER_5BIT_MASK);
	/* According to the EDID spec, the red_y = 0x40 | ((db[9] >> 3) & EDID_DOLBY_LOWER_5BIT_MASK). */
	v2->red_y = 0x40 | ((db[9] >> 3) & EDID_DOLBY_LOWER_5BIT_MASK);
	/* According to the EDID spec, the green_x = 0x00 | ((db[6] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK). */
	v2->green_x = 0x00 | ((db[6] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK);
	/* According to the EDID spec, the green_y = 0x80 | ((db[7] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK). */
	v2->green_y = 0x80 | ((db[7] >> 1) & EDID_DOLBY_LOWER_7BIT_MASK);
	/* According to the EDID spec, the blue_x = 0x20 | (db[8] & EDID_DOLBY_LOWER_3BIT_MASK). */
	v2->blue_x = 0x20 | (db[8] & EDID_DOLBY_LOWER_3BIT_MASK);
	/* According to the EDID spec, the blue_y = 0x08 | (db[9] & EDID_DOLBY_LOWER_3BIT_MASK). */
	v2->blue_y = 0x08 | (db[9] & EDID_DOLBY_LOWER_3BIT_MASK);
}

static void edid_parse_dolbyvision(struct hdmitx_connector *connector, const u8 *db)
{
	u32 db_len;
	u8 version;
	struct dolby_property *dolby = &connector->dolby;

	db_len = cea_db_payload_len(db);

	version = (db[5] >> 5); /* db's 5th byte right shift 5 bits. */

	switch (version) {
	case EDID_DOLBY_VSVDB_VERSION_0:
		if (db_len != EDID_DOLBY_VSVDB_VERSION_0_LEN)
			break;

		dolby->support_v0 = true;
		edid_parse_dolbyvision_v0(&dolby->v0, db);
		break;
	case EDID_DOLBY_VSVDB_VERSION_1:
		if (db_len < 11) /* if db_len is less than 11, is invalid. */
			break;

		dolby->support_v1 = true;
		edid_parse_dolbyvision_v1(&dolby->v1, db);
		break;
	case EDID_DOLBY_VSVDB_VERSION_2:
		if (db_len < EDID_DOLBY_VSVDB_VERSION_2_LEN)
			break;

		dolby->support_v2 = true;
		edid_parse_dolbyvision_v2(&dolby->v2, db);
		break;
	default:
		break;
	}
}

static void edid_parse_cuva_hdr(struct hdmitx_connector *connector, const u8 *db)
{
	u32 db_len;
	struct cuva_property *cuva = &connector->cuva;

	db_len = cea_db_payload_len(db);
	if (db_len < 13) /* if db_len is less than 13, is invalid. */
		return;

	cuva->system_start_code = db[5]; /* system_start_code is db[5]. */
	cuva->version_code = (db[6] >> 4) & 0xf; /* version_code is (db[6] >> 4) & 0xf. */
	/* display_max_lum is (db[10] << 24) | (db[9] << 16) | (db[8] << 8) | db[7]. */
	cuva->display_max_lum = (db[10] << 24) | (db[9] << 16) | (db[8] << 8) | db[7];
	/* display_min_lum is (db[12] << 8) | db[11]. */
	cuva->display_min_lum = (db[12] << 8) | db[11];
	/* monitor_mode_support is (db[13] >> 6) & 0x1 */
	cuva->monitor_mode_support = (db[13] >> 6) & 0x1;
	/* rx_mode_support (db[13] >> 7) & 0x1 */
	cuva->rx_mode_support = (db[13] >> 7) & 0x1;
}

static void edid_parse_hdmi_forum_vsdb_for_vrr(struct hdmitx_connector *connector,
	const u8 *hf_vsdb)
{
	struct vrr_property *vrr = &connector->vrr;

	vrr->m_delta = !!(hf_vsdb[8] & (0x1 << 5)); /* the bit 5 of hf_vsdb's 8th byte. */
	vrr->cinema_vrr = !!(hf_vsdb[8] & (0x1 << 4)); /* the bit 4 of hf_vsdb's 8th byte. */
	vrr->cnm_vrr = !!(hf_vsdb[8] & (0x1 << 3)); /* the bit 3 of hf_vsdb's 8th byte. */
	vrr->fva = !!(hf_vsdb[8] & (0x1 << 2)); /* the bit 2 of hf_vsdb's 8th byte. */
	vrr->allm = !!(hf_vsdb[8] & (0x1 << 1)); /* the bit 1 of hf_vsdb's 8th byte. */
	vrr->fapa_start_locat = !!(hf_vsdb[8] & (0x1 << 0)); /* the bit 0 of hf_vsdb's 8th byte. */
	vrr->vrr_min = hf_vsdb[9] & 0x3f; /* the lower 6 bits of hf_vsdb's 9th byte(&0x3f). */
	/* According to the EDID spec, the vrr_max = ((hf_vsdb[9] & 0xc0) << 2) | hf_vsdb[10]. */
	vrr->vrr_max = ((hf_vsdb[9] & 0xc0) << 2) | hf_vsdb[10];
}

static void edid_parse_hdmi_forum_vsdb_for_dsc(struct hdmitx_connector *connector,
	const u8 *hf_vsdb)
{
	struct dsc_property *dsc = &connector->dsc;

	dsc->dsc_10bpc = !!(hf_vsdb[11] & (0x1 << 0)); /* the bit 0 of hf_vsdb's 11th byte. */
	dsc->dsc_12bpc = !!(hf_vsdb[11] & (0x1 << 1)); /* the bit 1 of hf_vsdb's 11th byte. */
	dsc->dsc_16bpc = !!(hf_vsdb[11] & (0x1 << 2)); /* the bit 2 of hf_vsdb's 11th byte. */
	dsc->all_bpp = !!(hf_vsdb[11] & (0x1 << 3)); /* the bit 3 of hf_vsdb's 11th byte. */

	dsc->y420 = !!(hf_vsdb[11] & (0x1 << 6)); /* the bit 6 of hf_vsdb's 11th byte. */
	dsc->dsc_1p2 = !!(hf_vsdb[11] & (0x1 << 7)); /* the bit 7 of hf_vsdb's 11th byte. */
	dsc->dsc_max_rate = (hf_vsdb[12] >> 4) & 0xf; /* the upper 4 bits of hf_vsdb's 12th byte(&0xf). */
	dsc->max_slice = hf_vsdb[12] & 0xf; /* the lower 4 bits of hf_vsdb's 12th byte(&0xf). */
	dsc->total_chunk_bytes = hf_vsdb[13] & 0x3f; /* the upper 6 bits of hf_vsdb's 13th byte(&0x3f). */
}

static void edid_parse_hdmi_forum_vsdb(struct hdmitx_connector *connector,
	const u8 *hf_vsdb)
{
	u32 db_len;
	u64 temp;
	u32 max_tmds_clock;
	struct hdmitx_display_info *display = &connector->display_info;
	struct scdc_property *scdc = &connector->scdc;
	struct color_property *color = &connector->color;

	db_len = cea_db_payload_len(hf_vsdb);
	display->has_hdmi_infoframe = true;

	if (db_len < 7) /* if db_len is less than 7, no more description. */
		return;

	if (hf_vsdb[6] & 0x80) { /* the bit 7 of hf_vsdb's 6th byte. (&0x80) */
		scdc->present = true;
		if (hf_vsdb[6] & 0x40) /* the bit 6 of hf_vsdb's 6th byte. (&0x40) */
			scdc->rr_capable = true;
	}
	display->dvi_dual = !!(hf_vsdb[7] & (0x1 << 1)); /* the bit 1 of hf_vsdb's 7th byte. */
	display->max_frl_rate = (hf_vsdb[7] >> 4) & 0xf; /* the upper 4 bits of hf_vsdb's 7th byte(&0xf). */
	color->depth.y420_30 = !!(hf_vsdb[7] & HDMITX_EDID_YCBCR420_DC_30); /* the bit 0 of hf_vsdb's 7th byte. */
	color->depth.y420_36 = !!(hf_vsdb[7] & HDMITX_EDID_YCBCR420_DC_36); /* the bit 1 of hf_vsdb's 7th byte. */
	color->depth.y420_48 = !!(hf_vsdb[7] & HDMITX_EDID_YCBCR420_DC_48); /* the bit 2 of hf_vsdb's 7th byte. */

	/*
	 * All HDMITX 2.0 monitors must support scrambling at a rate greater than 340 MHz.
	 * According to the norm, three factors confirm this:
	 * - Availability of the HF-VSDB block in the EDID (selected)
	 * - Non-zero Max_TMDS_Char_Rate submitted in HF-VSDB (Let's check it)
	 * - Provide SCDC support (Let's check).
	 * Let's see.
	 */
	if (hf_vsdb[5]) { /* check the hf_vsdb's 5th byte. */
		/* max clock is 5000 KHz times block value */
		temp = hf_vsdb[5] * 5000;
		max_tmds_clock = (u32)temp;
		if (max_tmds_clock > 340000) { /* tmds clock is more than 340000KHz. */
			display->max_tmds_clock = max_tmds_clock;
			dpu_pr_debug("HF-VSDB: max TMDS clock %d kHz\n", display->max_tmds_clock);
		}

		if (scdc->present) {
			/* Few sinks support scrambling for cloks < 340M */
			if ((hf_vsdb[6] & 0x8))  /* check the bit 3 of hf_vsdb's 6th byte. */
				scdc->lte_340mcsc = true;
		}
	}

	if (db_len < 10) /* if db_len is less than 10, no vrr description. */
		return;

	edid_parse_hdmi_forum_vsdb_for_vrr(connector, hf_vsdb);

	if (db_len < 13)  /* if db_len is less than 13, no dsc description. */
		return;

	/* dsc->ccbpci */
	edid_parse_hdmi_forum_vsdb_for_dsc(connector, hf_vsdb);
}

static void edid_parse_hdmi_deep_color_info(struct hdmitx_connector *connector,
	const u8 *hdmi)
{
	struct color_property *color = &connector->color;
	u32 dc_bpc = 0;

	/* HDMITX supports at least 8 bpc */
	color->depth.bpc = 8;

	if (cea_db_payload_len(hdmi) < 6)  /* if hdmi length is less than 6, no more description. */
		return;

	if (hdmi[6] & HDMITX_EDID_HDMITX_DC_30) { /* the bit 4 of hdmi's 6th byte. */
		dc_bpc = 10; /* 10bit */
		color->depth.rgb_30 = true;
		dpu_pr_debug("%s: HDMITX sink does deep color 30.\n", connector->name);
	}

	if (hdmi[6] & HDMITX_EDID_HDMITX_DC_36) { /* the bit 5 of hdmi's 6th byte. */
		dc_bpc = 12; /* 12bit */
		color->depth.rgb_36 = true;
		dpu_pr_debug("%s: HDMITX sink does deep color 36.\n", connector->name);
	}

	if (hdmi[6] & HDMITX_EDID_HDMITX_DC_48) { /* the bit 6 of hdmi's 6th byte. */
		dc_bpc = 16; /* 16bit */
		color->depth.rgb_48 = true;
		dpu_pr_debug("%s: HDMITX sink does deep color 48.\n", connector->name);
	}

	if (dc_bpc == 0) {
		dpu_pr_debug("%s: No deep color support on this HDMITX sink.\n", connector->name);
		return;
	}

	dpu_pr_debug("%s: Assigning HDMITX sink color depth as %d bpc.\n", connector->name, dc_bpc);
	color->depth.bpc = dc_bpc;

	/*
	 * Deep color support mandates RGB444 support for all video
	 * modes and forbids YCRCB422 support for all video modes per
	 * HDMITX 1.3 spec.
	 */
	color->format.rgb = true;
	color->depth.rgb_24 = true;

	/* YCRCB444 is optional according to spec. */
	if (hdmi[6] & HDMITX_EDID_HDMITX_DC_Y444) { /* the bit 3 of hdmi's 6th byte. */
		color->format.ycbcr444 = true;
		color->depth.y444_24 = true;
		color->depth.y444_30 = color->depth.rgb_30;
		color->depth.y444_36 = color->depth.rgb_36;
		color->depth.y444_48 = color->depth.rgb_48;
		dpu_pr_debug("%s: HDMITX sink does YCRCB444 in deep color.\n", connector->name);
	}

	/*
	 * Spec says that if any deep color mode is supported at all,
	 * then deep color 36 bit must be supported.
	 */
	if (!(hdmi[6] & HDMITX_EDID_HDMITX_DC_36)) /* the bit 5 of hdmi's 6th byte. */
		dpu_pr_debug("%s: HDMITX sink should do DC_36, but does not!\n", connector->name);
}
static void edid_parse_hdmi_vsdb_video(struct hdmitx_connector *connector, const u8 *db)
{
	struct hdmitx_display_info *info = &connector->display_info;
	u8 len = cea_db_payload_len(db);

	/* forum vsdb & vsdb can also set true. add by tangqy */
	info->has_hdmi_infoframe = true;

	if (len >= 6) /* if length is more than 6, has dvi_dual descrition. */
		info->dvi_dual = db[6] & 1; /* the bit 0 of hdmi's 6th byte. */

	if (len >= 7) { /* if length is more than 7, has max_tmds_clock decription. */
		info->max_tmds_clock = (u32)(db[7] * 5000); /* max_tmds_clock is db[7] * 5000. */
		if (!info->max_tmds_clock) {
			info->max_tmds_clock = EDID_DEFAULT_DVI_MAX_TMDS;
			dpu_pr_warn("zero max_tmds,fix to %u KHz\n", EDID_DEFAULT_DVI_MAX_TMDS);
		}
	}

	dpu_pr_debug("HDMITX: DVI dual %d, max TMDS clock %d kHz\n", info->dvi_dual, info->max_tmds_clock);

	edid_parse_hdmi_deep_color_info(connector, db);
}

static s32 edid_parse_cea_audio_for_sample_rate(struct audio_property *audio, struct edid_ext_info info)
{
	bool flag = false;

	/* if fmt_code is 0 or fmt_code is 0xf and ext_code is <= 3 or == 9 or > 0xd, is invalid. */
	flag = (info.fmt_code == 0) || (info.fmt_code == 0xf && (info.ext_code <= 3 || info.ext_code == 9 ||
		info.ext_code > 0xd));
	if (flag)
		return -1;
	/* if fmt_code is >= 1 and <=14, or fmt_code is 0xf and ext_code is <=10. */
	flag = (info.fmt_code >= 1 && info.fmt_code <= 14) || (info.fmt_code == 0xf && info.ext_code <= 10);
	if (flag) {
		audio->sad[info.sad_count].max_channel = (info.byte0 & EDID_AUDIO_CHANNEL_MASK) + 1;
	} else if (info.fmt_code == 0xf && info.ext_code == 0xd) { /* if fmt_code is 0xf and ext_code is 0xd. */
		/* max_channel is byte0 & 0x07 >> 0 | byte0 & 0x80) >> 4 | byte1 & 0x80) >> 3 */
		audio->sad[info.sad_count].max_channel = (((info.byte0 & 0x07) >> 0) |
												  ((info.byte0 & 0x80) >> 4) | /* need right shift 4 bits */
												  ((info.byte1 & 0x80) >> 3)) + 1; /* need right shift 3 bits */
	} else {
		audio->sad[info.sad_count].max_channel = 0;
	}

	audio->sad[info.sad_count].samp_32k = !!(info.byte1 & (0x1 << 0)); /* the bit 0 of byte1. */
	audio->sad[info.sad_count].samp_44p1k = !!(info.byte1 & (0x1 << 1)); /* the bit 1 of byte1. */
	audio->sad[info.sad_count].samp_48k = !!(info.byte1 & (0x1 << 2)); /* the bit 2 of byte1. */
	audio->sad[info.sad_count].samp_88p2k = !!(info.byte1 & (0x1 << 3)); /* the bit 3 of byte1. */
	audio->sad[info.sad_count].samp_96k = !!(info.byte1 & (0x1 << 4)); /* the bit 4 of byte1. */
	audio->sad[info.sad_count].samp_176p4k = !!(info.byte1 & (0x1 << 5)); /* the bit 5 of byte1. */
	audio->sad[info.sad_count].samp_192k = !!(info.byte1 & (0x1 << 6)); /* the bit 6 of byte1. */

	/* if fmt_code is 0xf, and ext_code is >= 4 and <=6, or ext_code is >=8 and ext_code is <=10. */
	flag = ((info.ext_code >= 4 && info.ext_code <= 6) || (info.ext_code >= 8 && info.ext_code <= 10)) &&
		(info.fmt_code == 0xf);
	if (flag) {
		audio->sad[info.sad_count].samp_176p4k = false;
		audio->sad[info.sad_count].samp_192k = false;
	} else if ((info.fmt_code == 0xf) && (info.ext_code == 12)) { /* if fmt_code is 0xf, and ext_code is 12. */
		audio->sad[info.sad_count].samp_32k = false;
		audio->sad[info.sad_count].samp_88p2k = false;
		audio->sad[info.sad_count].samp_176p4k = false;
	}

	return 0;
}

static void edid_parse_cea_audio_for_other_info(struct audio_property *audio, struct edid_ext_info info)
{
	bool flag = false;

	/* if fmt_code is 1, or ext_code is 13. */
	flag = info.fmt_code == 1 || info.ext_code == 13;
	if (flag) {
		audio->sad[info.sad_count].width_16 = !!(info.byte2 & (0x1 << 0)); /* the bit 0 of byte2. */
		audio->sad[info.sad_count].width_20 = !!(info.byte2 & (0x1 << 1)); /* the bit 1 of byte2. */
		audio->sad[info.sad_count].width_24 = !!(info.byte2 & (0x1 << 2)); /* the bit 2 of byte2. */
	} else {
		audio->sad[info.sad_count].width_16 = false;
		audio->sad[info.sad_count].width_20 = false;
		audio->sad[info.sad_count].width_24 = false;
	}
	/* if fmt_code is >= 2 and <= 8. */
	flag = info.fmt_code >= 2 && info.fmt_code <= 8;
	if (flag) {
		audio->sad[info.sad_count].max_bit_rate = (u32)(info.byte2 * 8000); /* max_bit_rate is byte2 * 8000. */
	} else {
		audio->sad[info.sad_count].max_bit_rate = 0;
	}

	flag = info.fmt_code >= 9 && info.fmt_code <= 13; /* fmt_code is in range[9-13]. */
	if (flag) {
		audio->sad[info.sad_count].dependent = info.byte2;
	} else if (info.ext_code == 11 || info.ext_code == 12) { /* if ext_code is 11 or 12. */
		audio->sad[info.sad_count].dependent = info.byte2 & 0x7; /* dependent is byte2 & 0x7. */
	} else {
		audio->sad[info.sad_count].dependent = 0;
	}

	if (info.fmt_code == 14) { /* if fmt_code is 14. */
		audio->sad[info.sad_count].profile = info.byte2 & 0x7; /* profile is byte2 & 0x7. */
	} else {
		audio->sad[info.sad_count].profile = 0;
	}

	flag = info.ext_code >= 8 && info.ext_code <= 10; /* if ext_code is >=8 and <= 10. */
	if (flag) {
		audio->sad[info.sad_count].len_1024_tl = !!(info.byte2 & (0x1 << 2)); /* the bit 2 of byte2. */
		audio->sad[info.sad_count].len_960_tl = !!(info.byte2 & (0x1 << 1)); /* the bit 1 of byte2. */
		audio->sad[info.sad_count].mps_l = !!(info.byte2 & (0x1 << 0)); /* the bit 0 of byte2. */
	} else if (info.ext_code >= 4 && info.ext_code <= 6) { /* if ext_code is >=4 and <= 6. */
		audio->sad[info.sad_count].len_1024_tl = !!(info.byte2 & (0x1 << 2)); /* the bit 2 of byte2. */
		audio->sad[info.sad_count].len_960_tl = !!(info.byte2 & (0x1 << 1)); /* the bit 1 of byte2. */
		audio->sad[info.sad_count].mps_l = false;
	} else {
		audio->sad[info.sad_count].len_1024_tl = false;
		audio->sad[info.sad_count].len_960_tl = false;
		audio->sad[info.sad_count].mps_l = false;
	}
}

static void edid_parse_cea_audio(struct hdmitx_connector *connector, const struct edid *edid)
{
	u8 *edid_ext = NULL;
	u32 i, j, db_len;
	u32 start_offset, end_offset;
	struct edid_ext_info info;
	struct audio_property *audio = &connector->audio;

	if (memset_s(&info, sizeof(info), 0x00, sizeof(info)) != EOK) {
		dpu_pr_err("edid_ext_info memset_s fail.\n");
		return;
	}

	edid_ext = edid_find_cea_extension(edid);
	if (edid_ext == NULL)
		return;

	audio->basic = ((edid_ext[3] & EDID_BASIC_AUDIO) != 0); /* the bit 6 of edid_ext's 3th byte. */
	if (!audio->basic)
		dpu_pr_debug("monitor no support basic audio!\n");

	if (cea_db_offsets(edid_ext, &start_offset, &end_offset))
		return;

	/* see CTA-861-G 7.5.2 */
	for_each_cea_db(edid_ext, i, start_offset, end_offset) {
		if (cea_db_tag(&edid_ext[i]) != EDID_AUDIO_TAG)
			continue;

		audio->basic = true;
		db_len = cea_db_payload_len(&edid_ext[i]) + 1;
		/* short audio descriptor is 3-bytes */
		for (j = 1; (j < db_len) && (info.sad_count < MAX_SAD_AUDIO_CNT); j += 3) {
			info.byte0 = edid_ext[i + j + 0]; /* byte0 */
			info.byte1 = edid_ext[i + j + 1]; /* byte1 */
			info.byte2 = edid_ext[i + j + 2]; /* byte2 */
			/* need right shift 3 bits. */
			info.fmt_code = (info.byte0 & EDID_AUDIO_FORMAT_MASK) >> 3;
			/* need right shift 3 bits. */
			info.ext_code = info.fmt_code == 0xf ? (info.byte2 & EDID_AUDIO_EXT_TYPE_CODE) >> 3 : 0;
			audio->sad[info.sad_count].fmt_code = info.fmt_code;
			audio->sad[info.sad_count].ext_code = info.ext_code;

			if (edid_parse_cea_audio_for_sample_rate(audio, info) != 0)
				continue;

			edid_parse_cea_audio_for_other_info(audio, info);

			info.sad_count++;
		}
	}
	audio->sad_count = info.sad_count;
}

static void edid_parse_cea_ext_data_block(struct hdmitx_connector *connector,
	const u8 *db)
{
	if (cea_db_is_hdmi_vsdb(db)) {
		edid_parse_hdmi_vsdb_video(connector, db);
		edid_parse_hdmi_vsdb_audio(connector, db);
	}

	if (cea_db_is_hdmi_forum_vsdb(db))
		edid_parse_hdmi_forum_vsdb(connector, db);

	if (cea_db_is_y420cmdb(db))
		edid_parse_y420cmdb_bitmap(connector, db);

	if (cea_db_is_vcdb(db))
		edid_parse_vcdb(connector, db);

	if (cea_db_is_colorimetry_db(db))
		edid_parse_colorimetry(connector, db);

	if (cea_db_is_hdr_static_db(db))
		edid_parse_hdr_static(connector, db);

	if (cea_db_is_hdr_dynamic_db(db))
		edid_parse_hdr_dynamic(connector, db);

	if (cea_db_is_dolbyvision(db))
		edid_parse_dolbyvision(connector, db);

	if (cea_db_is_cuva_hdr_db(db))
		edid_parse_cuva_hdr(connector, db);
}

static void edid_parse_cea_ext(struct hdmitx_connector *connector,
	const struct edid *edid)
{
	struct hdmitx_display_info *info = &connector->display_info;
	struct color_property *color = &connector->color;
	const u8 *edid_ext = edid_find_cea_extension(edid);
	u32 i, start, end;
	const u8 *db = NULL;

	if (edid_ext == NULL)
		return;

	info->cea_rev = edid_ext[1];

	/* The existence of a CEA block should imply RGB support */
	color->format.rgb = true;
	color->depth.rgb_24 = true;

	if (edid_ext[3] & EDID_CEA_YCRCB444) { /* the bit 5 of edid_ext's 3th byte. */
		color->format.ycbcr444 = true;
		color->depth.y444_24 = true;
	}
	if (edid_ext[3] & EDID_CEA_YCRCB422) /* the bit 4 of edid_ext's 3th byte. */
		color->format.ycbcr422 = true;

	if (cea_db_offsets(edid_ext, &start, &end))
		return;

	for_each_cea_db(edid_ext, i, start, end) {
		db = &edid_ext[i];
		edid_parse_cea_ext_data_block(connector, db);
	}
}

static u32 edid_add_color_info(struct hdmitx_connector *connector, const struct edid *edid)
{
	struct color_property *color = &connector->color;

	/*
	 * Digital sink with "DFP 1.x compliant TMDS" according to EDID 1.3?
	 * For such displays, the DFP spec 1.0, section 3.10 "EDID support"
	 * tells us to assume 8 bpc color depth if the EDID doesn't have
	 * extensions which tell otherwise.
	 */
	if ((color->depth.bpc == 0) && (edid->revision < 4) && /* revision is less than 4. */
		(edid->input & HDMITX_EDID_DIGITAL_TYPE_DVI)) {
		color->depth.bpc = 8; /* bpc is 8. */
		dpu_pr_debug("%s: Assigning DFP sink color depth as %d bpc.\n", connector->name, color->depth.bpc);
	}

	/* Only defined for 1.4 with digital displays */
	if (edid->revision < 4)
		return 0;

	switch (edid->input & HDMITX_EDID_DIGITAL_DEPTH_MASK) {
	case HDMITX_EDID_DIGITAL_DEPTH_6:
		color->depth.bpc = 6; /* bpc is 6. */
		break;
	case HDMITX_EDID_DIGITAL_DEPTH_8:
		color->depth.bpc = 8; /* bpc is 8. */
		break;
	case HDMITX_EDID_DIGITAL_DEPTH_10:
		color->depth.bpc = 10; /* bpc is 10. */
		break;
	case HDMITX_EDID_DIGITAL_DEPTH_12:
		color->depth.bpc = 12; /* bpc is 12. */
		break;
	case HDMITX_EDID_DIGITAL_DEPTH_14:
		color->depth.bpc = 14; /* bpc is 14. */
		break;
	case HDMITX_EDID_DIGITAL_DEPTH_16:
		color->depth.bpc = 16; /* bpc is 16. */
		break;
	case HDMITX_EDID_DIGITAL_DEPTH_UNDEF:
	default:
		color->depth.bpc = 0;
		break;
	}

	dpu_pr_debug("%s: Assigning EDID-1.4 digital sink color depth as %d bpc.\n", connector->name, color->depth.bpc);

	color->format.rgb = true;
	if (edid->features & HDMITX_EDID_FEATURE_RGB_YCRCB444)
		color->format.ycbcr444 = true;

	if (edid->features & HDMITX_EDID_FEATURE_RGB_YCRCB422)
		color->format.ycbcr422 = true;

	return 0;
}

/* to fix: if need
   add_compat_info_dsc
   edid_add_compat_info
   edid_add_compat_modes
 */

static void edid_add_display_info(struct hdmitx_connector *connector, const struct edid *edid)
{
	u16 i, name;
	struct hdmitx_display_info *info = &connector->display_info;
	struct base_property *base = &connector->base;

	info->width_cm = edid->width_cm;
	info->height_cm = edid->height_cm;

	base->revision = edid->revision;
	base->version = edid->version;
	base->ext_block_num = edid->extensions;

	// to fix : base->vendor info if need

	if (edid->revision < 3)  /* check if revision is less than 3. */
		return;

	// drv_hdmitx_connector_update_compat_info
	edid_parse_cea_ext(connector, edid);
	edid_add_color_info(connector, edid);
	// edid_add_compat_info
}

bool drv_hdmitx_edid_data_is_zero(const u8 *in_edid, u32 length)
{
	if (in_edid == NULL) {
		dpu_pr_err("ptr is null\n");
		return false;
	}
	return edid_data_is_zero(in_edid, length);
}

bool drv_hdmitx_edid_block_is_valid(u8 *raw_edid, u32 block, bool print_bad_edid)
{
	if (raw_edid == NULL) {
		dpu_pr_err("ptr is null\n");
		return false;
	}
	return edid_block_is_valid(raw_edid, block, print_bad_edid);
}

u32 drv_hdmitx_edid_add_modes(struct hdmitx_connector *connector, const struct edid *edid)
{
	u32 num_modes = 0;

	if (edid == NULL || connector == NULL) {
		dpu_pr_err("null pointer.\n");
		return num_modes;
	}
	if (!edid_data_is_valid(edid)) {
		dpu_pr_debug("edid isn't valid!\n");
		return num_modes;
	}

	edid_parse_cea_audio(connector, edid);

	connector->display_info.max_tmds_clock = EDID_DEFAULT_DVI_MAX_TMDS;
	/*
	 * CEA-861-F adds ycbcr capability map block, for HDMITX 2.0 sinks.
	 * To avoid multiple parsing of same block, lets parse that map
	 * from sink info, before parsing CEA modes.
	 */
	edid_add_display_info(connector, edid);

	/*
	 * The EDID specification pattern should be prioritized in the following order:
	 * - Preferred detail mode
	 * - Other detailed modes in the basic block
	 * - Detailed mode of the extension block
	 * - CVT 3-byte code mode
	 * - Standard time code
	 * - Time code for setup
	 * - mode inferred from GTF or CVT range information
	 *
	 * We're absolutely right.
	 *
	 * What is the sequence of other modes in the extension block?
	 */
	num_modes += edid_add_detailed_modes(connector, edid);

	num_modes += edid_add_standard_modes(connector, edid);

	num_modes += edid_add_established_modes(connector, edid);

	num_modes += edid_add_cea_modes(connector, edid);

	// num_modes += edid_add_compat_modes

	return num_modes;
}

