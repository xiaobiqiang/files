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

netsnmp_so_loc=
[ "`cat /etc/os-release | grep PRETTY_NAME | cut -d '"' -f 2`" == "deepin GNU/Linux 15.0 (kui)" ] && netsnmp_so_loc=/usr/lib/sw_64-linux-gnu
[ "`cat /etc/os-release | grep PRETTY_NAME | cut -d '"' -f 2`" == "NeoKylin 5 (Five)" ] && netsnmp_so_loc=/usr/lib
netsnmp_so="libnetsnmp.so libnetsnmpagent.so"
for so in ${netsnmp_so}; do
	[ ! -f ${netsnmp_so_loc}/${so} ] && ln -s ${netsnmp_so_loc}/`ls ${netsnmp_so_loc}/ | grep ${so}.[0-9][0-9].[0-9].` ${netsnmp_so_loc}/${so}
done

if [ "`cat /etc/os-release | grep PRETTY_NAME | cut -d '"' -f 2`" == "deepin GNU/Linux 15.0 (kui)" ]; then 
	[ ! -f /usr/bin/sw_64sw6-sunway-linux-gnu-gcc ] && ln -s /usr/bin/gcc /usr/bin/sw_64sw6-sunway-linux-gnu-gcc
	[ ! -f /usr/bin/sw_64sw6-sunway-linux-gnu-ld ] && ln -s /usr/bin/ld /usr/bin/sw_64sw6-sunway-linux-gnu-ld
	[ ! -f /usr/bin/sw_64sw6-unknown-linux-gnu-ar ]&& ln -s /usr/bin/ar /usr/bin/sw_64sw6-unknown-linux-gnu-ar
	[ ! -f /usr/bin/sw_64sw6-unknown-linux-gnu-nm ]&& ln -s /usr/bin/nm /usr/bin/sw_64sw6-unknown-linux-gnu-nm
	[ ! -f /usr/bin/sw_64sw6-unknown-linux-gnu-strip ]&& ln -s /usr/bin/strip /usr/bin/sw_64sw6-unknown-linux-gnu-strip
	[ ! -f /usr/bin/sw_64sw6-unknown-linux-gnu-objdump ]&& ln -s /usr/bin/objdump /usr/bin/sw_64sw6-unknown-linux-gnu-objdump
	[ ! -f /usr/bin/sw_64sw6-unknown-linux-gnu-objcopy ]&& ln -s /usr/bin/objcopy /usr/bin/sw_64sw6-unknown-linux-gnu-objcopy
	[ ! -f /usr/bin/sw_64sw2-unknown-linux-gnu-gcc ]&& ln -s /usr/bin/gcc /usr/bin/sw_64sw2-unknown-linux-gnu-gcc
	[ ! -f /usr/bin/sw_64sw2-unknown-linux-gnu-ld ]&& ln -s /usr/bin/ld /usr/bin/sw_64sw2-unknown-linux-gnu-ld
	[ ! -f /usr/bin/sw_64sw2-unknown-linux-gnu-ar ]&& ln -s /usr/bin/ar /usr/bin/sw_64sw2-unknown-linux-gnu-ar
	[ ! -f /usr/bin/sw_64sw2-unknown-linux-gnu-nm ]&& ln -s /usr/bin/nm /usr/bin/sw_64sw2-unknown-linux-gnu-nm
	[ ! -f /usr/bin/sw_64sw2-unknown-linux-gnu-strip ]&& ln -s /usr/bin/strip /usr/bin/sw_64sw2-unknown-linux-gnu-strip
	[ ! -f /usr/bin/sw_64sw2-unknown-linux-gnu-objdump ]&& ln -s /usr/bin/objdump /usr/bin/sw_64sw2-unknown-linux-gnu-objdump
	[ ! -f /usr/bin/sw_64sw2-unknown-linux-gnu-objcopy ]&& ln -s /usr/bin/objcopy /usr/bin/sw_64sw2-unknown-linux-gnu-objcopy
fi

cp module/Makefile_hengwei.in module/Makefile.in
./autogen.sh
./configure --enable-hengwei=yes --with-spl=$1 --prefix=/usr --sbindir=/sbin
make -j 16 && make install

cp -f /lib/modules/$(uname -r)/extra/mpt3sas/mpt3sas.ko /lib/modules/$(uname -r)/kernel/drivers/scsi/mpt3sas/mpt3sas.ko

if [[ $? == 0 ]]; then
	echo "install mpt3sas.ko success"
else 
	echo "install mpt3sas.ko failed"
fi

