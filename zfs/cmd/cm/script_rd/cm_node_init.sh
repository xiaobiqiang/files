#!/bin/bash

source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

#初始化/etc/prodigy.conf文件
function cm_node_init_prodigycfg()
{
    local maxnodenum=$1
    local cfgfile='/etc/prodigy.conf'
cat > $cfgfile <<PRODIGYCFG
CONTROLLER_AMOUNT=${maxnodenum}
CONTROLLER_VENDOR=SBB
CONTROLLER_TYPE=B10
SYSTEM_STMF_KS=ENABLE
ENCLOSURE_VENDOR=LS
ENCLOSURE_SLOT=24
ENCLOSURE_DISK_TYPE=SAS
ENCLOSURE_INTERFACE_TYPE=SAS
ENCLOSURE_ENCRYPT_TYPE=OFF
ENCLOSURE_AMOUNT=0
PRODIGYCFG
    return $CM_OK
}

function cm_node_gethostname()
{
    local strarr=('a' 'b' 'c' 'd' 'e' 'f' 'g' 'h' 'i' 'j' \
                   'k' 'l' 'm' 'n' 'o' 'p' 'q' 'r' 's' 't' \
                   'u' 'v' 'w' 'x' 'y' 'z')
    local arrlen=${#strarr[*]}
    local myname=""
    local index=0
    local numid=$1
    if [ $numid -gt 0 ]; then
        ((numid=$numid-1))
    fi
    while((1))
    do
        ((index=$numid%$arrlen))
        myname="${strarr[$index]}$myname"
        ((numid=($numid-$index)/$arrlen))
        if [ $numid -eq 0 ]; then
            break
        fi
    done
    echo "df"$myname
    return $CM_OK
}

function cm_node_init_host()
{
    local myid=$1
    #后续考虑从ipmitool中读取设备类型
    local defhostype='DF5000C'
    local cfgfile='/etc/hostid'
    
    echo '# DO NOT EDIT' >$cfgfile
    case ${defhostype} in
        DF3000C)
            echo '"sub___r"' >> $cfgfile
        ;;
        DF5000C)
            echo '"sud___r"' >> $cfgfile
        ;;
        DF8000C)
            echo '"sug___r"' >> $cfgfile
        ;;
        *)
            #默认 DF5000C
            echo '"sud___r"' >> $cfgfile
        ;;
    esac
    
    #计算底层识别的字符串ID
    local strid=`/var/cm/script/cm_shell_exec.sh cm_get_hoststrid $myid`
    echo \"${strid}\" >> ${cfgfile}
    
    #根据ID计算hostname
    local myname=`cm_node_gethostname $myid`
    echo $myname >/etc/nodename
    
    return $CM_OK
}

function cm_node_init_mip()
{
    local mip=$1
    local mport=`cm_get_localmanageport`
    local cfgfile="/etc/hostname.${mport}"
    
    echo "$mip" > $cfgfile
    echo "netmask 255.255.255.0" >> $cfgfile
    echo "up" >> $cfgfile
    return $CM_OK
}

