#!/bin/bash
function check_file_exist()
{
  if [ ! -f $1 ]; then
    return 0
  fi
  return 1
}

function check_dir_exist()
{
  if [ ! -d $1 ]; then
    return 0
  fi
  return 1
}


function check_imgdir()
{
  check_dir_exist "./image"
  if [ $? -eq 0 ]; then
    mkdir "./image"
  fi
}

function build_uboot()
{
  source ./setenv
  echo building u-boot...
  cd bootable/bootloader/u-boot
  make omap3_nowplus_config
  make
  cd -
}

function build_kernel()
{
  echo building kernel...
  source ./setenv
  cd kernel
  export CROSS_COMPILE="../prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-"
  make omap_nowplus_defconfig
  make -j4
  export KDIR=`pwd`
  make -C ./modules
  cd -
}

function build_wl1271()
{
  check_file_exist "./kernel/arch/arm/boot/zImage"
  if [ $? -eq 0 ]; then
    build_kernel
  fi

  source ./setenv
  export KERNEL_DIR=`pwd`/kernel
  export HOST_PLATFORM=zoom2
  export PROPRIETARY_SDIO=y


  cd ./hardware/ti/wlan/wl1271/platforms/os/linux/
  make clean
  make
  cd -

  cd ./hardware/ti/wlan/wl1271_softAP/platforms/os/linux/
  make clean
  make
  cd -
}

function build_android()
{
  echo building android...
  
  check_file_exist "./kernel/arch/arm/boot/zImage"
  if [ $? -eq 0 ]; then
    build_kernel
  fi

  check_file_exist "./hardware/ti/wlan/wl1271/platforms/os/linux/tiwlan_drv.ko"
  if [ $? -eq 0 ]; then
    build_wl1271
  fi

  source ./setenv
  . build/envsetup.sh  
  lunch nowplus-eng  
  make -j4
  check_imgdir
  rm -rf ./image/system
  cp -raf ./out/target/product/nowplus/system ./image/
  find ./kernel/ -name "*.ko" | xargs -I {} cp {} ./image/system/lib/modules/
  cp ./hardware/ti/wlan/wl1271/platforms/os/linux/tiwlan_drv.ko ./image/system/etc/wifi/
  cp ./hardware/ti/wlan/wl1271_softAP/platforms/os/linux/tiap_drv.ko ./image/system/etc/wifi/softap/
}

function make_boot()
{
  #offest 0x50000
  uboot_max=327680

  check_imgdir
  rm -f ./image/boot.bin
  dd if=/dev/zero of=./image/boot.bin count=$uboot_max bs=1
  dd if=./bootable/bootloader/u-boot/u-boot.bin of=./image/boot.bin conv=notrunc
  cat ./kernel/arch/arm/boot/uImage >> ./image/boot.bin
  cd ./image/
  cp ./boot.bin ./zImage
  tar cvf i8320.tar zImage
  rm zImage
  cd -
}

function build_boot()
{
  echo building boot.bin...
  check_dir_exist "./out/target/product/nowplus/recovery/root"
  if [ $? -eq 0 ]; then
    build_android
  fi

  source ./setenv
  cd kernel
  #make distclean
  export CROSS_COMPILE="../prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-"
  make omap_nowplus_defconfig
  old='CONFIG_INITRAMFS_SOURCE=""'
  new='CONFIG_INITRAMFS_SOURCE="../out/target/product/nowplus/recovery/root"\nCONFIG_INITRAMFS_ROOT_UID=0\nCONFIG_INITRAMFS_ROOT_GID=0\nCONFIG_INITRAMFS_COMPRESSION_NONE=y\n# CONFIG_INITRAMFS_COMPRESSION_GZIP is not set'
  sed -i "s|$old|$new|g" ./.config
  make uImage -j4
  cd -
  check_imgdir
  cp ./kernel/arch/arm/boot/uImage ./image/recovery.img
  make_boot
}

function build_rootfs()
{
  echo building ramdisk...
  check_dir_exist "./out/target/product/nowplus/root"
  if [ $? -eq 0 ]; then
    build_android
  fi

  source ./setenv
  cd kernel
  #make distclean
  export CROSS_COMPILE="../prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-"
  make omap_nowplus_defconfig
  old='CONFIG_INITRAMFS_SOURCE=""'
  new='CONFIG_INITRAMFS_SOURCE="../out/target/product/nowplus/root"\nCONFIG_INITRAMFS_ROOT_UID=0\nCONFIG_INITRAMFS_ROOT_GID=0\nCONFIG_INITRAMFS_COMPRESSION_NONE=y\n# CONFIG_INITRAMFS_COMPRESSION_GZIP is not set'
  sed -i "s|$old|$new|g" ./.config
  make uImage -j4
  cd -
  check_imgdir
  cp ./kernel/arch/arm/boot/uImage ./image/boot.img
}

function clean()
{
  rm -rf ./out/target/product/nowplus/*.img
  rm -rf ./out/target/product/nowplus/root
  rm -rf ./out/target/product/nowplus/system
  rm -rf ./out/target/product/nowplus/recovery
  rm -f ./hardware/ti/wlan/wl1271/platforms/os/linux/tiwlan_drv.ko
}

function usage()
{
  echo "./build.sh [ARGS] (uboot, kernel, android, boot, rootfs, wl1271, clean, all)."
  exit 0
}

check_file_exist "./setenv"
if [ $? -eq 1 ]; then
  source ./setenv
else
  echo "Can not found ./setenv"
  exit -1;
fi

if [ $# -ne 1 ]
then
  usage
fi


case "$1" in
  "uboot")
	  build_uboot
	  ;;
  "kernel")
	  build_kernel
	  ;;
  "android")
	  build_android
	  ;;
  "boot")
	  build_boot
	  ;;
  "rootfs")
	  build_rootfs
	  ;;
  "wl1271")
	  build_wl1271
	  ;;
  "clean")
	  clean
	  ;;
  "all")
	  build_uboot
	  build_kernel
	  build_android
	  build_boot
	  build_rootfs
	  ;;
  *) 
	  usage
	  ;;
esac
