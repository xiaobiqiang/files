#!/bin/bash
#=====================================================================
#该脚本用于生成protoinfo_cm文件
#=====================================================================
OUTPUT_FILE='protoinfo_cm'


function protoinfo_cm_other()
{
    local dir=$1
    local predir=$2
    cd $dir
    find ./ -type d |sed 's/^\.\///g' |sed '/^$/d' |while read line
    do
        echo "d none $predir/$line 755 root sys" 
    done
    
    find ./ -type f |sed 's/^\.\///g' |while read line
    do
        echo "f none $predir/$line 755 root sys"
    done
    cd - 1>/dev/null
    
}
cat './protoinfo_def' >${OUTPUT_FILE}
protoinfo_cm_other '../static' 'var/cm/static' >>${OUTPUT_FILE}
protoinfo_cm_other '../script' 'var/cm/script' >>${OUTPUT_FILE}
protoinfo_cm_other '../script_py2' 'var/cm/script' >>${OUTPUT_FILE}
protoinfo_cm_other '../script_rd' 'usr/local/cm/script' >>${OUTPUT_FILE}
exit 0


