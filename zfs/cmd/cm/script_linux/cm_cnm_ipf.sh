#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'
CM_IPF_CFG='/etc/ipf/ipf.conf'

function cm_cnm_ipf_add()
{
    local act=$1
    local nic=$2
    local ip=$3
    local port=$4
    local quick=' quick'
    local from='all'
    CM_LOG "[${FUNCNAME}:${LINENO}]$*"
    if [ "X$nic" == "Xany" ]; then
        nic=''
    else
        nic=" on $nic"
    fi
    if [ "X$ip" == "Xany" ] && [ "X$port" == "X0" ]; then
        from='all'
        quick=''
    elif [ "X$ip" == "Xany" ]; then
        from="from any to any port=$port"
    elif [ "X$port" == "X0" ]; then
        from="from $ip to any"
    else
        from="from $ip to any port=$port"
    fi
    CM_LOG "[${FUNCNAME}:${LINENO}]$act in${quick}${nic} $from"
    
    if [ "X$act" == "Xpass" ]; then
        echo "$act in${quick}${nic} $from" > $CM_IPF_CFG".tmp"
        cat $CM_IPF_CFG >> $CM_IPF_CFG".tmp"
        mv $CM_IPF_CFG".tmp" $CM_IPF_CFG        
    else
        echo "$act in${quick}${nic} $from" >> $CM_IPF_CFG
    fi
    ipf -Fa -f $CM_IPF_CFG
    return $CM_OK
}

function cm_cnm_ipf_deny()
{
    cm_cnm_ipf_add 'block' $*
    return $?
}

function cm_cnm_ipf_allow()
{
    cm_cnm_ipf_add 'pass' $*
    return $?
}

function cm_cnm_ipf_delete()
{
    local operate=$1 
    local nic=$2
    local ip=$3
    local port=$4
    local partern="$operate in.*"
    CM_LOG "[${FUNCNAME}:${LINENO}]$*"
    if [ "X$nic" != "Xany" ]; then
        partern="${partern} on ${nic}.*"
    fi
    
    if [ "X$ip" != "Xany" ] && [ "X$port" != "X0" ]; then
        partern="$partern from $ip to any port=$port"
    elif [ "X$ip" != "Xany" ]; then
        partern="$partern from $ip to any"
    elif [ "X$port" != "X0" ]; then
        partern="$partern from any to any port=$port"
    else
        partern="$partern all"
    fi
    CM_LOG "[${FUNCNAME}:${LINENO}]$partern"
    egrep -v "$partern" $CM_IPF_CFG >$CM_IPF_CFG".tmp"
    mv $CM_IPF_CFG".tmp" $CM_IPF_CFG
    ipf -Fa -f $CM_IPF_CFG
    return $CM_OK
}

function get_ipf_info()
{
    local row=($1)
    local state=${row[0]}
    local conf_item=$2
    local col_num=${#row[*]}
    if [ "X$conf_item" == "Xstate" ];then
        echo $state
        return $CM_OK
    fi
    for((i=0; i<=$col_num; i++))
    do  
        case ${row[$i]} in
            $conf_item)
            ((i++))
            echo ${row[i]}
            return $CM_OK
            ;;
            *)
            ;;
        esac  
    done
    echo "any"
    return $CM_OK  
}
function format_ipf_info()
{
    local status=$2
    local row=`echo "$1"|sed "s/=/ /g"`
    local state=`get_ipf_info "$row" "state"`
    local nic=`get_ipf_info "$row" "on"`
    local ip=`get_ipf_info "$row" "from"`
    local port=`get_ipf_info "$row" "port"`    
    echo "$state $nic $ip $port $status"
    return $CM_OK
}

function cm_cnm_ipf_getbatch()
{
    local status=`svcs ipfilter |sed 1d|awk '{print $1}'`
    sed "/^#/d" $CM_IPF_CFG|egrep 'pass|block'|while read line
    do
        format_ipf_info "$line" "$status"
    done    
    return $CM_OK
}

function cm_cnm_ipf_count()
{
    local record=`sed "/^#/d" $CM_IPF_CFG|egrep 'pass|block'|wc -l`
    echo $record
    return $CM_OK
}

function cm_cnm_ipf_update()  
{
    local update=$1
    svcadm $update ipfilter
    return $?
}

cm_cnm_ipf_$*
exit $?