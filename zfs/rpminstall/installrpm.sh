#!/bin/bash

RPM_DIR="rpm"
SPL_RPM=$RPM_DIR"/spl"
ZFS_RPM=$RPM_DIR"/zfs"
GUI_DIR="/gui"
SYSTEM_DIR="../etc/systemd/system"
RELY_DEB="deepinrely.tar.gz"
VERSION_FILE="release"
UNAME_R=`uname -r`
MODULE_DIR="/lib/modules/${UNAME_R}"
INITRAMFS=initramfs-${UNAME_R}.img
CENTOS_V3_BOOT_IMG="/boot/${INITRAMFS}"

OS_RELEASE_DEEPIN="deepin"	#deepin
OS_RELEASE_CENTOS="centos"	#centos
OS_RELEASE_NEOKYLIN="neokylin"	#neokylin
OS_RELEASE_KYLIN="kylin"	#kylin
OS_RELEASE_ALL=($OS_RELEASE_DEEPIN $OS_RELEASE_CENTOS $OS_RELEASE_NEOKYLIN $OS_RELEASE_KYLIN)
OS_RELEASE=`cat /etc/os-release | grep -w ID | sed 's/"//g' | cut -d '=' -f 2`

KERNEL_VERSION_V3="3"
KERNEL_VERSION_V4="4"
KERNEL_VERSION=`uname -r | cut -d '-' -f 1 | cut -d '.' -f 1`

function is_supported_system()
{
	local is_supported_os=0
	
	echo "Now Operating System Platform is ${OS_RELEASE}..."
	
	for pf in `echo ${OS_RELEASE_ALL[*]}`; do
		[[ $OS_RELEASE == $pf ]] && is_supported_os=1
	done
	
	[ $is_supported_os -eq 0 ] && echo "Unsupported System Platform --> ${OS_RELEASE}!"
	[ $is_supported_os -eq 0 ] && exit 1
}

