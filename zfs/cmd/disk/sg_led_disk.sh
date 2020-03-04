#!/bin/bash

get_enc()
{
	local sas_addr=$1
	local ret=0
	local encs=`sg_map -i|grep -v sd|grep -v Marvell|awk -F' ' '{print $1}'`
    	for enc in $encs
    	do
		ret=$(sg_ses -p 0xa $enc|grep $sas_addr|wc -l)
		if [ x$ret != x"0" ];then
			echo $enc
			return
		fi
    	done
}

main()
{
	local paranum=$#
    	if [ $paranum -ne 2 ];then
		return -1;
    	fi
	local sd_path=$1
	local ops=$2
	local sd_name=`ls -l $sd_path|grep -v part|awk -F' ' '{print substr($(11),7)}'`
	local sg_name=`sg_map -sd |grep "/$sd_name"|awk -F' ' '{print $1}'`
	local sas_addr=`sg_inq -i $sg_name|awk '/\[0x/ {print substr($1,4,16)}'|sed -n '2p'`
	local enc=$(get_enc $sas_addr)
	local slot=`sg_ses --sas-addr=0x$sas_addr $enc|awk -F: '/device slot number:/ {print substr($4,2)}'`
	
	if [ "$ops"x == "fault"x ];then
		sg_ses --dev-slot-num=$slot --set=ident  $enc
	elif [ "$ops"x == "active"x ];then
		sg_ses --dev-slot-num=$slot --clear=ident  $enc
	elif [ "$ops"x == "force_active"x ];then
		sg_ses --dev-slot-num=$slot --clear=ident  $enc
		sg_ses --dev-slot-num=$slot --clear=locate  $enc
		sg_ses --dev-slot-num=$slot --clear=fault  $enc
		sg_ses --dev-slot-num=$slot --clear=missing  $enc
	fi
}

main $1 $2
exit $?