function cm_node_update_rd_files()
{
    local tmprootdir=$1
    
    #stmf相关
    local PRODUCT_ID=1
    [ -f /tmp/stmf_sbd.conf ] && rm -f /tmp/stmf_sbd.conf
    sed '/sbd_product_id/d' /kernel/drv/stmf_sbd.conf >> /tmp/stmf_sbd.conf
    cat /tmp/stmf_sbd.conf |echo 'sbd_product_id="'${PRODUCT_ID}'";' >> /tmp/stmf_sbd.conf
    cp /tmp/stmf_sbd.conf /kernel/drv/stmf_sbd.conf
    
    #AVS相关
    [ -d /etc/hb ]&&rm -rf /etc/hb
    mkdir -p /etc/hb
    mkdir /etc/hb/avs
    touch /etc/hb/avs/cluster_remote_avs.conf
    touch /etc/hb/avs/cluster_local_avs.conf
    touch /etc/hb/avs/avs_host.conf

cat > /etc/hb/values << HBVALUES
farm.properties.cluster_id=1
HBVALUES
    
    #备份IB信息
    /var/cm/script/cm_cnm.sh ibinit "${tmprootdir}/etc/"
    #更新其他信息
    cp /etc/host* ${tmprootdir}/etc/
    cp /etc/nodename ${tmprootdir}/etc/
    cp /etc/prodigy.conf ${tmprootdir}/etc/
    
    cp /kernel/drv/stmf_sbd.conf ${tmprootdir}/kernel/drv/
    cp /sbin/dfboot.sh ${tmprootdir}/sbin/
    [ -f /etc/inet/static_route ]&&cp /etc/inet/static_routes ${tmprootdir}/etc/inet/
    
    sync
    
    #以下是从RD包中拷贝出来，防止手动替换后部分文件没有更新
    if [ -d ${tmprootdir}/var/cm ]; then
        cp -r ${tmprootdir}/var/cm/* /var/cm/
    fi
    sync
    return $CM_OK
}

function cm_node_update_rd()
{
    #获取分区大小
    local freespace=`df -h |grep ' /root$' |awk '{print $4}'`
    freespace=`/var/cm/script/cm_cnm.sh zpool_data_change_m ${freespace}`
    
    local cfgfile="/root/boot/grub/menu.lst"
    #获取当前启动路径
    local defaultval=`grep ^default ${cfgfile} |awk '{print $2}'`
    
    if [ "X$defaultval" == "X" ]; then
        echo "get defaultval fail"
        return $CM_FAIL
    fi
    ((defaultval=$defaultval+1))
    local currd=`grep ^module ${cfgfile} |sed -n ${defaultval}p |awk '{print $2}'`
    currd="/root${currd}"
    echo "current RD: $currd"
    
    #获取RD包大小
    local rdsize=`ls -lh ${currd} |awk '{print $5}'`
    rdsize=`/var/cm/script/cm_cnm.sh zpool_data_change_m ${rdsize}`
    echo "freespace: ${freespace} MB, rd package: ${rdsize} MB"
    
    #留50MB的预留空间
    ((rdsize=$rdsize+50))
    if [ $freespace -lt $rdsize ]; then
        echo "freespace: ${freespace} MB too small!"
        return $CM_FAIL
    fi
    
    #备份当前RD包
    local rdbk=`date "+%Y%m%d%H%M%S"`
    rdbk="${currd}.${rdbk}"
    echo "backup RD to: ${rdbk}"
    cp $currd $rdbk
    
    #更新当前RD包
    local tmprootdir="/tmp/minirdroot"
    echo "unpack RD ..."
    root_archive_dfusion unpack $currd $tmprootdir
    local iret=$?
    if [ $iret -ne 0 ]; then
        echo "unpack $currd fail"
        return $CM_FAIL
    fi
    echo "update files ..."
    cm_node_update_rd_files $tmprootdir
    
    echo "pack $currd ..."
    root_archive_dfusion pack $currd $tmprootdir
    iret=$?
    if [ $iret -ne 0 ]; then
        echo "pack $currd fail"
        return $CM_FAIL
    fi
    return $CM_OK
}

function cm_node_init_after()
{
    #检查安装GUI
    echo "check install GUI ..."
    /var/cm/script/cm_gui_install.sh >>/dev/null 2>/dev/null
    
    #关闭clusterd, 防止集群未建立导池
    echo "disable clusterd"
    svcadm disable clusterd
    return $CM_OK
}

function cm_node_init()
{
    local nodenum=$1
    local mid=$2
    local ipaddr=$3
    
    #校验节点数
    if [ "X$nodenum" == "X" ]; then
        printf "Please enter total node number: ";
        read nodenum
    fi
    cm_check_isnum $nodenum
    local iret=$?
    if [ $iret -ne $CM_OK ]; then
        echo "Input value error!"
        return $CM_FAIL
    fi
    if [ $nodenum -lt 2 ]; then
        echo "node num [$nodenum] not support!"
        return $CM_FAIL
    fi
    
    #校验节点ID
    if [ "X$mid" == "X" ]; then
        printf "Please enter hostid: ";
        read mid
    fi
    cm_check_isnum $mid
    iret=$?
    if [ $iret -ne $CM_OK ]; then
        echo "Input value error!"
        return $CM_FAIL
    fi
    if [ $mid -lt 1 ] || [ $mid -gt $nodenum ]; then
        echo "hostid [$mid] not support!"
        return $CM_FAIL
    fi
    
    #校验节点管理IP
    if [ "X$ipaddr" == "X" ]; then
        printf "Please enter IP address: ";
        read ipaddr
    fi
    cm_check_isipaddr $ipaddr
    iret=$?
    if [ $iret -ne $CM_OK ]; then
        echo "Input [ $ipaddr ]value error!"
        return $CM_FAIL
    fi
    #检测IP是否在使用
    iret=`ifconfig -a |grep " ${ipaddr} "`
    if [ "X$iret" == "X" ]; then
        #当前节点没有配置, 检测其他节点是否使用
        ping $ipaddr 1 >/dev/null 2>/dev/null
        iret=$?
        if [ $iret -eq $CM_OK ]; then
            echo "IP [ $ipaddr ] used by other node!"
            return $CM_FAIL
        fi
    fi
    
    cm_node_init_prodigycfg $nodenum
    cm_node_init_host $mid
    cm_node_init_mip $ipaddr
    
    echo "start update RD package"
    cm_node_update_rd
    iret=$?
    if [ $iret -ne $CM_OK ]; then
        echo "update RD package fail !!"
        return $iret
    fi
    echo "update RD package successfully"
    
    cm_node_init_after
    
    echo "Complete. Now please reboot your machine."
    return $CM_OK
}

cm_node_init $*
exit $?

