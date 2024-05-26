# codec reg check in call

ifdef CONFIG_CODEC_CHECK
	ccflags-y += -I$(srctree)/drivers/devkit/audiokit/
endif

obj-$(CONFIG_CODEC_CHECK)	:= codec_check.o
