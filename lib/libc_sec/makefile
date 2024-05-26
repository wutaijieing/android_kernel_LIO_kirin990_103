#***********************************************************#
# moudle name & core define
#***********************************************************#
MODULE_NAME := libc_secshared
TOP_DIR := $(patsubst %/vendor/open_source/bounds_checking_function,%,$(CURDIR))

#***********************************************************#
# flags
#***********************************************************#
# Default CFLAGS and LFLAGS are defined in rule.mk
# only EXTRA_CFLAGS and EXTRA_LFLAGS are defined here

#CFLAGS := -Wall -Wno-multichar -Wno-write-strings \
          -fstack-protector \
          -fPIC \
          -D_FORIFY_SOURCE=2 -O2 \
          -g \
          -z defs \
          -ftrapv \

EXTRA_CFLAGS :=

#LFLAGS := -shared \
          -s \
          -Wl,-z,relro \
          -Wl,-z,now \
          -Wl,-z,noexecstack \

EXTRA_LFLAGS :=

#***********************************************************#
# include directories
#***********************************************************#
INC_DIR := $(CURDIR)/src \
           $(CURDIR)/include \

#***********************************************************#
# source directories
#***********************************************************#
SRC_DIR := $(CURDIR)/src
SRC_C_FILES :=
SRC_CPP_FILES :=

#***********************************************************#
# dependent libs
#***********************************************************#
DEPENDENT_LIBS := #-lxxx

#***********************************************************#
# macro defines
#***********************************************************#
CC_DEFINES += \

#***********************************************************
# include rules. must be droped at the bottom
#***********************************************************
include $(CURDIR)/rule.mk
