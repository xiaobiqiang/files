#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

function cm_cnm_route_getbatch()
{
	route -n|egrep -v 'Kernel|Destination'|awk '{print $1" "$2" "$3}'
	return $CM_OK
}

function cm_cnm_route_count()
{
	len=`route -n|egrep -v 'Kernel|Destination'|wc -l`
	echo $len
	return $CM_OK
}

function cm_cnm_route_create()
{
	local destination=$1
	local netmask=$2
	local geteway=$3
	num=`route -n|grep "$destination"|grep "$netmask"|grep "$geteway"|wc -l`
	if [ $num -ne 0 ]; then
		CM_LOG "[${FUNCNAME}:${LINENO}] already exsit"
		return $CM_ERR_ALREADY_EXISTS
	fi
	route add -net $destination netmask $netmask gw $geteway 2>/dev/null
	local iRet=$?
	if [ $iRet -ne $CM_OK ]; then
		CM_LOG "[${FUNCNAME}:${LINENO}] $destination $geteway $netmask fail"
	fi
	return $iRet
}	

function cm_cnm_route_delete()
{
	local destination=$1
	local netmask=$2
	local geteway=$3
	num=`route -n|grep "$destination"|grep "$netmask"|grep "$geteway"|wc -l`
	if [ $num -eq 0 ]; then
		CM_LOG "[${FUNCNAME}:${LINENO}] no exsit"
		return $CM_ERR_NOT_EXISTS
	fi
	route delete -net $destination netmask $netmask gw $geteway 2>/dev/null
	local iRet=$?
	if [ $iRet -ne $CM_OK ]; then
		CM_LOG "[${FUNCNAME}:${LINENO}] $destination $geteway $netmask fail"
	fi
	return $iRet
}

function main()
{
	local paramnum=$#
	local iRet=$CM_OK
	case $1 in
		getbatch)
		if [ $paramnum -ne 1 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_route_getbatch
		iRet=$?
		;;
		count)
		if [ $paramnum -ne 1 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_route_count
		iRet=$?
		;;
		insert)
		if [ $paramnum -ne 4 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_route_create $2 $3 $4
		iRet=$?
		;;
		delete)
		if [ $paramnum -ne 4 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_route_delete $2 $3 $4
		iRet=$?
		;;
		*)
		;;
	esac
	return $iRet
}

main $*
exit $?