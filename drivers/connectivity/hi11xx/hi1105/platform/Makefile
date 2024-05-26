mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))

ifeq ($(_PRE_PRODUCT_VERSION), 1161)
include $(mkfile_dir)/116x.mk
else
include $(mkfile_dir)/110x.mk
endif
