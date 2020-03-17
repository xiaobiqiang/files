#!/bin/bash

source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'
CM_MOD_NAME=`basename $0 |sed 's/.sh$//g'`
CM_SCRPIT_LOCAL=$(dirname `readlink -f $0`)/$(basename $0)
CM_COPY_DB='/var/cm/data/cm_nascopy.db'
CM_NODE_DB='/var/cm/data/cm_node.db'

function cm_cnm_domain_ad_set_krb5()
{
    local cfgfile='/etc/krb5/krb5.conf'
    local backfile=${cfgfile}".backup"
    local tmpfile=${cfgfile}".tmp"
    local domainname=$1
    local domainupcase=`echo $domainname |tr '[a-z]' '[A-Z]'`
    local dc=$2
    if [ ! -f $cfgfile ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]$cfgfile not found!"
        return $CM_FAIL
    fi
    
    if [ ! -f $backfile ]; then
        cp $cfgfile $backfile
    fi
    
    CM_LOG "[${FUNCNAME}:${LINENO}]set default_realm[$domainname] dc[$dc]"
cat > $tmpfile<<_set_krb5_
[libdefaults]
    default_realm = ${domainupcase}

[realms]
    ${domainupcase} = {
                    kdc = ${dc}.${domainname}
           admin_server = ${dc}.${domainname}
         kpasswd_server = ${dc}.${domainname}
       kpasswd_protocol = SET_CHANGE
    }

[domain_realm]
    .${domainname} = ${domainupcase}

[logging]
    default = FILE:/var/krb5/kdc.log
    kdc = FILE:/var/krb5/kdc.log
    kdc_rotate = {
          period = 1d
        versions = 10
    }

[appdefaults]
    kinit = {
          renewable = true
        forwardable = true
    }

_set_krb5_
    mv $tmpfile $cfgfile
    
    #修改/etc/resolv.conf
    cfgfile='/etc/resolv.conf'
    tmpfile=$cfgfile".tmp"
    local domain=`grep ^domain $cfgfile`
    if [ "X$domain" == "X" ]; then
        echo "domain $domainname" >>$cfgfile
        CM_LOG "[${FUNCNAME}:${LINENO}]add [$domainname]"
    else
        sed 's/domain.*/domain '$domainname'/g' $cfgfile >$tmpfile
        mv $tmpfile $cfgfile
        CM_LOG "[${FUNCNAME}:${LINENO}]update [$domainname]"
    fi
    return $CM_OK
}

function cm_cnm_dnsserver_get()
{
    #ipmaster ipslave
    local cfgfile='/etc/resolv.conf'
    cat $cfgfile |awk '$1=="nameserver"{printf $2" "}END{print ""}' |sed '/^$/d'
    return $CM_OK
}
function cm_cnm_dnsserver_set()
{
    local ipmaster=$1
    local ipslave=$2
    local cfgfile='/etc/resolv.conf'
    local tmpfile=$cfgfile".tmp"
    local backfile=${cfgfile}".backup"
    local dnsback="/etc/nsswitch.conf.backup"
    local krbcfgfile='/etc/krb5/krb5.conf'
    local krbbackfile=${krbcfgfile}".backup"
    
    CM_LOG "[${FUNCNAME}:${LINENO}]set [$ipmaster] [$ipslave]"
    sed '/nameserver/d' $cfgfile > $tmpfile
    
    if [ ! -f $backfile ]; then
        cp $cfgfile $backfile
    fi
    
    if [ ! -f $dnsback ]; then
        cp /etc/nsswitch.dns $dnsback
    fi 

    if [ ! -f $krbbackfile ]; then
        cp $krbcfgfile $krbbackfile
    fi    
    
    if [ "X${ipmaster}" != "X" ]; then
        echo "nameserver $ipmaster" >> $tmpfile
    fi
    if [ "X${ipslave}" != "X" ]; then
        echo "nameserver $ipslave" >> $tmpfile
    fi
    mv $tmpfile $cfgfile
    
    cm_multi_exec "cp /etc/nsswitch.dns /etc/nsswitch.conf; sharectl set -p lmauth_level=2 smb"
    return $CM_OK
}

#AD域查询
function cm_cnm_domain_ad_get()
{
    local cfgfile='/etc/krb5/krb5.conf'
    local domain_ctl=`cat $cfgfile|grep -w 'admin_server'|awk -F'.' '{print $1}'`
    local domain=`cat $cfgfile|grep -w 'admin_server'|sed "s/$domain_ctl.//g"`
    domain_ctl=`echo $domain_ctl|awk -F'=' '{print $2}'`
    local state
    if [ `smbadm list|grep $domain|wc -l` -lt 2 ]; then
        state='off'
    else
        state='on'
    fi
    echo "$domain $domain_ctl $state"
    return 0
}

#AD域配置
function cm_cnm_domain_ad_set_local()
{
    local uname=$2
    local upwd=$3
    local domain=$1
/usr/bin/expect <<EOF
set timeout 5
spawn smbadm join -u $uname $domain
expect {
    "no" {
    send "yes\n"
    expect "password"
    send "$upwd\n"
    }
    "password" {
    send "$upwd\n"
    }
}
expect {
    "failed" {
    puts stderr "Join Failed"
    close
    exit 1
    }
    
    "Successful" {
    puts stderr "Join Successful"
    close
    exit 0
    }
}
expect eof
catch wait ret
exit [lindex \$ret 3]
EOF
    local iRet=$?
    CM_LOG "[${FUNCNAME}:${LINENO}]$domain $uname iRet[$iRet]"
    return $iRet
}

function cm_cnm_domain_ad_exit()
{
    local krbcfgfile='/etc/krb5/krb5.conf'
    local krbbackfile=${krbcfgfile}".backup"
    local dnsmcfgfile='/etc/resolv.conf'
    local dnsmbackfile=${dnsmcfgfile}".backup"
    local dnsback="/etc/nsswitch.conf.backup"
    
    if [ -f $krbbackfile ]; then
        cm_multi_exec "cp $krbbackfile $krbcfgfile"
    fi
    
    if [ -f $dnsmbackfile ]; then
        cm_multi_exec "cp $dnsmbackfile $dnsmcfgfile"
    fi

    if [ -f $dnsback ]; then
        cm_multi_exec "cp $dnsback /etc/nsswitch.dns"
        cm_multi_exec "cp $dnsback /etc/nsswitch.conf;sharectl set -p lmauth_level=4 smb"
    fi  

/usr/bin/expect <<EOF
set timeout 5
spawn smbadm join -w workgroup
expect {
    "no" {
    send "yes\n"
    }
}
expect eof
catch wait ret
exit [lindex \$ret 3]
EOF
}

function cm_cnm_domain_ad_set()
{
    cm_multi_exec "$CM_SCRPIT_LOCAL domain_ad_set_local $*" &
    return $CM_OK
}

