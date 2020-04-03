#!/bin/bash
function install_disk()
{
    mkdir -p /var/fm
    cp /usr/local/sbin/sg_led_disk.sh /var/fm/
    chmod 755 /var/fm/sg_led_disk.sh
}

function install_python()
{
    if [ ! -f /bin/python ]; then
        python_dir=`which python`
        ln -s $python_dir /bin/python
    fi
}

function install_service()
{
    systemctl enable zfs-boot.service
    systemctl enable stmf-boot.service
    systemctl enable configd.service
    systemctl enable pppt-boot.service
    systemctl enable stmf-svc.service

    systemctl enable fc-boot.service
    systemctl enable fmd.service
    systemctl enable ceres_cm.service
    systemctl enable cm_multi_server.service
    #systemctl enable zfs-clusterd.service
    systemctl enable iscsitsvc.service
    
}

install_disk
install_python
install_service
