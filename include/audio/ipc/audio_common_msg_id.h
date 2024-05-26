/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: hifi&ap&kernel msg id
 * Create: 2021-12-3
 */

#ifndef AUDIO_IPC_AUDIO_COMMON_MSG_ID_H
#define AUDIO_IPC_AUDIO_COMMON_MSG_ID_H

enum AUDIO_COMMON_MSG_ENUM {
    /* AP音频驱动与HIFI音频通道模块交互消息ID */
    ID_AP_AUDIO_PCM_OPEN_REQ            = 0xDD25,
    ID_AP_AUDIO_PCM_CLOSE_REQ           = 0xDD26,
    ID_AP_AUDIO_PCM_HW_PARA_REQ         = 0xDD27,
    ID_AP_AUDIO_PCM_HW_FREE_REQ         = 0xDD28,
    ID_AP_AUDIO_PCM_PREPARE_REQ         = 0xDD29,
    ID_AP_AUDIO_PCM_TRIGGER_REQ         = 0xDD2A,
    ID_AP_AUDIO_PCM_POINTER_REQ         = 0xDD2B,
    ID_AP_AUDIO_PCM_SET_BUF_CMD         = 0xDD2C,
    ID_AUDIO_AP_PCM_PERIOD_ELAPSED_CMD  = 0xDD2D,
    ID_AUDIO_AP_PCM_XRUN                = 0xDD2E,

    /* HIFI音频通道模块内部交互消息ID */
    ID_AUDIO_UPDATE_PLAY_BUFF_CMD       = 0xDD2E,
    ID_AUDIO_UPDATE_CAPTURE_BUFF_CMD    = 0xDD2F,

    /* HIFI PCM模块与audio player模块内部交互消息ID */
    ID_AUDIO_UPDATE_PLAYER_BUFF_CMD     = 0xDD31,

    /* The DTS of the AP and Hi-Fi exchange messages */
    ID_AP_AUDIO_SET_DTS_ENABLE_CMD      = 0xDD36, /* The AP instructs the Hi-Fi to enable DTS */
    /* ID_AUDIO_PLAYER_SET_DTS_ENABLE_IND  0xDD37               内部消息占用 */
    ID_AP_AUDIO_SET_DTS_DEV_CMD         = 0xDD38,
    ID_AP_AUDIO_SET_DTS_GEQ_CMD         = 0xDD39,
    ID_AP_AUDIO_SET_AUDIO_EFFECT_PARAM_CMD = 0xDD3A,
    ID_AP_AUDIO_SET_DTS_GEQ_ENABLE_CMD  = 0xDD3B,
    /* AP and HIFI control the external headset hifi codec interactive messages */
    ID_AP_AUDIO_SET_EXCODEC_BYPASS_CMD  = 0xDD3D,

    /* AP和HIFI的控制外置耳机hifi codec交互消息 */
    ID_AP_AUDIO_SET_ENABLE_AEC_CMD      = 0xDD3E,

    /* AP音频驱动与录音模块交互消息 */
    ID_AP_DSP_VOICE_RECORD_START_CMD    = 0xDD40,
    ID_AP_DSP_VOICE_RECORD_STOP_CMD     = 0xDD41,

    /* voicePP MSG_ID */
    ID_AP_VOICEPP_START_REQ             = 0xDD42, /* start VOICEPP */
    ID_VOICEPP_MSG_START                = ID_AP_VOICEPP_START_REQ,
    ID_VOICEPP_AP_START_CNF             = 0xDD43,
    ID_AP_VOICEPP_STOP_REQ              = 0xDD44, /* stop VOICEPP */
    ID_VOICEPP_AP_STOP_CNF              = 0xDD45,
    ID_VOICEPP_MSG_END                  = 0xDD4A,

