#!/bin/bash

set -e

# --- Configuration ---
export WORKSPACE=$(pwd)/workspace
IMAGE_FILE="rk3399_uefi_sd_card_gpt.img"
IMAGE_SIZE_SECTORS=262144
SECTOR_SIZE=512
IMAGE_SIZE_BYTES=$((IMAGE_SIZE_SECTORS * SECTOR_SIZE))

# --- Functions ---

clone_and_build() {
    echo ">>> Cloning and building EDK2..."
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
    echo ">>> Build finished successfully."
}

create_image() {
    echo ">>> ディスクイメージを作成中..."
    cd $WORKSPACE

    # works.sh からの変数定義を build.sh のコンテキストに合わせて調整
    IMG_FILE="${IMAGE_FILE}" # 既存の IMAGE_FILE 変数を使用
    TOTAL_SECTORS=${IMAGE_SIZE_SECTORS} # 既存の IMAGE_SIZE_SECTORS 変数を使用
    SECTOR_SIZE=${SECTOR_SIZE} # 既存の SECTOR_SIZE 変数を使用
    TOTAL_SIZE_MB=$((TOTAL_SECTORS * SECTOR_SIZE / 1024 / 1024))

    # バイナリファイルのパスを定義
    IDBLOADER_BIN="edk2-platforms/Platform/Rockchip/Rk3399Pkg/Tools/Bin/idbloader.img"
    UEFI_IMG="RK3399_SDK_UEFI.img"
    TRUST_IMG="edk2-platforms/Platform/Rockchip/Rk3399Pkg/Tools/Bin/trust.img"

    # ビルド成果物が存在するか確認
    if [ ! -f "$IDBLOADER_BIN" ] || [ ! -f "$UEFI_IMG" ] || [ ! -f "$TRUST_IMG" ]; then
        echo "エラー: ビルド成果物が見つかりません。先に './build.sh build' を実行してください。"
        exit 1
    fi

    # 既存のイメージファイルを削除
    rm -f ${IMG_FILE}

    # ゼロで埋められたイメージファイルを作成
    echo "ゼロで埋められたイメージファイル ${IMG_FILE} を ${TOTAL_SIZE_MB}MB のサイズで作成中..."
    dd if=/dev/zero of=$IMG_FILE bs=1M count=${TOTAL_SIZE_MB}

    # parted を使用して GPT パーティションテーブルとパーティションを作成
    echo "parted を使用して GPT パーティションテーブルとパーティションを作成中..."
    parted -s $IMG_FILE mklabel gpt

    # パーティションを作成
    # parted の mkpart はファイルシステムタイプを要求しますが、後で上書きするためプレースホルダーとして 'fat32' または 'ext2' を使用します。
    # その後、適切なフラグを設定します。

    # パーティション 1: loader1
    parted -s $IMG_FILE mkpart loader1 64s 8063s
    parted -s $IMG_FILE name 1 loader1
    parted -s $IMG_FILE set 1 bios_grub on # BIOS ブートパーティションに最も近いフラグ

    # パーティション 2: reserved1
    parted -s $IMG_FILE mkpart reserved1 8064s 8191s
    parted -s $IMG_FILE name 2 reserved1

    # パーティション 3: reserved2
    parted -s $IMG_FILE mkpart reserved2 8192s 16383s
    parted -s $IMG_FILE name 3 reserved2

    # パーティション 4: loader2
    parted -s $IMG_FILE mkpart loader2 16384s 24575s
    parted -s $IMG_FILE name 4 loader2
    parted -s $IMG_FILE set 4 bios_grub on

    # パーティション 5: atf
    parted -s $IMG_FILE mkpart atf 24576s 32767s
    parted -s $IMG_FILE name 5 atf
    parted -s $IMG_FILE set 5 bios_grub on

    # パーティション 6: efi_esp
    parted -s $IMG_FILE mkpart efi_esp fat32 32768s 262110s
    parted -s $IMG_FILE name 6 efi_esp
    parted -s $IMG_FILE set 6 esp on

    # バイナリファイルをそれぞれのオフセット（セクターオフセット）に書き込み
    echo "idbloader.img を書き込み中..."
    dd if=$IDBLOADER_BIN of=$IMG_FILE bs=$SECTOR_SIZE seek=64 conv=notrunc

    echo "RK3399_SDK_UEFI.img を書き込み中..."
    dd if=$UEFI_IMG of=$IMG_FILE bs=$SECTOR_SIZE seek=16384 conv=notrunc

    echo "trust.img を書き込み中..."
    dd if=$TRUST_IMG of=$IMG_FILE bs=$SECTOR_SIZE seek=24576 conv=notrunc

    echo ">>> イメージファイル ${IMG_FILE} の作成が完了しました。"
}

usage() {
    echo "Usage: $0 [mode]"
    echo
    echo "Modes:"
    echo "  <none>  - Run both 'build' and 'image' steps."
    echo "  build   - Clones repositories and builds the firmware only."
    echo "  image   - Creates the disk image from existing build artifacts."
    echo "  help    - Show this help message."
}

# --- Main Logic ---
MODE=$1

if [ -z "$MODE" ]; then
    MODE="all"
fi

case "$MODE" in
    all)
        clone_and_build
        create_image
        ;;
    build)
        clone_and_build
        ;;
    image)
        create_image
        ;;
    help|-h|--help)
        usage
        ;;
    *)
        echo "Error: Invalid mode '$MODE'."
        usage
        exit 1
        ;;
esac