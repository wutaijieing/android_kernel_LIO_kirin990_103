# MD5: ecfc45c6fac955abea22a1f1bd0e4d3f
CFG_CONFIG_MODEM_FULL_DUMP := YES
CFG_CONFIG_MODEM_MINI_DUMP := YES
CFG_DIAG_VERSION_ENG := YES
CFG_CONFIG_VERSION_ENG := YES
CFG_CONFIG_EDMA_DEBUG := YES
CFG_CONFIG_RSRACC_DEBUG := YES
CFG_CPUFREQ_DEBUG_INTERFACE:=YES
CFG_CONFIG_PM_OM_DEBUG:=YES
CFG_CONFIG_OS_INCLUDE_LP := YES
CFG_CONFIG_CSHELL := YES
CFG_CONFIG_UART_SHELL := YES
CFG_CONFIG_OS_INCLUDE_SHELL := YES
CFG_CONFIG_MEM_DEBUG := YES
CFG_CONFIG_ICC_DEBUG := YES
CFG_CONFIG_LLT_MDRV := NO
CFG_FEATURE_CHR_OM := FEATURE_ON
CFG_CONFIG_DIAG_NETLINK := YES
ifeq ($(CFG_MODEM_SANITIZER),FEATURE_ON)
CFG_CONFIG_MODEM_ASLR_DEBUG := NO
else
CFG_CONFIG_MODEM_ASLR_DEBUG := YES
endif
CFG_CONFIG_BALONG_CORESIGHT := YES
CFG_CONFIG_WATCHPOINT := YES
CFG_CONFIG_LRCCPU_PM_DEBUG := YES
