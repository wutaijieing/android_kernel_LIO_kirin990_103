#ifndef __CHARPRO_AUTOFS_H__
#define __CHARPRO_AUTOFS_H__

enum ip_regulator_id {
	MEDIA1_SUBSYS_ID = 0,
	MEDIA2_SUBSYS_ID,
	VIVOBUS_ID,
	VCODECSUBSYS_ID,
	DSI_ID,
	DP_ID,
	CSI_ID,
	IDI2AXI_ID,
	IDI2AXI1_ID,
	DSSSUBSYS_ID,
	DSSLITE1_ID,
	VDEC_ID,
	M1CDC_ID,
	ISPSUBSYS_ID,
	ISP_R8_ID,
	VENC_ID,
	ASP_ID,
	G3D_ID,
};

struct ip_regulator_autofs g_ip_autofs_list[] = {
	{ VIVOBUS_ID, "vivobus" },
	{ VCODECSUBSYS_ID, "vcodecbus" },
	{ VDEC_ID, "vdecbus" },
	{ VENC_ID, "vencbus" },
	{ DSSSUBSYS_ID, "dssbus" }
};

#endif