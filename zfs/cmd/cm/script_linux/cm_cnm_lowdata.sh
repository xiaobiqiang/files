#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

function cm_cnm_lowdata_getbatch()
{
	zfs list -H -t filesystem -o name,lowdata,lowdata_period,lowdata_delete_period,lowdata_period_unit,lowdata_criteria|grep '/'|while read line
	do
		echo -n "$line "
		local arr=($line)
		zfs migrate status ${arr[0]}|grep "^total"|awk -F':' '{printf $2} END{print ""}'
	done
}

function cm_cnm_lowdata_get()
{
	local nas=$1
	local arr=`zfs list -H -t filesystem -o name,lowdata,lowdata_period,lowdata_delete_period,lowdata_period_unit,lowdata_criteria $nas 2>/dev/null`
	if [ "X" != "X$arr" ]; then
		echo "$arr \c"
		zfs migrate status $nas|grep "^total"|awk -F':' '{printf $2} END{print ""}'
	fi

}

function cm_cnm_dirlowdata_getbatch()
{
	local nas=$1
	local status
	local timeout
	local unit
	local delete_time
	local criteria
	local dir_list=`ls -l /$nas|grep '^d'|awk '{print $9}'`
	local name=`echo $nas|awk -F'/' '{print $1"/"$2}'`
	local dir_perf

	name_len=`echo $name|wc -L`
	((name_len=$name_len+1))
	dir_perf=`echo ${nas:$name_len}`
	
	for dir in $dir_list
	do
		dir_bak=$dir
		if [ "X" != $dir_perf"X" ];then
			dir=$dir_perf"/"$dir
		fi
		
		status=`zfs get dirlowdata@$dir $name|egrep -v NAME|awk '{print $3}'`
		if [ $status = '-' ]; then
			status='3'
		fi
		
		timeout=`zfs get dirlowdata_period@$dir $name|egrep -v NAME|awk '{print $3}'`
		if [ $timeout = '-' ]; then
			timeout='7'
		fi
		
		unit=`zfs get dirlowdata_period_unit@$dir $name|egrep -v NAME|awk '{print $3}'`
		if [ $unit = '-' ]; then
			unit='3'
		fi
		
		delete_time=`zfs get dirlowdata_delete_period@$dir $name|egrep -v NAME|awk '{print $3}'`
		if [ $delete_time = '-' ]; then
			delete_time='0'
		fi
		
		criteria=`zfs get  dirlowdata_criteria@$dir $name|egrep -v NAME|awk '{print $3}'`
		if [ $criteria = '-' ]; then
			criteria='0'
		fi
		
		echo "$dir_bak $status $timeout $unit $delete_time $criteria"
	done
}

function cm_cnm_dirlowdata_get()
{
	local nas=$1
	local dir=$2
	local name=`echo $nas|awk -F'/' '{print $1"/"$2}'`
    local dir_perf

    name_len=`echo $name|wc -L`
    ((name_len=$name_len+1))
    dir_perf=`echo ${nas:$name_len}`

    dir_bak=$dir
    if [ "X" != $dir_perf"X" ];then
        dir=$dir_perf"/"$dir
    fi

	
	local status=`zfs get dirlowdata@$dir $name|egrep -v NAME|awk '{print $3}'`
	if [ $status = '-' ]; then
		status='3'
	fi
		
	local timeout=`zfs get dirlowdata_period@$dir $name|egrep -v NAME|awk '{print $3}'`
	if [ $timeout = '-' ]; then
		timeout='7'
	fi
		
	local unit=`zfs get dirlowdata_period_unit@$dir $name|egrep -v NAME|awk '{print $3}'`
	if [ $unit = '-' ]; then
		unit='3'
	fi
		
	local delete_time=`zfs get dirlowdata_delete_period@$dir $name|egrep -v NAME|awk '{print $3}'`
	if [ $delete_time = '-' ]; then
		delete_time='0'
	fi
		
	local criteria=`zfs get  dirlowdata_criteria@$dir $name|egrep -v NAME|awk '{print $3}'`
	if [ $criteria = '-' ]; then
		criteria='0'
	fi
		
	echo "$status $timeout $unit $delete_time $criteria"
}

