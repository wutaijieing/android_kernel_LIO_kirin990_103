/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:
 * Create: 2019/01/04
 */

#ifndef CRYPTO_ENHANCE_POWER_H
#define CRYPTO_ENHANCE_POWER_H

#include <linux/of_platform.h>
#include <linux/types.h>

#define DEV_HIEPS_VLOTAGE_POLL_ID  41
#define HIEPS_SUSPEND_DELAY_TIME   20

#define CHECK_RESULT_GOTO(ret, lable)       do { \
						if (HIEPS_OK != (ret)) {\
							pr_err("%s-%d:failed! err=%d\n", __func__, __LINE__, ret);\
							goto lable;\
						}\
						} while (0);

#define CHECK_RESULT_PRINT(ret)             do {\
						if (HIEPS_OK != (ret)) {\
							pr_err("%s-%d:failed! err=%d\n", __func__, __LINE__, ret);\
						}\
						} while (0);
/* hieps profile id. */
typedef enum {
	PROFILE_080V  = 0, /* 0.80V */
	PROFILE_070V,      /* 0.70V */
#ifndef CONFIG_CRYPTO_ENHANCE_WITHOUT_CPU
	PROFILE_065V,      /* 0.65V */
#endif
	PROFILE_060V,      /* 0.60V */
	MAX_PROFILE,
} hieps_profile_id;

typedef enum {
	HIEPS_POWER_SUCCESS = 0x5A,
	HIEPS_POWER_FAILED  = 0xA5,
} hieps_power_result;

typedef enum {
	HIEPS_POWER_STATUS_ON  = 0x37,
	HIEPS_POWER_STATUS_OFF = 0xC8,
} hieps_power_status;

/* hieps voltage command. */
enum {
	HIEPS_VOLTAGE_ON      = 0xA001,
	HIEPS_VOLTAGE_OFF     = 0xA002,
	HIEPS_VOLTAGE_UPDATE  = 0xA003,
};

/*
 * @brief      : hieps_change_voltage : change hieps voltage.
 *
 * @param[in]  : profile : the profile to change.
 *
 * @return     : HIEPS_OK:successfuly, others: failed.
 */
int32_t hieps_change_voltage(const uint32_t profile);

/*
 * @brief      : hieps_power_cmd: power hieps and reture result to ATF.
 *
 * @param[in]  : profile : the profile to power.
 * @param[in]  : cmd : the command: power on or off hieps.
 *
 * @return     : HIEPS_OK:successfuly, others: failed.
 */
int32_t hieps_power_cmd(const uint32_t profile, uint32_t cmd);

/*
 * @brief      : hieps_suspend : hieps suspend process.
 *
 * @param[in]  : pdev : hieps devices.
 * @param[in]  : state : hieps state.
 *
 * @return     : HIEPS_OK:successfuly, others: failed.
 */
int32_t hieps_suspend(struct platform_device *pdev, struct pm_message state);

/*
 * @brief      : hieps_resume : hieps resume process.
 *
 * @param[in]  : pdev : hieps devices.
 *
 * @return     : HIEPS_OK:successfuly, others: failed.
 */
int32_t hieps_resume(struct platform_device *pdev);

#endif /* CRYPTO_ENHANCE_POWER_H */
