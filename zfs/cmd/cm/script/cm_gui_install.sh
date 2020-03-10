#!/bin/bash

function cm_gui_install()
{
    #安装GUI
    local guifile=$1
    local definstall=0
    if [ "X$guifile" == "X" ]; then
        guifile='/var/guitar/GUI.tar '
        definstall=1
    fi
    
    if [ -f $guifile ]; then
        #停GUI服务
        guictl disable
        
        rm -rf /gui/*
        cp ${guifile} /gui/gui.tar
        cd /gui
        tar -vxf gui.tar
        rm -rf lost+found
        rm -f gui.tar
        ./prepare.sh
        cd - 1>/dev/null 2>/dev/null
        
        #启动GUI服务
        guictl enable
        
        if [ $definstall -eq 1 ]; then
            rm -f $guifile
            /var/cm/script/cm_shell_exec.sh check_add_ipmiuser
            /var/cm/script/cm_shell_exec.sh cm_cluster_nas_check
        fi 
    fi
    return 0
}

cm_gui_install $*
exit $?
