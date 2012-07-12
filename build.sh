#!/bin/bash
source ./setenv

function build_uboot()
{
	echo building u-boot...
	cd bootable/bootloader/u-boot
	make omap3_nowplus_config
	make
}

function build_kernel()
{
	echo building kernel...
	cd kernel
    export CROSS_COMPILE="../prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-"
	make omap_nowplus_defconfig
	make -j4
	export KDIR=`pwd`
	make -C ./modules
}

function build_android()
{
	echo building android...
	. build/envsetup.sh  
	lunch nowplus-eng  
	make -j4
}

function make_boot()
{
	#offest 0x50000
	uboot_max=327680

	rm -f boot.bin
	dd if=/dev/zero of=boot.bin count=$uboot_max bs=1
	dd if=./bootable/bootloader/u-boot/u-boot.bin of=boot.bin conv=notrunc
	cat ./kernel/arch/arm/boot/uImage >> boot.bin
}

function build_boot()
{
	echo building boot.bin...
	cd kernel
    #make distclean
    export CROSS_COMPILE="../prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-"
	make omap_nowplus_defconfig
	old='CONFIG_INITRAMFS_SOURCE=""'
	new='CONFIG_INITRAMFS_SOURCE="../out/target/product/nowplus/recovery/root"\nCONFIG_INITRAMFS_ROOT_UID=0\nCONFIG_INITRAMFS_ROOT_GID=0\nCONFIG_INITRAMFS_COMPRESSION_NONE=y\n# CONFIG_INITRAMFS_COMPRESSION_GZIP is not set'
	sed -i "s|$old|$new|g" ./.config
	make uImage -j4
	cd ..
	make_boot
}

function usage()
{
	echo "./build.sh [ARGS] (uboot, kernel, android, all)."
	exit 0
}

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
	"all")
		build_uboot
		build_kernel
		build_android
		;;
	*) 
		usage
		;;
esac
