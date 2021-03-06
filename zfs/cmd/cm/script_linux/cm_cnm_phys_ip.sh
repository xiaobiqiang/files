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
     ifconfig -a|grep -B 1 'inet '|grep -v "-" \
        | sed 'N;s/\n//g'|grep -v "lo"|awk '{print $1" "$6" "$8}'\
        |while read line
    do
        local info=($line)
        local name=${info[0]}
        name=${name%?}
        local ip=${info[1]}
        local mask=${info[2]}
        echo "$name $ip $mask"
    done
    return $CM_OK
}

function cm_cnm_phys_ip_count()
{
    local len=`ifconfig -a|grep 'inet '|egrep -v "127.0.0.1"|wc -l`
    echo $len
    return $CM_OK
}

function cm_cnm_phys_ip_create()
{
    local ip=$1
    local netmask=$2
    local interface=$3
    local os_type=`cm_systerm_version_get`
    local iRet=$CM_OK
    check_ip $ip
    if [ $? -ne $CM_OK ];then
        return $CM_PARAM_ERR
    fi
    ###### 对应例如eth0:1这种情形 #########
    # local sec_port=`echo $interface |grep ':'|wc -l`
    # if [ $sec_port -eq 1 ];then
    #    if [ $netmask = 'null' ]; then
    #        ifconfig $interface $ip 
    #    else
    #        ifconfig $interface $ip netmask $netmask 
    #    fi
    #    iRet=$?
    #    return $iRet
    # fi
    
    if [ $os_type -ne $CM_OS_TYPE_DEEPIN ];then
        local dir='/etc/sysconfig/network-scripts/ifcfg-'
        echo "DEVICE=$interface" > $dir$interface
        echo "ONBOOT=yes" >> $dir$interface
        echo "BOOTPROTO=static" >> $dir$interface
        echo "IPADDR=$ip" >> $dir$interface
        if [ $netmask = 'null' ]; then
            echo "NETMASK=255.255.255.0" >> $dir$interface
        else
            echo "NETMASK=$netmask" >> $dir$interface
        fi
    else
        local dir='/etc/network/interfaces'
        echo "auto $interface" >> $dir
        echo "allow-hotplug $interface" >> $dir
        echo "iface $interface inet static" >> $dir
        echo "address $ip" >> $dir
        if [ $netmask = 'null' ]; then
            echo "netmask 255.255.255.0" >> $dir
        else
            echo "netmask $netmask" >> $dir
        fi
    
    fi
    
    if [ $netmask = 'null' ]; then
        ifconfig $interface $ip 
    else
        ifconfig $interface $ip netmask $netmask 
    fi
    iRet=$?
    return $iRet
}

function cm_cnm_phys_ip_delete()
{
    local ipaddr=$1
    local nic=$2
    local mport=`cm_get_localmanageport`
    local os_type=`cm_systerm_version_get`
    CM_LOG "[${FUNCNAME}:${LINENO}]$nic $ipaddr"
    if [ "X$nic" == "X$mport" ]; then
        return $CM_ERR_NOT_SUPPORT
    fi
    local info=($(ifconfig -a|grep -B 1 'inet '|grep -v "-" \
        | sed 'N;s/\n//g'|grep -v "lo"|awk '{print $1" "$6}'\
        |grep $ipaddr))
    local ip=${info[1]}
    if [ "X$ipaddr" != "X$ip" ]; then
        return
    fi
    local name=${info[0]}
    name=${name%?}
    if [ "X$name" == "X$mport" ]; then
        return $CM_ERR_NOT_SUPPORT
    fi
    if [ "X$nic" != "X" ] && [ "X$nic" != "X$name" ]; then
        return
    fi
    if [ $ipaddr = `grep 'ip ' /var/cm/data/cm_cluster.ini|awk '{print $3}'` ]; then
        return $CM_ERR_NOT_SUPPORT
    fi
    ip addr del $ipaddr dev $name
    if [ $os_type -ne $CM_OS_TYPE_DEEPIN ];then
        rm -f /etc/sysconfig/network-scripts/ifcfg-$name
    else
        netmask_line=`grep -n -w "iface $name" /etc/network/interfaces|awk -F':' '{print $1}'`
        ((netmask_line=$netmask_line+2))
        sed "$netmask_line"d /etc/network/interfaces>/tmp/interfaces
        sed "/$name/d" /tmp/interfaces|sed "/$ipaddr/d" > /tmp/interfaces_bak
        cat /tmp/interfaces_bak>/etc/network/interfaces
    fi
    #rm -f /etc/hostname.$name

    return $CM_OK
}

function cm_cnm_phys_ip_physgetbatch()
{
    local list=($( ip addr|grep -w mtu|grep -v lo|grep -v dummy|awk '{print $2" "$5" "$9}'))
    local len=${#list[@]}
    ((cols=$len/3))
    
    for (( i=0; i<$cols; i=i+1 ))
    do
        ((j=i*3))
        local name=${list[j]}
        name=${name%?}
        local media="Ethernet"
        local mtu=${list[((j=$j+1))]}
        if [ "X" = "X"$mtu ]; then
            mtu=0
        fi
        typeset -l state
        state=${list[((j=$j+1))]}
        typeset -l duplex
        if [ ` ethtool $name | grep Full|wc -l` -ne 0 ]; then
            duplex='full'
        else
            duplex='half'
        fi
        local speed=`ethtool $name|grep Speed|awk '{printf("%u\n",$2)}'`
        if [ "X" = "X"$speed ]; then
            speed=0
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
    local list=($(ip addr|grep -w mtu|grep -w '$name'|grep -v lo|awk '{print $5" "$9}'))
    local mtu=${list[0]}
    typeset -l state
    state=${list[1]}
    local media="Ethernet"
    typeset -l duplex
    if [ ` ethtool $name | grep Full|wc -l` -ne 0 ]; then
        duplex='full'
    else
        duplex='half'
    fi
    local speed=`ethtool $name|grep Speed|awk '{printf("%u\n",$2)}'`
    if [ "X" = "X"$speed ]; then
        speed=0
    fi
    local mac=`ifconfig $name 2>/dev/null|grep -w ether |awk '{printf $2}'`
    if [ "X" = "X"$mac ]; then
        mac='null'
    fi
    
    
    echo "$name $media $state $speed $duplex $mtu $mac"
}

function cm_cnm_phys_ip_physcount()
{
    ip addr|grep -w mtu|grep -v lo|grep -v dummy|wc -l
}

function cm_cnm_phys_ip_physupdate()
{
    local name=$1
    local mtu=$2
    
    ifconfig $name mtu $mtu up
    if [ $? -ne $CM_OK ]; then
        return $CM_FAIL
    fi
    
    return $CM_OK
}

cm_cnm_phys_ip_$*
exit $?