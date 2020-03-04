#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

function cm_cnm_user_insert_pwd()
{
	local user=$1
	local pwd=$2

    echo $user:$pwd | chpasswd

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
	#echo $smbpasswd >>/var/smb/smbpasswd
}

function cm_cnm_user_request_true()
{
	local name=$1
	local shadow=$2
	local smbpasswd=$3
	
	sed "/$name:/d" /etc/shadow > /etc/shadow_copy
	echo "$shadow" >> /etc/shadow_copy
	mv /etc/shadow_copy /etc/shadow
	
	#sed "/$name:/d" /var/smb/smbpasswd > /var/smb/smbpasswd_copy
	#echo "$smbpasswd" >> /var/smb/smbpasswd_copy
	#mv /var/smb/smbpasswd_copy /var/smb/smbpasswd
}

function cm_cnm_user_delete()
{
	local iRet=$CM_OK
	local uid=$1
	local name=`cat /etc/passwd |grep "x:$uid:"|awk -F':' '{print $1}'`
	
	#sed "/$name/d" /var/smb/smbpasswd > /var/smb/smbpasswd_copy
	#mv /var/smb/smbpasswd_copy /var/smb/smbpasswd
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

cm_cnm_user_$*
exit $?
