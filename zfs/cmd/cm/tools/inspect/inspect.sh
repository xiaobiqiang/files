#!/bin/bash

source $(dirname `readlink -f $0`)/'inspect_common.sh'

function main_help()
{
    echo "$0 help"
    echo "$0 init"
    echo "$0 list [mod]"
    echo "$0 exec [mod] [func]"
    return 0
}

function main_init()
{
    inspect_init
    return $?
}

function main_list()
{
    local mod=$1
    if [ "X$mod" == "X" ]; then
        ls ${INSPECT_PWD} |awk '{print $1}' |grep 'inspect_' |grep -v common|sed 's/inspect_//g' |sed 's/.sh//g'
    else
        ${INSPECT_PWD}"inspect_${mod}.sh"
    fi
    return 0
}

function main_exec()
{
    local mod=$1
    local func=$2
    if [ "X$mod" == "X" ]; then
        ls ${INSPECT_PWD} |awk '{print $1}' |grep 'inspect_' |grep -v common |while read line
        do
            ${INSPECT_PWD}${line} inspect_all
        done
    elif [ "X$func" == "X" ]; then
        ${INSPECT_PWD}"inspect_${mod}.sh" inspect_all
        return $?
    else
        ${INSPECT_PWD}"inspect_${mod}.sh" ${func}
        return $?
    fi
    return 0
}

if [ "X$1" == "X" ]; then
    main_help
    exit $?
else
    main_$*
    exit $?
fi
