#!/bin/bash

source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'
#==============================================================================
#调用说明
#./cm_shell_exec.sh <functionname> <arg1> <arg2> <...>
#==============================================================================

#==============================================================================
#在v7版本适配 stmfadm list_all_luns
#输出:
#600144F00100000000005CA5F0920001 poola/lun1
#==============================================================================
function stmfadm_list_all_luns()
{
    stmfadm list-lu -v 2>/dev/null |sed 's/ //g' \
        |egrep "LUName|OperationalStatus|Alias|DataFile|AccessState" \
        |awk -F':' '$1=="LUName"{print ""}{printf $2" "}END{print ""}' \
        |sed '/^$/d' \
        |awk '{if(($2=="Online")){if(($5=="Active")){print $1" "$4}else{print $1" "$3}}}' \
        |awk -F'/' '{print $1$(NF-1)"/"$NF}'
    return $?
}

#==============================================================================
#节点性能统计,除了内存之外其他是累加的情况，C代码中计算差值
#输出 (bandwidth)rbytes obytes (iops)reads writes (cpu)ticks ilds (mem)rate 
#==============================================================================
function cm_pmm_nics()
{
    cat /proc/net/dev|grep ':'|egrep -v 'lo:'|awk '{print $1" "$2" "$10}'|while read line
    do
        local info=($line)
        local name=${info[0]}
        name=${name%?}
        local read=${info[1]}
        local write=${info[2]}
        
        echo "$name $read $write"
    done
    return 0
}

function cm_pmm_node_stat()
{
    local bandwidth=`cm_pmm_nics |awk 'BEGIN{ob=0;rb=0}{ob+=$2;rb+=$3}END{print ob" "rb}'`
    #*100是为了uint64取值，之后会/100取得正确结果
    local iops=`iostat -dx|sed "1,3d"|awk 'BEGIN{rs=0;ws=0}{rs+=$4;ws+=$5}END{print rs*100" "ws*100}'`
    local cpu=`iostat -c | grep '\.'|egrep -v Linux|awk '{print ($1+$3)*100" "$6*100}'`
    local mem=`free -t|grep Mem|awk '{print $3/$2*100}'`
    
    if [ "X$bandwidth" == "X " ]; then
        bandwidth="0 0"
    fi
    if [ "X$iops" == "X " ]; then
        iops="0 0"
    fi
    if [ "X$cpu" == "X " ]; then
        cpu="0 0"
    fi
    if [ "X$mem" == "X" ]; then
        mem="0"
    fi
    echo "$bandwidth $iops $cpu $mem"
    return 0
}

#==============================================================================
#fc端口信息查询
#输出:
#500143800637a024 Initiator qlc online 1Gb2Gb4Gb 4Gb
#500143800637a026 Initiator qlc online 1Gb2Gb4Gb 4Gb
#==============================================================================
function cm_cnm_fcinfo_getbatch()
{
    fcinfo hba-port |egrep 'Port WWN|Port Mode|State|Driver Name|Supported Speeds|Current Speed'|awk -F':' '{print $2}'|awk  'NR%6!=0{printf $1$2$3$4" "} NR%6==0{print $1}'
    return $?
}

