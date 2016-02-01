adb remount
adb push out/target/product/generic/system/bin/NfcMiddlewareSimulator /system/bin/
#adb push external/nfccsimulator/log/nexus6p_arf04.2.0_nfcon_filtered.log /etc/
adb push external/nfccsimulator/log/nexus6p_arf04.2.0_p2p_rflost.log /etc/
adb push external/nfccsimulator/conf/libnfc-brcm.conf /etc/
adb push external/nfccsimulator/conf/libnfc-nxp.conf /etc/
adb shell setenforce 0
adb shell cat /proc/kmsg

