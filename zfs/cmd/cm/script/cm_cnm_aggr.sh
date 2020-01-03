#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'


#name
#ip
#netmask
#mtu
#mac
#state
#POLICY
#LACPACTIVITY
#LACPTIMER
#adapter

function to_ip()
{
	num=$1
	ip1=`echo ${num:0:2}`
	((ip1=16#$ip1))
	ip2=`echo ${num:2:2}`
	((ip2=16#$ip2))
	ip3=`echo ${num:4:2}`
	((ip3=16#$ip3))
	ip4=`echo ${num:6:2}`
	((ip4=16#$ip4))
	echo $ip1.$ip2.$ip3.$ip4
}

function cm_cnm_aggr_getbatch()
{
	local iRet=$CM_OK
	local rows=`dladm show-aggr|awk 'NR>1{print $1":"$2":"$4":"$5}'`
	for row in $rows
	do
		local name=`echo $row|awk -F':' '{printf $1}'`
		local POLICY=`echo $row|awk -F':' '{printf $2}'`
		local LACPACTIVITY=`echo $row|awk -F':' '{printf $3}'`
		local LACPTIMER=`echo $row|awk -F':' '{printf $4}'`
		local state=`dladm show-link|grep $name|awk '{printf $4}'` 
		#local ip=`ifconfig $name|grep inet|awk '{print $2}'`
		
		local netmask=`ifconfig $name|grep inet|awk '{print $4}'`
		netmask=$(to_ip $netmask)
		
		local mtu=`dladm show-link|grep $name|awk '{printf $3}'`
		#local mac=`ifconfig $name|grep ether|awk '{print $2}'`
		if [ `ifconfig $name 2>/dev/null|wc -l` -eq 0 ]; then
			local mac='null'
			local ip='null'
		else
			local ip=`ifconfig $name|grep inet|awk '{print $2}'`
			local mac=`ifconfig $name|grep ether|awk '{print $2}'`
		fi
 		echo $name $state $ip $netmask $mtu $mac $POLICY $LACPACTIVITY $LACPTIMER
	done
	return $iRet
}

function cm_cnm_aggr_getnetmask()
{
	local name=$1
	local netmask=`ifconfig $name|grep inet|awk '{print $4}'`
	netmask=$(to_ip $netmask)
	
	echo $netmask
}

function cm_cnm_aggr_count()
{
	local iRet=$CM_OK
	local cut=`dladm show-link|grep aggr|wc -l`
	echo $cut
	return $iRet
}

function cm_cnm_aggr_getkey()
{
	local name=$1
	local cut=`echo ${name:4:1}`
	echo $cut
}


function cm_cnm_aggr_update()
{
	local ip=$1
	local netmask=$2
	local name=$3
	
	if [ $ip = 'null' ] && [ $netmask = 'null' ]; then
		return
	fi
	
	if [ $netmask = 'null' ]; then
		netmask=$(cm_cnm_aggr_getnetmask $name)
		ifconfig $name unplumb
		ifconfig $name plumb $ip netmask $netmask up
		echo $ip > /etc/hostname.$name
		echo "netmask $netmask" >> /etc/hostname.$name
		echo up >> /etc/hostname.$name
		return
	fi
	
	if [ $ip = 'null' ]; then
		ip=`ifconfig $name| grep inet| awk '{print $2}'`
		ifconfig $name netmask $netmask up
		echo $ip > /etc/hostname.$name
		echo "netmask $netmask" >> /etc/hostname.$name
		echo up >> /etc/hostname.$name
		return 
	fi
	
	ifconfig $name plumb $ip netmask $netmask up
	echo $ip > /etc/hostname.$name
	echo "netmask $netmask" >> /etc/hostname.$name
	echo up >> /etc/hostname.$name
}

function main()
{
	local iRet=$CM_OK
	local paramnum=$#

	case $1 in
		getbatch)
		if [ $paramnum -ne 1 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_aggr_getbatch
		iRet=$?
		;;
		count)
		if [ $paramnum -ne 1 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_aggr_count
		iRet=$?
		;;
		getkey)
		cm_cnm_aggr_getkey $2
		iRet=$?
		;;
		getnetmask)
		cm_cnm_aggr_getnetmask $2
		iRet=$?
		;;
		update)
		cm_cnm_aggr_update $2 $3 $4
		iRet=$?
		;;
		*)
		;;
	esac
	return $iRet
}

main $*
exit $?