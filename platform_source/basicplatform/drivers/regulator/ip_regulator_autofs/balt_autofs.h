#ifndef __BALT_AUTOFS_H__
#define __BALT_AUTOFS_H__

enum ip_regulator_id {
	MEDIA1_SUBSYS_ID = 0,
	MEDIA2_SUBSYS_ID,
	VIVOBUS_ID,
	VCODECSUBSYS_ID,
	DSSSUBSYS_ID,
	ISPSUBSYS_ID,
	IVP_ID,
	VDEC_ID,
	VENC_ID,
	ISP_R8_ID,
	ASP_ID,
	G3D_ID,
	NPU_ID,
	IVP1_ID,
	HIFACE_ID,
	ARPP_ID,
	IPP_ID,
};

struct ip_regulator_autofs g_ip_autofs_list[] = {
	{ VIVOBUS_ID, "vivobus" },
	{ VCODECSUBSYS_ID, "vcodecbus" },
	{ VDEC_ID, "vdecbus" },
	{ VENC_ID, "vencbus" },
	{ IVP_ID, "ivpbus" },
	{ IVP1_ID, "ivp1bus" }
};

#endif