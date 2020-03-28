#!/bin/bash

RPM_DIR="./rpm"
SPL_RPM=$RPM_DIR"/spl"
ZFS_RPM=$RPM_DIR"/zfs"
GUI_DIR="/gui"

function install_gui()
{
    gui_tar=`ls ./ |grep G1`
    if [  "X"$gui_tar = "X" ]; then
        echo "no GUI"
        return
    fi

    rm -rf $GUI_DIR
    
    if [ ! -d $GUI_DIR ]; then
        mkdir -p $GUI_DIR
    fi
    
    cp $gui_tar $GUI_DIR
    cd $GUI_DIR
    tar -xvf $gui_tar
    ./prepare.sh
    cd -

    if [ ! -f /usr/local/lib/libcmjni.so ]; then
        if [ `uname -m` = "sw_64" ]; then
            ln -s /lib/libcmjni.so.0.0.0 /usr/local/lib/libcmjni.so 
        else
            ln -s /usr/lib64/libcmjni.so.0.0.0 /usr/local/lib/libcmjni.so
        fi
    fi
}

function install()
{
    if [  ! -f zfsonlinuxrpm.tar.gz ]; then
        echo "no rpm"
        return
    fi
    
    tar -xzvf zfsonlinuxrpm.tar.gz
    
    cd $SPL_RPM
    rpm -ivh ./*
    cd -
    
    cd $ZFS_RPM
    rpm -ivh ./* --nodeps --force
    cd -
    
    if [ `uname -m` = "sw_64" ]; then
        cp  /lib/modules/$(uname -r)/extra/zfs/mpt3sas/mpt3sas.ko  /lib/modules/$(uname -r)/kernel/drivers/scsi/mpt3sas/mpt3sas.ko
    fi
    install_gui
}


function unload()
{
    rpm -qa | grep 0.6.5.9 |xargs rpm -e
    
    rm -rf /usr/local/sbin/*
    rm -rf /usr/local/lib/*
    rm -rf /lib/modules/$(uname -r)/extra/*
    systemctl stop ceres_cm 2>/dev/null
    rm -rf /usr/local/bin/ceres*
    rm -rf /var/cm
    rm -rf /usr/lib/systemd/system/ceres_cm.service
}


$*