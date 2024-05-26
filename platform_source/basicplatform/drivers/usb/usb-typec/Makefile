obj-$(CONFIG_HISI_TCPC)				+= hisi-tcpc/
obj-$(CONFIG_VENDOR_USB_TYPEC)			+= hisi-usb-typec.o

ifeq ($(CONFIG_TCPC_CLASS),y)
obj-$(CONFIG_VENDOR_USB_TYPEC)			+= oem-typec-adapter.o
else
obj-$(CONFIG_VENDOR_USB_TYPEC)		        += vendor-typec-adapter.o
endif
