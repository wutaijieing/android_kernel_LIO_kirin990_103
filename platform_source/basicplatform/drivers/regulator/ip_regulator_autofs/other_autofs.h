#ifndef __OTHER_AUTOFS_H__
#define __OTHER_AUTOFS_H__

enum ip_regulator_id {
	VIVOBUS_ID = 0,
	DSSSUBSYS_ID,
	VCODECSUBSYS_ID,
	VDEC_ID,
	VENC_ID,
	ISPSUBSYS_ID,
	IVP_ID,
	DEBUGSUBSYS_ID,
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