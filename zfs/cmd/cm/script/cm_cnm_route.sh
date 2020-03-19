#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

function cm_cnm_route_getbatch()
{
    local myips=`ifconfig -a |grep -w inet |awk '{printf $2"$|"}END{print "127.0.0.1$"}'`
    netstat -nrv|grep '\.' |awk '{print $1" "$2" "$3}' |egrep -v "$myips"
    return $CM_OK
}

function cm_cnm_route_count()
{
    local myips=`ifconfig -a |grep -w inet |awk '{printf $2"$|"}END{print "127.0.0.1$"}'`
    netstat -nrv|grep '\.' |awk '{print $1" "$2" "$3}' |egrep -v "$myips" |wc -l
    return $CM_OK
}

function cm_cnm_route_create()
{
    local destination=$1
    local netmask=$2
    local geteway=$3
    num=`netstat -nrv|grep "\."|egrep -v 'default|localhost'|grep "^$destination"|grep "$geteway"|wc -l`
    if [ $num -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] already exsit"
        return $CM_ERR_ALREADY_EXISTS
    fi
    route add -net $destination $geteway -netmask $netmask
    local iRet=$?
    if [ $iRet -ne $CM_OK ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $destination $geteway $netmask fail"
        iRet=$CM_FAIL
    fi
    return $iRet
}	

function cm_cnm_route_delete()
{
    local destination=$1
    local netmask=$2
    local geteway=$3
    num=`netstat -nrv|grep "\."|egrep -v 'default|localhost'|grep "^$destination"|grep "$geteway"|wc -l`
    if [ $num -eq 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] no exsit"
        return $CM_ERR_NOT_EXISTS
    fi
    route delete -net $destination $geteway -netmask $netmask
    local iRet=$?
    if [ $iRet -ne $CM_OK ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $destination $geteway $netmask fail"
        iRet=$CM_FAIL
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