function install_gui()
{
    local gui_tar=`ls ./ | grep G1`
	local cmjni_dir=
	
	[ -z $gui_tar ] && echo "WARNNING: Can Not Find GUI tarball..."
	[ -z $gui_tar ] && return 1
	
    [ -d $GUI_DIR ] && rm -rf $GUI_DIR
	mkdir -p $GUI_DIR
    
    cp $gui_tar ${GUI_DIR}/
	
    cd $GUI_DIR
    tar -xvf $gui_tar
    ./prepare.sh
    cd -

    if [ ! -f /usr/local/lib/libcmjni.so ]; then
		[[ $OS_RELEASE == $OS_RELEASE_DEEPIN || $OS_RELEASE == $OS_RELEASE_NEOKYLIN ]] && cmjni_dir=/lib
		[[ $OS_RELEASE == $OS_RELEASE_KYLIN ]] && cmjni_dir=/usr/lib
		[[ $OS_RELEASE == $OS_RELEASE_CENTOS ]] && cmjni_dir=/lib64
		
		[ ! -z $cmjni_dir ] && ln -s ${cmjni_dir}/libcmjni.so.0.0.0 /usr/local/lib/libcmjni.so
    fi
    chmod 755 /etc/rc.d/rc.local
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

function install_scsi_tools()
{
    tar -xzvf scsi_tool.tar.gz
    cp -f scsi/sg_* /usr/local/bin
    cp -f scsi/lsscsi /usr/local/bin
	
	[[ $OS_RELEASE == $OS_RELEASE_CENTOS ]] && cp -f scsi/lib/* /lib64/
	[[ $OS_RELEASE == $OS_RELEASE_DEEPIN || $OS_RELEASE == $OS_RELEASE_NEOKYLIN \
		|| $OS_RELEASE == $OS_RELEASE_KYLIN ]] && cp -f scsi/lib/* /lib/
}

function install_java()
{
    local jdk_file=$1
    
	[ -z $jdk_file ] && return 1
    
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
    local rely_nmae=`echo $RELY_DEB | sed 's/.tar.gz//g'`
	
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

    mkdir -p ${MODULE_DIR}/kernel/net/netlink
    cp $rely_nmae/cn.ko ${MODULE_DIR}/kernel/net/netlink
    depmod -a
    
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
    rpm -ivh *rpm --nodeps --force
    cd -
    
    install_java $rely_rpm/jdk8.tar
    
    rm -rf centosrely
}

function install_kylin_rely()
{	
	local kylin_rely="kylinrely.tar.gz"
	
	[ ! -f $kylin_rely ] && echo "FATAL:Can Not Find ${kylin_rely}, Aborting..."
	[ ! -f $kylin_rely ] && return 1
	
	tar -xvf $kylin_rely

	cd kylinrely
	
	cp -f gelf.h /usr/include/
	cp -f libelf.h /usr/include/
	dpkg --force-overwrite -i ./*.deb
	
	cd -
	
	rm -rf kylinrely
}

function install_zfsonlinux_version()
{
    cp -f $VERSION_FILE /etc/
}

# install_mptsas2 only for centos-3.10.0
function install_mpt2sas()
{
	local img_attr=
	
	[ -f mpt2sas.ko.gz ] && gzip -d mpt2sas.ko.gz
	[ ! -f mpt2sas.ko ] && echo "WARNNING: Can Not Find mpt2sas.ko..."
	[ ! -f mpt2sas.ko ] && return 1
	
	cp -f mpt2sas.ko ${MODULE_DIR}/kernel/drivers/scsi/mpt3sas/
	
	[ -z $CENTOS_V3_BOOT_IMG ] && echo "WARNNING: Can Not Find Boot Image..."
	[ -z $CENTOS_V3_BOOT_IMG ] && return 1
	
	img_attr=`file ${CENTOS_V3_BOOT_IMG} | awk '{print $3}'`
	[[ $img_attr == cpio ]] && return 0
	
	#gzip initramfs
    cp -f $CENTOS_V3_BOOT_IMG  ${CENTOS_V3_BOOT_IMG}_bak
	
    mkdir centos_v3_initramfs
    cp  -f $CENTOS_V3_BOOT_IMG centos_v3_initramfs/
	
    cd centos_v3_initramfs
	
    gzip -dc $INITRAMFS | cpio -ivd
    rm -rf $INITRAMFS

    cp -f ../mpt2sas.ko lib/modules/${UNAME_R}/kernel/drivers/scsi/mpt3sas/
    find . 2>/dev/null | cpio -c -o | gzip > ../$INITRAMFS
    cp -f ../$INITRAMFS $CENTOS_V3_BOOT_IMG
    cd -
	
	rm -rf $INITRAMFS
}

# install_mptsas2 only for jiagu
function install_drbd()
{
    local drbdrpm=`ls | grep drbd`
	local drbddir=`echo $drbdrpm | sed 's/.tar.gz//g'`
	
    [ -z $drbdrpm ] && echo "WARNNING: Can Not Find DRBD RPM..."
	[ -z $drbdrpm ] && return 1
	
	tar -zxvf $drbdrpm
	
    rpm -ivh ${drbddir}/drbd-rpm/*
    rpm -ivh ${drbddir}/drbd-utils-rpm/*
	
	rm -rf $drbddir
}

function install_vmlinux1()
{
    [ ! -f vmlinux1 ] && return 1
	
    cp vmlinux1 /boot/vmlinuz-${UNAME_R}
    mv ${MODULE_DIR}/kernel/net/netlink/cn.ko ${MODULE_DIR}/kernel/net/netlink/cn.ko.bak
	
    cd ${MODULE_DIR}
	
    rm modules.dep
    depmod -a
	
	cd -
}

function install_some_other_dependency()
{
	/usr/local/sbin/rpm_install.sh
}

function install_copy_mpt3sas()
{
	cp -f ${MODULE_DIR}/extra/zfs/mpt3sas/mpt3sas.ko ${MODULE_DIR}/kernel/drivers/scsi/mpt3sas/mpt3sas.ko
}

function install_ceres_cm()
{
	/usr/cm/script/cm_rpm.sh
}

function install_systemd_service()
{
	mkdir -p /lib/systemd/system-preset/
	cp system/*service /lib/systemd/system
	cp system/*target /lib/systemd/system
	cp system/50-zfs.preset /lib/systemd/system-preset/
}

function install_lib_zpool_so()
{
	[ ! -d /lib64 ] && return 1
	
	mv /lib64/* /usr/lib/
	rm -rf /lib64
}

function install_centos_kernel_version_v3()
{
	install_mptsas2
	install_drbd
	install_type_rpm
}

function install_centos_kernel_version_v4()
{
	install_type_rpm
	install_copy_mpt3sas
}

function install_unique_platform_kylin()
{
	install_type_deb
	install_lib_zpool_so
	install_systemd_service
	install_ceres_cm
	install_copy_mpt3sas
	install_some_other_dependency
}

function install_unique_platform_deepin()
{
	install_type_deb
	install_systemd_service
	install_ceres_cm
	install_copy_mpt3sas
	install_vmlinux1
	install_some_other_dependency
	
	systemctl disable network-manager.service
}

function install_unique_platform_neokylin()
{
	install_type_rpm
	install_copy_mpt3sas
}

function install_unique_platform_centos()
{
	[[ ${KERNEL_VERSION} == ${KERNEL_VERSION_V3} ]] && install_centos_kernel_version_v3
	[[ ${KERNEL_VERSION} == ${KERNEL_VERSION_V4} ]] && install_centos_kernel_version_v4
}	

function install_post()
{
	cd $MODULE_DIR
	depmod -a
	cd -
}

function install()
{
	is_supported_system

	tar -xzvf zfsonlinuxrpm.tar.gz
	tar -xzvf scsi_tool.tar.gz
	
	install_unique_platform_${OS_RELEASE}
        
    install_gui
    install_scsi_tools
    install_zfsonlinux_version
	
	install_post
    
    rm -rf rpm
    rm -rf scsi
}


function unload()
{
	rm -rf /var/fm/*
	rm -rf /usr/local/lib/*
	[[ $OS_RELEASE == $OS_RELEASE_CENTOS || $OS_RELEASE == $OS_RELEASE_NEOKYLIN ]] \
		&& rpm -qa | grep 0.6.5.9 | xargs rpm -e
	[[ $OS_RELEASE == $OS_RELEASE_DEEPIN || $OS_RELEASE == $OS_RELEASE_KYLIN ]] \
		&& dpkg -l | grep 0.6.5.9 | awk '{print $2}' | xargs dpkg -r
    
    rm -rf /usr/local/sbin/*
    rm -rf /lib/modules/${UNAME_R}/extra/*
    systemctl stop ceres_cm 2>/dev/null
    rm -rf /usr/local/bin/ceres*
    rm -rf /var/cm
    rm -rf /lib/systemd/system/ceres_cm.service
}

function install_rely()
{
	install_${OS_RELEASE}_rely
}

function help()
{   
    echo "usage:"
    echo ""
    echo "  unload:           ./installrpm.sh unload"
    echo "  install:          ./installrpm.sh install"
    echo "  install rely:     ./installrpm.sh install_rely"
}


$*
