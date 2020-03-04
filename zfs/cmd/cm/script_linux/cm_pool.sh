#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

CM_DISK_DB='/var/cm/data/cm_db_disk.db'
#CM_DISK_DB='/tmp/cm_db_disk_test.db'
CM_RAID0=''
CM_RAID1='mirror'
CM_RAID5='raidz1'
CM_RAID6='raidz2'
CM_RAID7='raidz3'
CM_DISK_DB_CACHE='/tmp/cm_db_disk.db'

function cm_pool_destroy_xml()
{
    local pool=$1
    local datafile='/tmp/pool.xml'
    
    if [ ! -f $datafile ]; then
        return $CM_OK
    fi
    local tmpfile=$datafile".tmp"
    local isset=0
    rm -f $tmpfile
    cat $datafile |while read line
    do
        case "$line" in
            *"<pool "*)
                if [[ "$line" == *"\"$pool\""* ]]; then
                    isset=1
                fi
            ;;
            *"</pool>"*)
                if [ $isset -eq 1 ]; then
                    isset=2
                fi
            ;;
            *)
            ;;
        esac
        if [ $isset -eq 0 ]; then
            echo "$line" >> $tmpfile
        elif [ $isset -eq 2 ]; then
            isset=0
        fi
    done
    if [ -f $tmpfile ]; then
        mv $tmpfile $datafile
    fi
    return $CM_OK
}

function cm_pool_destroy()
{
    local pool=$1
    
    CM_LOG "[${FUNCNAME}:${LINENO}] start:$pool"
    
    #从缓存文件中删除
    #cm_pool_destroy_xml "$pool"
    
    #关共享
    zfs list -H -t filesystem -o name |grep -w "$pool" |while read nas
    do
        zfs set sharenfs=off $nas >> ${CM_LOG_DIR}${CM_LOG_FILE} 2>&1
        zfs set sharesmb=off $nas >> ${CM_LOG_DIR}${CM_LOG_FILE} 2>&1
    done
    
    #销毁存储池
    zpool destroy -f "$pool" >> ${CM_LOG_DIR}${CM_LOG_FILE} 2>&1
    CM_LOG "[${FUNCNAME}:${LINENO}] end: $pool $?"
    return $CM_OK
}

function cm_pool_local_enid()
{
    local interid=`hostid`
    interid=`printf %d "0x$interid"`
    ((interid=(($interid+1)>>1)*1000))
    echo $interid
    return 0
}

#=======================================================================================
#校验参数是否正常
#参数格式: "data" "data" "group" "spare" 
#           raid: 0, 1, 5, 6, 7
#           data: 数据盘数量 大于0
#           group: 组数, raid0始终为1，大于1时针对raid10,raid50,raid60,raid70
#           spare: 热备盘数量
#=======================================================================================
function cm_pool_check_param()
{
    local iret=$CM_OK
    local raid=$1
    local data=$2
    local spare=$4
    local group=$3
    if [ $group -eq 0 ]; then
        return $iret
    fi
    case $raid in
        0)
            if [ $data -lt 1 ]; then
                iret=$CM_PARAM_ERR
            fi
        ;;
        
        1)
            if [ $data -lt 2 ]; then
                iret=$CM_PARAM_ERR
            fi
        ;;
        
        5)
            if [ $data -lt 3 ]; then
                iret=$CM_PARAM_ERR
            fi
        ;;
        
        6)
            if [ $data -lt 4 ]; then
                iret=$CM_PARAM_ERR
            fi
        ;;
        
        7)
            if [ $data -lt 5 ]; then
                iret=$CM_PARAM_ERR
            fi
        ;;
        *)
        iret=$CM_PARAM_ERR
        ;;
    esac
    return $iret
}

function cm_pool_get_raid_str()
{
    local raid=$1
    case $raid in
        1)
            echo $CM_RAID1
        ;;
        5)
            echo $CM_RAID5
        ;;
        6)
            echo $CM_RAID6
        ;;
        7)
            echo $CM_RAID7
        ;;
        *)
        ;;
    esac
}

