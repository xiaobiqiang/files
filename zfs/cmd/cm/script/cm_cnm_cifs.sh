#!/bin/bash
source '/var/cm/script/cm_types.sh'
#==================================================================================
#==================================================================================
#                                  CIFS功能管理
# 使用说明:
# 批量查询  : cm_cnm_cifs.sh getbatch <dir> <offset> <total>
# 记录数查询: cm_cnm_cifs.sh count    <dir>
# 添加记录  : cm_cnm_cifs.sh add      <dir> <domain> <nametype> <name> <permission>
# 修改记录  : cm_cnm_cifs.sh update   <dir> <domain> <nametype> <name> <permission>
# 删除记录  : cm_cnm_cifs.sh delete   <dir> <domain> <nametype> <name>
# 精确查询  : cm_cnm_cifs.sh get      <dir> <domain> <nametype> <name>
#==================================================================================
#==================================================================================

#常量定义
CM_NAS_PERISSION_RW_SET="full_set"
CM_NAS_PERISSION_RO_SET="r-x---a-R-c--s"
CM_NAS_PERISSION_RD_SET="modify_set"

CM_NAME_USER_STR="user"
CM_NAME_GROUP_STR="group"

#==================================================================================
#                                  批量查询
# 输入参数:
#       <dir>  
#       <offset>
#       <total>
# 回显数据:
# index domain nametype name permission
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_cifs_getbatch()
{
    local dir=$1
    local offset=$2
    local total=$3
    local index=0
    local domain=$CM_DOMAIN_LOCAL
    
    ((index=$offset))
    #脚本里下标从1开始计算,并跳过第一行
    ((offset=$offset+2))
    #获取结束行号
    ((total=$offset+$total-1))
    
    local rows=`ls -dV "$dir" 2>/dev/null |sed -n "${offset},${total}p"`
    for row in $rows
    do
        #使用:分割
        local cols=${row//:/ }
        #转换为数组
        local colarr=($cols)
        local col_num=${#colarr[*]}
        
        local nametype=$CM_NAME_USER
        local permission=$CM_NAS_PERISSION_RW
        local name="unknow"
        
        if [ $col_num -eq 4 ]; then
            #everyone@ rwxpdDaARWc--s fd----- allow
            nametype=$CM_NAME_GROUP
            name=${colarr[0]}
            permission=${colarr[1]}
        elif [ $col_num -eq 5 ]; then
            if [ ${colarr[0]}"X" = "groupX" ]; then
                nametype=$CM_NAME_GROUP
            fi
            name=${colarr[1]}
            permission=${colarr[2]}
        else
            return $CM_FAIL
        fi
        
        #转换权限
        case $permission in
            "rwxpdDaARWcCos")
            permission=$CM_NAS_PERISSION_RW
            ;;
            "rwxpdDaARWc--s")
            permission=$CM_NAS_PERISSION_RW
            ;;
            "rwxp--aARWcCos")
            permission=$CM_NAS_PERISSION_RW
            ;;
            *)
            permission=$CM_NAS_PERISSION_RO
            ;;
        esac
        name=`/var/cm/script/cm_cnm_user.sh getnamebyid "$nametype" "$name"`
        echo "$index $permission $name"
        ((index=$index+1))
    done
    return $CM_OK
}

#==================================================================================
#                                  记录数查询
# 输入参数:
#       <dir>  
# 回显数据:
# 
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_cifs_count()
{
    local dir=$1
    local cnt=`ls -dV "$dir" 2>/dev/null |sed 1d |wc -l`
    
    echo "$cnt"
    return $CM_OK
}

#==================================================================================
#                                  获取用户名
# 输入参数:
#       <nametype>
#       <name>
# 回显数据:
#       <user|group:><name>
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_cifs_user_get()
{
    local nametype=$1
    local name=$2
    
    if [ $nametype -eq $CM_NAME_GROUP ]; then
        if [ "X${name:0-1:1}" = 'X@' ]; then
            user=$name
        else
            user=$CM_NAME_GROUP_STR":"$name
        fi
    else
        user=$CM_NAME_USER_STR":"$name
    fi
    echo $user
    return $CM_OK
}

