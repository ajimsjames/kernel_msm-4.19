#!/bin/bash
cat arch/arm/boot/zImage /tmp/dtb12_cats22_clean.dtb > arch/arm/boot/zImage-dtb
echo "Appended DTB created: $(ls -lh arch/arm/boot/zImage-dtb)"
