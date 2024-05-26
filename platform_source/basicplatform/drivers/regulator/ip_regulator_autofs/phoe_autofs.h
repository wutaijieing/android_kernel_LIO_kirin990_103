#ifndef __PHOE_AUTOFS_H__
#define __PHOE_AUTOFS_H__

enum ip_regulator_id {
	VIVOBUS_ID = 0,
	VCODECSUBSYS_ID,
	DSSSUBSYS_ID,
	ISPSUBSYS_ID,
	IVP_ID,
	VDEC_ID,
	VENC_ID,
	ISP_R8_ID,
	VENC2_ID,
	HIFACE_ID,
	MEDIA1_SUBSYS_ID,
	MEDIA2_SUBSYS_ID,
	NPU_ID,
	G3D_ID,
	ASP_ID,
};

struct ip_regulator_autofs g_ip_autofs_list[] = {
	{ VIVOBUS_ID, "vivobus" },
	{ VCODECSUBSYS_ID, "vcodecbus" },
	{ VDEC_ID, "vdecbus" },
	{ VENC_ID, "vencbus" },
	{ ISPSUBSYS_ID, "ispa7bus" },
	{ IVP_ID, "ivpbus" },
	{ VENC2_ID, "venc2bus" }
};

#endif