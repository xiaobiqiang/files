#!/bin/bash
cd cmd/cm/build/
chmod 755 ./makeam.sh
./makeam.sh
cd -
cp module/Makefile_hengwei.in module/Makefile.in
./autogen.sh
./configure --enable-hengwei=yes --with-spl=$1
make -j 16 && make install
