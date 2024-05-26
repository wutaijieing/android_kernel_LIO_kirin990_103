ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DDBG_AUDIO_TRACE
endif

obj-$(CONFIG_AUDIO_TRACE) += audio_trace.o
obj-$(CONFIG_AUDIO_TRACE) += trace_utils.o
obj-$(CONFIG_AUDIO_TRACE) += pcm_trace.o
obj-$(CONFIG_AUDIO_TRACE) += mem_trace.o
