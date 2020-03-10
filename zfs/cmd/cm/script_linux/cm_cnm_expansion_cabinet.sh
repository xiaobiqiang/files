#!/bin/bash

CM_CNM_CABINET_SBB_SOLT=16
CM_CNM_CABINET_AIC_SOLT=24

cm_cnm_cabinet_listids=($(sqlite3 /var/cm/data/cm_db_disk.db "select distinct enid from record_t where enid>22 and enid<1000"))
cm_cnm_cabinet_sbbid=$(sqlite3 /var/cm/data/cm_node.db "select sbbid from record_t WHERE idx=$(hostid)")

function cm_cnm_expansion_cabinet_main()
{
    local i=1
    local ret=null
    local sys_type=2
    local dev_type=3
    local U_num=24
    local slot=1
    local enid=0
    sys_type=`prtdiag -v|sed -n 1p|awk -F':' '{print $2}'|awk '{print $1}'`
    if [ "$sys_type"x  == "AIC"x ]; then
        dev_type=1
        U_num=4
        slot=24
    else
        dev_type=0
        U_num=2
        slot=16
    fi
    echo $cm_cnm_cabinet_sbbid $enid $dev_type $U_num $slot
    dev_type=2
    slot=24
    for listid in ${cm_cnm_cabinet_listids[@]}
    do      
        ((enid=$listid-22))
         U_num=`sg_ses --page=0 /dev/es/ses$i|sed -n 1p|awk '{print $3}'|awk -F'U' '{print $1}'`
         if [ "X${U_num}" == "X" ]; then
            U_num=4
         fi
         echo $cm_cnm_cabinet_sbbid $enid $dev_type $U_num $slot
        ((i=$i+1))
    done
}

if [ "X$1" == "Xcount" ]; then
    ls /dev/es/ |wc -l
else
    cm_cnm_expansion_cabinet_main
fi
exit 0
