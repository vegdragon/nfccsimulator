#adb shell NfcMiddlewareSimulator r /etc/nexus6p_arf04.2.0_nfcon_filtered.log
adb shell NfcMiddlewareSimulator r /etc/nexus6p_arf04.2.0_p2p_rflost.log
adb shell logcat -c
adb shell logcat -v time | tee nfc_on.log