    /* ID of the message exchanged between the AP audio driver and the HIFI audio player module */
    ID_AP_AUDIO_PLAY_START_REQ          = 0xDD51, /* The AP to start Hifi audio player request command */
    ID_AUDIO_AP_PLAY_START_CNF          = 0xDD52, /* dsp start up audio player call back AP confirm */
    ID_AP_AUDIO_PLAY_PAUSE_REQ          = 0xDD53, /* The AP stop Hifi audio player request command */
    ID_AUDIO_AP_PLAY_PAUSE_CNF          = 0xDD54, /* dsp stop audio player call back AP confirm */
    /* dsp notifies AP audio player data playback or interrupt indication */
    ID_AUDIO_AP_PLAY_DONE_IND           = 0xDD56,
    ID_AP_AUDIO_PLAY_UPDATE_BUF_CMD     = 0xDD57, /* The AP notifies the Hi-Fi of a new data block update command */
    ID_AP_AUDIO_PLAY_QUERY_TIME_REQ     = 0xDD59, /* AP search dsp audio player play progress request */
    ID_AP_AUDIO_PLAY_WAKEUPTHREAD_REQ   = 0xDD5A,
    /* Hifi reply to the AP audio player playback progress confirmation command */
    ID_AUDIO_AP_PLAY_QUERY_TIME_CNF     = 0xDD60,

    /* AP音频驱动与audio loop环回交互消息ID */
    ID_AP_AUDIO_LOOP_BEGIN_REQ          = 0xDD65, /* AP启动Hifi begin audio loop request命令 */
    ID_AUDIO_AP_LOOP_BEGIN_CNF          = 0xDD66, /* Hifi回复AP begin audio loop结果confirm命令 */
    ID_AP_AUDIO_LOOP_END_REQ            = 0xDD67, /* AP启动Hifi end audio loop request命令 */
    ID_AUDIO_AP_LOOP_END_CNF            = 0xDD68, /* Hifi回复AP end audio loop结果confirm命令 */
    ID_AP_AUDIO_REAL_TIME_ACCESS_REQ    = 0xDD69, /* AP set the Auxiliary Listening Function */
    ID_AP_AUDIO_PLAY_SET_VOL_CMD        = 0xDD70, /* AP sets the lowlatency play volume command */

    ID_AUDIO_PLAYER_START_REQ           = 0xDD71, /* audio_player_open_req */
    ID_AUDIO_PLAYER_STOP_REQ            = 0xDD73, /* audio_player_close_req */

    ID_AP_AUDIO_RECORD_PCM_HOOK_CMD     = 0xDD7A, /* AP notifies dsp catch PCM data */
    ID_ALP_PROXY_START_REQ              = 0xDD74,
    ID_ALP_PROXY_STOP_REQ               = 0xDD75,
    ID_ALP_PROXY_GRANT_REQ              = 0xDD76,
    ID_ALP_PROXY_PREEMPT_REQ            = 0xDD77,
    ID_AUDIO_AP_UPDATE_PCM_BUFF_CMD     = 0xDD7C,

    ID_AP_AUDIO_DYN_EFFECT_GET_PARAM    = 0xDD7D,
    ID_AP_AUDIO_DYN_EFFECT_GET_PARAM_CNF = 0xDD7E,
    ID_AP_AUDIO_DYN_EFFECT_TRIGGER      = 0xDD7F,

    ID_ULP_AP_AUDIO_RT_MSG              = 0xDD80,

    /* enhance msgid between ap and hifi */
    ID_AP_DSP_ENHANCE_START_REQ         = 0xDD81,
    ID_DSP_AP_ENHANCE_START_CNF         = 0xDD82,
    ID_AP_DSP_ENHANCE_STOP_REQ          = 0xDD83,
    ID_DSP_AP_ENHANCE_STOP_CNF          = 0xDD84,
    ID_AP_DSP_ENHANCE_SET_DEVICE_REQ    = 0xDD85,
    ID_DSP_AP_ENHANCE_SET_DEVICE_CNF    = 0xDD86,

