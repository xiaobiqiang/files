#!/bin/bash

dofun()
{
    	if [ $# -lt 1 ];then
       		return 
    	fi

	if [ "$1" == "make" ];then
		cd sg3_utils/
		chmod +x autogen.sh
		chmod +x configure
		./autogen.sh
		./configure
		make
		
		cd ../lsscsi/
		chmod +x autogen.sh
		chmod +x configure
		./autogen.sh
		./configure
		make

	elif [ "$1" == "install" ];then
		cd sg3_utils/
		make install

		cd ../lsscsi/
		make install
	fi
}

dofun $1
