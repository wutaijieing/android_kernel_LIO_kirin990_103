#ifndef __BOST_AUTOFS_H__
#define __BOST_AUTOFS_H__

enum ip_regulator_id {
	MEDIA1_SUBSYS_ID = 0,
	VIVOBUS_ID,
	VCODECSUBSYS_ID,
	IVP_ID,
	DSSSUBSYS_ID,
	ISPSUBSYS_ID,
	VDEC_ID,
	VENC_ID,
	ISP_SRT_ID,
	G3D_ID,
	ASP_ID,
	MMBUF_ID,
	MEDIA2_SUBSYS_ID,
};

struct ip_regulator_autofs g_ip_autofs_list[] = {
	{ VIVOBUS_ID, "vivobus" },
	{ VCODECSUBSYS_ID, "vcodecbus" },
	{ VDEC_ID, "vdecbus" },
	{ VENC_ID, "vencbus" },
	{ ISPSUBSYS_ID, "ispa7bus" },
	{ IVP_ID, "ivpbus" }
};

#endif
