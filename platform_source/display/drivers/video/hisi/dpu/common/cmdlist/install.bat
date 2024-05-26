adb remount
adb push signed\lib\modules\4.14.116\extra /vendor/lib/modules/

adb shell lsmod

adb shell rmmod /vendor/lib/modules/extra/testcase/test.ko
adb shell rmmod /vendor/lib/modules/extra/kunit/kunit.ko
adb shell rmmod /vendor/lib/modules/extra/libcmdlist.ko

adb shell insmod /vendor/lib/modules/extra/libcmdlist.ko
adb shell insmod /vendor/lib/modules/extra/kunit/kunit.ko
adb shell insmod /vendor/lib/modules/extra/testcase/test.ko

pause

adb shell dmesg -c > kernel.log