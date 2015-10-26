adb remount
adb push out/target/product/generic/system/bin/nfccsimulator /system/bin/
adb push external/nfccsimulator/nfc_on_off_filtered.log /etc/
adb shell cat /proc/kmsg

