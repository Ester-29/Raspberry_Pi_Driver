#!/bin/bash
# run_test.sh - runtest tu dong cho soft_rtc
# Chay trong thu muc da "make":   sudo ./run_test.sh
#   Raspberry Pi: script tu nap overlay (dtoverlay -d . soft-rtc) neu chua co
#   May ao x86  : khong co Device Tree -> script tu nap soft_rtc_dev.ko

DRV=soft-rtc
PASS=0
FAIL=0
ok()  { echo "[PASS] $*"; PASS=$((PASS + 1)); }
bad() { echo "[FAIL] $*"; FAIL=$((FAIL + 1)); }

[ "$(id -u)" -eq 0 ] || { echo "Hay chay voi sudo"; exit 2; }
cd "$(dirname "$0")" || exit 2

# Dang chay tren Raspberry Pi (co Device Tree cua Pi) hay may ao x86?
IS_PI=0
grep -q "Raspberry Pi" /proc/device-tree/model 2>/dev/null && IS_PI=1

# 1. Nap driver (neu chua nap)
if ! lsmod | grep -q '^soft_rtc '; then
    insmod ./soft_rtc.ko || { bad "insmod soft_rtc.ko (xem: sudo dmesg | tail)"; exit 1; }
fi
ok "driver soft_rtc da nap"

# 2. Phai co device khop: Pi -> node Device Tree | VM -> soft_rtc_dev.ko
if [ "$IS_PI" = 1 ]; then
    if [ ! -d /proc/device-tree/soc/rtc@7e003000 ] && [ -f soft-rtc.dtbo ]; then
        echo "       chua co node DT -> nap overlay: dtoverlay -d . soft-rtc"
        dtoverlay -d . soft-rtc
    fi
    if [ -d /proc/device-tree/soc/rtc@7e003000 ]; then
        ok "Device Tree co node /soc/rtc@7e003000"
    else
        bad "khong co node DT (xem: dtoverlay -l, file soft-rtc.dtbo)"; exit 1
    fi
else
    if ! lsmod | grep -q '^soft_rtc_dev '; then
        insmod ./soft_rtc_dev.ko || { bad "insmod soft_rtc_dev.ko"; exit 1; }
    fi
    ok "x86 khong co Device Tree -> tao device bang soft_rtc_dev.ko"
fi
sleep 1

# 3. Tim /dev/rtcN cua driver minh (tranh test nham rtc0 cua VirtualBox)
RTC=""
for d in /sys/class/rtc/rtc*; do
    grep -q "^$DRV" "$d/name" 2>/dev/null && RTC=$(basename "$d")
done
if [ -z "$RTC" ]; then
    bad "khong thay rtc nao cua $DRV -> probe chua chay (xem dmesg)"; exit 1
fi
DEV=$(basename "$(readlink -f "/sys/class/rtc/$RTC/device")")
ok "probe() OK: /dev/$RTC <- $(cat "/sys/class/rtc/$RTC/name")"

# 3b. Tren Pi: driver phai chay HW mode (doc thanh ghi that cua SoC)
if [ "$IS_PI" = 1 ]; then
    MODE=$(dmesg | grep "$DEV: " | grep -oE '(HW|SW) mode' | tail -n 1)
    if [ "$MODE" = "HW mode" ]; then
        ok "HW mode: driver doc bo dem System Timer qua MMIO"
    else
        bad "lan probe gan nhat khong phai HW mode ('$MODE') -> kiem tra 'reg' trong DT"
    fi
fi

# 4. Doc qua sysfs: RTC phai tang dung bang thoi gian thuc (do bang /proc/uptime),
#    khong so voi hang so 5 -> khong FAIL oan khi may ao bi khung
U1=$(cut -d' ' -f1 /proc/uptime); T1=$(cat "/sys/class/rtc/$RTC/since_epoch")
sleep 5
T2=$(cat "/sys/class/rtc/$RTC/since_epoch"); U2=$(cut -d' ' -f1 /proc/uptime)
D=$((T2 - T1)); R=$(awk -v a="$U1" -v b="$U2" 'BEGIN { printf "%.0f", b - a }')
if [ $((D - R)) -ge -1 ] && [ $((D - R)) -le 1 ]; then
    ok "sysfs since_epoch tang $D s, thoi gian thuc ~$R s"
else
    bad "sysfs since_epoch tang $D s nhung thoi gian thuc ~$R s"
fi

# 5. Test chuc nang qua ioctl (doc / dat / kiem thu bien)
if ./rtc_test "/dev/$RTC"; then ok "rtc_test: moi test ioctl deu dat"
else bad "rtc_test co test FAIL (xem o tren)"; fi

# 6. unbind -> remove() ; bind -> probe() chay lai
echo "$DEV" > "/sys/bus/platform/drivers/$DRV/unbind"; sleep 1
if [ ! -e "/sys/bus/platform/drivers/$DRV/$DEV" ]; then ok "unbind: remove() chay, /dev/$RTC bi go"
else bad "unbind that bai"; fi
echo "$DEV" > "/sys/bus/platform/drivers/$DRV/bind"; sleep 1
if [ -e "/sys/bus/platform/drivers/$DRV/$DEV" ]; then ok "bind: probe() chay lai"
else bad "bind that bai"; fi

echo
echo "===== KET QUA: $PASS PASS, $FAIL FAIL ====="
echo "--- dmesg lien quan ---"
dmesg | grep -E "soft_rtc|$DEV" | tail -n 15
echo "--- go bo sau khi test xong ---"
if [ "$IS_PI" = 1 ]; then
    echo "sudo dtoverlay -r soft-rtc && sudo rmmod soft_rtc"
else
    echo "sudo rmmod soft_rtc_dev soft_rtc"
fi
[ "$FAIL" -eq 0 ]
