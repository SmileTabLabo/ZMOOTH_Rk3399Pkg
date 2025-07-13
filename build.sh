#!/bin/bash

set -e

# Set up the workspace
export WORKSPACE=$(pwd)/workspace
mkdir -p $WORKSPACE
cd $WORKSPACE

# Clone or update source code
if [ ! -d "edk2" ]; then
    git clone https://github.com/tianocore/edk2.git
fi
(cd edk2 && git fetch && git checkout 46f4c9677c615d862649459392f8f55b3e6567c2)

if [ ! -d "edk2-non-osi" ]; then
    git clone https://github.com/tianocore/edk2-non-osi.git
fi
(cd edk2-non-osi && git fetch && git checkout 1e2ca640be54d7a4d5d804c4f33894d099432de3)

if [ ! -d "edk2-platforms" ]; then
    git clone https://github.com/tianocore/edk2-platforms.git
fi
(cd edk2-platforms && git fetch && git checkout 861c200cda1417539d46fe3b1eba2b582fa72cbb)

if [ ! -d "edk2-platforms/Platform/Rockchip" ]; then
    git clone https://github.com/shantur/rk3399-edk2.git edk2-platforms/Platform/Rockchip
else
    (cd edk2-platforms/Platform/Rockchip && git pull)
fi

# Compile the EDK2
export GCC5_AARCH64_PREFIX=aarch64-linux-gnu-
export PACKAGES_PATH=$WORKSPACE/edk2:$WORKSPACE/edk2-platforms:$WORKSPACE/edk2-non-osi

. edk2/edksetup.sh

# The key fix: Remove -Werror from the BaseTools makefiles to prevent build failures with modern GCC.
sed -i 's/-Werror//g' edk2/BaseTools/Source/C/Makefiles/header.makefile

make -C edk2/BaseTools
build -a AARCH64 -t GCC5 -p edk2-platforms/Platform/Rockchip/Rk3399Pkg/Rk3399-SDK.dsc -b DEBUG
edk2-platforms/Platform/Rockchip/Rk3399Pkg/Tools/loaderimage --pack --uboot Build/Rk3399-SDK/DEBUG_GCC5/FV/RK3399_SDK_UEFI.fd RK3399_SDK_UEFI.img

# Create image file
IMAGE_FILE="rk3399_uefi_sd_card_gpt.img"
IMAGE_SIZE=$((262144 * 512))

IDBLOADER_BIN="edk2-platforms/Platform/Rockchip/Rk3399Pkg/Tools/Bin/idbloader.img"
UEFI_IMG="RK3399_SDK_UEFI.img"
TRUST_IMG="edk2-platforms/Platform/Rockchip/Rk3399Pkg/Tools/Bin/trust.img"

dd if=/dev/zero of=${IMAGE_FILE} bs=1 count=0 seek=${IMAGE_SIZE}

LOOP_DEVICE=$(sudo losetup --show -fP ${IMAGE_FILE})

sudo parted ${LOOP_DEVICE} mklabel gpt
sudo parted ${LOOP_DEVICE} mkpart loader1 64s 8063s
sudo parted ${LOOP_DEVICE} mkpart reserved1 8064s 8191s
sudo parted ${LOOP_DEVICE} mkpart reserved2 8192s 16383s
sudo parted ${LOOP_DEVICE} mkpart loader2 16384s 24575s
sudo parted ${LOOP_DEVICE} mkpart atf 24576s 32767s
sudo parted ${LOOP_DEVICE} mkpart esp fat32 32768s 262143s
sudo parted ${LOOP_DEVICE} set 6 esp on

sudo dd if=${IDBLOADER_BIN} of=${LOOP_DEVICE}p1 bs=512 seek=64
sudo dd if=${UEFI_IMG} of=${LOOP_DEVICE}p4 bs=512 seek=0
sudo dd if=${TRUST_IMG} of=${LOOP_DEVICE}p5 bs=512 seek=0

sudo losetup -d ${LOOP_DEVICE}

echo "Image file ${IMAGE_FILE} created successfully."
