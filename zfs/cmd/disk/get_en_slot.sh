#!/bin/bash 

function usage ()
{
    echo "usage: $0"
    exit 255
}

#echo $#
if [[ $# -gt 0 ]] ; then 
    usage
fi

cpu_arch=$(uname -m)

if [[ $cpu_arch == "sw_64" ]]; then
	sas3show | awk -F: '/Enclosure/{print $1, ":", substr($2,3,6)}; /Slot/{print $1, ":", $2}; /sas_address/{print "Addr :", substr($2,3,15)}'
	qemu-x86_64 /usr/gnemul/qemu-x86_64/bin/MegaCli64 -PDList -aAll | awk -F: '/Enclosure Device ID/{print "Enclosure :", substr($2,2)}; /Slot Number/{print "Slot :", substr($2,2)}; /WWN:/{sub(/^[ \t\r\n]+/,"",$2); print "Addr :", substr($2,1,15)}'
elif [[ $cpu_arch == "x86_64" ]];then
	sas3ircu 0 display | awk -F: '/Enclosure #/{print substr($1, 3, 9), ":", $2}; /\<Slot/{print substr($1, 3, 4), ":", $2}; /GUID/{print "Addr :", $2}'
fi
#qemu-x86_64 /usr/gnemul/qemu-x86_64/bin/MegaCli64 -PDList -aAll | awk -F: '/Slot Number/{print "Slot :", $2}'
#qemu-x86_64 /usr/gnemul/qemu-x86_64/bin/MegaCli64 -PDList -aAll | awk -F: '/WWN/{print "WWN :", $2;}'
#qemu-x86_64 /usr/gnemul/qemu-x86_64/bin/MegaCli64 -PDList -aAll | awk -F: '/Inquiry Data/{ serial_num=substr($2,1,8); print "Serial :", serial_num;}'
#qemu-x86_64 /usr/gnemul/qemu-x86_64/bin/MegaCli64 -PDList -aAll | awk -F: -v wwn=$dk_wwn '/Enclosure Device ID/{en=$2};/Slot Number/{slot=$2}; /WWN/{if($2 == " "wwn) print en,slot,$2;}'
