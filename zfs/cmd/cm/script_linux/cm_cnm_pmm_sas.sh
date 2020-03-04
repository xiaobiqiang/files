#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

CM_PMM_SAS_FILE=/tmp/sas_data.$$
function get_without_repetition_name()
{
    local src_names=($1)

    local tmp_arr[0]=${src_names[0]}
    echo ${src_names[0]}

    local index=0;
    for name in ${src_names[@]}
    do
        local flag=0
        for ((i=0;$i<=$index;++i))
        do
            if [ "$name"x == "${tmp_arr[$i]}"x ];then
                ((flag++))
            fi
        done
        ((index++))
        if [ $flag -eq 0 ];then
            tmp_arr[$index]=$name
            echo $name
        fi
    done
}

function cm_cnm_sas_getbatch()
{
    local sas_names=$(kstat -m mdi -c iopath|grep -w name:|awk '{print $2}'|awk -F'_' '{print $2}'|sort|uniq)
    echo "$sas_names"
}

function cm_cnm_sas_get_data()
{
    local obj=$1
    local obj_ret=0
    local numbers=$(cat $CM_PMM_SAS_FILE|grep $obj|awk '{print $2}')
    for num in ${numbers[@]}
    do
       ((obj_ret=$obj_ret+$num))
    done
    echo $obj_ret
}

function cm_cnm_sas_get()
{
    local sas=$1
    kstat -m mdi -c iopath -n *mpt_$sas > $CM_PMM_SAS_FILE
    local writes=$(cm_cnm_sas_get_data writes)
    local reads=$(cm_cnm_sas_get_data reads)
    local nwritten=$(cm_cnm_sas_get_data nwritten)
    local nread=$(cm_cnm_sas_get_data nread)
    echo $writes $reads $nwritten $nread
    rm $CM_PMM_SAS_FILE
}

function cm_cnm_sas_check()
{
    local sas=$1
    if [ $(kstat -m mdi -c iopath -n *mpt_$sas|grep $sas|wc -l) -ne $CM_OK ];then
        echo $CM_OK
    fi
    echo $CM_FAIL
}

function cm_cnm_pmm_sas_main()
{
    local cmd=$1
    local var=$2

    local iRet=$CM_OK
    case $cmd in
        getbatch)
            cm_cnm_sas_getbatch
            iRet=$?
        ;;
        get)
            cm_cnm_sas_get $2
            iRet=$?
        ;;
        check)
            cm_cnm_sas_check $2
            iRet=$?
        ;;
    esac
    return $iRet
}

cm_cnm_pmm_sas_main $1 $2
exit $?
