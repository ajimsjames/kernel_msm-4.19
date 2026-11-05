#!/bin/bash
set -e

CLANG_DIR=/home/aju/Android/voltageos/prebuilts/clang/host/linux-x86/clang-r563880c
GCC64_DIR=/home/aju/Android/voltageos/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9
GCC32_DIR=/home/aju/Android/voltageos/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9

export PATH=$CLANG_DIR/bin:$GCC64_DIR/bin:$GCC32_DIR/bin:$PATH

ARGS=(
    ARCH=arm64
    CC=clang
    CLANG_TRIPLE=aarch64-linux-gnu-
    CROSS_COMPILE=aarch64-linux-android-
    CROSS_COMPILE_ARM32=arm-linux-androideabi-
    LD=ld.lld
    AR=llvm-ar
    NM=llvm-nm
    OBJCOPY=llvm-objcopy
    OBJDUMP=llvm-objdump
    STRIP=llvm-strip
    DTC_EXT=/usr/bin/dtc
)

cd /home/aju/Android/kernel_4.19

# Ensure CC=clang is passed to make so it detects clang instead of gcc 4.9
make "${ARGS[@]}" olddefconfig
make -j$(nproc) "${ARGS[@]}" Image.gz-dtb dtbs

# Build qm215-cats22.dtb
make "${ARGS[@]}" vendor/qcom/qm215-cats22.dtb