function cm_cnm_domain_config_set()
{
    cm_multi_send /etc/krb5/krb5.conf /etc/krb5
    cm_multi_send /etc/resolv.conf /etc/
    return $CM_OK
}

function cm_cnm_domain_cluster_exit()
{
    cm_multi_exec "$CM_SCRPIT_LOCAL domain_ad_exit"
    return $CM_OK
}


function cm_cnm_zpool_data_change_m()
{
    local val=$1
    local final=${val: -1}
    local relval=${val%?}
    case $final in
        P)
            relval=`echo $relval*1024*1024*1024 |bc`
        ;;
        T)
            relval=`echo $relval*1024*1024 |bc`
        ;;
        G)
            relval=`echo $relval*1024 |bc`
        ;;
        M)
        ;;
        *)
        relval=0
        ;;
    esac
    echo $relval |awk -F'.' '{print $1}'
    return 0
}

function cm_cnm_zpool_data_back_m()
{
    local val=$1
    local final=('M' 'G' 'T' 'P')
    local index=0
    
    while((1))
    do
        if [ `echo "$val/1024"|bc` -ge 1 ]; then
            val=`echo "scale=2;$val/1024" |bc`
            ((index++))
        else
            break
        fi        
    done
    echo "$val${final[$index]}"
    return 0
}

function cm_cnm_zpool_status_get()
{
    local step=0
    local poolname=''
    local dataused=0
    local datatotal=0
    local lowused=0
    local lowtotal=0
    local hostname=`hostname`
    zpool status |sed '/c0t/d' |sed '/spares/d' |while read line
    do
        local name=($line)
        case ${name[0]} in
            NAME)
                step=1
                poolname=''
                dataused=0
                datatotal=0
                lowused=0
                lowtotal=0
            ;;
            Metadatas)
                step=10
            ;;
            Lowdatas)
                step=20
            ;;
            raidz*)
                local tused=`cm_cnm_zpool_data_change_m ${name[8]}`
                local ttotal=`cm_cnm_zpool_data_change_m ${name[9]}`
                if [ $step -eq 2 ]; then
                    dataused=`echo $dataused+$tused |bc`
                    datatotal=`echo $datatotal+$ttotal |bc`
                fi
                if [ $step -eq 20 ]; then
                    lowused=`echo $lowused+$tused |bc`
                    lowtotal=`echo $lowtotal+$ttotal |bc`
                fi
            ;;
            erro*)
                if [ $step -gt 1 ]; then
                    echo "$hostname $poolname $datatotal $dataused $lowtotal $lowused"
                fi
            ;;
            *)
                if [ $step -eq 1 ]; then
                    poolname=${name[0]}
                    step=2
                fi
            ;;
        esac
    done
    return 0
}

function cm_cnm_pool_each()
{
    cm_multi_exec "zfs list -H -o name,used,avail |sed '/\//d'" |while read pool
    do
        local info=($pool)
        local used=`cm_cnm_zpool_data_change_m ${info[1]}`
        local avail=`cm_cnm_zpool_data_change_m ${info[2]}`
        local total=0
        ((total=$used+$avail))
        echo "${info[0]} $total $used"
    done
    return 0
}

function cm_cnm_pool_stat_in()
{
    if [ ! -f /var/cm/data/cm_cnm_pool_stat_in.flag ]; then
        echo "0 0 0 0"
        return 0
    fi
    
    local info=(`cm_multi_exec "$CM_SCRPIT_LOCAL zpool_status_get" |awk 'BEGIN{a=0;b=0;c=0;d=0}{a+=$3;b+=$4;c+=$5;d+=$6}END{print a" "b" "c" "d}'`)
    
    local dataused=${info[1]}
    local datatotal=${info[0]}
    local lowused=${info[3]}
    local lowtotal=${info[2]}
    
    #dataused=`echo "$dataused*8/9"|bc`
    #datatotal=`echo "$datatotal*8/9"|bc`
    #lowused=`echo "$lowused*8/9"|bc`
    #lowtotal=`echo "$lowtotal*8/9"|bc`
    echo "$datatotal $dataused $lowtotal $lowused"
    return 0
}

function cm_cnm_pool_stat()
{
    local info=(`cm_multi_exec "$CM_SCRPIT_LOCAL zpool_status_get" |awk 'BEGIN{a=0;b=0;c=0;d=0}{a+=$3;b+=$4;c+=$5;d+=$6}END{print a" "b" "c" "d}'`)
    
    local dataused=${info[1]}
    local datatotal=${info[0]}
    local lowused=${info[3]}
    local lowtotal=${info[2]}
    
    #dataused=`echo "$dataused*8/9"|bc`
    #datatotal=`echo "$datatotal*8/9"|bc`
    #lowused=`echo "$lowused*8/9"|bc`
    #lowtotal=`echo "$lowtotal*8/9"|bc`
    
    dataused=`cm_cnm_zpool_data_back_m $dataused`
    datatotal=`cm_cnm_zpool_data_back_m $datatotal`
    lowused=`cm_cnm_zpool_data_back_m $lowused`
    lowtotal=`cm_cnm_zpool_data_back_m $lowtotal`
    echo "datatotal: $datatotal dataused: $dataused"
    echo "lowtotal: $lowtotal lowused: $lowused"
    return 0
}

