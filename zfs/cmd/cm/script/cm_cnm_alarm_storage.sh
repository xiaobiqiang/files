#!/bin/bash
source '/var/cm/script/cm_types.sh'
function unit_conversion()
{
    local param=$1
        local sizes=0
        if [[ $param =~ "K" ]]; then
            sizes=1
        elif [[ $param =~ "M" ]]; then
            sizes=`echo $param|sed 's/M//g'`
        elif [[ $param =~ "G" ]]; then
            sizes=`echo $param|sed 's/G//g'`
            sizes=`echo "scale=0;$sizes*1024"|bc -l`
        elif [[ $param =~ "P" ]]; then
            sizes=`echo $param|sed 's/P//g'`
            sizes=`echo "scale=0;$sizes*1024*1024*1024"|bc -l`
        else 
            sizes=`echo $param|sed 's/T//g'`
            sizes=`echo "scale=0;$sizes*1024*1024"|bc -l`
        fi
        echo $sizes
}

function storage_type()
{
    local name=$1
    local mount=$2
	local iRet=$CM_OK
     
    if [[ $name =~ "/" ]] && [ $mount = "-" ]; then
        echo "2,\c"
        return 
    fi

    if [[ $name =~ "/" ]]; then
        echo "1,\c"
        return 
    fi
    
    echo "0,\c"
	return $CM_OK
}

function alarm_status()
{
    local used=$1
    local avail=$2
    local quota=$3
    local volsize=$4
    local percent
    local total
    local iRet=$CM_OK
    
    used=$(unit_conversion $used)
    avail=$(unit_conversion $avail)
    total=`echo "scale=0;$used+$avail"|bc -l`
    if [ $quota = "none" -o $quota = "-" ]; then
        percent=`echo "scale=0;$used*100/$total"|bc -l`
        iRet=$?
    else
        quota=$(unit_conversion $quota)
        percent=`echo "scale=0;$used*100/$quota"|bc -l`
        iRet=$?
    fi
    
    if [ $volsize != "-" ]; then
        volsize=$(unit_conversion $volsize)
        percent=`echo "scale=0;$used*100/$volsize"|bc -l`
        iRet=$?
    fi


    echo "$percent"
    return $iRet
}

function alarm_check()
{
    local iRet=$CM_OK
    zfs list -H -o name,used,avail,quota,volsize,mountpoint,type,referenced |while read line
    do
        local infos=($line)
        local name=${infos[0]}
        local used=${infos[1]}
        local avail=${infos[2]}
        local quota=${infos[3]}
        local volsize=${infos[4]}
        local mountpoint=${infos[5]}
        
        if [ "X${infos[6]}" == "Xvolume" ]; then
            used=${infos[7]}
        fi
        
        echo "$name,\c"
        storage_type $name $mountpoint
        iRet=$?
        if [ $iRet != $CM_OK ]; then
            exit $iRet
        fi
        alarm_status $used $avail $quota $volsize
        iRet=$?
        if [ $iRet != $CM_OK ]; then
            exit $iRet
        fi
    done
    return 0
}

alarm_check
exit $?