#=======================================================================================
#默认选盘方式
#       <data:"raid|data|group|spare">
#       <meta:"raid|data|group|spare>
#       <low:"raid|data|group|spare">
#=======================================================================================
function cm_pool_create_each_select_default()
{
    local raidname=$1
    local cnt=$2
    local createfile=$3
    local localenid=`cm_pool_local_enid`
    local nextenid=1000
    ((nextenid+=$localenid))
       
    local sql="SELECT count(id) FROM record_t WHERE enid>=$localenid AND enid<$nextenid AND status='free'"
    sql=`sqlite3 $CM_DISK_DB_CACHE "$sql"`
    if [ $sql -lt $cnt ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $raidname need:$cnt free:$sql!"
        return $CM_FAIL
    fi
    
    sql="SELECT id,enid,slotid FROM record_t WHERE enid>=$localenid AND enid<$nextenid AND status='free' ORDER BY enid,slotid LIMIT $cnt"
    #记录当前选用的硬盘
    echo "$raidname" >> ${CM_LOG_DIR}${CM_LOG_FILE}
    sqlite3 $CM_DISK_DB_CACHE "$sql" >> ${CM_LOG_DIR}${CM_LOG_FILE}
    echo ' \' >> $createfile
    sql="SELECT id FROM record_t WHERE enid>=$localenid AND enid<$nextenid AND status='free' ORDER BY enid,slotid LIMIT $cnt"
    local idlists=`sqlite3 $CM_DISK_DB_CACHE "$sql" |awk '{printf $1" "}'`
    echo -n "\t\t$raidname $idlists" >> $createfile
    #更新临时数据库中之前选中硬盘的状态
    sqlite3 $CM_DISK_DB_CACHE "UPDATE record_t SET status='busy' WHERE id IN($sql)"
    return $CM_OK
}

function cm_pool_create_each_select_distributed()
{
    local raidname=$1
    local cnt=$2
    local createfile=$3
    local localenid=`cm_pool_local_enid`
    local nextenid=1000
    local sql=''    
    sql="SELECT distinct(enid/1000*1000) FROM record_t ORDER BY enid"
    local enidarr=(`sqlite3 $CM_DISK_DB_CACHE "$sql"`)
    local enidnum=${#enidarr[*]}
    if [ $enidnum -eq 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] enidnum:$enidnum"
        return $CM_FAIL
    fi
    #最大enid
    local enidmax=${enidarr[$enidnum-1]}
    
    #检测是否能够均匀分配
    local diskspare=0
    ((diskspare=$cnt%$enidnum))
    #每个节点最少选择的硬盘数量
    local diskmin=0
    ((diskmin=$cnt/$enidnum))
    #每个节点最多选择的硬盘数量
    local diskmax=0   
    ((diskmax=($cnt+$enidnum-1)/$enidnum))
    
    echo "$raidname" >> ${CM_LOG_DIR}${CM_LOG_FILE}
    echo ' \' >> $createfile
    echo -n "\t\t$raidname " >> $createfile
    #将ENID进行排序，目的让本节点排在最前面
    local i=0
    local checkresultfile="/tmp/${FUNCNAME}_checkresultfile"
    for((i=0;i<$enidnum;i++))
    do
        local tmpenid=${enidarr[$i]}
        if [ $tmpenid -lt $localenid ]; then
            ((tmpenid+=$enidmax))
        fi
        echo $tmpenid
    done |sort |while read line
    do
        local curneed=$diskmin
        if [ $diskspare -gt 0 ]; then
            curneed=$diskmax
            ((diskspare=$diskspare-1))
        fi
        if [ $curneed -eq 0 ];then
            #前面已经选完成，不需要再选硬盘
            break
        fi
        localenid=$line
        if [ $localenid -gt $enidmax ]; then
            #将ENID还原
            ((localenid-=$enidmax))
        fi
        ((nextenid=$localenid+1000))
        
        #判断当前是否有充足的硬盘
        sql="SELECT count(id) FROM record_t WHERE enid>=$localenid AND enid<$nextenid AND status='free'"
        sql=`sqlite3 $CM_DISK_DB_CACHE "$sql"`
        if [ $sql -lt $curneed ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}]enid:$localenid $raidname need:$curneed free:$sql!"
            echo "$CM_FAIL">$checkresultfile
            break
        fi
        #记录当前选用的硬盘
        sql="SELECT id,enid,slotid FROM record_t WHERE enid>=$localenid AND enid<$nextenid AND status='free' ORDER BY enid,slotid LIMIT $curneed"
        sqlite3 $CM_DISK_DB_CACHE "$sql" >> ${CM_LOG_DIR}${CM_LOG_FILE}
        
        #选硬盘
        sql="SELECT id FROM record_t WHERE enid>=$localenid AND enid<$nextenid AND status='free' ORDER BY enid,slotid LIMIT $curneed"
        local idlists=`sqlite3 $CM_DISK_DB_CACHE "$sql" |awk '{printf $1" "}'`
        echo -n "$idlists" >> $createfile
        #更新临时数据库中之前选中硬盘的状态
        sqlite3 $CM_DISK_DB_CACHE "UPDATE record_t SET status='busy' WHERE id IN($sql)"
    done
    
    if [ -f $checkresultfile ]; then
        iret=`cat $checkresultfile`
        rm -f $checkresultfile
        return $iret
    fi
    return $CM_OK
}
#=======================================================================================
#<poolname> <type> <file> <policy>
#
#=======================================================================================
function cm_pool_create_each()
{
    local dataname=$1
    local selectfunc='cm_pool_create_each_select_default'
    if [ "X$dataname" == "Xdata" ]; then
        dataname=''        
    fi
    local policy=$4
    if [ "X$policy" == "X1" ]; then
        selectfunc='cm_pool_create_each_select_distributed'
    fi
    local info=$2
    local createfile=$3
    if [ "X$info" == "X" ] || [ "X$info" == "X-" ] ; then
        #忽略该项
        return $CM_OK
    fi 
    
    info=(`echo "$info" |sed 's/|/ /g'`)
    local raid=${info[0]}
    local data=${info[1]}
    local spare=${info[3]}
    local group=${info[2]}
    #校验配置参数
    cm_pool_check_param "$raid" "$data" "$spare" "$group"
    local iret=$?
    if [ $iret -ne $CM_OK ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] checkparam fail $iret"
        return $iret
    fi
    if [ "X$group" == "" ] || [ $group -lt 1 ]; then
        if [ "X$dataname" == "X" ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] data null!"
            return $CM_PARAM_ERR
        fi
        return $CM_OK
    fi
    
    if [ $raid -eq 0 ]; then
        group=1
    fi
    
    #计算每一组需要的硬盘数量
    local raidx=0
    if [ $raid -gt 4 ]; then
        ((raidx=$raid-4))
    fi
    ((raidx=$raidx+$data))
    
    local raidname=`cm_pool_get_raid_str "$raid"`
    if [ "X$dataname" != "X" ]; then
        echo ' \' >> $createfile
        echo -n "\t$dataname" >> $createfile
        #记录日志文件
        echo "$dataname" >> ${CM_LOG_DIR}${CM_LOG_FILE}
    fi
    
    #添加数据盘
    local index=$group
    local checkresultfile="/tmp/${FUNCNAME}_checkresultfile"
    for((;index>0;index--))
    do
        ${selectfunc} "$raidname" "$raidx" "$createfile"
        iret=$?
        if [ $iret -ne $CM_OK ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] $raidname $raidx fail $iret"
            echo "$iret">$checkresultfile
            break
        fi
    done
    if [ -f $checkresultfile ]; then
        iret=`cat $checkresultfile`
        rm -f $checkresultfile
        return $iret
    fi
    #添加热备
    if [ "X$spare" != "" ] && [ $spare -gt 0 ]; then
        ${selectfunc} "${dataname}spare" "$spare" "$createfile"
        iret=$?
        if [ $iret -ne $CM_OK ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] ${dataname}spare $spare fail $iret"
            return $iret
        fi
    fi
    return $CM_OK
}

#=======================================================================================
#<create|add> <force> <policy> <poolname> 
#       <data:"raid|data|group|spare">
#       <meta:"raid|data|group|spare>
#       <low:"raid|data|group|spare">
#=======================================================================================
function cm_pool_ca()
{
    local act=$1
    local isforce=$2
    if [ "X$isforce" == "X1" ]; then
        isforce='-f'
    else
        isforce=''
    fi
    local policy=$3
    local poolname=$4
    local iret=$CM_OK
    local data=$5    
    if [ "X$data" == "X" ]; then
        data='-'
    fi
    
    local meta=$6
    if [ "X$meta" == "X" ]; then
        meta='-'
    fi
    
    local low=$7
    if [ "X$low" == "X" ]; then
        low='-'
    fi
    
    local diskparam=("$data" "$meta" "$low")
    local paramname=("data" "meta" "low")
    #数据盘参数
    if [ "X$data" == "X" ] || [ "X$data" == "X-" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] data null!"
        return $CM_PARAM_ERR
    fi
    
    if [ -f $CM_DISK_DB_CACHE ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] already run!"
        return $CM_ERR_BUSY
    fi
    
    if [ ! -f $CM_DISK_DB ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $CM_DISK_DB none!"
        return $CM_ERR_BUSY
    fi
    
    cp $CM_DISK_DB $CM_DISK_DB_CACHE
    #转换为绝对enid (底层22-999为本地盘,1022-*为远端盘，转换为1000-1999,2000-2999,...)
    local localenid=`cm_pool_local_enid`
    #更新数据库中enid (有所id-22,从0开始计数;)
    sqlite3 $CM_DISK_DB_CACHE "UPDATE record_t SET enid=enid-22"
    #将本地enid转换为绝对enid
    sqlite3 $CM_DISK_DB_CACHE "UPDATE record_t SET enid=enid+$localenid WHERE enid<1000"
    
    local createfile=`date "+%Y%m%d%H%M%S"`
    createfile="/tmp/${FUNCNAME}_${createfile}.sh"
    echo '#!/bin/bash' >$createfile
    echo -n "zpool ${act} ${isforce} ${poolname}" >> $createfile
    
    local index=0
    for((index=0;index<3;index++))
    do
        cm_pool_create_each "${paramname[$index]}" "${diskparam[$index]}" "$createfile" "$policy"
        iret=$?
        if [ $iret -ne $CM_OK ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] check ${paramname[$index]} ${diskparam[$index]} fail $iret!"
            rm -f $CM_DISK_DB_CACHE
            rm -f $createfile
            return $iret
        fi
    done

    echo "" >>$createfile
    echo 'exit $?' >>$createfile
    
    rm -f $CM_DISK_DB_CACHE
    chmod 755 $createfile
    cat $createfile >> ${CM_LOG_DIR}${CM_LOG_FILE}
    $createfile >> ${CM_LOG_DIR}${CM_LOG_FILE} 2>&1
    iret=$?
    rm -f $createfile
    return $iret
}

function cm_pool_add()
{
    CM_LOG "[${FUNCNAME}:${LINENO}]$*"
    cm_pool_ca "add" $*
    local iret=$?
    CM_LOG "[${FUNCNAME}:${LINENO}]$* iret=$iret"
    return $iret
}

function cm_pool_create()
{
    CM_LOG "[${FUNCNAME}:${LINENO}]$*"
    cm_pool_ca "create" $*
    local iret=$?
    CM_LOG "[${FUNCNAME}:${LINENO}]$* iret=$iret"
    return $iret
}

function cm_pool_disk_demo()
{
    #节点数量
    local nodenum=10 
    #每组扩展柜梳理
    local extnum=16
    #每个柜子硬盘槽位数
    local eachnum=24
    local testdb=$CM_DISK_DB
    rm -f $testdb
    touch $testdb
    local sql='CREATE TABLE record_t (id VARCHAR(64),sn VARCHAR(64),vendor VARCHAR(64),nblocks VARCHAR(64),blksize VARCHAR(64),gsize DOUBLE,dim VARCHAR(64),enid INT,slotid INT,rpm INT,status VARCHAR(64),led VARCHAR(32))'
    
    sqlite3 $testdb "$sql"
    
    local i=0
    
    ((nodenum=($nodenum+1)>>1))
    ((extnum=$extnum+1))
    for((i=1;i<=$nodenum;i++))
    do
        local j=0
        local enid=1000
        ((enid=$i*$enid+22))
        for((j=0;j<$extnum;j++,enid++))
        do
            local k=1
            for((k=1;k<=$eachnum;k++))
            do
                sql="INSERT INTO record_t VALUES('c0t5000enid${enid}slot${k}d0','Z292J81Q00009$enid$k','SEAGATE','5860533168','512',2.73,'T',$enid,$k,7200,'free','normal')"
                sqlite3 $testdb "$sql"
            done
        done
    done
    return $CM_OK
}

#=======================================================================================
#=======================================================================================
cm_pool_$*
exit $?

