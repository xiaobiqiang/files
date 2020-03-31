#!/bin/bash


SPL_DIR="../../spl"
ZFS_DIR=".."
RPM_DIR="rpm"


if [ ! -d $RPM_DIR ]; then
	mkdir  -p $RPM_DIR/zfs
	mkdir  -p $RPM_DIR/spl
fi


UNAME_FLAG=""
if [ `uname -a |grep x86_64|wc -l` -eq 0 ]; then
	UNAME_FLAG="sw_64"
else
	UNAME_FLAG="x86_64"
fi

cp $SPL_DIR/*.$UNAME_FLAG.rpm $RPM_DIR/spl
cp $ZFS_DIR/*.$UNAME_FLAG.rpm $RPM_DIR/zfs


tar -zcvf zfsonlinuxrpm.tar.gz $RPM_DIR


rm -rf $RPM_DIR
