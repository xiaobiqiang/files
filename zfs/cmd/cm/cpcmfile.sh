#!/bin/bash

if [ ! -d /var/cm ]; then
    mkdir -p /var/cm
fi

if [ ! -d /var/cm/script ]; then
    mkdir -p /var/cm/script 2>/dev/null
    cp -rf ./script_linux/* /var/cm/script
    cp -rf ./script_py2/* /var/cm/script
    chmod 755 /var/cm/script/*
fi

if [ ! -d /var/cm/static ]; then
    mkdir -p /var/cm/static
    cp -rf ./static/* /var/cm/static
fi

if [ ! -d /var/cm/log ]; then
    mkdir -p /var/cm/log
fi

if [ ! -d /var/cm/data ]; then
    mkdir -p /var/cm/data
fi


if [ ! -f /usr/sbin/cm_multi_exec ]; then
    ln -s /var/cm/script/cm_multi_exec.py /usr/sbin/cm_multi_exec
fi

if [ ! -f /usr/sbin/cm_multi_send ]; then
    ln -s /var/cm/script/cm_multi_send.py /usr/sbin/cm_multi_send
fi

if [ ! -f /usr/sbin/cm_disk ]; then
    ln -s /var/cm/script/cm_disk.sh /usr/sbin/cm_disk
fi

if [ ! -f /usr/sbin/cm_topo ]; then
    ln -s /var/cm/script/cm_topo.sh /usr/sbin/cm_topo
fi

chmod 755 ./build/makeam.sh
