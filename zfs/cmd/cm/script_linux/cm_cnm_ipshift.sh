#!/bin/bash
source '/var/cm/script/cm_types.sh'

function cm_cnm_ipshift_getbatch()
{
	local filesystem=$1
	zfs get all $filesystem |grep "failover:" |awk '$4=="local"{print $1" "$2" "$3}' |sed 's/failover://g' |sed 's/,/ /g'
	return $CM_OK
}

function cm_cnm_ipshift_create()
{
	local filesystem=$1
	local adapter=$2
	local num=$3
	local ip=$4
	local netmask=$5
	
	zfs set failover:$adapter:$num=$ip,$netmask $filesystem
	return $CM_OK
}

function cm_cnm_ipshift_delete()
{
	local filesystem=$1
	local ip=$2
	
	local adapter=`zfs get all $filesystem| grep $ip| awk '{print $2}'|awk -F':' '{printf $2}'`
	local num=`zfs get all $filesystem| grep $ip| awk '{print $2}'|awk -F':' '{printf $3}'`
	if [ "X"$num = "X" ]; then
		zfs inherit failover:$adapter $filesystem
	else
		zfs inherit failover:$adapter:$num $filesystem
	fi
	return $CM_OK
}

function cm_cnm_ipshift_count()
{
	local filesystem=$1
	local cut=`zfs get all $filesystem | grep failover |grep -w 'local' |wc -l`
	echo $cut
	return $CM_OK
}

function main()
{
	local paramnum=$#
	local iRet=$CM_OK
	case $1 in
		getbatch)
		if [ $paramnum -ne 2 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_ipshift_getbatch $2
		iRet=$?
		;;
		create)
		if [ $paramnum -ne 6 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_ipshift_create $2 $3 $4 $5 $6
		iRet=$?
		;;
		delete)
		if [ $paramnum -ne 3 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_ipshift_delete $2 $3 
		iRet=$?
		;;
		count)
		if [ $paramnum -ne 2 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_ipshift_count $2
		iRet=$?
		;;
		*)
        	;;
	esac
	return $iRet
}

main $*
exit $?