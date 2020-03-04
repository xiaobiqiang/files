#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

function cm_cnm_user_insert_pwd()
{
	local user=$1
	local pwd=$2

/usr/bin/expect<<-EOF
spawn passwd $user
expect "*Password:"
send "$pwd\r"
expect "*Password:"
send "$pwd\r"
expect eof
EOF

}

function cm_cnm_user_insert()
{
	local user=$1
	local path=$2
	local uid=$3
	local gid=$4
	local iRet=$CM_OK
	

	if [ $path != "null" ]; then
		useradd -u $uid -g $gid -d $path $user
		iRet=$?
	else
		useradd -u $uid -g $gid $user
		iRet=$?
	fi

	return $iRet
}

function cm_cnm_user_update()
{
	local name=$1
	local gid=$2
	local path=$3
	local uid=$4
	local iRet=$CM_OK
	
	if [ $gid -ne 0 ]; then
		usermod -g $gid $name
		iRet=$?
		if [ $iRet != $CM_OK ];then
			return $CM_FAIL
		fi
	fi
	
	if [ $uid -ne 0 ]; then
		usermod -u $uid $name
		iRet=$?
		if [ $iRet != $CM_OK ];then
			return $CM_FAIL
		fi
	fi
	
	if [ $path != 'null' ]; then
		usermod -d $path $name
		iRet=$?
		if [ $iRet != $CM_OK ];then
			return $CM_FAIL
		fi
	fi
	
	return $iRet
}

function cm_cnm_user_request_false()
{
	local passwd=$1
	local shadow=$2
	local smbpasswd=$3
	
	echo $passwd>>/etc/passwd
	echo $shadow>>/etc/shadow
	echo $smbpasswd >>/var/smb/smbpasswd
}

function cm_cnm_user_request_true()
{
	local name=$1
	local shadow=$2
	local smbpasswd=$3
	
	sed "/$name:/d" /etc/shadow > /etc/shadow_copy
	echo "$shadow" >> /etc/shadow_copy
	mv /etc/shadow_copy /etc/shadow
	
	sed "/$name:/d" /var/smb/smbpasswd > /var/smb/smbpasswd_copy
	echo "$smbpasswd" >> /var/smb/smbpasswd_copy
	mv /var/smb/smbpasswd_copy /var/smb/smbpasswd
}

function cm_cnm_user_delete()
{
	local iRet=$CM_OK
	local uid=$1
	local name=`cat /etc/passwd |grep "x:$uid:"|awk -F':' '{print $1}'`
	
	sed "/$name/d" /var/smb/smbpasswd > /var/smb/smbpasswd_copy
	mv /var/smb/smbpasswd_copy /var/smb/smbpasswd
	userdel $name
	iRet=$?
	
	return $iRet
}

function cm_cnm_user_get()
{
	local name=$1
	cat /etc/passwd|grep "^$name:"|awk -F':' '{print $3" "$4" "$1" "$6}'
}

function cm_cnm_user_getbatch()
{
	local gid=$1
	if [ $gid = "null" ]; then
		cat /etc/passwd | awk -F':' '($3>99&&$3<50000) {print $3 " " $4 " " $1 " " $6}'|sort -n
	else
		cat /etc/passwd |awk -F':' '($3>99&&$3<50000)&&($4=='$gid') {print $3 " " $4 " " $1 " " $6}'|sort -n
	fi
}

function cm_cnm_user_count()
{
	local gid=$1
	if [ $gid = "null" ]; then
		cat /etc/passwd | awk -F':' '($3>99&&$3<50000) {print $3}'| wc -l
	else
		cat /etc/passwd |awk -F':' '($3>99&&$3<50000)&&($4=='$gid') {print $3}'| wc -l
	fi
}


function cm_cnm_user_maxid()
{
	local cut=`cat /etc/passwd| awk -F':' '{print $3}'| sort -n | awk -F':' 'BEGIN {max=99}{if(($1>99&&$1<50000)&&($1-max==1)) max=$1} END{printf max}'`
	((cut=$cut+1))
	echo $cut
}

function cm_cnm_user_explorer_init()
{
    mkdir -p '/tmp/explorer/flag'
    
    return $CM_OK
}

