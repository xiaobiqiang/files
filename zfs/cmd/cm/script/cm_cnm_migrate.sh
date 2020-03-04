#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

function cm_cnm_migrate_insert()
{
	local iRet=$CM_OK
	local cut
	local type=$1
	local path=$2
	local pool=$3
	local lunid=$4
	local nid=$5
	
	ip=`ceres_cmd node| grep $nid|awk '{print $4}'`
	if [ $type -eq 0 ]; then
		cut=`ceres_exec $ip "zfs lun_migrate create -g $lunid /dev/rdsk/$path $pool 2>&1 | grep fail|wc -l"`
		if [ $cut -ne 0 ]; then
			iRet=1
		fi
	fi
	
	if [ $type -eq 1 ]; then 
		cut=`ceres_exec $ip "zfs lun_migrate create /dev/zvol/rdsk/$path $pool 2>&1 | grep fail|wc -l"`
		if [ $cut -ne 0 ]; then
			iRet=1
		fi
	fi
	
	return $iRet
}

function cm_cnm_migrate_update()
{
	local iRet=$CM_OK
	local type=$1
	local path=$2
	local operation=$3
	if [ $type -eq 0 ]; then
		if [ $operation -eq 0 ]; then
			zfs lun_migrate stop /dev/rdsk/$path
			iRet=$?
		fi
		
		if [ $operation -eq 1 ]; then
			zfs lun_migrate restart /dev/rdsk/$path
			iRet=$?
		fi
	fi
		
	if [ $type -eq 1 ]; then 
		if [ $operation -eq 0 ]; then
			zfs lun_migrate stop /dev/zvol/rdsk/$path
			iRet=$?
		fi
		
		if [ $operation -eq 1 ]; then
			zfs lun_migrate restart /dev/zvol/rdsk/$path
			iRet=$?
		fi
	fi
	return $iRet
}

function cm_cnm_migrate_get()
{
	local iRet=$CM_OK
	local type=$1
	local path=$2
	if [ $type -eq 0 ]; then 
		local progress=`zfs lun_migrate check -n /dev/dsk/$path`
		iRet=$?
		if [ "$progress" == "this lun migrate have done or not exit" ]; then
			progress=101
		fi
	fi
	
	if [ $type -eq 1 ]; then 
		local progress=`zfs lun_migrate check -n /dev/zvol/rdsk/$path`
		iRet=$?
		if [ "$progress" == "this lun migrate have done or not exit" ]; then
			progress=101
		fi
	fi
	
	echo "$progress\c"
	return $iRet
}

function cm_cnm_migrate_scan()
{
	local hostname=`hostname`
	local hostip=`ceres_cmd node|grep $hostname|awk '{print $4}'`
	local master=`ceres_cmd master| grep node_ip| awk '{print $3}'`
	local allnode=`ceres_cmd node|awk 'NR>1{print $4}'`
	
	cfgadm -al -o show_SCSI_LUN &
	
	if [ "X$master" = "X$hostip" ]; then
	for node in $allnode
	do
		if [ "X$node" = "X$master" ]; then
			continue
		fi
		ceres_exec $node "/var/cm/script/cm_cnm_migrate.sh scan 2>/dev/null"
	done
	fi
}

function cm_cnm_migrate_task()
{
    local nid=$1
    local path=$2
    local pool=$3
    local type=$4

    node=`clumgt zfs list $path |grep df|awk '{print $6}'`
    ip=`ceres_cmd node|grep $node|awk '{print $4}'`
    OLD_IFS="$IFS"
    IFS="/"

    luname=($path)
    luname=${luname[1]}

    if [ $type -eq 0 ]; then 
        cut=`ceres_exec $ip "zfs lun_migrate check -n /dev/rdsk/$pool"/lm_"$luname |sed '/this.*/d'"`
        if [ "X"$cut = "X" ]; then
            echo "101"
        else
            echo $cut
        fi
    fi

    if [ $type -eq 1 ]; then 
        cut=`ceres_exec $ip "zfs lun_migrate check -n /dev/zvol/rdsk/$pool"/lm_"$luname |sed '/this.*/d'"`
        if [ "X"$cut = "X" ]; then
            echo "101"
        else
            echo $cut
        fi
    fi
}

function main()
{	
	local iRet=$CM_OK
	local paramnum=$#
	case $1 in
		insert)
		if [ $paramnum -ne 6 ]; then
            return $CM_PARAM_ERR
        fi		
		cm_cnm_migrate_insert $2 $3 $4 $5 $6
		iRet=$?
		;;
		update)
		if [ $paramnum -ne 4 ]; then
            return $CM_PARAM_ERR
        fi		
		cm_cnm_migrate_update $2 $3 $4
		iRet=$?
		;;
		get)
		if [ $paramnum -ne 3 ]; then
            return $CM_PARAM_ERR
        fi	
		cm_cnm_migrate_get $2 $3
		iRet=$?
		;;
		scan)
		if [ $paramnum -ne 1 ]; then
            return $CM_PARAM_ERR
        fi	
		cm_cnm_migrate_scan
		iRet=$?
		;;
		task)
		if [ $paramnum -ne 5 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_migrate_task $2 $3 $4 $5
		iRet=$?
		;;
		*)
		;;
	esac
	
	return $iRet
}

main $*
exit $?