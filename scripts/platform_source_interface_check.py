#!/usr/bin/env python3
# coding=utf-8
#版权信息：华为技术有限公司，版本所有(C) 2010-2022
import os
import sys
import subprocess
import logging


def transfer_file_to_set(file_name):
    symbol_set = set()

    if not os.path.isfile(file_name):
        logging.error("file %s is not existed", file_name)
        return False, symbol_set
    with open(file_name, 'r') as fp:
        lines = fp.readlines()

    for line in lines:
        line = line.strip()
        if line:
            symbol_set.add(line)
    return True, symbol_set


if __name__ == "__main__":
    if len(sys.argv) != 4:
        logging.error("Error: needs 3 input parameters")
        exit(-1)

    base_interface_file = sys.argv[1]
    current_interface_file = sys.argv[2]
    mod_name = sys.argv[3]

    ret, base_interface_set = transfer_file_to_set(base_interface_file)
    if not ret:
        logging.error("Error: get base interface failed!!!")
        exit(-1)

    ret, current_interface_set = transfer_file_to_set(current_interface_file)
    if not ret:
        logging.error("Error: get current interface failed!!!")
        exit(-1)

    if base_interface_set.issubset(current_interface_set):
        logging.error("%s interface check pass!", mod_name)
        exit(0)
    else:
        logging.error("Error: %s loss interface: %s", mod_name, str(base_interface_set - current_interface_set))
        exit(-1)