function cm_cnm_user_explorer_back()
{
    local flagdir='/tmp/explorer/flag/'
    local tmpdir='/tmp/explorer/'
    local dir=$1
    local name=$2
    local find=$3
    
    touch $flagdir$name"_flag"
    ls -1f "$dir" |grep "$find" > $tmpdir$name
    #find $dir -name "*$find*" -exec basename {} \; > $tmpdir$name
    rm $flagdir$name"_flag"
}

function cm_cnm_user_explorer_create()
{
    local tmpdir='/tmp/explorer/'
    local dir=$1
    local name=$2
    local find=$3
    
    if [ ! -d $dir ]; then
        return $CM_FAIL
    fi
    cm_cnm_user_explorer_back $dir $name $find &
    
    return $CM_OK
}

function cm_cnm_user_explorer_count()
{
    local tmpdir='/tmp/explorer/'
    local name=$1
    cat $tmpdir"/"$name | wc -l
    
    return $CM_OK
}

function cm_cnm_user_explorer_count_flag()
{
    local flagdir='/tmp/explorer/flag/'
    local name=$1
    
    if [ -f $flagdir$name"_flag" ]; then
        echo 1
    else    
        echo 0
    fi
}

function cm_cnm_user_isexist()
{
    local id=$1
    local type=$2 #0:user 1:group
    local name
    
    if [ $type -eq 0 ]; then
        name=`cat /etc/passwd | awk -F':' '$3=='$id' {printf $1}'`
        if [ `cat /etc/passwd | grep -w "$name" | wc -l` -eq 0 ]; then
            return $CM_FAIL
        fi
        
        return $CM_OK
    fi
    
    if [ $type -eq 1 ]; then
        name=`cat /etc/group | awk -F':' '$3=='$id' {printf $1}'`
        if [ `cat /etc/group | grep -w "$name" | wc -l` -eq 0 ]; then
            return $CM_FAIL
        fi
        
        return $CM_OK
    fi
}

function cm_cnm_user_cache_getbatch()
{
    local domain=$1
    local type=$2
    
    if [ "X$domain" == "X" ] || [ "X$type" == "X" ]; then
        return $CM_PARAM_ERR
    fi
    if [ $domain -eq $CM_DOMAIN_LOCAL ]; then
        if [ $type -eq $CM_NAME_USER ]; then
            cat /etc/passwd |awk -F':' '($3>=100&&$3<=60000){print $3" "$1}'
        else
            cat /etc/group |awk -F':' '($3>=100&&$3<=60000){print $3" "$1}'
        fi
    elif [ $domain -eq $CM_DOMAIN_AD ]; then
        local dname=`smbadm list |sed -n 2p |awk '{print $2}' |sed 's/\[//g' |sed 's/\]//g'`
        if [ "X$dname" == "X" ]; then
            return $CM_OK
        fi
        if [ $type -eq $CM_NAME_USER ]; then
            idmap dump -n |egrep "^winuser:.*@"$dname |awk -F':' '{print $3" "$2}' |awk -F'@' '{print $1}'
        else
            idmap dump -n |egrep "^wingroup:.*@"$dname |awk -F':' '{print $3" "$2}' |awk -F'@' '{print $1}'
        fi
    else
        return $CM_OK
    fi
    return $CM_OK
}

function cm_cnm_user_cache_count()
{
    local domain=$1
    local type=$2
    
    if [ "X$domain" == "X" ] || [ "X$type" == "X" ]; then
        return $CM_PARAM_ERR
    fi
    if [ $domain -eq $CM_DOMAIN_LOCAL ]; then
        if [ $type -eq $CM_NAME_USER ]; then
            cat /etc/passwd |awk -F':' 'BEGIN{cnt=0}($3>=100&&$3<=60000){cnt++}END{print cnt}'
        else
            cat /etc/group |awk -F':' 'BEGIN{cnt=0}($3>=100&&$3<=60000){cnt++}END{print cnt}'
        fi
    elif [ $domain -eq $CM_DOMAIN_AD ]; then
        local dname=`smbadm list |sed -n 2p |awk '{print $2}' |sed 's/\[//g' |sed 's/\]//g'`
        if [ "X$dname" == "X" ]; then
            echo '0'
            return $CM_OK
        fi
        if [ $type -eq $CM_NAME_USER ]; then
            idmap dump -n |egrep "^winuser:.*@"$dname |wc -l |awk '{print $1}'
        else
            idmap dump -n |egrep "^wingroup:.*@"$dname |wc -l |awk '{print $1}'
        fi
    else
        echo '0'
    fi
    return $CM_OK
}

