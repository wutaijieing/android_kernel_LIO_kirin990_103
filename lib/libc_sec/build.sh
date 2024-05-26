#!/bin/bash
# Copyright Huawei Technologies Co., Ltd. 2010-2020. All rights reserved.

set -e

ROOT_DIR=$(readlink -f $(dirname "$0")/../../..)
BUILD_DIR=$(readlink -f $(dirname "$0"))

source "${ROOT_DIR}"/vendor/hisi/ap/build/core/setup_env.sh

#=============== init make args =========================
OBB_TARGET_PRODUCT="$1"
OBB_OUT_PRODUCT_DIR="$2"
OBB_PRODUCT_LOG_DIR="$3"
MODULE_NAME="$4"

#================== do make module ============================
set -x
(cd "${ROOT_DIR}" && make -C "${BUILD_DIR}" PRODUCT_NAME=${OBB_TARGET_PRODUCT} O=${OBB_OUT_PRODUCT_DIR} 1>${OBB_PRODUCT_LOG_DIR}/obuild_${MODULE_NAME}.log 2>&1)
set +x
