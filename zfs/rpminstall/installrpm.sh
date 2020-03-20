#!/bin/bash

RPM_DIR="./rpm"
SPL_RPM=$RPM_DIR"/spl"
ZFS_RPM=$RPM_DIR"/zfs"

function install()
{
    if [  ! -f zfsonlinuxrpm.tar.gz ]; then
        echo "no rpm"
        return
    fi
    
    tar -xzvf zfsonlinuxrpm.tar.gz
    
    cd $SPL_RPM
    rpm -ivh ./*
    cd -
    
    cd $ZFS_RPM
    rpm -ivh ./* --nodeps --force
    cd -
}


function unload()
{
    rpm -qa | grep 0.6.5.9 |xargs rpm -e
}


$*