    /* AP messages exchanged between the audio driver and the command module */
    ID_AP_AUDIO_ENHANCE_SET_DEVICE_IND  = 0xDD91,
    ID_AP_AUDIO_CMD_MLIB_SET_PARA_CMD   = 0xDD92,
    ID_AP_AUDIO_CMD_SET_SOURCE_CMD      = 0xDD95,
    ID_AP_AUDIO_CMD_SET_DEVICE_CMD      = 0xDD96,
    ID_AP_AUDIO_CMD_SET_MODE_CMD        = 0xDD97, /* AUDIO_CMD_COMMON_SET_STRU */
    ID_AP_AUDIO_CMD_SET_MMI_MODE_CMD    = 0xDD98, /* AUDIO_CMD_COMMON_SET_STRU */
    ID_AP_AUDIO_CMD_SET_ANGLE_CMD       = 0xDD99,
    ID_AP_AUDIO_CMD_SET_AUDIO_EFFECT_PARAM_CMD = 0xDD9A,
    ID_AP_AUDIO_CMD_SET_AUDIO_EFFECT_PARAM_CNF = 0xDD9B,
    ID_AP_AUDIO_CMD_SET_MONO_CHANNEL_MODE_CMD  = 0xDD9C,
    ID_AP_AUDIO_CMD_SET_TYPEC_DEVICE_MASK_CMD = 0xDD9D, /* AUDIO_CMD_COMMON_SET_STRU */

    /* HIFI音频通道模块内部交互消息ID */
    ID_AUDIO_AP_PCM_TRIGGER_CNF         = 0xDDA0,
    ID_AUDIO_PCM_TIMER_REQ              = 0xDDA1,
    ID_DSP_PCM_DELAY_START_CMD          = 0xDDA2,
    ID_DSP_PCM_RESTART_CMD              = 0xDDA3,
    ID_AUDIO_PLAYER_STOP_CNF_CHECK_REQ  = 0xDDA8,
    ID_AP_AUDIO_PCM_SET_VOIP_EFFECT_STATUS_REQ = 0xDDA9,

    ID_AUDIO_UPDATE_FM_OUT_BUFF_CMD     = 0xDDAA,
    ID_AUDIO_UPDATE_FM_IN_BUFF_CMD      = 0xDDAB,
    ID_AUDIO_SET_STREAM_CALLBACK_CMD    = 0xDDAE,

    /* for 3mic */
    /* AP notify the HIFI that the 3Mic/4Mic channel has been established */
    ID_AP_AUDIO_ROUTING_COMPLETE_REQ    = 0xDDC0,
    ID_AUDIO_AP_DP_CLK_EN_IND           = 0xDDC1, /* dsp notifies AP open/close Codec DP clk */
    ID_AP_AUDIO_DP_CLK_STATE_IND        = 0xDDC2, /* AP notifies dsp £¬Codec DP clk state */
    ID_AUDIO_AP_OM_DUMP_CMD             = 0xDDC3, /* dsp notifies AP dump log */
    ID_AUDIO_AP_FADE_OUT_REQ            = 0xDDC4, /* dsp notifies AP fadeout */
    ID_AP_AUDIO_FADE_OUT_IND            = 0xDDC5, /* AP notifies dsp fadeout completed */

    /* for voice BSD */
    ID_AP_AUDIO_BSD_CONTROL_REQ         = 0xDDC6, /* hal notify the HIFI, BSD start or stop voice detection */
    ID_AUDIO_AP_OM_CMD                  = 0xDDC9,
    /*
     * For test: The hal layer notifies the HIFI
     * triggers the corresponding HIFI to be reset independently through a specified risky operation.
     */
    ID_AP_HIFI_MOCKER_HIFI_RESET_CMD    = 0xDDCA,
    ID_AP_AUDIO_STR_CMD                 = 0xDDCB, /* AP send string to dsp,resolve in dsp */
    ID_AUDIO_AP_VOICE_BSD_PARAM_CMD     = 0xDDCC, /* VOICE BSD reported patameter */
    /* Obtaining HiFi Call Status Parameters */
    ID_AP_ENABLE_MODEM_LOOP_REQ         = 0xDDCD, /* the audio hal notify dsp to start/stop  MODEM LOOP */
    ID_AUDIO_AP_3A_CMD                  = 0xDDCE,
    ID_AP_ENABLE_AT_DSP_LOOP_REQ        = 0xDDCF, /* notify dsp to start/stop dsp LOOP from slimbus to i2s */

    ID_AUDIO_AT_DSP_LOOP_TX_REQ         = 0xDDD0,
    ID_DSP_AP_HOOK_DATA_CMD             = 0xDDD1, /* dsp send pcm data to ap for om */

    /* VOICE ENUM START */
    /* A核通知HIFI C 核复位消息ID */
    ID_AP_HIFI_CCPU_RESET_REQ           = 0xDDE1,