function cm_cnm_client_stat()
{
    local data=''
    cm_multi_exec 'netstat |egrep "nfsd|microsoft-ds" |grep ESTABLISHED' \
        |awk '{print $2" "$1}' \
        |while read line
    do
        data=($line)
        local clientinfo=${data[0]}
        local serverinfo=${data[1]}
        clientinfo=${clientinfo%.*}
        serverinfo=${serverinfo##*.}
        
        if [ "X$clientinfo" == "X" ] || [ "X$serverinfo" == "X" ]; then
            continue
        fi
        
        if [ "X$serverinfo" == "Xnfsd" ]; then
            serverinfo="NFS"
        else
            serverinfo="SMB"
        fi
        echo "$clientinfo $serverinfo"
    done |sort -u
    return 0
}

function cm_cnm_node_rdma_checkaddlocal()
{
    local CM_CLUSTER_INI='/var/cm/data/cm_cluster.ini'
    local CM_RDMA_INI='/var/cm/data/cm_cluster_rdma.ini'
    local hostn=`hostname`
    local hosti=`cm_get_localid`
    local iret=$CM_OK
    local nics=(`egrep "^nic " $CM_CLUSTER_INI |sed 's/rdma_rpc//g'|awk -F'=' '{print $2}' |sed 's/\,/ /g'`)
    local index=0
    cnt=${#nics[*]}
    local checkresultfile="/tmp/${FUNCNAME}_checkresultfile"
    echo -n "${hostn}_${hosti} =" >> $CM_RDMA_INI
    for((index=0;index<$cnt;index++))
    do
        #获取远端节点集群通信IP
        local rdmaip=`sed -n 1p /etc/hostname.${nics[$index]}`
        if [ "X$rdmaip" == "X" ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}]/etc/hostname.${nics[$index]} not config"
            echo $CM_ERR_NOT_EXISTS > $checkresultfile
            break
        fi
        #检查集群通信IP是否联通
        ping $rdmaip 1 1>/dev/null 2>/dev/null
        iret=$?
        if [ $iret -ne 0 ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] /etc/hostname.${nics[$index]} $rdmaip ping fail"
            echo $CM_ERR_CONN_NONE > $checkresultfile
            break
        fi
        echo -n " ${rdmaip}" >> $CM_RDMA_INI
    done
    
    echo "" >>$CM_RDMA_INI
    if [ -f $checkresultfile ]; then
        iret=`cat $checkresultfile`
        rm -f $checkresultfile
        #删除已经添加的配置
        sed "/${hostn}/d" $CM_RDMA_INI > ${CM_RDMA_INI}".tmp"
        mv ${CM_RDMA_INI}".tmp" $CM_RDMA_INI
        return $iret
    fi
    return $CM_OK
}

function cm_cnm_node_rdma_checkadd()
{
    local iret=$CM_OK
    local remoteip=$1
    local hostn=$2
    local hosti=$3
    CM_LOG "[${FUNCNAME}:${LINENO}]start $*"
    local CM_CLUSTER_INI='/var/cm/data/cm_cluster.ini'
    local CM_RDMA_INI='/var/cm/data/cm_cluster_rdma.ini'
    #格式: <hostname>_<hostid> = <ip1> <ip2>
    local nic=`grep 'nic' $CM_CLUSTER_INI |grep rdma_rpc`
    if [ "X$nic" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] not config"
        return $CM_OK
    fi
    if [ ! -f $CM_RDMA_INI ]; then
        touch $CM_RDMA_INI
    fi
    #检查自己是否在配置文件中
    local myhostname=`hostname`
    local cnt=`egrep "^${myhostname}_" $CM_RDMA_INI`
    if [ "X$cnt" == "X" ]; then
        cm_cnm_node_rdma_checkaddlocal
        iret=$?
        if [ $iret -ne $CM_OK ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] local fail"
            return $iret
        fi
    fi

    #检查主机名是否重复
    cnt=`egrep "^${hostn}_" $CM_RDMA_INI`
    if [ "X$cnt" != "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $hostn already in cfg"
        return $CM_ERR_ALREADY_EXISTS
    fi    #保留非rdma_rpc的网口
    
    local nics=(`egrep "^nic " $CM_CLUSTER_INI |sed 's/rdma_rpc//g'|awk -F'=' '{print $2}' |sed 's/\,/ /g'`)
    local index=0
    cnt=${#nics[*]}
    local checkresultfile="/tmp/${FUNCNAME}_checkresultfile"
    echo -n "${hostn}_${hosti} =" >> $CM_RDMA_INI
    for((index=0;index<$cnt;index++))
    do
        #获取远端节点集群通信IP
        local rdmaip=`ceres_exec $remoteip "sed -n 1p /etc/hostname.${nics[$index]}"`
        if [ "X$rdmaip" == "X" ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] ip:$remoteip /etc/hostname.${nics[$index]} not config"
            echo $CM_ERR_NOT_EXISTS > $checkresultfile
            break
        fi
        
        #检查集群通信IP是否联通
        ping $rdmaip 1 1>/dev/null 2>/dev/null
        iret=$?
        if [ $iret -ne 0 ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] ip:$remoteip /etc/hostname.${nics[$index]} $rdmaip ping fail"
            echo $CM_ERR_CONN_NONE > $checkresultfile
            break
        fi
        echo -n " ${rdmaip}" >> $CM_RDMA_INI
    done
    
    echo "" >>$CM_RDMA_INI
    if [ -f $checkresultfile ]; then
        iret=`cat $checkresultfile`
        rm -f $checkresultfile
        #删除已经添加的配置
        sed "/${hostn}/d" $CM_RDMA_INI > ${CM_RDMA_INI}".tmp"
        mv ${CM_RDMA_INI}".tmp" $CM_RDMA_INI
        return $iret
    fi
    CM_LOG "[${FUNCNAME}:${LINENO}]end $*"
    return $CM_OK
}

function cm_cnm_node_rdma_checkdel()
{
    local hostn=$1
    local CM_CLUSTER_INI='/var/cm/data/cm_cluster.ini'
    local CM_RDMA_INI='/var/cm/data/cm_cluster_rdma.ini'
    #格式: <hostname>_<hostid> = <ip1> <ip2>
    local nic=`grep 'nic' $CM_CLUSTER_INI |grep rdma_rpc`
    if [ "X$nic" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] not config"
        return $CM_OK
    fi
    
    sed "/${hostn}/d" $CM_RDMA_INI > ${CM_RDMA_INI}".tmp"
    mv ${CM_RDMA_INI}".tmp" $CM_RDMA_INI
    return $CM_OK
}

function cm_cnm_node_rdma_add()
{
    local iret=$CM_OK
    CM_LOG "[${FUNCNAME}:${LINENO}]start"
    local myname=`hostname`
    local CM_CLUSTER_INI='/var/cm/data/cm_cluster.ini'
    local CM_RDMA_INI='/var/cm/data/cm_cluster_rdma.ini'
    local nodedb=$CM_NODE_DB_FILE
    #格式: <hostname>_<hostid> = <ip1> <ip2>
    local nic=`grep 'nic' $CM_CLUSTER_INI |grep rdma_rpc`
    if [ "X$nic" == "X" ]; then
        return $CM_OK
    fi
    
    if [ ! -f $CM_RDMA_INI ]; then
        touch $CM_RDMA_INI
    fi
    
    local hostlist=`zfs clustersan list-host |grep hostname |awk '{printf $3" "}END{print ""}'`
    sqlite3 $nodedb "SELECT name,ipaddr,idx FROM record_t WHERE name!='$myname'" |sed 's/|/ /g' |while read line
    do
        local info=($line)
        local name=${info[0]}
        local ipaddr=${info[1]}
        local idx=${info[2]}
        local cnt=`grep "${name}_" $CM_RDMA_INI`
        if [ "X$cnt" == "X" ]; then
            cm_cnm_node_rdma_checkadd ${info[1]} ${info[0]} ${info[2]}
        fi
        cnt=`echo "$hostlist" |grep -w $name`
        if [[ "X$cnt" != "X" ]]; then
            continue
        fi
        
        info=(`grep "${name}_" $CM_RDMA_INI |sed 's/[_=\,]/ /g'`)
        local cnt=${#info[*]}
        local index=0
        for((index=2; index<$cnt; index++))
        do
            zfs clustersan rpcto hostname=${info[0]} hostid=${info[1]} ip=${info[$index]}
            iret=$?
            CM_LOG "[${FUNCNAME}:${LINENO}]rdma_rpc ${info[0]} ${info[1]} ${info[$index]} iRet=$iRet"
        done
    done
    CM_LOG "[${FUNCNAME}:${LINENO}]end"
    return $CM_OK
}

function cm_cnm_ifconfig()
{
    local ifs=`dladm show-link -p -o link |awk '{printf $1" "}END{print ""}'`
    while true
    do
        printf "Do you wish to set other ipaddr?[y/n] (default: n): ";
        read val
        if [ "X$val" != "Xy" ] && [ "X$val" != "XY" ]; then
            break
        fi
        printf "Current link ($ifs), please enter the name: ";
        read linkname
        if [[ "$ifs" != *"$linkname"* ]]; then
            echo "The name '$linkname' not exist!"
            continue
        fi
        printf "ifconfig $linkname, please enter the ipaddr: ";
        read ipaddr
        cm_check_isipaddr "$ipaddr"
        local iret=$?
        if [ $iret -ne 0 ]; then
            echo "ipaddr error, set fail!"
            continue
        fi
        echo "$ipaddr" >/etc/hostname."$linkname"
        echo "netmask 255.255.255.0" >>/etc/hostname."$linkname"
        echo "up" >>/etc/hostname."$linkname"
    done
    return 0
}

function cm_cnm_ibinit()
{
    local cfgfile='/etc/path_to_inst'
    local topath=$1
    local frompath="${topath}/path_to_inst"
    local cnt=`egrep "\"ibd\"|\"hermon\"" $cfgfile |wc -l |awk '{print $1}'`
    
    if [ $cnt -eq 0 ]; then
        #没有IB相关
        CM_LOG "[${FUNCNAME}:${LINENO}]no ib"
        return 0
    fi
    
    if [ ! -d $topath ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]path $topath not exist"
        return 0
    fi
    
    CM_LOG "[${FUNCNAME}:${LINENO}]ibcnt $cnt"
    if [ ! -f $frompath ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]touch $frompath"
        touch $frompath
    fi
    cnt=`egrep "\"ibd\"|\"hermon\"" $frompath |wc -l |awk '{print $1}'`
    if [ $cnt -gt 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]ib found $cnt"
        return 0
    fi
    CM_LOG "[${FUNCNAME}:${LINENO}]ibcfg addto $frompath"
    egrep "\"ibd\"|\"hermon\"" $cfgfile >> $frompath
    return 0
}

function cm_cnm_fd_exist_check()
{
    local path=$1
    local nid=$2
}
function cm_cnm_copy_update()
{
    local path=$1
    local nid=$2
    local num=$3
    local hostname=`sqlite3 $CM_NODE_DB "select name from record_t where id=$nid"`
    if [ "X$hostname" == "X" ];then
        return $CM_ERR_NOT_EXISTS
    fi
    local fdcheck=`cm_multi_exec $hostname "[ -d $path ] && echo 1 || echo 0"`
    if [ $fdcheck -eq 0 ];then
        return $CM_ERR_NOT_EXISTS
    fi
    if [ $num -lt 1 ] || [ $num -gt 3 ];then
        return $CM_PARAM_ERR
    fi
    local record_check=`sqlite3 $CM_COPY_DB "select count(nas) from copy_t where nas='$path'"`
    if [ $record_check -eq 0 ];then
        cm_multi_exec "sqlite3 $CM_COPY_DB \"INSERT INTO copy_t (nas,copynum) VALUES ('$path',$num)\""
    else
        cm_multi_exec "sqlite3 $CM_COPY_DB \"UPDATE copy_t SET copynum=$num WHERE nas='$path'\""
    fi
    return $CM_OK
}

function cm_cnm_clumgt_status()
{
    local name=`hostname`
    local stat="online"
    local ip="127.0.0.1"
    local mport=`cm_get_localmanageport`
    if [ -f /etc/hostname.${mport} ]; then
        ip=`sed -n 1p /etc/hostname.${mport}`
    fi
    local version=`head -n 1 /lib/release | sed 's/^[ t]*//'|cut -d' ' -f5`
    local hostid=`hostid`
    local uptime=`uptime |sed 's/.*up //g'|awk -F',' '{for(i=1;i<NF-3;i++){printf $i}}'`
    local clusterstat=`clusterinfo|awk '{print $3 }'|sed -n '$p'`
    local systime=`date '+%Y-%m-%d %H:%M'`
    local mem=`prtconf | grep 'Memory'| cut -d: -f2|sed 's/^ *//'`
    local gui_ver="2.0.0"
    
    echo "------------------------"
    echo "name:$name"
    echo "stat:$stat"
    echo "ip:$ip"
    echo "version:$version"
    echo "uptime:$uptime"
    echo "stat:$clusterstat"
    echo "hostid:$hostid"
    echo "systime:$systime"
    echo "mem:$mem"
    echo "gui_ver:$gui_ver"
}

function cm_cnm_get_lun_stmfid()
{
    local ver=`cm_systerm_version_get`
    local lun=$1
    if [ $ver -eq $CM_SYS_VER_SOLARIS_V7R16 ]; then
        /var/cm/script/cm_shell_exec.sh stmfadm_list_all_luns |egrep $"$lun"^ |awk '{print $1}'
        return 0
    fi
    stmfadm list-all-luns |egrep $"$lun"^ |awk '{print $1}'
    return 0
}

function cm_cnm_snapshot_rollback()
{
    local dir=$1
    local snap=$2
    CM_LOG "[${FUNCNAME}:${LINENO}]$*"
    local ftype=`zfs get type $dir |sed 1d |awk '{print $3}'`
    
    if [ "X$ftype" != "Xvolume" ]; then
        CM_EXEC_CMD "zfs rollback -rf ${dir}@${snap}"
        return $?
    fi
    
    local stmfid=`cm_cnm_get_lun_stmfid "$dir"`
    if [ "X$stmfid" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]stmfid null"
        return $CM_FAIL
    fi
    
    local datafile=`stmfadm list-lu -v $stmfid |grep 'Data File' |awk '{print $4}'`
    if [ "X$datafile" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]$stmfid get Data File fail"
        return $CM_FAIL
    fi
    
    local access=`stmfadm list-lu -v $stmfid |grep 'Access' |awk '{print $4}'`
    if [ "X$access" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]$stmfid get Access fail"
        return $CM_FAIL
    fi
    
    #记录当前视图
    local listview=`date "+%Y%m%d%H%M%S"`
    listview="/tmp/${FUNCNAME}_$listview.tmp"
    stmfadm list-view -l ${stmfid} 2>/dev/null |sed 's/ //g' |awk -F':' '{print $2}' |xargs -n4 > ${listview}
    
    #删除lu
    CM_EXEC_CMD "stmfadm delete-lu ${stmfid}"
    local iret=$?
    if [ $iret -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]${stmfid} del fail $iret"
        rm -f ${listview}
        return $iret
    fi
    
    #执行回滚
    CM_EXEC_CMD "zfs rollback -rf ${dir}@${snap}"
    iret=$?
   
    #导入lu
    CM_EXEC_CMD "stmfadm import-lu ${datafile}"
    
    #还原视图
    if [ -f ${listview} ]; then
        cat ${listview} |while read line
        do
            local info=($line)
            local cnt=${#info[*]}
            CM_LOG "[${FUNCNAME}:${LINENO}]$line"
            if [ $cnt -ne 4 ]; then
                continue
            fi
            local cmd="stmfadm add-view -c"
            if [ "X${info[1]}" != "XAll" ]; then
                cmd="${cmd} -h ${info[1]}"
            fi
            if [ "X${info[2]}" != "XAll" ]; then
                cmd="${cmd} -t ${info[2]}"
            fi
            CM_EXEC_CMD "${cmd} -n ${info[3]} ${stmfid}"
        done
        rm -f ${listview}
    fi
    
    #设置zfs:single_data
    if [ "X$access" == "XActive" ]; then
        CM_EXEC_CMD "zfs set zfs:single_data=1 $dir"
    fi
    CM_LOG "[${FUNCNAME}:${LINENO}]$* $iret"
    return $iret
}

function cm_cnm_filedir_create()
{
    local ftype=$2
    local parentdir=$1
    local fname=$3
    local fperm=$4
    
    if [ "X$parentdir" == "X"] || [ ! -d ${parentdir} ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]parentdir $parentdir err"
        return $CM_ERR_NOT_EXISTS
    fi
    
    if [ "X$ftype" == "X" ] || [ "X$fname" == "X" ] || [ "X$ftype" != "X1" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]$ftype:$fname err"
        return $CM_PARAM_ERR
    fi
    mkdir "${parentdir}"/"${fname}" >>${CM_LOG_DIR}${CM_LOG_FILE} 2>&1
    local iRet=$?
    if [ $iRet -eq $CM_OK ] && [ "X$fperm" != "X" ] && [ $fperm -gt 0 ]; then
        chmod $fperm "${parentdir}"/"${fname}" >>${CM_LOG_DIR}${CM_LOG_FILE} 2>&1
    fi
    if [ $iRet -eq 2 ]; then
        iRet=$CM_ERR_ALREADY_EXISTS
    fi
    return $iRet
}

function cm_cnm_filedir_update()
{
    local nname=$4
    local parentdir=$1
    local fname=$2
    local fperm=$3
    local iRet=$CM_OK
    if [ "X$parentdir" == "X"] || [ ! -d ${parentdir} ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]parentdir $parentdir err"
        return $CM_ERR_NOT_EXISTS
    fi
    
    if [ "X$fname" == "X" ] ; then
        CM_LOG "[${FUNCNAME}:${LINENO}]$fname err"
        return $CM_PARAM_ERR
    fi
    if [ "X$fperm" != "X" ] && [ $fperm -gt 0 ]; then
        chmod $fperm "${parentdir}"/"${fname}" >>${CM_LOG_DIR}${CM_LOG_FILE} 2>&1
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            return $iRet
        fi
    fi
    
    if [ "X$nname" == "X" ]; then
        mv "${parentdir}"/"${fname}" "${parentdir}"/"${nname}" >>${CM_LOG_DIR}${CM_LOG_FILE} 2>&1
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            return $iRet
        fi
    fi
    return $iRet
}

function cm_cnm_filedir_delete()
{
    local parentdir=$1
    local fname=$2
    local iRet=$CM_OK
    if [ "X$parentdir" == "X"] || [ ! -d ${parentdir} ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]parentdir $parentdir err"
        return $CM_ERR_NOT_EXISTS
    fi
    
    if [ "X$fname" == "X" ] ; then
        CM_LOG "[${FUNCNAME}:${LINENO}]$fname err"
        return $CM_PARAM_ERR
    fi
    rm -rf "${parentdir}"/"${fname}" >>${CM_LOG_DIR}${CM_LOG_FILE} 2>&1
    iRet=$?
    return $iRet
}

function cm_cnm_localtask_sql()
{
    local nid='null'
    local progress='null'
    local status='null'
    local starttm='null'
    local endtm='null'
    local params=$1
    if [ "X$params" != "X" ]; then
        params=`echo "$params" |sed 's/,/ /g'`
        params=($params)
        nid=${params[0]}
        progress=${params[1]}
        status=${params[2]}
        starttm=${params[3]}
        endtm=${params[4]}
    fi
    local cond=""
    
    if [ "X$progress" != "X" ] && [ "$progress" != "null" ]; then
        cond=" WHERE progress=$progress"
    fi
    
    if [ "X$status" != "X" ] && [ "$status" != "null" ]; then
        if [ "X$cond" == "X" ]; then
            cond=" WHERE status=$status"
        else
            cond=" AND status=$status"
        fi
    fi
    
    if [ "X$starttm" != "X" ] && [ "$starttm" != "null" ]; then
        if [ "X$cond" == "X" ]; then
            cond=" WHERE start>=$starttm"
        else
            cond=" AND start>=$starttm"
        fi
    fi
    
    if [ "X$endtm" != "X" ] && [ "$endtm" != "null" ]; then
        if [ "X$cond" == "X" ]; then
            cond=" WHERE end<=$endtm"
        else
            cond=" AND end<=$endtm"
        fi
    fi
    echo "$cond"
    return 0
}
function cm_cnm_localtask_getbatch()
{
    local dbfile="/var/cm/data/cm_localtask.db"
    local params=$1
    local cond=`cm_cnm_localtask_sql "$params"`
    local sql="SELECT * FROM record_t $cond"
    local nid=`echo $params |awk -F',' '{print $1}'`
    
    #"tid|nid|progress|status|start|end|desc"
    if [ "X$nid" == "X" ] || [ "X$nid" == "Xnull" ] || [ "X$nid" == "X0" ]; then
        cm_multi_exec "sqlite3 $dbfile '$sql'" |sed 's/\|/ /g' |sort -nrk 5
    else
        nid=`sqlite3 $CM_NODE_DB_FILE "SELECT name FROM record_t WHERE id=$nid LIMIT 1"`
        if [ "X$nid" != "X" ]; then
            cm_multi_exec $nid "sqlite3 $dbfile '$sql'" |sed 's/\|/ /g' |sort -nrk 5
        fi
    fi
    return 0
}

function cm_cnm_localtask_count()
{
    local dbfile="/var/cm/data/cm_localtask.db"
    local params=$1
    local cond=`cm_cnm_localtask_sql "$params"`
    local sql="SELECT COUNT(tid) FROM record_t $cond"
    local nid=`echo $params |awk -F',' '{print $1}'`
    
    if [ "X$nid" == "X" ] || [ "X$nid" == "Xnull" ] || [ "X$nid" == "X0" ]; then
        cm_multi_exec "sqlite3 $dbfile '$sql'" |awk 'BEGIN{s=0}{s+=$1}END{print s}'
    else
        nid=`sqlite3 $CM_NODE_DB_FILE "SELECT name FROM record_t WHERE id=$nid LIMIT 1"`
        if [ "X$nid" == "X" ]; then
            echo "0"
        else
            cm_multi_exec $nid "sqlite3 $dbfile '$sql'"
        fi
    fi
    return 0
}

function cm_cnm_utctime()
{
    sqlite3 ${CM_NODE_DB_FILE} "select strftime('%s','now')"
    return 0
}

function cm_cnm_localtask_exec()
{
    local dbfile="/var/cm/data/cm_localtask.db"
    local taskid=$1
    local taskcmd=$2
    
    CM_LOG "[${FUNCNAME}:${LINENO}]start [$taskid] $taskcmd"
    sqlite3 $dbfile "UPDATE record_t SET progress=progress+5,status=1 WHERE tid=$taskid"
    
    $taskcmd
    local iret=$?
    CM_LOG "[${FUNCNAME}:${LINENO}]finish [$taskid] iret=$iret"
    local status=2
    if [ $iret -ne $CM_OK ]; then
        status=3
    fi
    local utcnow=`cm_cnm_utctime`
    sqlite3 $dbfile "UPDATE record_t SET progress=100,status=$status,end=$utcnow WHERE tid=$taskid"
    return $CM_OK
}

function cm_cnm_localtask_create()
{
    local dbfile="/var/cm/data/cm_localtask.db"
    local taskname="$1"
    local taskcmd="$2"
    local nid=`/var/cm/script/cm_shell_exec.sh cm_get_localcmid`
    local taskid=`sqlite3 $dbfile "SELECT seq+1 FROM sqlite_sequence WHERE name='record_t'"`
    if [ "X$taskid" == "X" ]; then
        taskid=1
    fi
    CM_LOG "[${FUNCNAME}:${LINENO}]task:$taskname [$taskid] $taskcmd"
    local utcnow=`cm_cnm_utctime`
    sqlite3 $dbfile "INSERT INTO record_t (nid,progress,status,start,end,desc) VALUES($nid,0,0,$utcnow,0,'$taskname')"
    
    echo "taskid: $taskid"
    cm_cnm_localtask_exec "$taskid" "$taskcmd" 1>/dev/null 2>/dev/null &
    
    return $CM_OK
}

function cm_cnm_lun_delete()
{
    local params="$1"
    local lunname=`echo $params |awk -F'|' '{print $1}'`
    local poolname=`echo $lunname |awk -F'/' '{print $1}'`
    lunname=`echo $lunname |awk -F'/' '{print $2}'`
    
    CM_LOG "[${FUNCNAME}:${LINENO}]$poolname/$lunname"
    
    local taskname="delete lun $poolname/$lunname"
    local taskcmd="/var/cm/script/cm_shell_exec.sh cm_lun_delete $poolname $lunname"
    
    #cm_cnm_localtask_create "$taskname" "$taskcmd" |awk '{print $2}'
    $taskcmd
    return $?
}

function cm_cnm_lun_create_exec()
{
    #lunname|lunsize|is_single|blocksize|is_compress|is_hot|write_policy|dedup|is_double|thold|qos|qos_val
    local params="$1"
    local paramnull="-"
    params=`echo "$params" |sed 's/\|/ /g'`
    params=($params)
    local lunname=${params[0]}
    local lunsize=${params[1]}
    local is_single=${params[2]}
    local blocksize=${params[3]}
    local is_compress=${params[4]}
    local is_hot=${params[5]}
    local write_policy=${params[6]}
    local dedup=${params[7]}
    local is_double=${params[8]}
    local thold=${params[9]}
    local qos=${params[10]}
    local qos_val=${params[11]}
    
    local option=""
    if [ "X${is_single}" != "X" ] && [ "X${is_single}" != "X${paramnull}" ] && [ $is_single -eq 1 ]; then
        option=${option}" -s"
    fi
    
    if [ "X${blocksize}" != "X" ] && [ "X${blocksize}" != "X${paramnull}" ]; then
        option=${option}" -b ${blocksize}K"
    fi
    
    if [ "X${is_compress}" != "X" ] && [ "X${is_compress}" != "X${paramnull}" ]; then
        if [ $is_compress -eq 1 ]; then
            is_compress="on"
        else
            is_compress="off"
        fi
        option=${option}" -o compression=${is_compress}"
    fi
    
    if [ "X${is_hot}" != "X" ] && [ "X${is_hot}" != "X${paramnull}" ]; then
        if [ $is_hot -eq 1 ]; then
            is_hot="on"
        else
            is_hot="off"
        fi
        option=${option}" -o appmeta=${is_hot}"
    fi
    
    if [ "X${write_policy}" != "X" ] && [ "X${write_policy}" != "X${paramnull}" ]; then
        option=${option}" -o origin:sync=${write_policy}"
    fi
    
    local cmd="zfs create ${option} -V ${lunsize} ${lunname}"
    CM_EXEC_CMD "$cmd"
    local iRet=$?
    if [ $iRet -ne $CM_OK ]; then
        return $iRet
    fi
    
    if [ "X${dedup}" != "X" ] && [ "X${dedup}" != "X${paramnull}" ]; then
        if [ $dedup -eq 1 ]; then
            dedup="on"
        else
            dedup="off"
        fi
        CM_EXEC_CMD "zfs set dedup=${dedup} ${lunname}"
    fi
    
    if [ "X${is_double}" != "X" ] && [ "X${is_double}" != "X${paramnull}" ]; then
        if [ $is_double -eq 1 ]; then
            is_double=0
        else
            is_double=1
        fi
        CM_EXEC_CMD "zfs set zfs:single_data=${is_double} ${lunname}"
    fi
    
    local sfver=`cm_systerm_version_get`
    if [ $sfver -eq ${CM_SYS_VER_SOLARIS_V7R16} ]; then
        return $CM_OK
    fi
    
    if [ "X${thold}" != "X" ] && [ "X${thold}" != "X${paramnull}" ]; then
        CM_EXEC_CMD "zfs set thold=${thold} ${lunname}"
    fi
    
    if [ "X${qos}" == "X" ] || [ "X${qos}" == "X${paramnull}" ]; then
        return $CM_OK
    fi
    
    local subcmd="set-kbps"
    if [ ${qos} -eq 1 ]; then
        subcmd="set-iops-limit"
    fi
    if [ "X${qos_val}" == "X" ] || [ "X${qos_val}" == "X${paramnull}" ]; then
        qos_val="0"
    fi
    local stmfid=`stmfadm list-all-luns 2>/dev/null |awk '$2=="'$lunname'"{print $1}'`
    if [ "X$stmfid" == "X" ]; then
        return $CM_OK
    fi
    cmd="stmfadm $subcmd -c -l $stmfid -n $qos_val"
    CM_EXEC_CMD "$cmd"
    return $CM_OK
}
function cm_cnm_lun_create()
{
    local params=$1
    local lunname=`echo $params |awk -F'|' '{print $1}'`
    
    #cm_cnm_localtask_create "create lun $lunname" "cm_cnm_lun_create_exec $params" |awk '{print $2}'
    cm_cnm_lun_create_exec "$params"
    return $?
}

function cm_cnm_lun_update_exec()
{
    #lunname|lunsize|is_single|blocksize|is_compress|is_hot|write_policy|dedup|is_double|thold|qos|qos_val
    local params="$1"
    local paramnull="-"
    params=`echo "$params" |sed 's/\|/ /g'`
    params=($params)
    local lunname=${params[0]}
    local lunsize=${params[1]}
    local is_single=${params[2]}
    local blocksize=${params[3]}
    local is_compress=${params[4]}
    local is_hot=${params[5]}
    local write_policy=${params[6]}
    local dedup=${params[7]}
    local is_double=${params[8]}
    local thold=${params[9]}
    local qos=${params[10]}
    local qos_val=${params[11]}
    local iRet=$CM_OK
    local stmfid=""
    
    if [ "X${lunsize}" != "X" ] && [ "X${lunsize}" != "X${paramnull}" ]; then
        stmfid=`stmfadm list-all-luns 2>/dev/null |awk '$2=="'$lunname'"{print $1}'`
        if [ "X$stmfid" == "X" ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}]$lunname stmfid null"
            return $CM_FAIL
        fi
        CM_EXEC_CMD "stmfadm modify-lu -c -s $lunsize $stmfid"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            return $iRet
        fi
        CM_EXEC_CMD "zfs set volsize=$lunsize $lunname"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            return $iRet
        fi
    fi
    
    if [ "X${is_compress}" != "X" ] && [ "X${is_compress}" != "X${paramnull}" ]; then
        if [ $is_compress -eq 1 ]; then
            is_compress="on"
        else
            is_compress="off"
        fi
        CM_EXEC_CMD "zfs set compression=${is_compress} ${lunname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            return $iRet
        fi
    fi
    
    if [ "X${is_hot}" != "X" ] && [ "X${is_hot}" != "X${paramnull}" ]; then
        if [ $is_hot -eq 1 ]; then
            is_hot="on"
        else
            is_hot="off"
        fi
        CM_EXEC_CMD "zfs set appmeta=${is_hot} ${lunname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            return $iRet
        fi
    fi
    
    if [ "X${write_policy}" != "X" ] && [ "X${write_policy}" != "X${paramnull}" ]; then
        CM_EXEC_CMD "zfs set origin:sync=${is_hot} ${lunname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            return $iRet
        fi
    fi

    if [ "X${dedup}" != "X" ] && [ "X${dedup}" != "X${paramnull}" ]; then
        if [ $dedup -eq 1 ]; then
            dedup="on"
        else
            dedup="off"
        fi
        CM_EXEC_CMD "zfs set dedup=${dedup} ${lunname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            return $iRet
        fi
    fi
    
    if [ "X${is_double}" != "X" ] && [ "X${is_double}" != "X${paramnull}" ]; then
        if [ $is_double -eq 1 ]; then
            is_double=0
        else
            is_double=1
        fi
        CM_EXEC_CMD "zfs set zfs:single_data=${is_double} ${lunname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            return $iRet
        fi
    fi
    
    local sfver=`cm_systerm_version_get`
    if [ $sfver -eq ${CM_SYS_VER_SOLARIS_V7R16} ]; then
        return $CM_OK
    fi
    
    if [ "X${thold}" != "X" ] && [ "X${thold}" != "X${paramnull}" ]; then
        CM_EXEC_CMD "zfs set thold=${thold} ${lunname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            return $iRet
        fi
    fi
    
    if [ "X${qos}" == "X" ] || [ "X${qos}" == "X${paramnull}" ]; then
        return $CM_OK
    fi
    
    local subcmd="set-kbps"
    if [ ${qos} -eq 1 ]; then
        subcmd="set-iops-limit"
    fi
    if [ "X${qos_val}" == "X" ] || [ "X${qos_val}" == "X${paramnull}" ]; then
        qos_val="0"
    fi
    if [ "X$stmfid" == "X" ]; then
        stmfid=`stmfadm list-all-luns 2>/dev/null |awk '$2=="'$lunname'"{print $1}'`
        if [ "X$stmfid" == "X" ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}]$lunname stmfid null"
            return $CM_FAIL
        fi
    fi
    cmd="stmfadm $subcmd -c -l $stmfid -n $qos_val"
    CM_EXEC_CMD "$cmd"
    return $?
}

function cm_cnm_lun_update()
{
    local params=$1
    local lunname=`echo $params |awk -F'|' '{print $1}'`
    CM_LOG "[${FUNCNAME}:${LINENO}]$lunname"
    cm_cnm_lun_update_exec "$1"
    return $?
}

function cm_cnm_nas_set()
{
    #nasname|blocksize|access|write_policy|is_compress|dedup|smb|abe|aclinherit|quota|qos|qos_avs
    local params=$1
    local failnum=0
    local paramnull="-"
    params=`echo "$params" |sed 's/\|/ /g'`
    params=($params)
    local nasname=${params[0]}
    local blocksize=${params[1]}
    local laccess=${params[2]}
    local write_policy=${params[3]}
    local is_compress=${params[4]}
    
    local dedup=${params[5]}
    local sharesmb=${params[6]}
    local abe=${params[7]}
    local aclinherit=${params[8]}
    local quota=${params[9]}
    
    local qos=${params[10]}
    local qos_avs=${params[11]}
    
    if [ "X${blocksize}" != "X" ] && [ "X${blocksize}" != "X${paramnull}" ]; then
        CM_EXEC_CMD "zfs set recordsize=${blocksize}K ${nasname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            ((failnum=$failnum+1))
        fi
    fi
    
    if [ "X${laccess}" != "X" ] && [ "X${laccess}" != "X${paramnull}" ]; then
        chmod ${laccess} /${nasname}
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            ((failnum=$failnum+1))
        fi
    fi
    
    if [ "X${write_policy}" != "X" ] && [ "X${write_policy}" != "X${paramnull}" ]; then
        local wp=("unkown" "disk" "poweroff" "mirror" "standard" "always")
        CM_EXEC_CMD "zfs set origin:sync=${wp[${write_policy}]} ${nasname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            ((failnum=$failnum+1))
        fi
    fi
    
    if [ "X${is_compress}" != "X" ] && [ "X${is_compress}" != "X${paramnull}" ]; then
        if [ ${is_compress} -eq 1 ]; then
            is_compress="on"
        else
            is_compress="off"
        fi
        CM_EXEC_CMD "zfs set compression=${is_compress} ${nasname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            ((failnum=$failnum+1))
        fi
    fi
    
    if [ "X${dedup}" != "X" ] && [ "X${dedup}" != "X${paramnull}" ]; then
        if [ ${dedup} -eq 1 ]; then
            dedup="on"
        else
            dedup="off"
        fi
        CM_EXEC_CMD "zfs set dedup=${dedup} ${nasname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            ((failnum=$failnum+1))
        fi
    fi
    
    if [ "X${sharesmb}" != "X" ] && [ "X${sharesmb}" != "X${paramnull}" ]; then
        if [ ${sharesmb} -eq 1 ]; then
            if [ "X${abe}" != "X" ] && [ "X${abe}" != "X${paramnull}" ] && [ $abe -eq 1 ]; then
                sharesmb="abe=true"
            else
                sharesmb="on"
            fi
        else
            sharesmb="off"
        fi
        CM_EXEC_CMD "zfs set sharesmb=${sharesmb} ${nasname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            ((failnum=$failnum+1))
        else
            if [ "X${abe}" != "X" ] && [ "X${abe}" != "X${paramnull}" ]; then
                /var/cm/script/cm_shell_exec.sh cm_set_abe ${nasname} "${abe}"
            fi
        fi
    fi
    
    if [ "X${aclinherit}" != "X" ] && [ "X${aclinherit}" != "X${paramnull}" ]; then
        local wp=("discard" "noallow" "restricted" "passthrough" "passthrough-x")
        CM_EXEC_CMD "zfs set origin:sync=${wp[$aclinherit]} ${nasname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            ((failnum=$failnum+1))
        fi
    fi
    
    if [ "X${quota}" != "X" ] && [ "X${quota}" != "X${paramnull}" ]; then
        local wp=("unkown" "disk" "poweroff" "mirror" "standard" "always")
        CM_EXEC_CMD "zfs set quota=${quota} ${nasname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            ((failnum=$failnum+1))
        fi
    fi
    
    local sfver=`cm_systerm_version_get`
    if [ $sfver -eq ${CM_SYS_VER_SOLARIS_V7R16} ]; then
        return $failnum
    fi
    
    if [ "X${qos}" != "X" ] && [ "X${qos}" != "X${paramnull}" ]; then
        CM_EXEC_CMD "zfs set bandwidth=${qos} ${nasname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            ((failnum=$failnum+1))
        fi
    fi
    
    if [ $sfver -eq ${CM_SYS_VER_SOLARIS_NOMASTER} ]; then
        return $failnum
    fi
    
    if [ "X${qos_avs}" != "X" ] && [ "X${qos_avs}" != "X${paramnull}" ]; then
        CM_EXEC_CMD "zfs set nasavsbw=${qos_avs} ${nasname}"
        iRet=$?
        if [ $iRet -ne $CM_OK ]; then
            ((failnum=$failnum+1))
        fi
    fi
    
    return $failnum
}

function cm_cnm_nas_create_exec()
{
    local params="$1"
    local nasname=`echo $params |awk -F'|' '{print $1}'`
    local iRet=$CM_OK
    local sfver=`cm_systerm_version_get`
    if [ $sfver -eq ${CM_SYS_VER_SOLARIS_NOMASTER} ]; then
        /var/cm/script/cm_shell_exec.sh cm_nas_create "$nasname"
        iRet=$?
    else
        CM_EXEC_CMD "zfs create ${nasname}"
        iRet=$?
    fi
    
    if [ $iRet -ne $CM_OK ]; then
        return $iRet
    fi
    cm_cnm_nas_set "$params"
    return $iRet
}

function cm_cnm_nas_create()
{
    local params=$1
    local nasname=`echo $params |awk -F'|' '{print $1}'`
    
    #cm_cnm_localtask_create "create nas $nasname" "cm_cnm_nas_create_exec $params" |awk '{print $2}'
    cm_cnm_nas_create_exec "$params"
    return $?
}

function cm_cnm_nas_update()
{
    local params=$1
    local nasname=`echo $params |awk -F'|' '{print $1}'`
    
    #cm_cnm_localtask_create "update nas $nasname" "cm_cnm_nas_set $params" |awk '{print $2}'
    cm_cnm_nas_set "$params"
    if [ $? -ne 0 ]; then
        return $CM_FAIL
    fi
    return $CM_OK
}

function cm_cnm_nas_delete()
{
    local params=$1
    local nasname=`echo $params |awk -F'|' '{print $1}'`
    
    #cm_cnm_localtask_create "delete nas $nasname" "zfs destroy -rRf $nasname" |awk '{print $2}'
    CM_EXEC_CMD "zfs destroy -rRf $nasname"
    return $?
}

function cm_cnm_nas_getbatch_v7()
{
    local opt="name,used,avail,quota,recordsize,compression,sync,dedup,sharesmb,aclinherit"
    local nasname=$1
    zfs list -H -t filesystem -o $opt $nasname |grep '/' \
        |while read line
    do
        local arr=($line)
        local name=${arr[0]}
        local laccess=`stat /${name} |grep Uid |cut -b 11-13`
        if [ "X$laccess" == "X" ]; then
            laccess=755
        fi
        echo "$line none none $laccess"
    done
    return 0
}

function cm_cnm_nas_getbatch_nomaster()
{
    local opt="name,used,avail,quota,recordsize,compression,sync,dedup,sharesmb,aclinherit,bandwidth"
    local nasname=$1
    zfs list -H -t filesystem -o $opt $nasname |grep '/' \
        |while read line
    do
        local arr=($line)
        local name=${arr[0]}
        local laccess=`stat /${name} |grep Uid |cut -b 11-13`
        if [ "X$laccess" == "X" ]; then
            laccess=755
        fi
        echo "$line none $laccess"
    done
    return 0
}

function cm_cnm_nas_getbatch_def()
{
    local opt="name,used,avail,quota,recordsize,compression,sync,dedup,sharesmb,aclinherit,bandwidth,nasavsbw"
    local nasname=$1
    zfs list -H -t filesystem -o $opt $nasname |grep '/' \
        |while read line
    do
        local arr=($line)
        local name=${arr[0]}
        local laccess=`stat /${name} |grep Uid |cut -b 11-13`
        if [ "X$laccess" == "X" ]; then
            laccess=755
        fi
        echo "$line $laccess"
    done
    return 0
}

function cm_cnm_nas_getbatch()
{
    local sfver=`cm_systerm_version_get`
    local nasname="$1"
    
    if [ $sfver -eq ${CM_SYS_VER_SOLARIS_NOMASTER} ]; then
        cm_cnm_nas_getbatch_nomaster "$nasname"
    elif [ $sfver -eq ${CM_SYS_VER_SOLARIS_V7R16} ]; then
        cm_cnm_nas_getbatch_v7 "$nasname"
    else
        cm_cnm_nas_getbatch_def "$nasname"
    fi
    return 0
}

${CM_MOD_NAME}"_$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8"
exit $?
