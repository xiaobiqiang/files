#!/bin/bash

UNAME_M=
UNAME_R=
PLATFORM=
CENTOS_3_10="centos-3.10"
CENTOS_4_4_15="centos-4.4.15"
DEEPIN="deepin"
ZB_KYLIN="zb-kylin"
YH_KYLIN="yh-kylin"

function choose_platform()
{
    local temp

    UNAME_M=`uname -m`
    UNAME_R=`uname -r`
    if [ x$UNAME_M == xx86_64 ]; then
        temp=`echo $UNAME_R | grep 3.10`
	if [[ -n $temp ]]; then
            PLATFORM=$CENTOS_3_10
        else
            PLATFORM=$CENTOS_4_4_15
        fi
    elif [ x$UNAME_M == xsw_64 ]; then
        temp=`echo $UNAME_R | grep deepin`
        if [[ -n $temp ]]; then
            PLATFORM=$DEEPIN
        else
            PLATFORM=$ZB_KYLIN
        fi
    elif [ x$UNAME_M == xaarch64 ]; then
        PLATFORM=$YH_KYLIN
    else
        PLATFORM=$DEEPIN
    fi 
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
    SNMP_DIR="cmd/fm/modules/common/snmp-trapgen"
    SNMP_HEADERS="/usr/include/net-snmp"

    if [ ! -d $SNMP_HEADERS ];then
        cd $SNMP_DIR
        tar -xvf ./net-snmp.tar
        cp -rf ./net-snmp/ /usr/include
        rm -rf ./net-snmp/
    fi

    if [ x$UNAME_M = xx86_64 ]; then
        netsnmp_so_loc=/usr/lib64
    elif [ x$PLATFORM = x$DEEPIN ]; then
        netsnmp_so_loc=/usr/lib/sw_64-linux-gnu
    elif [ x$PLATFORM = x$ZB_KYLIN ] || [ x$PLATFORM = x$YH_KYLIN ]; then
        netsnmp_so_loc=/usr/lib
    else
        netsnmp_so_loc=/usr/lib/sw_64-linux-gnu
    fi

    netsnmp_so="libnetsnmp.so libnetsnmpagent.so"
    for so in ${netsnmp_so}; do
        [ ! -f ${netsnmp_so_loc}/${so} ] && ln -s ${netsnmp_so_loc}/`ls ${netsnmp_so_loc}/ | grep ${so}.[0-9][0-9].[0-9].` ${netsnmp_so_loc}/${so}
    done
    cd $topdir
}

function prepare_compile_env()
{
    if [ x$PLATFORM == x$DEEPIN ]; then
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
    choose_platform
    prepare_cm
    prepare_snmp
    prepare_compile_env
}

function build()
{
    ./autogen.sh
    if [ x$UNAME_M = xx86_64 ]; then
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
