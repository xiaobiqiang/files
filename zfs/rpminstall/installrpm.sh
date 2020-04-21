#!/bin/bash

RPM_DIR="./rpm"
SPL_RPM=$RPM_DIR"/spl"
ZFS_RPM=$RPM_DIR"/zfs"
GUI_DIR="/gui"
SYSTEM_DIR="../etc/systemd/system"
RELY_DEB="deepinrely.tar.gz"
VERSION_FILE="release"
IS_DEEP=`uname -r|grep deepin|wc -l`

function install_gui()
{
    local gui_tar=`ls ./ |grep G1`
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

function install_type_rpm()
{
    cd $SPL_RPM
    rpm -ivh ./*
    cd -
    
    cd $ZFS_RPM
    rpm -ivh ./* --nodeps --force
    cd -
}

function install_type_deb()
{
    cd $SPL_RPM
    dpkg -i ./*
    cd -
    
    cd $ZFS_RPM
    dpkg --force-overwrite -i ./*
    cd -
}

function install_scsi()
{
    tar -xzvf scsi_tool.tar.gz
    cp scsi/sg_* /usr/local/bin
    cp scsi/lsscsi /usr/local/bin
    if [ `uname -m` = "sw_64" ]; then
        cp scsi/lib/* /lib
    else
        cp scsi/lib/* /lib64
    fi
}

function install_java()
{
    local jdk_file=$1
    
    if [ "X"$jdk_file = "X" ]; then
        return
    fi
    
    mkdir -p /usr/lib/jvm/
    if [ `ls /usr/lib/jvm/|grep jdk8|wc -l 2>/dev/null` -eq 0 ]; then
        cp $jdk_file /usr/lib/jvm/
        cd /usr/lib/jvm/
        tar -xvf jdk8.tar
        rm jdk8.tar
        cd - 
    fi
    
    if [ `cat /etc/profile|grep JAVA_HOME|wc -l` -eq 0 ]; then
        echo 'export JAVA_HOME=/usr/lib/jvm/jdk8' >> /etc/profile
        echo 'export PATH=$JAVA_HOME/bin:$PATH' >> /etc/profile
        echo 'export CLASSPATH=.:$JAVA_HONE/lib/dt.jar:$JAVA_HOME/lib/tools.jar' >> /etc/profile
    fi
}

function install_deepin_rely()
{
    local rely_nmae=`echo $RELY_DEB|sed 's/.tar.gz//g'`
    tar -xzvf $RELY_DEB
    
    cd $rely_nmae/deb-pkg
    dpkg -i ./*
    cd -
    
    install_java $rely_nmae/jdk8.tar

    ln -s /usr/lib/libxml2.so.2 /usr/lib/libxml2.so
    ln -s /usr/lib/libreadline.so.6.3 /usr/lib/libreadline.so.6 
    ln -s /usr/lib/libreadline.so.6 /usr/lib/libreadline.so
    ln -s /usr/lib/sw_64-linux-gnu/libnetsnmpagent.so.30.0.3 /usr/lib/sw_64-linux-gnu/libnetsnmpagent.so    
    ln -s /usr/lib/sw_64-linux-gnu/libnetsnmp.so.30.0.3 /usr/lib/sw_64-linux-gnu/libnetsnmp.so 

    mkdir -p /lib/modules/4.4.15-deepin-wutip/kernel/net/netlink
    cp $rely_nmae/cn.ko /lib/modules/4.4.15-deepin-wutip/kernel/net/netlink
    depmod
    
    rm -rf $rely_nmae
}

function install_centos_rely()
{
    local rely_rpm="centosrely.tar.gz"
    
    tar -xzvf $rely_rpm
    if [ `file /lib64/libsgutils2.so.2|grep symbolic|wc -l` -eq 0 ]; then
        rm /lib64/libsgutils2.so.2
        ln -s /lib64/libsgutils2.so.2.0.0 /lib64/libsgutils2.so.2
    fi
    
    cd centosrely
    rpm -ivh ./* --nodeps --force
    cd -
    
    install_java $rely_rpm/jdk8.tar
    
    rm -rf centosrely
}

function install_version()
{
    cp $VERSION_FILE /etc
}

function install()
{
    if [  ! -f zfsonlinuxrpm.tar.gz ]; then
        echo "no rpm"
        return
    fi
    
    tar -xzvf zfsonlinuxrpm.tar.gz
        
    if [ $IS_DEEP -eq 0 ]; then
        install_type_rpm
    else
        #if [ -f $RELY_DEB ]; then
        #    install_deepin_rely
        #fi

        install_type_deb
        
        mkdir -p /lib/systemd/system-preset/
        cp system/*service /lib/systemd/system
        cp system/*target /lib/systemd/system
        cp system/50-zfs.preset /lib/systemd/system-preset/

        /usr/cm/script/cm_rpm.sh
        /usr/local/sbin/rpm_install.sh
        systemctl disable network-manager.service
    fi
    
    if [ `uname -m` = "sw_64" ]; then
        cp  /lib/modules/$(uname -r)/extra/zfs/mpt3sas/mpt3sas.ko  /lib/modules/$(uname -r)/kernel/drivers/scsi/mpt3sas/mpt3sas.ko
    fi
        
    install_gui
    install_scsi
    install_version
    
    rm -rf rpm
    rm -rf scsi
}


function unload()
{

    if [ $IS_DEEP -eq 0 ]; then
        rpm -qa | grep 0.6.5.9 |xargs rpm -e
    else
        dpkg -l |grep 0.6.5.9|awk '{print $2}'|xargs dpkg -r
    fi
    
    rm -rf /usr/local/sbin/*
    rm -rf /usr/local/lib/*
    rm -rf /lib/modules/$(uname -r)/extra/*
    systemctl stop ceres_cm 2>/dev/null
    rm -rf /usr/local/bin/ceres*
    rm -rf /var/cm
    rm -rf /lib/systemd/system/ceres_cm.service
}


$*
