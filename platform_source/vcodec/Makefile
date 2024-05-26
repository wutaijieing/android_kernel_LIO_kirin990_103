
obj-$(CONFIG_VCODEC_VDEC) += vdec/
obj-$(CONFIG_VCODEC_VENC) += venc/

menuconfig:
	cd ../..;make ARCH=arm64 menuconfig KBUILD_KCONFIG=platform_source/vcodec/Kconfig KCONFIG_CONFIG=platform_source/vcodec/.config
