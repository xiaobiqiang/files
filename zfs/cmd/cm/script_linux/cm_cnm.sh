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

${CM_MOD_NAME}"_"$*
exit $?
