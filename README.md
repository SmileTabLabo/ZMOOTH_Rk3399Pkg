# Rockchip RK3399 UEFI Firmware

Now with ACPI...and (almost working) USB!

## Notes

This is a version of the Rockchip 3399 UEFI firmware from https://github.com/andreiw/rk3399-edk2

## Building on vanilla ubuntu 18.04

```
apt-get install build-essential acpica-tools nasm uuid-dev gcc-aarch64-linux-gnu
# If you want to use a different workspace, change the target directory below
export WORKSPACE=~/workspace
mkdir $WORKSPACE
cd $WORKSPACE
git clone https://github.com/tianocore/edk2.git
pushd edk2
git checkout 46f4c9677c615d862649459392f8f55b3e6567c2
popd
git clone https://github.com/tianocore/edk2-non-osi.git
pushd edk2-non-osi
git checkout 1e2ca640be54d7a4d5d804c4f33894d099432de3
popd
git clone https://github.com/tianocore/edk2-platforms.git
pushd edk2-platforms
git checkout 861c200cda1417539d46fe3b1eba2b582fa72cbb
popd
git clone https://github.com/andreiw/rk3399-edk2.git edk2-platforms/Platform/Rockchip
export GCC5_AARCH64_PREFIX=aarch64-linux-gnu-
export PACKAGES_PATH=$WORKSPACE/edk2:$WORKSPACE/edk2-platforms:$WORKSPACE/edk2-non-osi
. edk2/edksetup.sh
make -C edk2/BaseTools
build -a AARCH64 -t GCC5 -p edk2-platforms/Platform/Rockchip/Rk3399Pkg/Rk3399-SDK.dsc -b DEBUG
edk2-platforms/Platform/Rockchip/Rk3399Pkg/Tools/loaderimage --pack --uboot Build/Rk3399-SDK/DEBUG_GCC5/FV/RK3399_SDK_UEFI.fd RK3399_SDK_UEFI.img
```

## Flashing the UEFI firmware

  How to put this together? You need a GPT disk. SD card will be fine, or
  you can use rkdeveloptool to modify the eMMC.

  The start/end sectors are important!

    Number  Start (sector)    End (sector)  Size       Code  Name
       1              64            8063   3.9 MiB     FFFF  loader1   <-- Rk3399Pkg/Tools/Bin/idbloader.bin
       2            8064            8191   64.0 KiB    FFFF  reserved1
       3            8192           16383   4.0 MiB     FFFF  reserved2
       4           16384           24575   4.0 MiB     FFFF  loader2   <-- RK3399_SDK_UEFI.img
       5           24576           32767   4.0 MiB     FFFF  atf       <-- Rk3399Pkg/Tools/Bin/trust.img
       6           32768          262143   112.0 MiB   EF00  efi esp