    /* hifi回复ID_AP_HIFI_CCPU_RESET_REQ */
    ID_HIFI_AP_CCPU_RESET_CNF           = 0xDDE2,

    ID_AP_HIFI_REMOTE_SET_BUF_CMD       = 0xDDE3,
    ID_HIFI_AP_REMOTE_PERIOD_ELAPSED_CMD = 0xDDE4,

    ID_AP_HIFI_REMOTE_RECORD_START_REQ  = 0xDDE5,
    ID_HIFI_AP_REMOTE_RECORD_START_CNF  = 0xDDE6,

    ID_HIFI_AP_REMOTE_RECORD_DATA_ERR_IND = 0xDDE7,

    ID_AP_HIFI_REMOTE_RECORD_STOP_REQ   = 0xDDE8,
    ID_HIFI_AP_REMOTE_RECORD_STOP_CNF   = 0xDDE9,

    ID_AP_HIFI_REMOTE_PLAY_START_REQ    = 0xDDEA,
    ID_HIFI_AP_REMOTE_PLAY_START_CNF    = 0xDDEB,

    ID_AP_HIFI_REMOTE_PLAY_DATA_ERR_IND = 0xDDEC,

    ID_AP_HIFI_REMOTE_PLAY_STOP_REQ     = 0xDDED,
    ID_HIFI_AP_REMOTE_PLAY_STOP_CNF     = 0xDDEE,

    ID_HIFI_AP_REMOTE_VOICE_STOP_CMD    = 0xDDEF,
    /* VOICE ENUM END */
    /* A核通知hifi清空语音环形缓冲区的消息ID */
    ID_AP_HIFI_CLEAN_RINGBUF_CMD        = 0xDDF0,

    /* Obtaining HiFi Call Status Parameters */
    ID_AP_HIFI_REQUEST_VOICE_PARA_REQ   = 0xDF00, /* ap request voice msg */
    ID_HIFI_AP_SEND_VOICE_PARA          = 0xDF01, /* dsp replay voice msg */

    ID_HIFI_REQUEST_VOICE_PARA_CMD      = 0xDF02,
    ID_AP_HIFI_REQUEST_SMARTPA_PARA_REQ = 0xDF03,
    ID_HIFI_AP_REQUEST_SMARTPA_PARA_CNF = 0xDF04,
    ID_HIFI_REQUEST_SMARTPA_PARA_CMD    = 0xDF07,

    ID_AP_HIFI_REQUEST_SET_PARA_CMD     = 0xDF08, /* ap hal set parameter interface */
    ID_AP_HIFI_REQUEST_GET_PARA_CMD     = 0xDF09, /* ap hal get parameter interface */
    ID_AP_HIFI_REQUEST_GET_PARA_CNF     = 0xDF0A, /* ap hal get parameter feedback */

    /* voice memo cmd */
    ID_AP_HIFI_SET_OUTPUT_MUTE_CMD      = 0xDF0B,
    ID_AP_HIFI_SET_INPUT_MUTE_CMD       = 0xDF0C,
    ID_AP_HIFI_SET_AUDIO2VOICETX_CMD    = 0xDF0D,
    ID_AP_HIFI_SET_STREAM_MUTE_CMD      = 0xDF0E,
    ID_AP_HIFI_SET_FM_CMD               = 0xDF0F,

    ID_DSP_AP_BIGDATA_CMD               = 0xDF10, /* bigdata */
    ID_DSP_AP_SMARTPA_DFT               = 0xDF11,
    ID_DSP_AP_AUDIO_DB                  = 0xDF12,

    ID_AP_HIFI_SET_AUDIOMIX2VOICETX_CMD  = 0xDF20,
    ID_AP_HIFI_SET_MOS_TEST_CMD         = 0xDF21,
    ID_AP_AUDIO_CMD_SET_COMBINE_RECORD_FUNC_CMD = 0xDF22, /* hal notify dsp combine record cmd */
    ID_AP_HIFI_LP_MADTEST_START_CMD     = 0xDF23, /* mmi mad test start cmd */
    ID_AP_HIFI_LP_MADTEST_STOP_CMD      = 0xDF24, /* mmi mad test stop cmd */

