#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

function cm_cnm_route_getbatch()
{
	local rows=`netstat -nrv|grep '\.'|egrep -v 'lo0'|awk '{print $1":"$2":"$3}'`
	
	for row in $rows
	do
		echo $row | awk -F':' '{print $1" "$2" "$3}'
	done
	return $CM_OK
}

function cm_cnm_route_count()
{
	len=`netstat -rnv|grep '\.'|egrep -v 'lo0'|wc -l`
	echo $len
	return $CM_OK
}

function cm_cnm_route_create()
{
	local destination=$1
	local netmask=$2
	local geteway=$3
	num=`netstat -rv|grep "\."|egrep -v 'default|localhost'|grep "^$destination"|grep "$geteway"|wc -l`
	if [ $num -ne 0 ]; then
		CM_LOG "[${FUNCNAME}:${LINENO}] already exsit"
		return $CM_ERR_ALREADY_EXISTS
	fi
	route add -net $destination $geteway -netmask $netmask
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
	num=`netstat -rv|grep "\."|egrep -v 'default|localhost'|grep "^$destination"|grep "$geteway"|wc -l`
	if [ $num -eq 0 ]; then
		CM_LOG "[${FUNCNAME}:${LINENO}] no exsit"
		return $CM_ERR_NOT_EXISTS
	fi
	route delete -net $destination $geteway -netmask $netmask
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