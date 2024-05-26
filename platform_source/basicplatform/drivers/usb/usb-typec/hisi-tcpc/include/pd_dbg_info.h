#ifndef _PD_DBG_INFO_H_
#define _PD_DBG_INFO_H_

#ifndef LOG_TAG
#define LOG_TAG ""
#endif

#ifdef CONFIG_HISI_TCPC_DEBUG
#define D(format, arg...) pr_info(LOG_TAG "[%s]" format, __func__, ##arg)
#else
#define D(format, arg...) do {} while (0)
#endif
#define I(format, arg...) pr_info(LOG_TAG "[%s]" format, __func__, ##arg)
#define E(format, arg...) pr_err("[ERR]" LOG_TAG "[%s]" format, __func__, ##arg)

#endif /* _PD_DBG_INFO_H_ */
