#!/bin/bash
# chipset component build script encapsulation.
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

#cmd:
#./build_dkmd.sh 700 chip_type=es
# para1: chip_type, e.g., es or cs
# para2: dpu version, e.g., 700 or 160

set -e
set +e

LOCAL_PATH=$(pwd)
echo "local path: $LOCAL_PATH ++++++++++++++++++++++++++++++++ build begin ++++++++++++++++++++++++++++++++++"

if [ $# -lt 1 ]; then
	echo "please input paramater as this: ./build_dkmd.sh 700 chip_type=es [clean] or ./build_dkmd.sh 160 [clean]"
fi

KUNIT_PATH="$LOCAL_PATH/test/"
DKSM_PATH="$LOCAL_PATH/dksm/"
BACKLIGHT_PATH="$LOCAL_PATH/backlight/"
CMDLIST_PATH="$LOCAL_PATH/cmdlist/"
CONNECTOR_PATH="$LOCAL_PATH/connector/"
DPU_PATH="$LOCAL_PATH/dpu/begonia/"

make CONFIG_DKMD_DPU_VERSION=$1 $2 -C $KUNIT_PATH $3
make CONFIG_DKMD_DPU_VERSION=$1 $2 CONFIG_DKMD_DKSM=m -C $DKSM_PATH $3
make CONFIG_DKMD_DPU_VERSION=$1 $2 CONFIG_DKMD_BACKLIGHT=m -C $BACKLIGHT_PATH $3
make CONFIG_DKMD_DPU_VERSION=$1 $2 CONFIG_DKMD_CMDLIST_ENABLE=m -C $CMDLIST_PATH $3
make CONFIG_DKMD_DPU_VERSION=$1 $2 CONFIG_DKMD_DPU_CONNECTOR=m CONFIG_DKMD_DPU_PANEL=m CONFIG_DKMD_DPU_DP=m CONFIG_DKMD_DPU_OFFLINE=m -C $CONNECTOR_PATH $3
make CONFIG_DKMD_DPU_VERSION=$1 $2 CONFIG_DKMD_DPU_ENABLE=m CONFIG_DKMD_DPU_RES_MGR=m CONFIG_DKMD_DPU_DEVICE=m CONFIG_DKMD_DPU_COMPOSER=m -C $DPU_PATH $3

echo "local path: $LOCAL_PATH --------------------------------- build end -------------------------------------"

