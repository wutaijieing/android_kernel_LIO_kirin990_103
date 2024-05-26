adb remount
adb push imagecomposer /system/bin/
adb shell cp vendor/lib64/libhiion.so /system/lib64/
adb shell cp vendor/lib/libhiion.so /system/lib/

adb shell sync

adb push ".\Scene.sce" "/data/SceneManager/scenes/online/Scene.sce"
adb push ".\Layer0_1080x1920_RGBA8888.rgb" "/data/SceneManager/scenes/online/../../Resource/Layer0/RGBA8888/Layer0_1080x1920_RGBA8888.rgb"
adb shell cd '/data/SceneManager/scenes/online' "&&" imagecomposer -r60.000000 -t1 -d10 -e *.sce
pause