function cm_log_backup()
{
    local logdir='/var/cm/log'
    local backdir='/var/cm/log/backup'
    local datestr=`date "+%Y%m%d_%H%M%S"`
    local tarname="cm_log_${datestr}.tar"
    if [ ! -d $backdir ]; then
        mkdir -p $backdir
    fi
    mv $logdir/*.log $backdir/*.log
    tar -vxf $backdir/$tarname ${logdir}/*.log
    gzip $backdir/$tarname
    return $?
}

function cm_log_clear()
{
    local logdir='/var/cm/log'
    rm -f $logdir/*.log
    return $?
}

function check_add_ipmiuser()
{
    local devicetype=`prtdiag -v|sed -n 1p |awk '{print $3}'`
    if [ "X$devicetype" != "XSupermicro" ]; then
        return 0
    fi
    local cnt=`ipmitool user list 1 |grep -w admin |wc -l`
    if [ $cnt -gt 0 ]; then
        return 0
    fi
    local newuid=5
    ipmitool user set name $newuid admin
    ipmitool user set password $newuid admin
    ipmitool user priv $newuid 4 1
    ipmitool user enable $newuid
    return 0
}

function cm_cluster_nas_check()
{
    local cfgdir='/etc/fsgroup'
    local cfgfile='/etc/fsgroup/rpc_port.conf'
    if [ ! -d $cfgdir ]; then
        mkdir -p $cfgdir
    fi
    local mport=`cm_get_localmanageport`
    if [ ! -f /etc/hostname.${mport} ]; then
        return 0
    fi
    echo "rpc_port=${mport};" >$cfgfile
    local mgip=`cat /etc/hostname.${mport} |sed -n 1p`
    echo "rpc_addr=${mgip};" >>$cfgfile
    
    return 0
}

function cm_get_hoststrid()
{
    local numid=$1
    local strarr=('_' '`' 'a' 'b' 'c' 'd' 'e' 'f' 'g' 'h')
    local strnum=""
    local index=0
    while((1))
    do
        ((index=$numid%10))
        strnum="${strarr[$index]}$strnum"
        ((numid=($numid-$index)/10))
        if [ $numid -eq 0 ]; then
            break
        fi
    done
    echo $strnum
    return 0
}

function cm_get_localcmid()
{
    local mport=`cm_get_localmanageport`
    local interid=`ifconfig ${mport} | grep 'inet '|awk '{print $2}'|awk -F'.' '{myid=$3*512+$4;print myid}'`
    echo $interid
    return 0
}

function cm_get_nodeinfo()
{
    local myname=`hostname`
    local ramsize=`cat /proc/meminfo | grep MemTotal| awk '{print $2}'`
    ((ramsize=$ramsize/1024))
    local interid=`cm_get_localid`
    local ostype=`cm_os_type_get`
    local version=`cm_software_version`
    local devtype='AIC'
    devtype=`dmidecode | grep Maufacturer|awk '{print $2}'`
    
    local myip="127.0.0.1"
    local mport=`cm_get_localmanageport`
    myip=`ifconfig ${mport} | grep 'inet '|awk '{print $2}'`
    local snum=16
    if [ "X$devtype" == "XAIC" ]; then
        devtype=1
        snum=24
    else
        devtype=0
        snum=16
    fi
    echo "$interid $myname $devtype $snum $version $ramsize $myip"
    return 0
}

function cm_disk_update_cache()
{
    cp /var/cm/data/db_disk.db /var/cm/data/cm_db_disk.db 2>/dev/null
    return 0
}

function cm_period_5min()
{
    /var/cm/script/cm_topo.sh cache_update &
    /var/cm/script/cm_topo.sh savesnmap 1>/dev/null 2>/dev/null &
    return 0
}

function cm_lun_delete()
{
    local pool=$1
    local lun=$2
    local stmfid=`stmfadm list-all-luns 2>/dev/null |awk '$2=="'$pool'/'$lun'"{print $1}'`
    if [ "X$stmfid" != "X" ]; then
        local cnt=`stmfadm list-view -l $stmfid 2>/dev/null |wc -l|awk '{print $1}'`
        if [ $cnt -eq 0 ]; then
            local ostype=`cm_os_type_get`
            if [ $ostype -eq ${CM_OS_TYPE_ILLUMOS} ]; then
                stmfadm delete-lu -c $stmfid
            else
                local lustate=`stmfadm list-lu -v $stmfid |grep 'Access State' |awk '{printf $4}'`
                if [ "X$lustate" == "XActive" ]; then
                    stmfadm delete-lu $stmfid
                else
                    cm_multi_exec "stmfadm delete-lu $stmfid"
                fi
            fi
        else
            return $CM_ERR_LUNMAP_EXISTS
        fi
    else
        return $CM_ERR_NOT_EXISTS
    fi
    zfs destroy -rRf $pool/$lun
    return $?
}


function cm_masterip_check()
{
    local cfgfile=$1
    local tmp=`grep '^time' $cfgfile`
    if [ "X$tmp" == "X" ]; then
        return 1
    fi
    tmp=`grep -w 'ip' $cfgfile |sed 's/ //g' |awk -F'=' '{print $2}'`
    if [ "X$tmp" == "X" ]; then
        return 1
    fi
    tmp=`grep -w "$tmp" /etc/clumgt.config`
    if [ "X$tmp" != "X" ]; then
        return 1
    fi
    return 0
}

function cm_masterip_bind()
{
    local cfgfile='/var/cm/data/cm_cluster.ini'
    cm_masterip_check $cfgfile
    local ret=$?
    if [ "X$ret" == "X1" ]; then
        return 0
    fi
    
    local ntmsk=`grep -w 'netmask' $cfgfile |sed 's/ //g' |awk -F'=' '{print $2}'`
    if [ "X$ntmsk" == "X" ]; then
        return 0
    fi
    
    local tmp=`grep -w 'ip' $cfgfile |sed 's/ //g' |awk -F'=' '{print $2}'`
    ping $tmp -c 1 1>/dev/null 2>/dev/null
    ret=$?
    if [ "X$ret" == "X0" ]; then
        return 0
    fi
    
    if [ `ifconfig -a| grep "$tmp "|wc -l` -ne 0 ]; then
        return 0
    fi    
    local mport=`cm_get_localmanageport`
    ifconfig ${mport} add $tmp netmask $ntmsk up 1>/dev/null 2>/dev/null
    ret=$?
    if [ "X$ret" != "X0" ]; then
        return 1
    fi
    
    tmp=`grep -w 'gateway' $cfgfile |sed 's/ //g' |awk -F'=' '{print $2}'`
    if [ "X$tmp" == "X" ]; then
        return 0
    fi
    ret=`netstat -nr |grep -w "$tmp"`
    if [ "X$tmp" != "X" ]; then
        return 0
    fi
    route add default gw $tmp 1>/dev/null 2>/dev/null
    ret=$?
    return 0
}

function cm_masterip_release()
{
    local cfgfile='/var/cm/data/cm_cluster.ini'
    cm_masterip_check $cfgfile
    local ret=$?
    if [ "X$ret" == "X1" ]; then
        return 0
    fi
    
    local tmp=`grep -w 'ip' $cfgfile |sed 's/ //g' |awk -F'=' '{print $2}'`
    ret=`ifconfig -a |grep -w "$tmp"`
    if [ "X$ret" == "X" ]; then
        return 0
    fi
    local mport=`cm_get_localmanageport`
    ifconfig $mport removeif $tmp 1>/dev/null 2>/dev/null
    return $?
}

function cm_set_abe()
{
    local nasdir=$1
    local abe=$2
    local mountpoint=$nasdir
    local everyonedef='everyone@:r-x---a-R-c--s:-------:allow'
    local groupdef='group@:r-x---a-R-c--s:-------:allow'
    if [ "X${nasdir:0:1}" != 'X/' ]; then
        mountpoint=`zfs list -H -o mountpoint $nasdir 2>/dev/null`
        if [ "X$mountpoint" = "X" ]; then
            #echo "$nasdir not existed"
            return $CM_PARAM_ERR
        fi
    fi
    local rows=`ls -dV $mountpoint |egrep 'everyone@:|group@:'`
    if [ "X$abe" == "X1" ]; then
        #删除everyone group 权限
        for row in $rows
        do
            chmod A-${row} $mountpoint 2>/dev/null
        done
    else
        if [ "X$rows" == "X" ]; then
            chmod A+${groupdef} $mountpoint
            chmod A+${everyonedef} $mountpoint
        fi
    fi
    return 0
}

function cm_nas_create()
{
    local nasname=$1
    local cmd='zfs list -t filesystem -o node_id -H'
    local myids=`cm_multi_exec "$cmd"`
    interid=`echo $myids |awk '{for(i=1;i<=NF;i++)print $i}'|sort -n |awk 'BEGIN{id=1}$1<id{continue}$1==id{id++;continue}$1>id{break}END{print id}'`
    local policy=""
    local policy_file='/var/cm/data/cm_nas_create_policy.flag'
    if [ -f $policy_file ]; then
        policy='hash'
    else
        policy='route'
    fi
    zfs create -o node_id=$interid -o group_policy=$policy $nasname
    return $?
}

function cm_phys_ip_delete()
{
    local ipaddr=$1
    local nic=$2
    /var/cm/script/cm_cnm_phys_ip.sh delete $ipaddr $nic
    return $?
}

function cm_zfs_set()
{
    local name=$1
    local prop=$2
    local val=$3
    
    if [ "X$name" == "X" ] || [ "X$prop" == "X" ] || [ "X$val" == "X" ]; then
        return $CM_OK
    fi
    local oldval=`zfs get $prop $name |sed 1d |awk '{print $3}'`
    if [ "X$oldval" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]get $name $prop null"
        return $CM_FAIL
    fi
    
    if [ "X$oldval" == "X$val" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]set $name $prop $val same"
        return $CM_OK
    fi
    zfs set $prop=$val $name
    local res=$?
    CM_LOG "[${FUNCNAME}:${LINENO}]set $name $prop $val res=$res"
    return $res
}

function cm_pmm_get_lu()
{
    local lunname=$1
    kstat -m stmf -c misc -n stmf_lu_* -s lun-alias \
    |awk '$2!=""{print $2}' \
    |awk 'BEGIN{i=0}i==1{printf $1" "}i==2{print $1}{i++}i==3{i=0}' \
    |awk '$2=="/dev/zvol/rdsk/$lunname"{print $1}'|awk -F'_' '{printf $3}'
    return $?    
}

function cm_pmm_lun()
{
    local stmf_id=$1
    kstat -m stmf -n $stmf_id|egrep 'reads|writes|nread|nwritten'| awk '{printf $2" "}'
    return $?
}

function cm_pmm_lun_check()
{
    local name=$1
    grep -w $name /tmp/lu.xml |wc -l
}

function cm_pmm_nic()
{
    local name=$1
    local read=`cat /proc/net/dev|grep $name|awk '{print $2}'`
    local write=`cat /proc/net/dev|grep $name|awk '{print $10}'`
    
    echo "0 0 100000000 2 0 0 $write $write 0 $read $read"
    return $?
}

function cm_pmm_dup_ifspeed()
{
    local name=$1
    kstat -m link -n $name|egrep 'link_duplex|ifspeed'|awk '{printf $2" "}'
    return $?
}

function cm_pmm_nic_check()
{
    local name=$1
    ip link|grep $name|wc -l
}

function cm_pmm_disk_instance()
{
    local name=$1
    #prtconf -a /dev/rdsk/$name|grep -w disk|awk -F'#' '{print $2}'
    name=`cat /tmp/disk.xml |grep "$name"|awk -F'>' '{print $2}'|awk -F'<' '{print $1}'`
    local diskname=`ls -l $name|awk -F'->' '{print $2}'|awk -F'/' '{print $3}'`
    cat /proc/diskstats| grep -n -w "$diskname" |awk -F':' '{print $1}'
}

function cm_pmm_disk()
{
    local num=$1
    #kstat -m sd -c disk -i $num \
    #|egrep 'snaptime|reads|writes|nread|nwritten|wlentime|rlentime|wtime|rtime' |awk '{printf $2" "}' 
    
    local array=($(cat /proc/diskstats|sed -n "$num p"|awk '{print $4" "$6" "$8" "$10}'))
    local riops=${array[0]}
    local read=${array[1]}
    local wiops=${array[2]}
    local write=${array[3]}
    
    echo "0 $read $write $riops 0 0 0 0 $wiops 0"
    return $?
}

function cm_pmm_disk_check()
{
    local name=$1
    cat /tmp/disk.xml |grep "$name"|wc -l
}

function cm_pmm_nas()
{
    local name=$1
    local ostype=`cm_os_type_get`
    
    if [ $ostype -eq ${CM_OS_TYPE_ILLUMOS} ]; then
        kstat -m unix -c misc -n $name \
        |egrep 'ncreate|nmkdir|nsymlink|nremove|nrmdir|nreaddir|nread|nwrite|read_bytes|write_bytes|rlentime' \
        |awk '{if ($1=="nreadlink") printf "";else if ($1=="read_bytes") printf $2" "0" ";else printf $2" "}'
    else
        kstat -m unix -c misc -n $name \
        |egrep 'ncreate|nmkdir|nsymlink|nremove|nrmdir|nreaddir|nread|nwrite|read_bytes|write_bytes|rlentime' \
        |awk '{if ($1=="nreadlink") printf "";else printf $2" "}'
    fi
}
function cm_pmm_pool()
{
    local name=$1
    #kstat -m zfs -n $name|egrep 'nread|nwritten|reads|writes'|awk '{printf $2" "}'
    cat /proc/spl/kstat/zfs/$name/io |awk 'NR==3{print $1" "$2" "$3" "$4}'
}
function cm_pmm_cache()
{
    cat /proc/spl/kstat/zfs/arcstats \
    | awk '$1=="hits"{printf $2" "}
    $1=="misses"{printf $2" "}
    $1=="l2_hits"{printf $2" "}
    $1=="l2_misses"{printf $2" "}
    $1=="size"{printf $2" "}
    $1=="l2_size"{printf $2" "}'
}
#==============================================================================
#执行函数
#==============================================================================
$*
exit $?
