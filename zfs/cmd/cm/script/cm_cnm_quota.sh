#!/bin/bash
source '/var/cm/script/cm_types.sh'

#==================================================================================
#                                  批量查询
# 输入参数:
#       <usertype>  
#       <filesystem>
# 回显数据:
# filesystem name space used
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_quota_getbatch()
{
    local usertype=$1
    local filesystem=$2
    local offset=$3
    local total=$4
    if [ "X$offset" == "X" ]; then
        offset=0
    fi
    if [ "X$total" == "X" ]; then
        total=100
    fi
    ((total=$offset+$total))
    ((offset=$offset+1))
    if [ $usertype -eq 0 ]; then
        array_name=($(cat /etc/passwd|awk -F':' '$3>99&&$3<50000{print $1}' |sed -n ${offset},${total}p))
        len=${#array_name[@]}
        for (( i=0; i<$len; i=i+1 ))
        do
            space=`zfs get userquota@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
            softspace=`zfs get softuserquota@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
            used=`zfs get userused@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
            echo "$filesystem ${array_name[i]} $space $softspace $used"
        done 
    fi

    if [ $usertype -eq 1 ]; then
        array_name=($(cat /etc/group|awk -F':' '$3==1||($3>99&&$3<1000){print $1}' |sed -n ${offset},${total}p))
        len=${#array_name[@]}
        for (( i=0; i<$len; i=i+1 ))
        do
            space=`zfs get groupquota@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
            softspace=`zfs get softgroupquota@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
            used=`zfs get groupused@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
            echo "$filesystem ${array_name[i]} $space $softspace $used"
        done 
    fi
    return $CM_OK
}

#==================================================================================
#                                  修改记录
# 输入参数:
#       <usertype>
#       <name>
#       <filesystem>
#       <space>
#		<softspace>
# 回显数据:
#       null
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_quota_update()
{
    local usertype=$1
    local name=$2
    local filesystem=$3
    local space=$4
    local softspace=$5
    local domain=$6
    local iret=$CM_OK
    
    if [ "X$name" == "X" ]; then
        return $CM_PARAM_ERR
    else
        /var/cm/script/cm_cnm_user.sh test "$domain" "$usertype" "$name"
        iret=$?
        if [ $iret -ne $CM_OK ]; then
            return $iret
        fi
    fi
    
    if [ $space != "null" ]; then
        if [ $usertype -eq 0 ]; then
            zfs set userquota@$name=$space $filesystem
            if [ $? -ne 0 ]; then
                return $CM_FAIL
            fi
        fi
        if [ $usertype -eq 1 ]; then
            zfs set groupquota@$name=$space $filesystem
            if [ $? -ne 0 ]; then
                return $CM_FAIL
            fi
        fi
    fi

    if [ $softspace != "null" ]; then
        if [ $usertype -eq 0 ]; then
            zfs set softuserquota@$name=$softspace $filesystem
            if [ $? -ne 0 ]; then
                return $CM_FAIL
            fi
        fi
        if [ $usertype -eq 1 ]; then
            zfs set softgroupquota@$name=$softspace $filesystem
            if [ $? -ne 0 ]; then
                return $CM_FAIL
            fi
        fi
    fi
    return $CM_OK
}

#==================================================================================
#                                  删除记录
# 输入参数:
#       <usertype>
#       <name>
#       <filesystem>
# 回显数据:
#       null
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_quota_delete()
{
	local usertype=$1
	local name=$2
	local filesystem=$3
	if [ $usertype -eq 0 ]; then
		zfs set userquota@$name=none $filesystem
		zfs set softuserquota@$name=none $filesystem
	fi
	
	if [ $usertype -eq 1 ]; then
		zfs set groupquota@$name=none $filesystem
		zfs set softgroupquota@$name=none $filesystem
	fi
	return $CM_OK
}

#==================================================================================
#                                  记录数查询
# 输入参数:
#       <usertype>
#		<filesystem>	
# 回显数据:
# 		cut
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_quota_count()
{
    local usertype=$1
    local filesystem=$2
    if [ $usertype -eq 0 ]; then
        cut=`cat /etc/passwd | awk -F':' '($3>99&&$3<50000) {print $3}'| wc -l`
        echo "$cut"
    fi

    if [ $usertype -eq 1 ]; then
        cut=`cat /etc/group |awk -F':' '$3==1||($3>99&&$3<1000)' |wc -l`
        echo "$cut"
    fi	
    return $CM_OK
}

cm_cnm_quota_"$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" 
exit $?
