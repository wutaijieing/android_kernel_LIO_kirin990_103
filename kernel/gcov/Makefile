# SPDX-License-Identifier: GPL-2.0
ccflags-y := -DSRCTREE='"$(srctree)"' -DOBJTREE='"$(objtree)"'

obj-y := base.o fs.o
obj-$(CONFIG_GCOV_FORMAT_4_7) += gcc_base.o gcc_4_7.o
obj-$(CONFIG_CC_IS_CLANG) += clang.o

obj-$(CONFIG_HISI_KERNEL_GCOV) += hisi_gcov/