#==================================================================================
#                                  获取权限
# 输入参数:
#       <permission>
# 回显数据:
#       <full_set|read_set>:allow
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_cifs_permission_get()
{
    local permission=$1
    local abe=$2
    local inherit=$3
    if [ $permission -eq $CM_NAS_PERISSION_RW ]; then
        permission=$CM_NAS_PERISSION_RW_SET
    elif [ $permission -eq $CM_NAS_PERISSION_RD ]; then
        permission=$CM_NAS_PERISSION_RD_SET
    else
        permission=$CM_NAS_PERISSION_RO_SET
    fi
    
    if [ "X$inherit" == "Xtrue" ]; then
        permission="$permission:fd-----:allow"
    elif [ "X$abe" == "Xabe=true" ]; then
        permission="$permission:allow"
    else
        permission="$permission:fd-----:allow"
    fi
    echo $permission
    return $CM_OK
}
#==================================================================================
#                                  添加记录
# 输入参数:
#       <dir>
#       <domain>
#       <nametype>
#       <name>
#       <permission>
# 回显数据:
#       null
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_cifs_add()
{
    local dir=$1
    local domain=$2
    local nametype=$3
    local name=$4
    local permission=$5
    local abe=$6
    local inherit=$7
    
    if [ $domain -eq $CM_DOMAIN_AD ]; then
        #AD域用户场景，需要将用户名转换为UID
        name=`/var/cm/script/cm_cnm_user.sh ad_getidbyname "$nametype" "$name"`
        if [ "X$name" == "X" ]; then
            return $CM_ERR_NOT_EXISTS
        fi
    fi
    
    local user=`cm_cnm_cifs_user_get $nametype "$name"`

    #检查是否存在
    local cnt=`ls -dV "$dir" 2>/dev/null |grep "${user}:" |wc -l`
    if [ $cnt -ne 0 ]; then
        return $CM_ERR_ALREADY_EXISTS
    fi
    
    permission=`cm_cnm_cifs_permission_get $permission "$abe" "$inherit"`
    
    #echo "add $dir $user:$permission"
    chmod A+${user}":"${permission} "$dir" 2>/dev/null
    #echo "add $dir $domain $nametype $name $permission"
    return $?
}

#==================================================================================
#                                  修改记录
# 输入参数:
#       <dir>
#       <domain>
#       <nametype>
#       <name>
#       <permission>
# 回显数据:
#       null
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_cifs_update()
{
    local dir=$1
    local domain=$2
    local nametype=$3
    local name=$4
    local permission=$5
    local abe=$6
    local inherit=$7
    local iRet=$CM_ERR_NOT_EXISTS
    
    if [ $domain -eq $CM_DOMAIN_AD ]; then
        #AD域用户场景，需要将用户名转换为UID
        name=`/var/cm/script/cm_cnm_user.sh ad_getidbyname "$nametype" "$name"`
        if [ "X$name" == "X" ]; then
            return $CM_ERR_NOT_EXISTS
        fi
    fi
    
    local user=`cm_cnm_cifs_user_get $nametype "$name"`
    
    permission=`cm_cnm_cifs_permission_get $permission "$abe" "$inherit"`
    
    local rows=`ls -dV "$dir" 2>/dev/null |grep "${user}:"`
    
    #删除再增加，ACL中可以通过指定index修改，但是查询Index太麻烦
    for row in $rows
    do
        chmod A-${row} "$dir" 2>/dev/null
        iRet=$?
    done
    
    if [ $iRet -eq $CM_OK ]; then
        chmod A+${user}":"${permission} "$dir" 2>/dev/null
        iRet=$?
    fi
    return $iRet
}

#==================================================================================
#                                  删除记录
# 输入参数:
#       <dir>
#       <domain>
#       <nametype>
#       <name>
# 回显数据:
#       null
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_cifs_delete()
{
    local dir=$1
    local domain=$2
    local nametype=$3
    local name=$4
    
    if [ $domain -eq $CM_DOMAIN_AD ]; then
        #AD域用户场景，需要将用户名转换为UID
        name=`/var/cm/script/cm_cnm_user.sh ad_getidbyname "$nametype" "$name"`
        if [ "X$name" == "X" ]; then
            return $CM_ERR_NOT_EXISTS
        fi
    fi
    
    local user=`cm_cnm_cifs_user_get $nametype "$name"`
    local iRet=$CM_ERR_NOT_EXISTS
    
    local rows=`ls -dV "$dir" 2>/dev/null |grep "${user}:"`
    for row in $rows
    do
        chmod A-${row} "$dir" 2>/dev/null
        iRet=$?
    done
    
    return $iRet
}

