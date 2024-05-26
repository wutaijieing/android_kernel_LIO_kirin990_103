# soundwire driver
subdir-ccflags-y += -Wall -Werror

ccflags-y += -I$(srctree)/sound/soc/codecs/vendor/
ccflags-y += -I$(srctree)/sound/soc/codecs/vendor/fpga_test/
ccflags-y += -I$(srctree)/include/linux/platform_drivers/
ccflags-y += -I$(srctree)/sound/soc/codecs/vendor/adapter/
ccflags-y += -I$(srctree)/sound/soc/codecs/vendor/fpga_test/hi_audio_test
ccflags-y += -I$(srctree)/platform_source/audio/include

obj-y += soundwire.o
obj-y += soundwire_loop_test.o
obj-y += soundwire_ip_test.o
obj-y += soundwire_extend_test.o
obj-y += soundwire_enum.o
obj-y += soundwire_utils.o
#obj-y += dsp_img.o
obj-y += soundwire_3m_dpcfg.o
obj-y += soundwire_12m_dpcfg.o
