obj-$(CONFIG_TZDRIVER) += tzdriver/

ifneq (,$(findstring $(CONFIG_AB_APCP_NEW_INTERFACE),y))
obj-y += mdmbuf/
else
obj-y += socp/
endif

ifneq ($(link_kernel),false)
ifneq (,$(findstring $(CONFIG_HISI_BALONG_MODEM),y m))
ifneq (,$(findstring $(CONFIG_AB_APCP_NEW_INTERFACE),y))
obj-y += modem/
else
obj-y += ../hisi/modem/
endif
else
ifneq (,$(findstring $(CONFIG_MODEM_DRIVER),y m))
ifneq (,$(findstring $(CONFIG_AB_APCP_NEW_INTERFACE),y))
obj-y += modem/
else
obj-y += ../hisi/modem/
endif
endif
endif
endif
obj-$(CONFIG_DRG) += drg/
obj-y += audio/
