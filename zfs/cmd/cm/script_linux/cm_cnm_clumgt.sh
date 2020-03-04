#!/bin/bash

function clumgt_status()
{
    local nodes=`cat /etc/clumgt.config|awk '{print $1}'`
    local script='/var/cm/script/cm_cnm.sh'
    for node in $nodes
    do
        cm_multi_exec $node "$script clumgt_status"
    done
}

function clumgt_handle_common()
{
    if [[ $1 == "df"* ]]; then
        cmd=""
        len=$#
        argv=($*)
        for (( i=1; i<$len; i=i+1 ))
        do
            cmd=$cmd" ${argv[i]}"
        done
        cm_multi_exec $1 "$cmd"
        return $?
    else
        cm_multi_exec "$*"
        return $?
    fi
}



case $1 in 
    status)
    clumgt_status
    ;;
    *)
    clumgt_handle_common $*
    ;;
esac

exit $?
