#!/bin/bash
source '/var/cm/script/cm_types.sh'
#==================================================================================
#==================================================================================
#                                  CIFS���ܹ���
# ʹ��˵��:
# ������ѯ  : cm_cnm_cifs.sh getbatch <dir> <offset> <total>
# ��¼����ѯ: cm_cnm_cifs.sh count    <dir>
# ��Ӽ�¼  : cm_cnm_cifs.sh add      <dir> <domain> <nametype> <name> <permission>
# �޸ļ�¼  : cm_cnm_cifs.sh update   <dir> <domain> <nametype> <name> <permission>
# ɾ����¼  : cm_cnm_cifs.sh delete   <dir> <domain> <nametype> <name>
# ��ȷ��ѯ  : cm_cnm_cifs.sh get      <dir> <domain> <nametype> <name>
#==================================================================================
#==================================================================================

#��������
CM_NAS_PERISSION_RW_SET="full_set"
CM_NAS_PERISSION_RO_SET="r-x---a-R-c--s"
CM_NAS_PERISSION_RD_SET="modify_set"

CM_NAME_USER_STR="user"
CM_NAME_GROUP_STR="group"

#==================================================================================
#                                  ������ѯ
# �������:
#       <dir>  
#       <offset>
#       <total>
# ��������:
# index domain nametype name permission
# �� �� ֵ:
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
    #�ű����±��1��ʼ����,��������һ��
    ((offset=$offset+2))
    #��ȡ�����к�
    ((total=$offset+$total-1))
    
    local rows=`ls -dV "$dir" 2>/dev/null |sed -n "${offset},${total}p"`
    for row in $rows
    do
        #ʹ��:�ָ�
        local cols=${row//:/ }
        #ת��Ϊ����
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
        
        #ת��Ȩ��
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
#                                  ��¼����ѯ
# �������:
#       <dir>  
# ��������:
# 
# �� �� ֵ:
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
#                                  ��ȡ�û���
# �������:
#       <nametype>
#       <name>
# ��������:
#       <user|group:><name>
# �� �� ֵ:
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
#                                  ��ȡȨ��
# �������:
#       <permission>
# ��������:
#       <full_set|read_set>:allow
# �� �� ֵ:
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
#                                  ��Ӽ�¼
# �������:
#       <dir>
#       <domain>
#       <nametype>
#       <name>
#       <permission>
# ��������:
#       null
# �� �� ֵ:
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
        #AD���û���������Ҫ���û���ת��ΪUID
        name=`/var/cm/script/cm_cnm_user.sh ad_getidbyname "$nametype" "$name"`
        if [ "X$name" == "X" ]; then
            return $CM_ERR_NOT_EXISTS
        fi
    fi
    
    local user=`cm_cnm_cifs_user_get $nametype "$name"`

    #����Ƿ����
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
#                                  �޸ļ�¼
# �������:
#       <dir>
#       <domain>
#       <nametype>
#       <name>
#       <permission>
# ��������:
#       null
# �� �� ֵ:
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
        #AD���û���������Ҫ���û���ת��ΪUID
        name=`/var/cm/script/cm_cnm_user.sh ad_getidbyname "$nametype" "$name"`
        if [ "X$name" == "X" ]; then
            return $CM_ERR_NOT_EXISTS
        fi
    fi
    
    local user=`cm_cnm_cifs_user_get $nametype "$name"`
    
    permission=`cm_cnm_cifs_permission_get $permission "$abe" "$inherit"`
    
    local rows=`ls -dV "$dir" 2>/dev/null |grep "${user}:"`
    
    #ɾ�������ӣ�ACL�п���ͨ��ָ��index�޸ģ����ǲ�ѯIndex̫�鷳
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
#                                  ɾ����¼
# �������:
#       <dir>
#       <domain>
#       <nametype>
#       <name>
# ��������:
#       null
# �� �� ֵ:
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
        #AD���û���������Ҫ���û���ת��ΪUID
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
#                                  ��ȷ��ѯ
# �������:
#       <dir>
#       <domain>
#       <nametype>
#       <name>
# ��������:
#        index domain nametype name permission
# �� �� ֵ:
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
        #AD���û���������Ҫ���û���ת��ΪUID
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
    
    #ת��Ȩ��
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
# ������
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
