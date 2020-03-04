#!/bin/bash

source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

CM_TOPO_DB='/var/cm/data/cm_topo.db'
CM_NODE_DB='/var/cm/data/cm_node.db'
CM_DEVICE_DB='/tmp/cm_device.db'
CM_DISK_DB='/var/cm/data/cm_db_disk.db'

CM_FUNC_PRE='cm_topo_'

function cm_topo_help()
{
    echo "format:"
    echo "$0 local_device"
    echo "$0 getbatch [frame]"
    echo "$0 check_enidchange"
    echo "$0 savesnmap"
    echo "$0 init <dev_cfg_file>"
    echo "$0 initall <dev_cfg_file>"
    echo "$0 initnode <node_cfg_file>"
    echo "$0 initnodeall <node_cfg_file>"
    echo "$0 cache_update"
    echo "$0 table_off <enid>"
    echo "$0 pool_stat"
    return 0
}

function cm_topo_local_device()
{
    if [ ! -f $CM_DEVICE_DB ]; then
        echo "none"
    fi
    sqlite3 ${CM_DEVICE_DB} "SELECT serial,enid FROM device_t ORDER BY enid" |sed 's/|/ /g'
    return 0
}

function cm_topo_getbatch()
{
    local frame=$1
    if [ "X$frame" == "X" ]; then
        sqlite3 ${CM_TOPO_DB} "SELECT name,sn,Unum,slot,setx,enid FROM topo_t ORDER BY name,setx" |sed 's/|/ /g'
    else
        sqlite3 ${CM_TOPO_DB} "SELECT name,sn,Unum,slot,setx,enid FROM topo_t WHERE name='$frame' ORDER BY name,setx" |sed 's/|/ /g'
    fi
    return 0
}

function cm_topo_check_enidchange()
{
    local headerid=`/var/cm/script/cm_disk.sh local_enid`
    local sbbid=0
    ((sbbid=$headerid/1000))
    sqlite3 ${CM_TOPO_DB} "SELECT topo_t.enid,device_t.device,topo_t.sn FROM topo_t,device_t WHERE (topo_t.enid=0 OR topo_t.enid/1000=$sbbid) AND device_t.sn=topo_t.sn ORDER BY topo_t.enid" |sed 's/|/ /g' |while read line
    do
        local info=($line)
        local enid=${info[0]}
        local device=${info[1]}
        local sn=${info[2]}
        local curenid=`sqlite3 ${CM_DEVICE_DB} "SELECT enid FROM device_t WHERE serial='$device' LIMIT 1"`
        
        if [ "X$enid" == "X$curenid" ] || [ "X$curenid" == "X0" ]; then
            continue
        fi
        CM_LOG "[${FUNCNAME}:${LINENO}]sn:$sn device:$device enid:$enid newenid:$curenid "
        if [ "X$curenid" == "X" ]; then
            continue
        fi
        ceres_cmd exec "sqlite3 ${CM_TOPO_DB} \"UPDATE topo_t SET enid=$curenid WHERE sn='$sn'\""
    done
    return 0
}

function cm_topo_getdeviceid()
{
    local enid=$1
    local localenid=$2
    local tmpnum=0
    local devid=""
    if [ $enid -ge $localenid ]; then
        ((tmpnum=$enid-$localenid))
        if [ $tmpnum -lt 1000 ]; then
            #get from local
            devid=`sqlite3 ${CM_DEVICE_DB} "SELECT serial FROM device_t WHERE enid=$enid LIMIT 1"`
            echo $devid
            return 0
        fi
    fi
    #get from remote
    ((tmpnum=$enid%1000))
    ((localenid=$enid-$tmpnum))
    ((tmpnum=$enid/1000))
    local remoteip=`sqlite3 ${CM_NODE_DB} "SELECT ipaddr FROM record_t WHERE sbbid=$tmpnum LIMIT 1"`
    if [ "X$remoteip" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]get $tmpnum nodeip fail"
    fi
    devid=`ceres_exec $remoteip "/var/cm/script/cm_topo.sh getdeviceid $enid $localenid"`
    if [ "X$devid" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]get devid of $enid from $remoteip fail"
    fi
    echo $devid
    return 0
}

