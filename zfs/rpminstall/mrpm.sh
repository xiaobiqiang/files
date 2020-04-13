#!/bin/bash


SPL_DIR="../../spl"
ZFS_DIR=".."
RPM_DIR="rpm"
SCSI_DIR="../cmd/scsi_tools"
SYSTEM_DIR="../etc/systemd/system"
IS_DEEP=`uname -r|grep deepin|wc -l`


if [ ! -d $RPM_DIR ]; then
	mkdir  -p $RPM_DIR/zfs
	mkdir  -p $RPM_DIR/spl
	mkdir  -p scsi/lib     
fi


UNAME_FLAG=""
if [ `uname -a |grep x86_64|wc -l` -eq 0 ]; then
	UNAME_FLAG="sw_64"
else
	UNAME_FLAG="x86_64"
fi

if [ $IS_DEEP -eq 0 ]; then
    cp $SPL_DIR/*.$UNAME_FLAG.rpm $RPM_DIR/spl
    cp $ZFS_DIR/*.$UNAME_FLAG.rpm $RPM_DIR/zfs
else
    cd $ZFS_DIR
    if [ `ls |grep libcm|grep deb|wc -l` -eq 0 ]; then
        cm_rpm=`ls |grep libcm|grep rpm`
        alien $cm_rpm
    fi
    cd -
    cp $SPL_DIR/*.deb $RPM_DIR/spl
    cp $ZFS_DIR/*.deb $RPM_DIR/zfs

    mkdir ./system
    cp $SYSTEM_DIR/*service system
    cp $SYSTEM_DIR/50-zfs.preset system
    cp $SYSTEM_DIR/zfs.target system    
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
