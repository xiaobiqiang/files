#!/bin/bash 
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'
dir='/tmp/pool.xml'

function cm_cnm_pool_reconstruct_getbatch()
{
    zpool status -x 1>/dev/null 2>/dev/null
    local pool_num=`cat $dir | grep -n 'pool node'|wc -l |awk '{print $1}'`
    local start_line=(`cat $dir | grep -n 'pool node'|awk -F':' '{print $1}'`) 
    
    for(( num=0; num < pool_num ;num++ ))
    do
        first_ln=${start_line[num]}
        ((sta_line=$first_ln+9))
        status=`sed -n "${sta_line}p" $dir | awk -F'=' '{print $2}'|awk -F'""' '{print $2}'`
        if [ "$status" != 'none requested' ]; then
            finished=`sed -n "${sta_line}p" $dir | awk -F'=' '{print $2}'|awk -F'""' '{print $2}'|awk -F' ' '{print $1}'`
            if [ "$finished" == 'resilvered' ]; then
                speed="0M/s" 
                time_consum="0h0m"
                progress="100%"
                status="resilvered"
                pool_name=`sed -n "${first_ln}p" $dir |awk -F' ' '{print $3}'|awk -F'=' '{print $2}'|awk -F'""' '{print $2}'`
            elif [ "$finished" == 'resilver' ]; then
                speed=`sed -n "${sta_line}p" $dir |awk -F',' '{print $2}'|awk -F' ' '{print $7}'`
                local is_no_estimated=`sed -n "${sta_line}p" $dir|grep 'no estimated time'|wc -l`
                if [ $is_no_estimated -eq 1 ];then
                    time_consum="no_estimated_time"
                    progress=`sed -n "${sta_line}p" $dir |awk -F',' '{print $5}'|awk -F' ' '{print $1}'`
                else
                    time_consum=`sed -n "${sta_line}p" $dir |awk -F',' '{print $3}'|awk -F' ' '{print $1}'`
                    progress=`sed -n "${sta_line}p" $dir |awk -F',' '{print $4}'|awk -F' ' '{print $1}'`
                fi
                pool_name=`sed -n "${first_ln}p" $dir |awk -F' ' '{print $3}'|awk -F'=' '{print $2}'|awk -F'""' '{print $2}'`
                status=$finished
            else
                return $CM_OK
            fi
        echo "$pool_name $status $speed $time_consum $progress" 
        fi  
    done
    return $CM_OK
}

function cm_cnm_pool_reconstruct_count()
{
    zpool status -x 1>/dev/null 2>/dev/null
    local reconsnum=`cat $dir |grep 'scan description'|grep -v 'none requested'|grep 'resilver'|wc -l|awk -F' ' '{print $1}'`
    echo $reconsnum
    return $CM_OK
}


function main()
{
    local paramnum=$#
    local iRet=$CM_OK
    case $1 in
        getbatch)
        if [ $paramnum -ne 1 ]; then
            return $CM_PARAM_ERR
        fi
        cm_cnm_pool_reconstruct_getbatch
        iRet=$?
        ;;
        count)
        if [ $paramnum -ne 1 ]; then
            return $CM_PARAM_ERR
        fi
        cm_cnm_pool_reconstruct_count
        iRet=$?
        ;;
        *)
        ;;
    esac
    return $iRet
}
main $*
exit $?