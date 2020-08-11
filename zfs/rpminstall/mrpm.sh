#!/bin/bash

SPL_DIR="../../spl"
ZFS_DIR=".."
RPM_DIR="rpm"
SCSI_DIR="../cmd/scsi_tools"
SYSTEM_DIR="../etc/systemd/system"
HAS_DPKG=`which dpkg | wc -l`

OS_RELEASE_DEEPIN="deepin"	#deepin
OS_RELEASE_CENTOS="centos"	#centos
OS_RELEASE_NEOKYLIN="neokylin"	#neokylin
OS_RELEASE_KYLIN="kylin"	#kylin
OS_RELEASE=`cat /etc/os-release | grep -w ID | sed 's/"//g' | cut -d '=' -f 2`

[ -d $RPM_DIR ] && rm -rf $RPM_DIR
mkdir -p $RPM_DIR/zfs
mkdir -p $RPM_DIR/spl
mkdir -p scsi/lib

UNAME_FLAG=
[ $HAS_DPKG -eq 1 ] && UNAME_FLAG=`dpkg --print-architecture`
[ $HAS_DPKG -eq 0 ] && UNAME_FLAG=`lscpu | awk 'NR==1 {print $2}'`
[ -z $UNAME_FLAG ] && echo "Can not get system architecture..."
[ -z $UNAME_FLAG ] && exit 1

if [[ $OS_RELEASE == $OS_RELEASE_DEEPIN ]] || [[ $OS_RELEASE == $OS_RELEASE_KYLIN ]]; then
	cd $ZFS_DIR

	cm_rpm=`ls | grep libcm*.rpm`
	cm_deb=`ls | grep libcm*.deb`
	[ ! -z $cm_deb ] && rm -f $cm_deb
	[ -z $cm_rpm ] && echo 'None libcm rpm package...'
	[ -z $cm_rpm ] && exit 2

	alien --target=$UNAME_FLAG $cm_rpm

	cd -
	cp $SPL_DIR/*.deb $RPM_DIR/spl
	cp $ZFS_DIR/*.deb $RPM_DIR/zfs

	mkdir ./system
	cp $SYSTEM_DIR/*service system
	cp $SYSTEM_DIR/50-zfs.preset system
	cp $SYSTEM_DIR/zfs.target system    
elif [[ $OS_RELEASE == $OS_RELEASE_CENTOS ]] || [[ $OS_RELEASE == $OS_RELEASE_NEOKYLIN ]]; then
	cp $SPL_DIR/*.$UNAME_FLAG.rpm $RPM_DIR/spl
	cp $ZFS_DIR/*.$UNAME_FLAG.rpm $RPM_DIR/zfs
else
	echo "Unsupported System --> $OS_RELEASE"
	exit 2
fi

cd $SCSI_DIR
chmod 755 Makecurrent.sh
./Makecurrent.sh make 
./Makecurrent.sh install
cd -

cp /usr/local/bin/sg_* scsi
cp /usr/local/bin/lsscsi scsi
cp /usr/local/lib/libsgutils2* scsi/lib

tar -zcvf zfsonlinuxrpm.tar.gz $RPM_DIR
tar -zcvf scsi_tool.tar.gz scsi
git branch|grep '*'|awk '{print $2}' > release

rm -rf $RPM_DIR
rm -rf scsi
