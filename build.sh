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
	"all")
		build_android
		build_uboot
		build_kernel
		;;
	*) 
		usage
		;;
esac
