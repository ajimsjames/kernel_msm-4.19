#!/bin/bash
set -e

KERNEL_DIR="/home/aju/Android/kernel_4.19"
OUT_DIR="/home/aju/Android/Cat_S22_ARM64_RestlessOS_Flash"
DTC_BIN="/home/aju/Android/voltageos/out/host/linux-x86/bin/dtc"
TMP_DIR="/tmp/dtbo_build"

mkdir -p "$TMP_DIR"
mkdir -p "$OUT_DIR"

echo "[1/3] Preprocessing DTS with cpp..."
cpp -nostdinc \
    -I "$KERNEL_DIR/include" \
    -I "$KERNEL_DIR/arch/arm64/boot/dts" \
    -I "$KERNEL_DIR/arch/arm64/boot/dts/vendor-legacy" \
    -I "$KERNEL_DIR/arch/arm64/boot/dts/vendor-legacy/qcom" \
    -undef -x assembler-with-cpp \
    "$KERNEL_DIR/arch/arm64/boot/dts/vendor-legacy/qcom/qm215-cats22-overlay.dts" \
    > "$TMP_DIR/qm215-cats22-overlay.dts.preprocessed"

echo "[2/3] Compiling raw DTB overlay with dtc..."
"$DTC_BIN" -@ -I dts -O dtb \
    -o "$TMP_DIR/qm215-cats22-overlay.dtbo" \
    "$TMP_DIR/qm215-cats22-overlay.dts.preprocessed"

echo "[3/3] Packing into valid Qualcomm DTBO table container (magic 0xd7b7ab1e)..."
python3 "$KERNEL_DIR/tools/pack_dtbo_container.py" \
    "$TMP_DIR/qm215-cats22-overlay.dtbo" \
    "$OUT_DIR/dtbo.img"

# Also keep timestamped backup
BACKUP_DIR="/home/aju/Android/restless_os/dtbo/backup"
mkdir -p "$BACKUP_DIR"
cp "$OUT_DIR/dtbo.img" "$BACKUP_DIR/cats22_arm64_dtbo_$(date +%Y%m%d_%H%M%S).img"

echo "[+] Done: $OUT_DIR/dtbo.img is ready for flashing!"
