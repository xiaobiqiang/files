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

LSSCSI=/tmp/.lsscsi.txt
SASDISK=/tmp/.sasdisk

lsscsi -git > $LSSCSI 2>/dev/null
[  -f $LSSCSI ]&&enclosus=$(awk -F' ' '/enclosu/{print $6};' $LSSCSI)
index=22

for enc in $enclosus
do
        sg_ses -j $enc|
        grep 'device slot number' -A6|
        grep  -v attached|
        awk -F: '/device slot number/{printf "%d %s ",'"$index"',$4};/SAS address/{printf "%s \n",$2};'|
        egrep  0x[0-9a-zA-Z]{8}|
		grep -v 0x00000000
        index=$(($index+1))
done > $SASDISK

[  -f $SASDISK ]&&while read line
    do
        diskinfos=($line)
        addr=$(grep ${diskinfos[2]} $LSSCSI|awk -F' ' '{print $5};')

        echo "Enclosure : ${diskinfos[0]}"
        echo "Slot : ${diskinfos[1]}"
        echo "Addr : ${addr:1:15}"
    done < $SASDISK