function cm_topo_savesnmap()
{
    local headerid=`/var/cm/script/cm_disk.sh local_enid`
    sqlite3 ${CM_TOPO_DB} "CREATE TABLE IF NOT EXISTS device_t (enid INT,device VARCHAR(127),sn VARCHAR(127))"
    local cntdev=`sqlite3 ${CM_TOPO_DB} "SELECT COUNT(device) FROM device_t"`
    local cnttopo=`sqlite3 ${CM_TOPO_DB} "SELECT COUNT(sn) FROM topo_t"`
    
    if [ $cntdev -eq $cnttopo ]; then
        return 0
    fi
    
    sqlite3 ${CM_TOPO_DB} 'DELETE FROM device_t'
    sqlite3 ${CM_TOPO_DB} "SELECT enid,sn FROM topo_t ORDER BY enid" |sed 's/|/ /g' |while read line
    do
        local info=($line)
        local enid=${info[0]}
        local sn=${info[1]}
        local devid=`cm_topo_getdeviceid $enid $headerid`
        CM_LOG "[${FUNCNAME}:${LINENO}]sn:$sn device:$devid enid:$enid"
        if [ "X$devid" == "X" ]; then
            echo "get $enid devid fail"
            continue
        fi
        local sql="INSERT INTO device_t VALUES($enid, '$devid','$sn')"
        echo $sql
        sqlite3 ${CM_TOPO_DB} "$sql"
    done
    return 0
}

