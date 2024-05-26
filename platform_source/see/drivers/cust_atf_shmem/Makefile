#
# cust atf share memory makefile
#
subdir-ccflags-$(CONFIG_CUST_ATF_SHMEM)               += -Idrivers/platform_drivers

obj-$(CONFIG_CUST_ATF_SHMEM) += cust_atf_shmem.o
obj-$(CONFIG_ATF_SHMEM_TEST) += atf_shmem_test.o

subdir-ccflags-y += -Wdate-time -Wextra -Wfloat-equal -Wformat=2 -Wshadow
