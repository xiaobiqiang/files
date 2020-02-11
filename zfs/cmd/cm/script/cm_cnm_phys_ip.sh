#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'


#==================================================================================
#                              子网掩码地址转换
# 输入参数:
#       netmask
# 回显数据:
#   	$ip1.$ip2.$ip3.$ip4
#==================================================================================
function is_numeric
{
    local value=${1}

    # If "value" is not given, it is not numeric
    if [[ -z "${value}" ]]; then
        return $CM_PARAM_ERR
    fi

    # If not numeric, return 1
    if [[ $(expr "${value}" : '[0-9]*') -ne ${#value} ]]; then
        return $CM_PARAM_ERR
    fi

    # Numeric
    return $CM_OK
}

function check_ip()
{
    local value=${1}
#      -A octets $(IFS=. ; set -- ${value};  echo $*)
#    let i=$(set -- ${octets[*]};  echo $#)
    local i=`echo $value|sed 's/\./ /g'|awk '{for(i=1;i<=NF;i++)print $i}'|wc -l`
    local octets=(`echo $value|sed 's/\./ /g'`)    
    # There must be four octets
    if [[ ${i} -ne 4 ]]; then
        return $CM_PARAM_ERR
    fi
    
    # Each must be numeric and less than 256
    for octet in ${octets[*]}
    do
        is_numeric ${octet} || return 1
        if [[ ${octet} -gt 255 ]]; then
            return $CM_PARAM_ERR
        fi
    done
    # The first must not be zero
    local first=`echo $octets|awk '{print $1}'`
    if [ $first -eq 0 ];  then
        return $CM_PARAM_ERR
    fi
    if [ ${octets[0]} -eq 255 ] && [ ${octets[1]} -eq 255 ] && [ ${octets[2]} -eq 255 ] && [ ${octets[3]} -eq 255 ];then
        return $CM_PARAM_ERR
    fi

    return $CM_OK
}

function cm_cnm_numtoip()
{
    local num=$1
    local ip1=${num:0:2}
    ((ip1=16#$ip1))
    local ip2=${num:2:2}
    ((ip2=16#$ip2))
    local ip3=${num:4:2}
    ((ip3=16#$ip3))
    local ip4=${num:6:2}
    ((ip4=16#$ip4))
    echo "$ip1.$ip2.$ip3.$ip4"
}

function cm_cnm_phys_ip_getbatch()
{
    ifconfig -a |grep -v ether |sed 'N;s/\n//g' \
        |awk '{print $1" "$8" "$10}' |grep -v 'lo0' \
        |while read line
    do
        local info=($line)
        local name=${info[0]}
        name=${name%?}
        local ip=${info[1]}
        local mask=`cm_cnm_numtoip "${info[2]}"`
        echo "$name $ip $mask"
    done
    return $CM_OK
}

function cm_cnm_phys_ip_count()
{
    local port=$1
    local len=0
    if [ "X$port" == "X" ]; then
        len=`ifconfig -a4|grep 'flags='|egrep -v 'lo0'|wc -l`
    else
        len=`ifconfig -a4 |grep -v ether |sed 'N;s/\n//g' |grep -w $port |wc -l`
    fi
    echo $len
    return $CM_OK
}

function cm_cnm_phys_ip_create()
{
    local ip=$1
    local netmask=$2
    local interface=$3
    local iRet=$CM_OK
    check_ip $ip
    if [ $? -ne $CM_OK ];then
        return $CM_PARAM_ERR
    fi
    echo $ip > /etc/hostname.$interface
    if [ $netmask = 'null' ]; then
        echo "netmask 255.255.255.0" >> /etc/hostname.$interface
    else
        echo "netmask $netmask" >> /etc/hostname.$interface
    fi

    echo up >> /etc/hostname.$interface

    if [ $netmask = 'null' ]; then
        ifconfig $interface plumb $ip up
    else
        ifconfig $interface plumb $ip netmask $netmask up
    fi
    iRet=$?

    return $iRet
}

function cm_cnm_phys_ip_delete()
{
    local ipaddr=$1
    local nic=$2
    local mport=`cm_get_localmanageport`
    CM_LOG "[${FUNCNAME}:${LINENO}]$nic $ipaddr"
    if [ "X$nic" == "X$mport" ]; then
        return $CM_ERR_NOT_SUPPORT
    fi
    ifconfig -a |grep -v ether |sed 'N;s/\n//g' \
        |awk '{print $1" "$8}' |grep -v 'lo0' \
        |while read line
    do
        local info=($line)
        local ip=${info[1]}
        if [ "X$ipaddr" != "X$ip" ]; then
            continue
        fi
        local name=${info[0]}
        name=${name%?}
        if [ "X$name" == "X$mport" ]; then
            return $CM_ERR_NOT_SUPPORT
        fi
        if [ "X$nic" != "X" ] && [ "X$nic" != "X$name" ]; then
            continue
        fi
        ifconfig $name unplumb
        rm -f /etc/hostname.$name
    done
    return $CM_OK
}

function cm_cnm_phys_ip_physgetbatch()
{
    local list=($(dladm show-phys 2>/dev/null |sed 1d))
    local len=${#list[@]}
    ((cols=$len/6))
    
    for (( i=0; i<$cols; i=i+1 ))
    do
        ((j=i*6))
        local name=${list[j]}
        local media=${list[((j=$j+1))]}
        local state=${list[((j=$j+1))]}
        local speed=${list[((j=$j+1))]}
        local duplex=${list[((j=$j+1))]}
        local mtu=`ifconfig $name 2>/dev/null |grep mtu|awk '{printf $4}'`
        if [ "X" = "X"$mtu ]; then
            mtu=0
        fi
        local mac=`ifconfig $name 2>/dev/null|grep -w ether |awk '{printf $2}'`
        if [ "X" = "X"$mac ]; then
            mac='null'
        fi
        
        echo "$name $media $state $speed $duplex $mtu $mac"
    done
}

function cm_cnm_phys_ip_physget()
{
    local name=$1
    local list=`dladm show-phys $name 2>/dev/null |sed 1d|awk '{print $1" "$2" "$3" "$4" "$5}'`
    local mtu=`ifconfig $name 2>/dev/null |grep mtu|awk '{printf $4}'`
    if [ "X" = "X"$mtu ]; then
        mtu=0
    fi
    local mac=`ifconfig $name 2>/dev/null|grep -w ether |awk '{printf $2}'`
    if [ "X" = "X"$mac ]; then
        mac='null'
    fi
    
    echo "$list $mtu $mac"
}

function cm_cnm_phys_ip_physcount()
{
    dladm show-phys 2>/dev/null |sed 1d |wc -l
}

function cm_cnm_phys_ip_physupdate()
{
    local name=$1
    local mtu=$2
    
    ifconfig $name mtu $mtu up
    if [ $? -ne $CM_OK ]; then
        return $CM_FAIL
    fi
    
    cut=`cat /etc/hostname.$name|grep mtu | wc -l`
    if [ $cut -eq 0 ]; then
        echo 'mtu %u' >> /etc/hostname.$name
    else
        sed '/mtu/d' /etc/hostname.$name > /etc/hostnamecopy
        echo "mtu $mtu" >> /etc/hostnamecopy
        mv /etc/hostnamecopy /etc/hostname.$name
    fi
    
    return $CM_OK
}

cm_cnm_phys_ip_$*
exit $?