function cm_cnm_dirlowdata_cluster_get()
{
	list=`zfs list`
	master_nas=`sqlite3 /tmp/cm_cluster_nas_cache.db "select nas from nas_t where role='0' limit 1"`
	for nas in $master_nas
	do
		cut=`echo $list|grep  "$nas " |wc -l`
		if [ $cut -ne 0 ]; then
			zfs dirlowdatalist $nas|egrep -v 'LOW| -- '
		fi
	done
}

function cm_cnm_get_nas_nid()
{
	local name=$1
	ceres_cmd node |egrep -v NAME|awk '{print $2" "$3}'|while read line
	do
		array=($line)
		if [ `cm_multi_exec ${array[1]} "zfs list -t filesystem -o name,used|grep '/'|grep \"$name \" |wc -l"` -eq 1 ]; then
			echo ${array[0]}
			break
		fi
	done
	
	return $CM_OK
}

function cm_cnm_get_db_id()
{
	local name=$1
	local obj_id=$2
	
	sqlite3 /var/cm/data/cm_sche.db "select id from record_t where obj=$obj_id and name='$name'"
}

function cm_cnm_zpool_data_change_m()
{
    local val=$1
    local final=${val: -1}
    local relval=${val%?}
    case $final in
        P)
            relval=`echo $relval*1024*1024*1024 |bc`
        ;;
        T)
            relval=`echo $relval*1024*1024 |bc`
        ;;
        G)
            relval=`echo $relval*1024 |bc`
        ;;
        M)
        ;;
        *)
        relval=0
        ;;
    esac
    echo $relval |awk -F'.' '{print $1}'
    return 0
}

function volume_get_small_percent()
{
    local name=$1
    local used=$2
    local avail=$3
    local quota=$4
    local total
    
    if [ $quota = "none" ]; then
        total=$avail
    else
        total=$quota
    fi
    
    total=`cm_cnm_zpool_data_change_m $total`
    used=`cm_cnm_zpool_data_change_m $used`
    ((result=100*$used/$total))
    echo $result
}

