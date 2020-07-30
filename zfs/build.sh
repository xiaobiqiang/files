#!/bin/bash

UNAME_M=`uname -m`
UNAME_R=`uname -r`

KERNEL_VERSION3="3"
KERNEL_VERSION4="4"
KERNEL_VERSION=

CENTOS_PLATFORM_V3="centos-3"
CENTOS_PLATFORM_V4="centos-4"

CENTOS="CentOS Linux"
DEEPIN="deepin GNU/Linux"
ZB_KYLIN="NeoKylin" 	#jz
YH_KYLIN="Kylin" 		#ft
PLATFORM=

function choose_platform_and_kernel()
{
    local temp
    local platform=`cat /etc/os-release | grep -w NAME | cut -d '=' -f 2 | cut -d '"' -f 2`
    local kernel=`uname -r | cut -d '-' -f 1 | cut -d '.' -f 1`

    KERNEL_VERSION=$kernel

    [[ x"$platform" == x$ZB_KYLIN ]] && PLATFORM=$ZB_KYLIN
    [[ x"$platform" == x$YH_KYLIN ]] && PLATFORM=$YH_KYLIN
    [[ x"$platform" == x$DEEPIN ]] && PLATFORM=$DEEPIN
    if [[ x"$platform" == x$CENTOS ]]; then
        [[ x"$KERNEL_VERSION" == x$KERNEL_VERSION3 ]] && PLATFORM=$CENTOS_PLATFORM_V3
		[[ x"$KERNEL_VERSION" == x$KERNEL_VERSION4 ]] && PLATFORM=$CENTOS_PLATFORM_V4
    fi

    [ -z $PLATFORM ] && exit 1
    [ -z $KERNEL_VERSION ] && exit 1
}

function prepare_cm()
{
    topdir=`pwd`
    cd cmd/cm/
    chmod 755 ./cpcmfile.sh
    ./cpcmfile.sh
    cd build/
    chmod 755 ./makeam.sh
    ./makeam.sh
    cd $topdir
}

function prepare_snmp()
{
    netsnmp_so_loc=

    SNMP_DIR="cmd/fm/modules/common/snmp-trapgen"
    SNMP_HEADERS="/usr/include/net-snmp"

    if [ ! -d $SNMP_HEADERS ];then
        cd $SNMP_DIR
        tar -xvf ./net-snmp.tar
        cp -rf ./net-snmp/ /usr/include
        rm -rf ./net-snmp/
    fi

    [[ x$PLATFORM == x$CENTOS_PLATFORM_V3 ]] && netsnmp_so_loc=/usr/lib64
    [[ x$PLATFORM == x$CENTOS_PLATFORM_V4 ]] && netsnmp_so_loc=/usr/lib64
    [[ x$PLATFORM == x$DEEPIN ]] && netsnmp_so_loc=/usr/lib/sw_64-linux-gnu
    [[ x$PLATFORM == x$ZB_KYLIN ]] && netsnmp_so_loc=/usr/lib
    [[ x$PLATFORM == x$YH_KYLIN ]] && netsnmp_so_loc=/usr/lib/aarch64-linux-gnu

    [ -z $netsnmp_so_loc ] && exit 1

    netsnmp_so="libnetsnmp.so libnetsnmpagent.so"
    for so in ${netsnmp_so}; do
        [ ! -f ${netsnmp_so_loc}/${so} ] && ln -s ${netsnmp_so_loc}/`ls ${netsnmp_so_loc}/ | grep ${so}.[0-9][0-9].[0-9].` ${netsnmp_so_loc}/${so}
    done
    cd $topdir
}

function prepare_compile_env()
{
    if [[ x$PLATFORM == x$DEEPIN ]]; then
        [ ! -f /usr/bin/sw_64sw6-sunway-linux-gnu-gcc ] && ln -s /usr/bin/gcc /usr/bin/sw_64sw6-sunway-linux-gnu-gcc
        [ ! -f /usr/bin/sw_64sw6-sunway-linux-gnu-ld ] && ln -s /usr/bin/ld /usr/bin/sw_64sw6-sunway-linux-gnu-ld
        [ ! -f /usr/bin/sw_64sw6-sunway-linux-gnu-ar ]&& ln -s /usr/bin/ar /usr/bin/sw_64sw6-sunway-linux-gnu-ar
        [ ! -f /usr/bin/sw_64sw6-sunway-linux-gnu-nm ]&& ln -s /usr/bin/nm /usr/bin/sw_64sw6-sunway-linux-gnu-nm
        [ ! -f /usr/bin/sw_64sw6-sunway-linux-gnu-strip ]&& ln -s /usr/bin/strip /usr/bin/sw_64sw6-sunway-linux-gnu-strip
        [ ! -f /usr/bin/sw_64sw6-sunway-linux-gnu-objdump ]&& ln -s /usr/bin/objdump /usr/bin/sw_64sw6-sunway-linux-gnu-objdump
        [ ! -f /usr/bin/sw_64sw6-sunway-linux-gnu-objcopy ]&& ln -s /usr/bin/objcopy /usr/bin/sw_64sw6-sunway-linux-gnu-objcopy
        [ ! -f /usr/bin/sw_64sw2-sunway-linux-gnu-gcc ]&& ln -s /usr/bin/gcc /usr/bin/sw_64sw2-sunway-linux-gnu-gcc
        [ ! -f /usr/bin/sw_64sw2-sunway-linux-gnu-ld ]&& ln -s /usr/bin/ld /usr/bin/sw_64sw2-sunway-linux-gnu-ld
        [ ! -f /usr/bin/sw_64sw2-sunway-linux-gnu-ar ]&& ln -s /usr/bin/ar /usr/bin/sw_64sw2-sunway-linux-gnu-ar
        [ ! -f /usr/bin/sw_64sw2-sunway-linux-gnu-nm ]&& ln -s /usr/bin/nm /usr/bin/sw_64sw2-sunway-linux-gnu-nm
        [ ! -f /usr/bin/sw_64sw2-sunway-linux-gnu-strip ]&& ln -s /usr/bin/strip /usr/bin/sw_64sw2-sunway-linux-gnu-strip
        [ ! -f /usr/bin/sw_64sw2-sunway-linux-gnu-objdump ]&& ln -s /usr/bin/objdump /usr/bin/sw_64sw2-sunway-linux-gnu-objdump
        [ ! -f /usr/bin/sw_64sw2-sunway-linux-gnu-objcopy ]&& ln -s /usr/bin/objcopy /usr/bin/sw_64sw2-sunway-linux-gnu-objcopy
    fi
}

function pre_build()
{
    choose_platform_and_kernel
    prepare_cm
    prepare_snmp
    prepare_compile_env
}

function build()
{
    ./autogen.sh
    if [[ x$PLATFORM == x$CENTOS_PLATFORM_V3 ]] || [[ x$PLATFORM == x$CENTOS_PLATFORM_V4 ]]; then 
        ./configure --with-spl=$1
    else
        ./configure --with-spl=$1 --prefix=/usr --sbindir=/sbin
    fi
    make -j32 && make install
}

function post_build()
{
    if [ x$UNAME_M == xsw_64 ]; then
        cp -f /lib/modules/$(uname -r)/extra/mpt3sas/mpt3sas.ko /lib/modules/$(uname -r)/kernel/drivers/scsi/mpt3sas/mpt3sas.ko
    fi
}

pre_build
build
post_build
