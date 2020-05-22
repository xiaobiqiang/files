#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'
CM_IPTABLES_CFG='/etc/sysconfig/iptables'

if [ ! -f $CM_IPTABLES_CFG ];then
    mkdir -p /etc/sysconfig
    touch $CM_IPTABLES_CFG
fi

function cm_cnm_iptables_getbatch()
{
    iptables -L INPUT -n -v --line-numbers 2>/dev/null |sed 1,2d |egrep "ACCEPT |DROP " \
        |egrep -v "ctstate |udp " |sed 's/dpt://g' \
        |sed 's/ACCEPT/pass/g' |sed 's/DROP/block/g' \
        |awk '$7!="lo"{print $1" "$4" "$7" "$9" "$12}' \
        |while read line
    do
        local info=($line)
        local cnt=${#info[*]}
        local port="any"
        if [ $cnt -eq 5 ]; then
            port=${info[4]}
        fi
        local act=${info[1]}
        local nic=${info[2]}
        local ips=${info[3]}
        
        if [ "$nic" == '*' ];then
            nic="any"
        fi
        
        if [[ "X$ips" == "X0.0.0.0"* ]];then
            ips="any"
        fi
        echo "$act $nic $ips $port online"
    done 
    return 0
}

function cm_cnm_iptables_count()
{
    iptables -L INPUT -n -v --line-numbers 2>/dev/null |sed 1,2d |egrep "ACCEPT |DROP " \
        |egrep -v "ctstate |udp " |sed 's/dpt://g' \
        |awk 'BEGIN{c=0}$7!="lo"{c=c+1}END{print c}' 
    return 0
}

function cm_cnm_iptables_add()
{
    local act=$1  #pass | block
    local nic=$2  #any
    local ip=$3   #any 
    local port=$4 #0

    local cmd=""
    if [ "X$nic" != "X" ] && [ "X$nic" != "Xany" ]; then
        cmd="$cmd -i $nic"
    fi

    if [ "X$ip" != "X" ] && [ "X$ip" != "Xany" ]; then
        cmd="$cmd -s $ip"
    fi

    if [ "X$port" != "X" ] && [ "X$port" != "X0" ]; then
        cmd="$cmd -p tcp --dport $port"
    fi

    if [ "X$act" == "Xpass" ]; then
        cmd="iptables -I INPUT $cmd -j ACCEPT"
    else
        cmd="iptables -A INPUT $cmd -j DROP"
    fi
    $cmd
    local iRet=$?

    if [ $iRet -eq 0 ]; then
        iptables-save >${CM_IPTABLES_CFG}
    fi
    return $iRet
}

function cm_cnm_iptabls_getid()
{
    local act=$1  #pass | block
    local nic=$2  #any
    local ips=$3   #any 
    local port=$4 #0
    if [ "X$act" == "Xpass" ]; then
        act="ACCEPT"
    else
        act="DROP"
    fi
    iptables -L INPUT -n -v --line-numbers 2>/dev/null |sed 1,2d |egrep "$act " \
        |egrep -v "ctstate |udp " |sed 's/dpt://g' \
        |awk '{print $1" "$7" "$9" "$12}' \
        |while read line
    do
        local info=($line)
        local cnt=${#info[*]}
        local portx="0"
        if [ $cnt -eq 4 ]; then
            portx=${info[3]}
        fi
        if [ "X$portx" != "X$port" ]; then
            continue
        fi
        local nicx=${info[1]}
        local ipsx=${info[2]}
        if [ "$nicx" == '*' ];then
            nicx="any"
        fi
        
        if [ "X$nic" != "X$nicx" ]; then
            continue
        fi
        if [[ "X$ipsx" == "X0.0.0.0"* ]];then
            ipsx="any"
        fi
        if [ "X$ips" != "X$ipsx" ]; then
            continue
        fi
        echo "${info[0]}"
        break
    done
    return 0
}

function cm_cnm_iptables_delete()
{
    local id=`cm_cnm_iptabls_getid $*`
    if [ "X$id" == "X" ]; then
        return 0
    fi
    iptables -D INPUT $id
    local iRet=$?

    if [ $iRet -eq 0 ]; then
        iptables-save >${CM_IPTABLES_CFG}
    fi
    return $iRet
}

function cm_cnm_iptables_deny()
{
    cm_cnm_iptables_add 'block' $*
    return $?
}

function cm_cnm_iptables_allow()
{
    cm_cnm_iptables_add 'pass' $*
    return $?
}

function cm_cnm_iptables_update()
{
    return 0
}

function cm_cnm_iptables_check_nic()
{
    local nic=$1
    local num=`ifconfig -a|grep -w $nic|wc -l`
    echo $num
    return $?
}
cm_cnm_iptables_$*
exit $?