#!/bin/bash
#!/bin/awk
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

LOGTXT=/var/cm/log/.logtxt

function cm_cnm_check_ipaddr()
{
    local ipaddr=$1
    echo $ipaddr|grep "^[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}$" > /dev/null
    local ret=$?
    if [ $ret -ne 0 ]
    then
        CM_LOG "[${FUNCNAME}:${LINENO}]IP PARAM ERR"
        return $CM_PARAM_ERR
    fi
    OLD_IFS="$IFS" #IFS is fen ge fu
        IFS="."
        local arr=($ipaddr)
        IFS="$OLD_IFS"
        for num in ${arr[@]}
        do
        if [ $num -gt 255 ] || [ $num -lt 0 ];then
            CM_LOG "[${FUNCNAME}:${LINENO}]IP NUMBER ERR"
            return $CM_PARAM_ERR
        fi
        done
    return $CM_OK
}

function cm_cnm_deal_info()
{
    local src_info=$1
    if [[ "$src_info"x = "off"x ]];then
        echo total:0lines>$LOGTXT
        return $CM_ERR_NOT_EXISTS
    elif [[ $src_info =~ r[wo],anon ]];then
        echo total:1lines>$LOGTXT
        echo "${src_info%,*}        all        2">>$LOGTXT
        return $CM_ERR_NOT_EXISTS
    elif [[ $src_info =~ r[wo]=* ]];then
        OLD_IFS="$IFS" #IFS is fen ge fu
        IFS=","
        local ip_list=($src_info)
        IFS="$OLD_IFS"
        for sub_list in ${ip_list[@]}
        do
            if [[ $sub_list == root=* ]];then
                TMP_IFS="$IFS" #IFS is fen ge fu
                IFS=":"
                local root_ip_list=(${sub_list#*=})
                IFS="$TMP_IFS"
                echo total:${#root_ip_list[@]}lines>$LOGTXT
            elif [[ $sub_list == rw=* ]];then
                TMP_IFS="$IFS" #IFS is fen ge fu
                IFS=":"
                local rw_ip_list=(${sub_list#*=})
                IFS="$TMP_IFS"
                for ip in ${rw_ip_list[@]}
                do
                    echo "rw  $ip  0">>$LOGTXT
                done
            elif [[ $sub_list == ro=* ]];then
                TMP_IFS="$IFS" #IFS is fen ge fu
                IFS=":"
                local ro_ip_list=(${sub_list#*=})
                IFS="$TMP_IFS"
                for ip in ${ro_ip_list[@]}
                do
                    echo "ro  $ip  0">>$LOGTXT
                done
            else
                echo total:0lines>$LOGTXT
                return $CM_PARAM_ERR
            fi
        done
    else
        echo total:0lines>$LOGTXT
        return $CM_PARAM_ERR
    fi
    return $CM_OK
}

function cm_cnm_nfs_getbatch()
{
    local nas_info=$1
    local offset=`expr $2 + 1`
    local tatol=$3
    cm_cnm_deal_info $nas_info
    local ret=$?
    case $ret in
        $CM_OK)
            local end=`expr $offset + $tatol`
            awk "NR>$offset && NR<=$end" $LOGTXT
        ;;
        $CM_PARAM_ERR)
        return $CM_PARAM_ERR
        ;;
        $CM_ERR_NOT_EXISTS)
            awk "NR>1 && NR<=2" $LOGTXT
            return $CM_OK
        ;;
    esac
    return $CM_OK
}

function cm_cnm_nfs_count()
{
    local nas_info=$1
    cm_cnm_deal_info $nas_info
    local ret=$?
    case $ret in
        $CM_OK)
            local str_lines=`head -1 $LOGTXT`
            local tmp_lines=${str_lines#*:}
            local lines=${tmp_lines%lines}
            echo $lines
        return $CM_OK
        ;;
        $CM_PARAM_ERR)
            echo 0
            return $CM_PARAM_ERR
        ;;
        $CM_ERR_NOT_EXISTS)
            local str_lines=`head -1 $LOGTXT`
            local tmp_lines=${str_lines#*:}
            local lines=${tmp_lines%lines}
            echo $lines
            return $CM_ERR_NOT_EXISTS
        ;;
    esac
    return $CM_OK
}

function cm_cnm_nfs_add()
{
    #cmd List_result <permission> <ip> <path>
    local nas_info=$1
    local perm=$2
    local ipaddr=$3
    local path=$4
    cm_cnm_deal_info $nas_info
    local ret=$?
    if [[ "$ipaddr"x == "all"x ]];then
        zfs set sharenfs=$perm,anon=0 $path
        ret=$?
        if [ $ret -ne 0 ];then
            CM_LOG "[${FUNCNAME}:${LINENO}]zfs set anon fail"
            return $CM_FAIL
        fi
        return $CM_OK
    fi
    cm_cnm_check_ipaddr $ipaddr
    if [ $?x = ${CM_PARAM_ERR}x ];then
        return $CM_PARAM_ERR
    fi
    if [[ ! $perm == r[wo] ]];then
        return $CM_PARAM_ERR
    fi

    case $ret in
        $CM_OK)
            TMP_IFS="$IFS" #IFS is fen ge fu
            IFS=","
            local ip_list=($nas_info)
            IFS="$TMP_IFS"

            if [[ ${#ip_list[*]} -lt 2 ]];then
                return $CM_PARAM_ERR
            fi

            local flag=0
            for ((i=0;i < ${#ip_list[*]};i=$i+1));
            do
                if [[ ${ip_list[$i]} == "$perm"=* ]];then
                    if [[ ${ip_list[$i]} =~ $ipaddr ]];then
                        return $CM_OK
                    else
                        flag=1
                        ip_list[$i]=${ip_list[$i]}:$ipaddr
                    fi
                else
                    ip_list[$i]=`echo ${ip_list[$i]}|sed "s/:$ipaddr//g"|sed "s/$ipaddr://g"|sed "s/$ipaddr//g"`
                fi
            done
            if [[ $flag -eq 0 && "$perm"x == "ro"x ]];then
                if [[ ${ip_list[1]} == "rw=" ]];then
                    ip_list[1]=ro=$ipaddr
                else
                    ip_list[2]=ro=$ipaddr
                fi
            fi
            if [[ $flag -eq 0 && "$perm"x == "rw"x ]];then
                if !([[ ${ip_list[1]} == "ro=" ]]);then
                    ip_list[2]=${ip_list[1]}
                fi
                ip_list[1]=rw=$ipaddr
            fi
            if [[ ${ip_list[0]} == "root=" ]];then
                ip_list[0]="root=$ipaddr"
            else
                ip_list[0]=${ip_list[0]}:$ipaddr
            fi

            local logbol_cmd=${ip_list[0]}
            for ((i=1;i < ${#ip_list[*]};i=$i+1));
            do
                if [[ "${ip_list[$i]}"x == "rw="x || "${ip_list[$i]}"x == "ro="x ]];then
                    continue
                else
                    logbol_cmd=$logbol_cmd,${ip_list[$i]}
                fi
            done

            zfs set sharenfs=$logbol_cmd $path
            ret=$?
            if [ $ret -ne 0 ];then
                CM_LOG "[${FUNCNAME}:${LINENO}]zfs set ip fail"
                return $CM_FAIL
            fi
            cm_cnm_deal_info $logbol_cmd
            return $CM_OK
        ;;
        $CM_PARAM_ERR)
             return $CM_PARAM_ERR
        ;;
        $CM_ERR_NOT_EXISTS)
            zfs set sharenfs=root=$ipaddr,$perm=$ipaddr $path
            ret=$?
            if [ $ret -ne 0 ];then
                CM_LOG "[${FUNCNAME}:${LINENO}]zfs set ip fail"
                return $CM_FAIL
            fi
            return $CM_OK
        ;;
    esac
    return $CM_OK
}

function cm_cnm_nfs_delet()
{
    local nas_info=$1
    local ipaddr=$2
    local path=$3
    cm_cnm_deal_info $nas_info
    local ret=$?
    if [[ "$ipaddr"x == "all"x ]];then
        zfs set sharenfs=off $path
        ret=$?
        if [ $ret -ne 0 ];then
            CM_LOG "[${FUNCNAME}:${LINENO}]zfs set off fail"
            return $CM_FAIL
        fi
        return $CM_OK
    fi
    cm_cnm_check_ipaddr $ipaddr
    if [ $?x = ${CM_PARAM_ERR}x ];then
        return $CM_PARAM_ERR
    fi

    case $ret in
        $CM_OK)
            TMP_IFS="$IFS"
            IFS=","
            local ip_list=($nas_info)
            IFS="$TMP_IFS"
            local flag=0
            for ((i=0;i < ${#ip_list[*]};i=$i+1));
            do
                if [[ ${ip_list[$i]} =~ $ipaddr ]];then
                    flag=1
                    ip_list[$i]=`echo ${ip_list[$i]}|sed "s/:$ipaddr//g"|sed "s/$ipaddr://g"|sed "s/$ipaddr//g"`
                fi
            done
            if [[ $flag -eq 0 ]];then
                return $CM_OK
            fi
            local root_lenth=`echo ${ip_list[0]}|wc -L`
            if [ $root_lenth -lt 12 ];then
                zfs set sharenfs=off $path
                if [ $ret -ne 0 ];then
                    CM_LOG "[${FUNCNAME}:${LINENO}]zfs set ip fail"
                    return $CM_FAIL
                fi
                return  $CM_OK
            fi
            local logbol_cmd=${ip_list[0]}

            for ((i=1;i < ${#ip_list[*]};i=$i+1));
            do
                if [[ "${ip_list[$i]}"x == "rw="x || "${ip_list[$i]}"x == "ro="x ]];then
                    continue
                else
                    logbol_cmd=$logbol_cmd,${ip_list[$i]}
                fi
            done

            zfs set sharenfs=$logbol_cmd $path
            if [ $ret -ne 0 ];then
                CM_LOG "[${FUNCNAME}:${LINENO}]zfs set off fail"
                return $CM_FAIL
            fi
            cm_cnm_deal_info $logbol_cmd
            return $CM_OK
        ;;
        $CM_PARAM_ERR)
            return $CM_PARAM_ERR
        ;;
        $CM_ERR_NOT_EXISTS)
            return $CM_ERR_NOT_EXISTS
        ;;
    esac

    return $CM_OK
}

function cm_cnm_nfs_chstatus()
{
    #cmd <status> <path> permission
    local status=$1
    local path=$2
    local perm=$3
    if [[ "$status"x -ne "off"x && "$status" -ne "anon"x ]];then
        return $CM_PARAM_ERR
    fi
    if [[ "$status"x == "off"x ]];then
        zfs set sharenfs=off $path
        if [ $ret -ne 0 ];then
            CM_LOG "[${FUNCNAME}:${LINENO}]zfs set off fail"
            return $CM_FAIL
        fi
    fi

    if [[ "$status"x == "anon"x ]];then
        if [[ "$perm"x -ne "rw"x && "$perm"x -ne "ro"x ]];then
            return $CM_PARAM_ERR
        fi
        zfs set sharenfs=$perm,$status=0 $path
        if [ $ret -ne 0 ];then
            CM_LOG "[${FUNCNAME}:${LINENO}]zfs set anon fail"
            return $CM_FAIL
        fi
    fi
    return $CM_OK
}

function cm_cnm_main()
{
    local paranum=$#
    if [ $paranum -lt 2 ];then
            return $CM_PARAM_ERR
    fi
    local cmd=$1
    local nasdir=$2

    local nas_info=`zfs list -H -o sharenfs $nasdir 2>/dev/null`
    if [ "$nas_info"x = ""x ];then
        echo total:0lines>$LOGTXT
        CM_LOG "[${FUNCNAME}:${LINENO}]ERR PATH"
        return $CM_PARAM_ERR
    fi

    if [ "$nas_info"x = "-"x ];then
        echo tatol=0>$LOGTXT
        CM_LOG "[${FUNCNAME}:${LINENO}]THIS PATH IS SAN"
        return $CM_PARAM_ERR
    fi

    local iRet=$CM_ERR_NOT_SUPPORT

    case $cmd in
        getbatch)
            if [ $paranum -ne 4 ];then
                return $CM_PARAM_ERR
            fi
            #cmd List_result [offset] [total]
            cm_cnm_nfs_getbatch "$nas_info" "$3" "$4"
            iRet=$?
        ;;
        count)
            if [ $paranum -ne 2 ];then
                return $CM_PARAM_ERR
            fi
            #cmd List_result
            cm_cnm_nfs_count "$nas_info"
            iRet=$?
        ;;
        add)
            if [ $paranum -ne 4 ];then
                return $CM_PARAM_ERR
            fi
            #cmd List_result <permission> <ip> <path>
            cm_cnm_nfs_add "$nas_info" "$3" "$4" "$nasdir"
            iRet=$?
        ;;
        delet)
            if [ $paranum -ne 3 ];then
                return $CM_PARAM_ERR
            fi
            #cmd List_result <ip> <path>
            cm_cnm_nfs_delet "$nas_info" "$3" "$nasdir"
            iRet=$?
        ;;
        chstatus)
            if [ $paranum -lt 3 ];then
                return $CM_PARAM_ERR
            fi
            if [ $paranum -gt 4 ];then
                return $CM_PARAM_ERR
            fi
            #cmd <status> <path> permission
            cm_cnm_nfs_chstatus "$3" "$nasdir" "$4"
            iRet=$?
        ;;
    esac
    return $iRet
}

cm_cnm_main $1 $2 $3 $4
exit $?