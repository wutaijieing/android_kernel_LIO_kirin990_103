# SPDX-License-Identifier: GPL-2.0
# These files are disabled because they produce non-interesting flaky coverage
# that is not a function of syscall inputs. E.g. involuntary context switches.
KCOV_INSTRUMENT := n

obj-y := phase_feature_sysctl.o phase_feature_tracking.o phase_feature.o
obj-y += phase_feature_dctree.o
obj-y += phase_feature_kmeans.o
obj-y += phase_feature_helper.o
obj-$(CONFIG_PHASE_PERF) += phase_perf.o
