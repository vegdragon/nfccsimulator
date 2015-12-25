adb remount
adb push out/target/product/generic/system/bin/NfcMiddlewareSimulator /system/bin/
adb push external/nfccsimulator/nfc_on_off_filtered.log /etc/
adb push external/nfccsimulator/conf/libnfc-brcm.conf /etc/
adb push external/nfccsimulator/conf/libnfc-nxp.conf /etc/
adb shell cat /proc/kmsg