function cm_cnm_lowdata_volume_small()
{
    local percent=$1
    local list=($(zfs list -t filesystem -o name,used,avail,quota|grep '/'))
    len=${#list[@]}
    ((cols=$len/4))
    
    for (( i=0; i<$cols; i=i+1 ))
    do
        ((j=i*4))
        local name=${list[j]}
        local used=${list[((j=$j+1))]}
        local avail=${list[((j=$j+1))]}
        local quota=${list[((j=$j+1))]}
        
        local result=`volume_get_small_percent $name $used $avail $quota`
        if [ $result -ge $percent ]; then
            CM_EXEC_CMD "zfs migrate start $name -all"
        fi
    done
}

function cm_cnm_lowdata_volume_thread()
{
    local config="/var/cm/data/lowdata_config.ini"
    local percent=`cat $config 2>/dev/null|grep start|awk '{print $3}'`
    
    if [ "X"$percent = "X" ]; then
        return 
    fi
    
    if [ $percent -eq 0 ]; then
        return
    fi

    if [ ! -f /var/cm/data/cm_cnm_pool_stat_in.flag ]; then
        cm_cnm_lowdata_volume_small $percent
        return
    fi
    
    list=($(/var/cm/script/cm_cnm.sh zpool_status_get|awk '{print $2" "$3" "$4}'))
    len=${#list[@]}
    ((cols=$len/3))

    for (( i=0; i<$cols; i=i+1 ))
    do
        ((j=i*3))
        name=${list[j]}
        total=${list[((j=$j+1))]}
        used=${list[((j=$j+1))]}
        
        if [ $total -eq 0 ] || [ $used -eq 0 ]; then
            continue
        fi
        
        ((result=100*$used/$total))
        if [ $result -ge $percent ]; then
            naslist=($(zfs list -t filesystem|awk '{print $1}'|grep '/'|grep $name))
            for nas in $naslist
            do
                CM_EXEC_CMD "zfs migrate start $nas -all"
            done
        fi
    done
}

function cm_cnm_lowdata_volume_small_stop()
{
    local stop=$1
    local list=($(zfs list -t filesystem -o name,used,avail,quota|grep '/'))
    len=${#list[@]}
    ((cols=$len/4))
    
    for (( i=0; i<$cols; i=i+1 ))
    do
        ((j=i*4))
        local name=${list[j]}
        local used=${list[((j=$j+1))]}
        local avail=${list[((j=$j+1))]}
        local quota=${list[((j=$j+1))]}
        
        local result=`volume_get_small_percent $name $used $avail $quota`
        if [ $result -le $stop ]; then
            CM_EXEC_CMD "zfs migrate stop $name"
        fi
    done
}

function cm_cnm_lowdata_volume_stop()
{
    local config="/var/cm/data/lowdata_config.ini"
    local stop=`cat $config 2>/dev/null|grep stop|awk '{print $3}'`
   
    if [ "X"$stop = "X" ]; then
        return 
    fi
    
    if [ $stop -eq 0 ]; then
        return
    fi
    
    if [ ! -f /var/cm/data/cm_cnm_pool_stat_in.flag ]; then
        cm_cnm_lowdata_volume_small_stop $stop
        return
    fi
    
    list=($(/var/cm/script/cm_cnm.sh zpool_status_get|awk '{print $2" "$3" "$4}'))
    len=${#list[@]}
    ((cols=$len/3))

    for (( i=0; i<$cols; i=i+1 ))
    do
        ((j=i*3))
        name=${list[j]}
        total=${list[((j=$j+1))]}
        used=${list[((j=$j+1))]}
        
        if [ $total -eq 0 ] || [ $used -eq 0 ]; then
            continue
        fi
        
        ((result=100*$used/$total))
        if [ $result -le $stop ]; then
            naslist=($(zfs list -t filesystem|awk '{print $1}'|grep '/'|grep $name))
            for nas in $naslist
            do
                CM_EXEC_CMD "zfs migrate stop $nas"
            done
        fi
    done
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
		cm_cnm_lowdata_getbatch
		iRet=$?
		;;
		get)
		if [ $paramnum -ne 2 ]; then
			return $CM_PARAM_ERR
		fi
		cm_cnm_lowdata_get $2
		iRet=$?
		;;
		dirgetbatch)
		if [ $paramnum -ne 2 ]; then
			return $CM_PARAM_ERR
		fi
		cm_cnm_dirlowdata_getbatch $2
		iRet=$?
		;;
		dirget)
		if [ $paramnum -ne 3 ]; then
			return $CM_PARAM_ERR
		fi
		cm_cnm_dirlowdata_get $2 $3
		iRet=$?
		;;
		cluster)
		cm_cnm_dirlowdata_cluster_get
		iRet=$?
		;;
		getnid)
		if [ $paramnum -ne 2 ]; then
			return $CM_PARAM_ERR
		fi
		cm_cnm_get_nas_nid $2 
		iRet=$?
		;;
		getdbid)
		if [ $paramnum -ne 3 ]; then
			return $CM_PARAM_ERR
		fi
		cm_cnm_get_db_id $2 $3
		iRet=$?
		;;
		thread)
		if [ $paramnum -ne 1 ]; then
			return $CM_PARAM_ERR
		fi
		cm_cnm_lowdata_volume_thread
		iRet=$?
		;;
		thread_stop)
		if [ $paramnum -ne 1 ]; then
			return $CM_PARAM_ERR
		fi
		cm_cnm_lowdata_volume_stop
		iRet=$?
		;;         
		*)
		;;
	esac
	return $iRet
}

main $*
exit $?
