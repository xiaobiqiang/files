#!/bin/bash

function cm_gui_install()
{
    #��װGUI
    local guifile=$1
    local definstall=0
    if [ "X$guifile" == "X" ]; then
        guifile='/var/guitar/GUI.tar '
        definstall=1
    fi
    
    if [ -f $guifile ]; then
        #ͣGUI����
        guictl disable
        
        rm -rf /gui/*
        cp ${guifile} /gui/gui.tar
        cd /gui
        tar -vxf gui.tar
        rm -rf lost+found
        rm -f gui.tar
        ./prepare.sh
        cd - 1>/dev/null 2>/dev/null
        
        #����GUI����
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