    /* 马达相关 */
    ID_AP_HIFI_VIBRATOR_STATUS_CHANGE_CMD = 0xDF25, /* VIBRATOR MSG */
    ID_AP_CMD_VIBRATOR_TRIGGER          = 0xDF26,
    ID_VIBRATOR_BUFF_UPDATE_CMD         = 0xDF27,
    ID_AUDIO_AP_VIBRATOR_ELAPSED_CMD    = 0xDF28,

    ID_HIFI_AP_SYSCACHE_QUOTA_CMD       = 0xDF32, /* syscache quota MSG */

    /* voice_ap_msg_id_enum */
    ID_AP_HIFI_NV_REFRESH_IND           = 0xDF33,
    ID_HIFI_AP_OM_DATA_IND              = 0xDF34,
    ID_HIFI_AP_PCIE_REOPEN_IND          = 0xDF35,
    ID_AP_HIFI_CCPU_RESET_DONE_IND      = 0xDF36,
    ID_HIFI_AP_PCIE_CLOSE_IND           = 0xDF37,
    ID_AP_HIFI_CALL_STATE_REPORT_REQ    = 0xDF54,
    ID_HIFI_AP_CALL_STATE_REPORT_CNF    = 0xDF55,
    ID_AP_HIFI_RX_PCM_IND               = 0xDF65,
    ID_AP_HIFI_PCVOICE_TX_PCM_IND       = 0xDF66,
    ID_HIFI_AP_PCVOICE_RX_PCM_IND       = 0xDF67,

    ID_AP_DSP_I2S_TEST_POWER_REQ        = 0xDF38, /* AP notifies dsp to power on for i2s test */
    ID_DSP_AP_I2S_TEST_POWER_CNF        = 0xDF39, /* dsp returns to the AP to power off the result */

    ID_AUDIO_ULTRASONIC_IN_BUFF_UPDATE_CMD = 0xDF40,
    ID_AUDIO_ULTRASONIC_OUT_BUFF_UPDATE_CMD = 0xDF41,
    ID_AOIPC_RECV_MSG                   = 0xDF42,
    ID_DSP_AP_AUXHEAR_CMD               = 0xDF44,

    ID_AP_DSP_SET_SCENE_DENOISE_CMD     = 0xDF54,
    ID_AUDIO_SCENE_DENOISE_IN_BUFF_UPDATE_CMD = 0xDF55,
    ID_AP_DSP_HEARING_PROTECTION_SWITCH_CMD = 0xDF58, /* ap hal set hearing protection switch */
    ID_AP_DSP_HEARING_PROTECTION_NOTIFY_READ_PARAM_CMD = 0xDF59, /* ap hal set wired headset para */
    ID_AP_DSP_HEARING_PROTECTION_GET_VOLUME_CMD = 0xDF60, /* ap hal get hearing protection volume data info */
    ID_AP_DSP_HEARING_PROTECTION_GET_VOLUME_CNF = 0xDF61, /* ap hal get hearing protection volume data feedback */

    ID_AP_HIFI_SET_SOUND_ENHANCE_SWITCH_CMD = 0xDF62, /* ap hal set sound enhance switch */
    ID_AP_HIFI_SET_SOUND_ENHANCE_SCENE_CMD = 0xDF63, /* ap hal set sound enhance scene */
    ID_AP_HIFI_SET_SOUND_ENHANCE_PARA_CMD = 0xDF64, /* ap hal set sound enhance para */
    ID_AP_DSP_GET_REC_START_DMA_STAMP_REQ = 0xDF68,
    ID_DSP_AP_GET_REC_START_DMA_STAMP_CNF = 0xDF69,
    ID_AP_HIFI_SET_DPRX_CMD             = 0xDF70, /* DPRX MSG */

    ID_AUDIO_DPRX_UPDATE_OUT_BUFF_REQ   = 0xDF71,
    ID_AUDIO_DPRX_UPDATE_IN_BUFF_REQ    = 0xDF72,
    ID_AP_DSP_SET_FORCE_MUTE_MIC_CMD    = 0xDF73,

