#!/bin/bash

tar -xvf ./net-snmp.tar
cp -rf ./net-snmp/ /usr/include
rm -rf ./net-snmp/

exit $?
