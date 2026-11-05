#!/bin/bash
set -e

cd /home/aju/Android/kernel_4.19

echo "--- Cloning DTS Repositories ---"
[ -d "arch/arm64/boot/dts/vendor-legacy/.git" ] || git clone --depth 1 -b dts/master https://github.com/Mi-Thorium/kernel_devicetree_msm-4.19 arch/arm64/boot/dts/vendor-legacy
[ -d "arch/arm64/boot/dts/xiaomi-msm8937/.git" ] || git clone --depth 1 -b dts/mi8937/4.19/master https://github.com/Mi-Thorium/kernel_devicetree_xiaomi-msm8937 arch/arm64/boot/dts/xiaomi-msm8937
[ -d "arch/arm64/boot/dts/xiaomi-sdm439/.git" ] || git clone --depth 1 -b dts/mi439/4.19/master https://github.com/Mi-Thorium/kernel_devicetree_xiaomi-sdm439 arch/arm64/boot/dts/xiaomi-sdm439

echo "--- Cloning Techpack Repositories ---"
mkdir -p techpack
[ -d "techpack/xiaomi-msm8937/.git" ] || git clone --depth 1 -b techpack/mi8937/4.19/master https://github.com/Mi-Thorium/kernel_techpack_xiaomi-msm8937 techpack/xiaomi-msm8937
[ -d "techpack/xiaomi-sdm439/.git" ] || git clone --depth 1 -b techpack/mi439/4.19/master https://github.com/Mi-Thorium/kernel_techpack_xiaomi-sdm439 techpack/xiaomi-sdm439
[ -d "techpack/audio-legacy/.git" ] || git clone --depth 1 -b audio/4.19/mithorium/master https://github.com/Mi-Thorium/kernel_techpack_audio-legacy techpack/audio-legacy
[ -d "techpack/camera-legacy/.git" ] || git clone --depth 1 -b camera/LA.UM.8.6.r1-05300-89xx.0/4.19/mithorium https://github.com/Mi-Thorium/kernel_techpack_camera-legacy techpack/camera-legacy
[ -d "techpack/camera-legacy-m/.git" ] || git clone --depth 1 -b camera/LA.UM.5.3-10610-89xx.0/4.19/mithorium https://github.com/Mi-Thorium/kernel_techpack_camera-legacy techpack/camera-legacy-m

echo "--- Cloning Prima WLAN ---"
mkdir -p drivers/staging
[ -d "drivers/staging/prima/.git" ] || git clone --depth 1 -b wlan/LA.UM.9.6.4/mithorium/master https://github.com/Mi-Thorium/vendor_qcom_opensource_wlan_prima drivers/staging/prima

echo "All repositories cloned and linked successfully!"
