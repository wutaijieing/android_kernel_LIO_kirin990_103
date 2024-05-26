#***********************************************************
# input parameters
#***********************************************************
MODULE_NAME ?=
TOP_DIR ?=
SRC_DIR ?=
SRC_C_FILES ?=
SRC_CPP_FILES ?=
INC_DIR ?=
DEPENDENT_LIBS ?=
CC_DEFINES ?=
EXTRA_CFLAGS ?=
EXTRA_LFLAGS ?=

#*************************************************************************
# Direcotry configuration to output compiling result
#*************************************************************************
LOCAL_OBJ_DIR := $(TOP_DIR)/out/target/product/$(PRODUCT_NAME)/obj/$(MODULE_NAME)
LOCAL_LIBRARY_PATH := $(TOP_DIR)/out/target/product/$(PRODUCT_NAME)
LOCAL_LIBRARY := $(LOCAL_LIBRARY_PATH)/$(MODULE_NAME).so

#*************************************************************************
# Compiler-Specific Configuration
#*************************************************************************
LOCAL_LINARO_BASE := $(TOP_DIR)/vendor/tools/opensource/gcc-toolchains/aarch64/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin
LOCAL_GCC := $(CCACHE) $(LOCAL_LINARO_BASE)/aarch64-linux-gnu-gcc
LOCAL_GPP := $(CCACHE) $(LOCAL_LINARO_BASE)/aarch64-linux-gnu-g++

# C configuration
LOCAL_INC_DIR := $(foreach inc_path, $(INC_DIR), -I$(subst \,/,$(inc_path)))

LOCAL_C_FILES := $(filter %.c,$(SRC_C_FILES)) $(wildcard $(SRC_DIR)/*.c)
LOCAL_C_OBJS := $(patsubst $(SRC_DIR)/%.c, $(LOCAL_OBJ_DIR)/%.o, $(LOCAL_C_FILES))

LOCAL_CPP_FILES := $(filter %.cpp,$(SRC_CPP_FILES)) $(wildcard $(SRC_DIR)/*.cpp)
LOCAL_CPP_OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(LOCAL_OBJ_DIR)/%.o, $(LOCAL_CPP_FILES))

LOCAL_CFLAGS := -Wall -Wno-multichar -Wno-write-strings \
                -fstack-protector-all \
                -fPIC \
                -D_FORTIFY_SOURCE=2 -O2 \
                -g \
                -z defs \
                $(EXTRA_CFLAGS)

LOCAL_LFLAGS := -shared \
                -s \
                -Wl,-z,relro \
                -Wl,-z,now \
                -Wl,-z,noexecstack \
                $(EXTRA_LFLAGS)

#***********************************************************
# rules
#***********************************************************
.PHONY: all do_pre_build do_build
all:do_pre_build do_build

do_pre_build:
	@echo - do [$@]
	$(Q)mkdir -p $(LOCAL_OBJ_DIR)

do_build: $(LOCAL_C_OBJS) $(LOCAL_CPP_OBJS) $(LOCAL_LIBRARY)
	@echo - do [$@]

$(LOCAL_C_OBJS): $(LOCAL_OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo - do [$@]
	@mkdir -p $(@D)
	$(Q)$(LOCAL_GCC) -c $< -o $@ $(LOCAL_CFLAGS) $(LOCAL_INC_DIR) $(CC_DEFINES)

$(LOCAL_CPP_OBJS): $(LOCAL_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo - do [$@]
	@mkdir -p $(@D)
	$(Q)$(LOCAL_GPP) -c $< -o $@ $(LOCAL_CFLAGS) $(LOCAL_INC_DIR) $(CC_DEFINES)

$(LOCAL_LIBRARY): $(LOCAL_C_OBJS) $(LOCAL_CPP_OBJS)
	@echo - do [$@]
	$(Q)$(LOCAL_GPP) -o $@ $^ $(LOCAL_LFLAGS) -L $(LOCAL_LIBRARY_PATH) $(DEPENDENT_LIBS)

#***********************************************************
# Clean up
#***********************************************************
.PHONY: clean
clean:
	@echo - do [$@]
	$(Q)rm -rf $(LOCAL_OBJ_DIR)
	$(Q)rm -f $(LOCAL_LIBRARY)
