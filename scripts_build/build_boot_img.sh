#!/bin/bash
set -e

KERNEL_DIR="/home/aju/Android/kernel_4.19"
OUT_DIR="/home/aju/Android/Cat_S22_ARM64_RestlessOS_Stable_RW"
KERNEL_BIN="$KERNEL_DIR/arch/arm64/boot/Image.gz"
RAMDISK="/tmp/boot_unpack_info/ramdisk"
DTB="/tmp/boot_unpack_info/dtb"

if [ ! -f "$KERNEL_BIN" ]; then
    echo "[-] Kernel binary not found at $KERNEL_BIN. Run build_kernel.sh first!"
    exit 1
fi

if [ ! -f "$RAMDISK" ] || [ ! -f "$DTB" ]; then
    echo "[-] Extracting base ramdisk and dtb from existing working boot image..."
    mkdir -p /tmp/boot_unpack_info
    unpack_bootimg --boot_img /home/aju/Android/restless_os/boot/backup/boot_ksu_419_arm64_normal_20261106_155535.img --out /tmp/boot_unpack_info
fi

echo "[+] Packing boot.img with explicit androidboot.mode=normal override..."
mkbootimg \
  --kernel "$KERNEL_BIN" \
  --ramdisk "$RAMDISK" \
  --dtb "$DTB" \
  --cmdline "androidboot.mode=normal androidboot.hardware=qcom msm_rtb.filter=0x237 ehci-hcd.park=3 lpm_levels.sleep_disabled=1 androidboot.bootdevice=7824900.sdhci loop.max_part=7 printk.devkmsg=on androidboot.boot_devices=soc/7824900.sdhci cgroup.memory=nokmem,nosocket androidboot.selinux=permissive buildvariant=eng" \
  --base 0x80000000 \
  --pagesize 2048 \
  --kernel_offset 0x00008000 \
  --ramdisk_offset 0x01000000 \
  --tags_offset 0x00000100 \
  --dtb_offset 0x01f00000 \
  --os_version 16.0.0 \
  --os_patch_level 2026-01 \
  --header_version 2 \
  -o "$OUT_DIR/boot.img"

BACKUP_DIR="/home/aju/Android/restless_os/boot/backup"
mkdir -p "$BACKUP_DIR"
cp "$OUT_DIR/boot.img" "$BACKUP_DIR/boot_ksu_419_arm64_normal_$(date +%Y%m%d_%H%M%S).img"

echo "[+] Done: $OUT_DIR/boot.img packed successfully!"
ls -lh "$OUT_DIR/boot.img"
