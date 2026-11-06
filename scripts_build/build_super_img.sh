#!/bin/bash
set -e

LPMAKE="/usr/sbin/lpmake"
IMAGES_DIR="/home/aju/Android/restless_os/images"
OUT_FILE="/home/aju/Android/Cat_S22_ARM64_RestlessOS_Flash/super_restless_audio_fixed.img"

SYS_RAW="$IMAGES_DIR/system.img"
VEN_RAW="$IMAGES_DIR/vendor.img"

if [ ! -f "$SYS_RAW" ] || [ ! -f "$VEN_RAW" ]; then
    echo "[-] Raw partitions missing in $IMAGES_DIR"
    exit 1
fi

SYS_SIZE=$(stat -c%s "$SYS_RAW")
VEN_SIZE=$(stat -c%s "$VEN_RAW")

echo "[+] Building Dynamic Partitions (super image)..."
echo "    system: $SYS_SIZE bytes"
echo "    vendor: $VEN_SIZE bytes"

$LPMAKE \
    --metadata-size 65536 \
    --super-name super \
    --metadata-slots 2 \
    --device super:3581935616 \
    --group qti_dynamic_partitions:3577741312 \
    --partition system:readonly:$SYS_SIZE:qti_dynamic_partitions \
    --image system=$SYS_RAW \
    --partition vendor:readonly:$VEN_SIZE:qti_dynamic_partitions \
    --image vendor=$VEN_RAW \
    --sparse \
    --output "$OUT_FILE"

echo "[+] Done: Super image created at $OUT_FILE"
ls -lh "$OUT_FILE"
