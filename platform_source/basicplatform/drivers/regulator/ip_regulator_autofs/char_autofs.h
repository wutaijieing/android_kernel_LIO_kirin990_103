#ifndef __CHAR_AUTOFS_H__
#define __CHAR_AUTOFS_H__

enum ip_regulator_id {
	MEDIA1_SUBSYS_ID = 0,
	MEDIA2_SUBSYS_ID,
	VIVOBUS_ID,
	VCODECSUBSYS_ID,
	DSSSUBSYS_ID,
	DISPCH1_ID,
	IPP_ID,
	VDEC_ID,
	ISPSUBSYS_ID,
	ISP_R8_ID,
	VENCACLK_ID,
	VENC_ID,
	ASP_ID,
	G3D_ID,
};

struct ip_regulator_autofs g_ip_autofs_list[] = {
	{ VIVOBUS_ID, "vivobus" },
	{ VCODECSUBSYS_ID, "vcodecbus" },
	{ VDEC_ID, "vdecbus" },
	{ VENCACLK_ID, "vencaclk" },
	{ VENC_ID, "vencbus" },
	{ DSSSUBSYS_ID, "dssbus" }
};

#endif