    /* hifi_usb_mesg_id */
    ID_USB_MSG_START                    = 0xE800,
    /* HiFi HCD Agent与Hifi USB HCD模块交互 */
    ID_AP_HIFI_USB_RUNSTOP              = 0xE801,
    ID_AP_HIFI_USB_HCD_MESG             = 0xE802,
    ID_HIFI_AP_USB_HCD_MESG             = 0xE803,
    ID_HIFI_AP_USB_INIT                 = 0xE804,
    ID_HIFI_AP_USB_WAKEUP               = 0xE805,
    ID_AP_HIFI_USB_TEST                 = 0xE806,
    ID_AP_USE_HIFI_USB                  = 0xE807,
    ID_HIFI_AP_USB_SUSPENDED            = 0xE808,
    ID_HIFI_AP_USB_RUNNING              = 0xE809,

    /* usb处理中断的RT队列消息 */
    ID_USB_IRQ                          = 0xE820,
    ID_USB_TIMER_TIMEOUT                = 0xE821,
    /* usb处理中断的Normal队列消息 */
    ID_USB_HCD_MESG_START               = 0xE830,
    ID_USB_RUNSTOP                      = 0xE831,
    ID_USB_DEV_CONTROL                  = 0xE832,
    ID_USB_HUB_CONTROL                  = 0xE833,
    ID_USB_URB_CONTROL                  = 0xE834,
    ID_USB_EP_CONTROL                   = 0xE835,
    ID_USB_UPDATE_DEVICE                = 0xE836,
    ID_USB_DO_SUSPEND                   = 0xE837,
    ID_USB_DO_RESUME                    = 0xE838,
    ID_USB_KEY_PRESSED                  = 0xE839,
    ID_USB_DEBUG_PRINT                  = 0xE83E,
    ID_USB_HCD_MESG_END                 = 0xE83F,

    /* usb处理测试的Normal队列消息 */
    ID_USB_TEST_ISOC                    = 0xE840,
    ID_USB_TEST_DEBUG_PRINT             = 0xE841,
    ID_USB_TEST_SR_STATE_INQUIRE        = 0xE842,

    ID_USB_MSG_END                      = 0xE8FF,

    /* WAKEUP_MSG_ENUM */
    ID_AP_WAKEUP_START                  = 0xFD01,       /* acpu->dsp */
    ID_AP_WAKEUP_STOP                   = 0xFD02,       /* acpu->dsp */
    ID_AP_WAKEUP_PARAMETER_SET          = 0xFD03,       /* acpu->dsp */
    ID_AP_WAKEUP_PARAMETER_GET          = 0xFD04,       /* acpu->dsp */
    ID_HIFI_WAKEUP_GET_KEYWORD_RCV      = 0xFD05,       /* dsp->acpu */
    ID_HIFI_WAKEUP_PERIOD_ELAPSED_CMD   = 0xFD06,       /* dsp->acpu */
    ID_AP_AUDIO_CMD_SET_WAKEUP_SCENE_CMD = 0xFD07,      /* acpu->dsp */
    ID_HIFI_WAKEUP_START_RCV            = 0xFD08,       /* dsp->acpu */
    ID_HIFI_WAKEUP_STOP_RCV             = 0xFD09,       /* dsp->acpu */
    ID_HIFI_WAKEUP_PARAMETER_SET_RCV    = 0xFD0A,       /* dsp->acpu */
    ID_HIFI_WAKEUP_PARAMETER_GET_RCV    = 0xFD0B,       /* dsp->acpu */
    ID_DSP_E2E_DECODE                   = 0xFD0C,       /* dsp->dsp */
    ID_AUDIO_WAKEUP_IN_BUFF_UPDATE_CMD  = 0xFD0D,       /* dsp->dsp */
    ID_HIFI_INTERNAL_WAKEUP_DECODE      = 0xFD0E,       /* dsp->dsp */
    ID_HIFI_WAKEUP_RESTART              = 0xFD0F,       /* dsp->dsp */

    /* WAKEUP_LP_MSG_NUM */
    /* AP <--> HIFI */
    ID_AP_WAKEUP_LP_START               = 0xFD11,
    ID_AP_WAKEUP_LP_STOP                = 0xFD12,
    ID_AP_WAKEUP_LP_PARAMETER_SET       = 0xFD13,
    ID_AP_WAKEUP_LP_PARAMETER_GET       = 0xFD14,

