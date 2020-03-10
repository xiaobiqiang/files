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


iRet=$CM_OK
list=($(zfs list -H -o name,used,avail,quota,volsize,mountpoint))
len=${#list[@]}
((cols=$len/6))

for (( i=0; i<$cols; i=i+1 ))
do
    ((j=i*6))
    name=${list[j]}
    used=${list[((j=$j+1))]}
    avail=${list[((j=$j+1))]}
    quota=${list[((j=$j+1))]}
    volsize=${list[((j=$j+1))]}
    mountpoint=${list[((j=$j+1))]}
    
    if [ `grep \"$name\" /tmp/lu.xml|wc -l` -ne 0 ]; then 
        if [ `grep $name /tmp/lu.xml| grep "refreservation=\"none\""|wc -l` -eq 0 ]; then
            continue
        fi
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
