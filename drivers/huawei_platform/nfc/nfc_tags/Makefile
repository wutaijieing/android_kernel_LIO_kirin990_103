#
# Makefile for FDP NCI based NFC driver
#
NFC_TAGS_PLATFORM :=LCD_LONDON_PLATFORM

nfc_tags-objs = fm11n.o nta5332.o nfc_kit_ndef.o nfc_kit.o

ifeq ($(NFC_TAGS_PLATFORM),HUASHAN_PLATFORM)
	nfc_tags-objs += huashan_ndef.o
	ccflags-$(CONFIG_NFC_TAGS) := -DCONFIG_HUASHAN_PLATFORM=1
endif

ifeq ($(NFC_TAGS_PLATFORM),LCD_LONDON_PLATFORM)
	nfc_tags-objs += huashan_ndef.o
	ccflags-$(CONFIG_NFC_TAGS) := -DCONFIG_LCD_LONDON_PLATFORM=1
endif

obj-$(CONFIG_NFC_TAGS)     += nfc_tags.o