    /* HIFI <--> AP */
    ID_HIFI_WAKEUP_LP_GET_KEYWORD_ACK   = 0xFD15,
    ID_HIFI_WAKEUP_LP_PERIOD_ELAPSED_ACK = 0xFD16,
    ID_HIFI_WAKEUP_LP_START_ACK         = 0xFD17,
    ID_HIFI_WAKEUP_LP_STOP_ACK          = 0xFD18,
    ID_HIFI_WAKEUP_LP_PARAMETER_SET_ACK = 0xFD19,
    ID_HIFI_WAKEUP_LP_PARAMETER_GET_ACK = 0xFD1A,
    ID_HIFI_WAKEUP_LP_MMI_MAD_INTR_ACK  = 0xFD1B,
    ID_HIFI_WAKEUP_LP_ELAPSED_TIMEOUT_ACK = 0xFD10,

    /* HIFI <--> HIFI */
    ID_WAKEUP_LP_MAD_ISR_CMD            = 0xFD1C,
    ID_WAKEUP_LP_PCM_ISR_CMD            = 0xFD1D,
    ID_WAKEUP_LP_DECODE                 = 0xFD1E,
    ID_WAKEUP_LP_ENTER_WLP              = 0xFD1F,

    /* virtual_voice_proxy_voice_msg_id */
    ID_AP_VIRTUAL_VOICE_PROXY_RX_NTF    = 0xFD20,
    ID_VIRTUAL_VOICE_PROXY_RX_CNF       = 0xFD21,
    ID_VIRTUAL_VOICE_PROXY_TX_NTF       = 0xFD22,
    ID_AP_VIRTUAL_VOICE_PROXY_TX_CNF    = 0xFD23,
    ID_VIRTUAL_VOICE_PROXY_STATUS_IND   = 0xFD24,
    ID_VIRTUAL_VOICE_PROXY_STATUS_NTF   = 0xFD25,
    ID_AP_VIRTUAL_VOICE_PROXY_SET_MODE  = 0xFD26,
    ID_VIRTUAL_VOICE_PROXY_REQUEST_MIC_DATA = 0xFD27,
    ID_AP_VIRTUAL_VOICE_PROXY_SET_PROXY_STATUS = 0xFD28,

    /* WAKEUP_MSG_ENUM */
    ID_HIFI_WAKEUP_SET_MUTE             = 0xFD30,       /* dsp->acpu */

    ID_HIFI_ULTRASONIC_IN_PROCESS       = 0xFD43,

    /* USBAUDIO_MSG_ENUM */
    ID_AP_USBAUDIO_SEND_PROBE           = 0xFF00,       /* acpu->dsp */
    ID_HIFI_USBAUDIO_PROBE_RCV          = 0xFF01,       /* dsp->acpu */
    ID_AP_USBAUDIO_SEND_DISCONNECT      = 0xFF02,       /* acpu->dsp */
    ID_HIFI_USBAUDIO_DISCONNECT_RCV     = 0xFF03,       /* dsp->acpu */
    ID_HIFI_USBAUDIO_PIPEOUTINTERFACE_ON_RCV = 0xFF04,  /* dsp->acpu */
    ID_HIFI_USBAUDIO_PIPEOUTINTERFACE_OFF_RCV = 0xFF05, /* dsp->acpu */
    ID_HIFI_USBAUDIO_PIPEININTERFACE_ON_RCV = 0xFF06,   /* dsp->acpu */
    ID_HIFI_USBAUDIO_PIPEININTERFACE_OFF_RCV = 0xFF07,  /* dsp->acpu */
    ID_AP_USBAUDIO_SETINTERFACE_COMPLETE = 0xFF08,      /* acpu->dsp */
    ID_AP_USBAUDIO_NV_CHECK             = 0xFF09,       /* acpu->dsp */
    ID_HIFI_USBAUDIO_NV_CHECK_RCV       = 0xFF10,       /* dsp->acpu */
    ID_HIFI_USBAUDIO_DELAY_INTERFACE_OFF = 0xFF11,      /* dsp->dsp */
};

#endif // AUDIO_IPC_AUDIO_COMMON_MSG_ID_H
