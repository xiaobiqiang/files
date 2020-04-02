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

install_disk
install_python
