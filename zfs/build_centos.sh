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

netsnmp_so_loc=/usr/lib64
netsnmp_so="libnetsnmp.so libnetsnmpagent.so"
for so in ${netsnmp_so}; do
	[ ! -f ${netsnmp_so_loc}/${so} ] && ln -s ${netsnmp_so_loc}/`ls ${netsnmp_so_loc}/ | grep ${so}.[0-9][0-9].[0-9].` ${netsnmp_so_loc}/${so}
done

cp module/Makefile_centos.in module/Makefile.in
./autogen.sh
./configure --with-spl=$1 --prefix=
make && make install