function cm_topo_init()
{
    local cfgfile=$1
    if [ "X$cfgfile" == "X" ]; then
        echo "ERROR: format:$0 <cfgfile>"
        return 1
    fi
    
    if [ ! -f $cfgfile ]; then
        echo "ERROR: file[$cfgfile] not found"
        return 1
    fi
    
    if [ ! -f $CM_DEVICE_DB ]; then
        echo "ERROR: $CM_DEVICE_DB need update,please wait!"
        return 1
    fi
    
    local OLD_SCRIPT='/var/cm/script/cm_cnm_expansion_cabinet.sh'
    if [ -f ${OLD_SCRIPT} ]; then
        local rows=`wc -l $OLD_SCRIPT |awk '{print $1}'`
        if [ $rows > 4 ]; then
            mv $OLD_SCRIPT $OLD_SCRIPT".backup"
            echo '#!/bin/bash' >$OLD_SCRIPT
            echo 'echo 0' >>$OLD_SCRIPT
            echo 'exit 0' >>$OLD_SCRIPT
            chmod 755 $OLD_SCRIPT
        fi
    fi
    
    local TOPODBFILE=$CM_TOPO_DB
    if [ ! -f $TOPODBFILE ]; then
        touch $TOPODBFILE
        sqlite3 $TOPODBFILE "CREATE TABLE topo_t (enid INT,name VARCHAR(127),setx INT,typex INT,Unum INT,slot INT,sn VARCHAR(127))"
    fi
    
    sqlite3 $TOPODBFILE 'DELETE FROM topo_t'
    grep -v "^#" $cfgfile |sed '/^$/d' |while read line
    do
        local info=($line)
        local col_num=${#info[*]}
        if [ $col_num -ne 6 ]; then
            echo "ERROR: cfg[ $info ]"
            continue
        fi
 
        local framename=${info[0]}
        local sn=${info[1]}
        local unum=${info[2]}
        local slotnum=${info[3]}
        local postion=${info[4]}
        local enid=${info[5]}
        
        local typex=1
        ((typex=$enid%1000))
        if [ $typex -gt 0 ]; then
            typex=2
        elif [ $slotnum -gt 20 ]; then
            typex=1
        else
            typex=0
        fi
        local sql="INSERT INTO topo_t VALUES($enid,'$framename',$postion,$typex,$unum,$slotnum,'$sn')"
        echo $sql
        sqlite3 ${TOPODBFILE} "$sql"
    done
    local cnt=`sqlite3 ${TOPODBFILE} "SELECT COUNT(enid) FROM topo_t"`
    echo "Update $cnt records success!"
    
    cm_topo_savesnmap
    return 0
}

function cm_topo_initnode()
{
    local cfgfile=$1
    if [ "X$cfgfile" == "X" ]; then
        echo "ERROR: format:$0 <cfgfile>"
        return 1
    fi
    
    if [ ! -f $cfgfile ]; then
        echo "ERROR: file[$cfgfile] not found"
        return 1
    fi
    if [ ! -f $CM_NODE_DB ]; then
        touch $CM_NODE_DB
        sqlite3 ${CM_NODE_DB} "CREATE TABLE record_t (id INT,ipaddr VARCHAR(64),name VARCHAR(64),subdomain INT,sbbid INT,ismaster TINYINT,idx INT)"
    fi
    
    svcadm disable ceres_cm
    sqlite3 ${CM_NODE_DB} 'DELETE FROM record_t'
    #idx hostname ipaddr
    grep -v "^#" $cfgfile |sed '/^$/d' |while read line
    do
        local info=($line)
        local col_num=${#info[*]}
        if [ $col_num -ne 3 ]; then
            echo "ERROR: cfg[ $info ]"
            continue
        fi
        local idx=${info[0]}
        local name=${info[1]}
        local ipaddr=${info[2]}
        local id=`echo $ipaddr |awk -F'.' '{myid=$3*512+$4;print myid}'`
        local subdomain=0
        local sbbid=0
        local ismaster=0
        ((sbbid=($idx+1)>>1))
        ((ismaster=$idx&1))
        local sql="INSERT INTO record_t VALUES($id,'$ipaddr','$name',$subdomain,$sbbid,$ismaster,$idx)"
        echo $sql
        sqlite3 ${CM_NODE_DB} "$sql"    
    done    
    svcadm enable ceres_cm
    
    sqlite3 ${CM_NODE_DB} "SELECT * FROM record_t ORDER BY idx" |sed 's/|/ /g'
    return 0
}

function cm_topo_get_enid()
{
    local enclosure=$1
    local headerid=$2
    local disksn=`sqlite3 ${CM_DEVICE_DB} "SELECT serial FROM disk_t WHERE enclosure=$enclosure LIMIT 1"`
    
    if [ "X$disksn" == "X" ]; then
        echo '0'
        CM_LOG "[${FUNCNAME}:${LINENO}]enclosure:$enclosure headerid:$headerid disksn null"
        return 0
    fi
    
    local enid=`sqlite3 ${CM_DISK_DB} "SELECT enid FROM record_t WHERE sn='$disksn' LIMIT 1"`
    if [ "X$enid" == "X" ]; then
        echo '0'
        CM_LOG "[${FUNCNAME}:${LINENO}]enclosure:$enclosure headerid:$headerid enid null"
        return 0
    fi
    ((enid=$headerid+$enid-22))
    echo $enid
    return 0
}

function cm_topo_cache_update()
{
    local datestr=`date "+%Y%m%d%H%M%S"`
    local tmpfile="/tmp/cm_fmdtopo_$datestr.data"
    local fmdtopoexec='/usr/lib/fm/fmd/fmtopo'
    local dbfile=$CM_DEVICE_DB
    
    if [ ! -f $dbfile ]; then
        touch $dbfile
    fi
    
    sqlite3 ${dbfile} "CREATE TABLE IF NOT EXISTS device_t (serial VARCHAR(127),part VARCHAR(64),enclosure INT,enid INT)"
    sqlite3 ${dbfile} "CREATE TABLE IF NOT EXISTS disk_t (serial VARCHAR(127),device VARCHAR(64),enclosure INT, bay INT)"
    local localdiskcnt=`sqlite3 ${CM_DISK_DB} "SELECT COUNT(id) FROM record_t WHERE enid<1000"`
    if [ $localdiskcnt -eq 0 ]; then
        return 0
    fi
    local topodiskcnt=`sqlite3 ${dbfile} "SELECT COUNT(serial) FROM disk_t"`
    if [ $topodiskcnt -eq $localdiskcnt ]; then
        return 0
    fi
    
    local headerid=`/var/cm/script/cm_disk.sh local_enid`
    
    if [ "X$headerid" == "X" ]; then
        return 0
    fi
    
    local checkfile='/tmp/cm_topo_cache_update.flag'
    if [ -f $checkfile ]; then
        return 0
    fi
    
    sqlite3 ${CM_TOPO_DB} "CREATE TABLE IF NOT EXISTS device_t (enid INT,device VARCHAR(127),sn VARCHAR(127))"
    
    touch $checkfile
    
    $fmdtopoexec |grep serial |grep -v '?' |grep 'ses-enclosure' > $tmpfile
    sqlite3 ${dbfile} "DELETE FROM disk_t"
    #获取硬盘信息
    #N8H2BVHZ HGST-HUS726040AL5210 16 1
    #K7GJG0SL HGST-HUS726040AL5210 16 2
    #NHGMR8AK HGST-HUS726040AL5210 16 3
    #K7GHJ05L HGST-HUS726040AL5210 16 4
    grep disk $tmpfile |sed 's/[:/=]/ /g' |awk '{print $8" "$6" "$14" "$16}' |while read line
    do
        local info=($line)
        local col_num=${#info[*]}
        if [ $col_num -ne 4 ]; then
            continue
        fi
        local serial=${info[0]}
        local part=${info[1]}
        local enclosure=${info[2]}
        local bay=${info[3]}
        sqlite3 ${dbfile} "INSERT INTO disk_t VALUES('$serial','$part',$enclosure,$bay)" 
    done
    
    #获取设备信息
    #50015b21401870bf HA401-Expander 0
    #50015b21401cc77f 4U24SAS3swap 1
    #50015b2140183b7f 4U24SAS3swap 2
    sqlite3 ${dbfile} "DELETE FROM device_t"
    grep -v disk $tmpfile |sed 's/[:/=]/ /g' |awk '{print $7" "$9" "$13}' |while read line
    do
        local info=($line)
        local col_num=${#info[*]}
        if [ $col_num -ne 3 ]; then
            continue
        fi
        local serial=${info[0]}
        local part=${info[1]}
        local enclosure=${info[2]}
        local enid=`cm_topo_get_enid $enclosure $headerid`
        sqlite3 ${dbfile} "INSERT INTO device_t VALUES('$serial','$part',$enclosure,$enid)" 
    done
    
    rm -f $tmpfile
    
    cm_topo_check_enidchange
    rm -f $checkfile
    
    return 0
}

#==================================================================================
# fmtopo 一般数据格式：
# 	hc://:product-id=XXXX:server-id=:chassis-id=XXXX/ses-enclosure=XXXX/XXXX=XXXX
#
# 控制器数据一般格式：
#	hc://:product-id=XXXX:server-id=XXXX:chassis-id=XXXX/XXXX=XXXX/XXXX=XXXX
#========================================================================== ========

function cm_topo_alarm()
{
    local option=$1
    local alarm_id=$2
    local topo_info=$3
    local topo_enid
    local type_check=`echo $topo_info|grep -w 'ses-enclosure' |wc -l`
    if [ $type_check -eq 0 ]; then
        topo_enid=0
    else
        topo_enid=`echo $topo_info|awk -F'=' '{print $5}'| awk -F'/' '{print $1}'`
    fi
    local enid=`sqlite3 $CM_DEVICE_DB "SELECT enid FROM device_t WHERE enclosure=$topo_enid"`
    if [ "X$enid" == "X" ]; then
        return 1
    fi
    local sn=`sqlite3 $CM_TOPO_DB "SELECT sn FROM topo_t WHERE enid=$enid"`
    if [ "X$sn" == "X" ]; then
        sn=$enid
    fi
    local object_id=`echo $topo_info|awk -F'=' '{print $6}'`
    local dev_type=$enid
    ((dev_type=dev_type%1000))
    if [ $dev_type -ne 0 ]; then
        dev_type=1 #扩展柜
    else
        dev_type=0 #机头
    fi
    ((object_id=$object_id+1))
    #echo "$alarm_id $sn $topo_enid $object_id"
    ceres_cmd alarm $option $alarm_id "$dev_type,$sn,$object_id"
    return $?
}

function cm_topo_table_off()
{
    local enid=$1
    local tmpnum=0
    local iRet=$CM_OK
    local localid=`/var/cm/script/cm_disk.sh local_enid`
    local cut=0
    ((cut=$enid%1000))

    CM_LOG "[${FUNCNAME}:${LINENO}]enid:$enid"

    if [ $cut -eq 0 ]; then
        return $CM_PARAM_ERR
    fi

    ((tmpnum=$enid/1000))
    ((localid=$localid/1000))
    if [ $tmpnum -eq $localid ]; then
        local ses_id=`sqlite3 ${CM_TOPO_DB} "SELECT device_t.device FROM topo_t,device_t WHERE device_t.sn=topo_t.sn and topo_t.enid=$enid LIMIT 1"`
        if [ "X$ses_id" == "X" ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}]ses_id $enid null"
            return $CM_FAIL
        fi
        local dirs=`ls /dev/es/`
        for dir in $dirs
        do
            cut=`sg_ses -p 1 /dev/es/$dir |grep $ses_id|wc -l`
            if [ $cut -ne 0 ]; then
                sg_ses --descriptor=PowerSupply01 --clear=3:5:1 /dev/es/$dir
                iRet=$?
                CM_LOG "[${FUNCNAME}:${LINENO}]enid:$enid device_id:$ses_id iRet:$iRet"	
                return $iRet
            fi
        done
        CM_LOG "[${FUNCNAME}:${LINENO}]$ses_id $enid not found"
        return $CM_FAIL
    fi

    local remoteip=`sqlite3 ${CM_NODE_DB} "SELECT ipaddr FROM record_t WHERE sbbid=$tmpnum LIMIT 1"`
    if [ "X"$remoteip = "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]remoteip null"
        return $CM_FAIL
    fi
    ceres_exec $remoteip "/var/cm/script/cm_topo.sh table_off $enid"
    iRet=$?
    return $iRet
}

function cm_topo_initall()
{
    local destdir='/tmp/cm_topo_devlist/'
    local cfgfile=$1
    if [ "X$cfgfile" == "X" ]; then
        echo "ERROR: format:$0 <cfgfile>"
        return 1
    fi
    
    if [ ! -f $cfgfile ]; then
        echo "ERROR: file[$cfgfile] not found"
        return 1
    fi
    local filename=${cfgfile##*/}
    cm_multi_exec "mkdir -p $destdir"
    cm_multi_send $cfgfile $destdir
    filename=${destdir}${filename}
    cm_multi_exec "cm_topo cache_update"
    cm_multi_exec "cm_topo init $filename"
    cm_multi_exec "rm -rf $destdir"
    return 0
}

function cm_topo_initnodeall()
{
    local destdir='/tmp/cm_topo_nodelist/'
    local cfgfile=$1
    if [ "X$cfgfile" == "X" ]; then
        echo "ERROR: format:$0 <cfgfile>"
        return 1
    fi
    
    if [ ! -f $cfgfile ]; then
        echo "ERROR: file[$cfgfile] not found"
        return 1
    fi
    local filename=${cfgfile##*/}
    cm_multi_exec "mkdir -p $destdir"
    cm_multi_send $cfgfile $destdir
    filename=${destdir}${filename}
    cm_multi_exec "cm_topo initnode $filename"
    cm_multi_exec "rm -rf $destdir"
    return 0
}


function cm_topo_pool_stat()
{
    /var/cm/script/cm_cnm.sh pool_stat
    return 0
}

if [ "X$1" == "X" ]; then
    ${CM_FUNC_PRE}help
    exit $?
else
    ${CM_FUNC_PRE}$*
    exit $?
fi
