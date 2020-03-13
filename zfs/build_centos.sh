#!/bin/bash

topdir=`pwd`
cd cmd/cm/
chmod 755 ./cpcmfile.sh
./cpcmfile.sh

cd build/
chmod 755 ./makeam.sh
./makeam.sh
cd $topdir

SNMP_DIR="cmd/fm/modules/common/snmp-trapgen"
SNMP_HEADERS="/usr/include/net-snmp"

if [ ! -d $SNMP_HEADERS ];then
    cd $SNMP_DIR
    tar -xvf ./net-snmp.tar
    cp -rf ./net-snmp/ /usr/include
    rm -rf ./net-snmp/
fi


location_check=`ls -l /usr/lib|grep libnetsnmp|wc -l`
if [ $location_check -eq 0 ];then
    location="/usr/lib64"
else
    location="/usr/lib"
fi

if [ ! -f "$location/libnetsnmp.so" ];then
    libnetsnmp_ver=`ls -l $location|grep -w "libnetsnmp.so"|grep -v ">"|awk '{print $9}'`
    ln -s $location/$libnetsnmp_ver $location/libnetsnmp.so
fi

if [ ! -f "$location/libnetsnmpagent.so" ];then
    libnetsnmpagent_ver=`ls -l $location|grep -w "libnetsnmpagent.so"|grep -v ">"|awk '{print $9}'`
    ln -s $location/$libnetsnmpagent_ver $location/libnetsnmpagent.so
fi



cp module/Makefile_centos.in module/Makefile.in
./autogen.sh
./configure --with-spl=$1
make && make install