#==================================================================================
#                                  精确查询
# 输入参数:
#       <dir>
#       <domain>
#       <nametype>
#       <name>
# 回显数据:
#        index domain nametype name permission
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_cifs_get()
{
    local dir=$1
    local domain=$2
    local nametype=$3
    local name=$4
    if [ $domain -eq $CM_DOMAIN_AD ]; then
        #AD域用户场景，需要将用户名转换为UID
        name=`/var/cm/script/cm_cnm_user.sh ad_getidbyname "$nametype" "$name"`
        if [ "X$name" == "X" ]; then
            return $CM_ERR_NOT_EXISTS
        fi
    fi
    local user=`cm_cnm_cifs_user_get $nametype "$name"`
    local iRet=$CM_ERR_NOT_EXISTS
    
    local permission=`ls -dV "$dir" 2>/dev/null |grep "${user}:" |sed "s/${user}://g" |sed 's/ //g' |awk -F':' '{print $1}'`
    if [ "X$permission" == "X" ]; then
        return $CM_ERR_NOT_EXISTS
    fi
    
    #转换权限
    case $permission in
        "rwxpdDaARWcCos")
        permission=$CM_NAS_PERISSION_RW
        ;;
        "rwxpdDaARWc--s")
        permission=$CM_NAS_PERISSION_RW
        ;;
        *)
        permission=$CM_NAS_PERISSION_RO
        ;;
    esac
    name=`/var/cm/script/cm_cnm_user.sh ad_getnamebyid "$nametype" "$name"`
    echo "0 $permission $name"
    return $CM_OK
}

#==================================================================================
# 主函数
#==================================================================================
function cm_cnm_cifs_main()
{
    local paramnum=$#
    if [ $paramnum -lt 2 ]; then
        #echo "paramnum=$paramnum"
        return $CM_PARAM_ERR
    fi
    local cmd=$1
    local nasdir=$2
    local mountpoint=$nasdir
    local abe='off'
    local inherit='false'
    if [ "X${nasdir:0:1}" != 'X/' ]; then
        mountpoint=`zfs list -H -o mountpoint "$nasdir" 2>/dev/null`
        if [ "X$mountpoint" = "X" ]; then
            #echo "$nasdir not existed"
            return $CM_PARAM_ERR
        fi
        abe=`zfs get sharesmb $nasdir |sed '1d' |awk '{print $3}'`
        inherit=`zfs get aclinherit $nasdir |sed '1d' |awk '{print $3}'`
        if [ "X$inherit" == "Xpassthrough" ] || [ "X$inherit" == "Xpassthrough-x" ]; then
            inherit='true'
        else
            inherit='false'
        fi
    fi
    
    local iRet=$CM_ERR_NOT_SUPPORT
    
    case $cmd in
        getbatch)
        if [ $paramnum -lt 4 ]; then
            return $CM_PARAM_ERR
        fi
        cm_cnm_cifs_getbatch "$mountpoint" "$3" "$4"
        iRet=$?
        ;;
        count) 
        if [ $paramnum -lt 2 ]; then
            return $CM_PARAM_ERR
        fi
        cm_cnm_cifs_count "$mountpoint"
        iRet=$?
        ;;
        add)
        if [ $paramnum -lt 6 ]; then
            return $CM_PARAM_ERR
        fi
        cm_cnm_cifs_add "$mountpoint" "$3" "$4" "$5" "$6" "$abe" "$inherit"
        iRet=$?
        ;;
        update)
        if [ $paramnum -lt 6 ]; then
            return $CM_PARAM_ERR
        fi
        cm_cnm_cifs_update "$mountpoint" "$3" "$4" "$5" "$6" "$abe" "$inherit"
        iRet=$?
        ;;
        delete)
        if [ $paramnum -lt 5 ]; then
            return $CM_PARAM_ERR
        fi
        cm_cnm_cifs_delete "$mountpoint" "$3" "$4" "$5"
        iRet=$?
        ;;
        get)
        if [ $paramnum -lt 5 ]; then
            return $CM_PARAM_ERR
        fi
        cm_cnm_cifs_get "$mountpoint" "$3" "$4" "$5"
        iRet=$?
        ;;
        *)
        ;;
    esac
    return $iRet
}

#==================================================================================
cm_cnm_cifs_main "$1" "$2" "$3" "$4" "$5" "$6" "$7"
exit $?