function cm_cnm_user_ad_getidbyname()
{
    local uname=$2
    local utype=$1
    
    if [ "X$utype" == "X" ] || [ "X$uname" == "X" ]; then
        return $CM_PARAM_ERR
    fi
    
    if [ $utype -eq $CM_NAME_USER ]; then
        utype="winuser"
    else
        utype="wingroup"
    fi
    #获取域名
    local dname=`smbadm list |sed -n 2p |awk '{print $2}' |sed 's/\[//g' |sed 's/\]//g'`
    if [ "X$dname" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $utype $uname domain null"
        return $CM_ERR_NOT_EXISTS
    fi
    local uid=`idmap show -c ${utype}:"${uname}"@${dname} 2>/dev/null|awk -F':' '{print $3}'`
    if [ "X$uid" == "X60001" ];then
        CM_LOG "[${FUNCNAME}:${LINENO}] $utype $uname $uid"
        return $CM_ERR_NOT_EXISTS
    fi
    echo "$uid"
    return $CM_OK
}

function cm_cnm_user_getnamebyid_ad()
{
    local uid=$2
    local utype=$1
    local idtype=""
    local utypestr=""
    
    if [ "X$utype" == "X" ] || [ "X$uid" == "X" ]; then
        return $CM_PARAM_ERR
    fi
    
    if [ $utype -eq $CM_NAME_USER ]; then
        utypestr="winuser"
        idtype="uid"
    else
        utypestr="wingroup"
        idtype="gid"
    fi
    local uname=`idmap show -c ${idtype}:${uid} ${utypestr} 2>/dev/null |awk -F':' '{print $3}' |awk -F'@' '{print $1}'`
    if [ "X$uname" == "X" ]; then
        uname="$uid"
    fi
    echo "$uname"
    return $CM_OK
}

function cm_cnm_user_getnamebyid()
{
    local uid=$2
    local utype=$1
    local idtype=""
    local utypestr=""
    
    if [ "X$utype" == "X" ] || [ "X$uid" == "X" ]; then
        return $CM_PARAM_ERR
    fi
    
    cm_check_isnum "$uid"
    local iret=$?
    if [ $iret -eq 1 ]; then
        echo "$CM_DOMAIN_LOCAL $utype $uid"
        return $CM_OK
    fi
    
    if [ $utype -eq $CM_NAME_USER ]; then
        utypestr="winuser"
        idtype="uid"
    else
        utypestr="wingroup"
        idtype="gid"
    fi
    local uname=`idmap show -c ${idtype}:${uid} ${utypestr} 2>/dev/null |awk -F':' '{print $3}' |awk -F'@' '{print $1}'`
    if [ "X$uname" == "X" ]; then
        echo "$CM_DOMAIN_LOCAL $utype $uid"
    else
        echo "$CM_DOMAIN_AD $utype $uname"
    fi
    return $CM_OK
}

function cm_cnm_user_test()
{
    local uname=$3
    local utype=$2
    local domain=$1
    local cnt=0
    if [ $domain -eq $CM_DOMAIN_LOCAL ]; then
        if [ $utype -eq $CM_NAME_USER ]; then
            cnt=`egrep "^${uname}:" /etc/passwd |wc -l |awk '{print $1}'`
        else
            cnt=`egrep "^${uname}:" /etc/group |wc -l |awk '{print $1}'`
        fi
    elif [ $domain -eq $CM_DOMAIN_AD ]; then
        cm_cnm_user_ad_getidbyname "$utype" "$uname" >/dev/null
        if [ $? -eq $CM_OK ]; then
            cnt=1
        else
            cnt=0
        fi
    else
        return $CM_ERR_NOT_EXISTS
    fi
    
    if [ $cnt -eq 0 ]; then
        return $CM_ERR_NOT_EXISTS
    fi
    return $CM_OK
}

cm_cnm_user_"$1" "$2" "$3" "$4" "$5" "$6" "$7" "$7"
exit $?
