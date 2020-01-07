#!/bin/bash
cd ../
DIR=`pwd`
cd -
OPEN_DIR='$DIR/opensrc/'
BASE_DIR='$DIR/base/'
CLI_DIR='$DIR/cli/'
CM_DIR='$DIR/main/'
CMD_DIR='$DIR/cmd/'
EXEC_DIR='$DIR/exec/'
JNI_DIR='$DIR/jni/'
BUILD_DIR='$DIR/build/'
CFG_DIR='$DIR/config/'
CM_DIR='$DIR/main/'
JNI_DIR='$DIR/jni/'
projects="open base cli cfg cmd exec cm jni"

function make_am_include()
{
	local symbol=$1
	local amdir=$2
	
cat << EOF >>$amdir
AM_CPPFLAGS = \\
	-I\$(top)/base/config \\
	-I\$(top)/base/rpc \\
	-I\$(top)/base/common \\
	-I\$(top)/base/cmt \\
	-I\$(top)/base/xml \\
	-I\$(top)/base/db \\
	-I\$(top)/base/log \\
	-I\$(top)/base/include \\
	-I\$(top)/base/ini \\
	-I\$(top)/base/queue \\
	-I\$(top)/base/omi \\
	-I\$(top)/base/config \\
	-I\$(top)/base \\
	-I\$(top)/opensrc/sqlite3 \\
	-I\$(top)/opensrc/json \\
	-I\$(top)/opensrc/mxml \\
	-I\$(top)/opensrc/iniparser \\
EOF

	case $symbol in
		base)
		echo "	-I\$(top)/main/node">>$amdir
		return
		;;
		config)
		echo "	-I\$(top)/main/node">>$amdir
		return
		;;
		open)
		;;
		cmd)
		;;
		cli)
		echo "	-I\$(top)/config \\">>$amdir
		;;
		main)
		echo "	-I\$(top)/config \\">>$amdir
		;;
		"exec")
		;;
		jni)
		echo "	-I\$(top)/config \\">>$amdir
		;;
		*)
		;;
	esac
	
	echo "	-I\$(top)/$symbol \\">>$amdir
	local arrays=($( ls -l ../$symbol |grep ^d|awk '{print $9}'))
	local cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	-I\$(top)/$symbol/${arrays[i]}" >>$amdir
		else
			echo "	-I\$(top)/$symbol/${arrays[i]} \\" >>$amdir
		fi
	done	
}

function make_am_open()
{
	local am=$OPEN_DIR"Makefile.am"
	echo "lib_LTLIBRARIES = libcmopensrc.la">$am
	
	echo "libcmopensrc_la_SOURCES = \\">>$am
	cd $OPEN_DIR
	local arrays=($(du -a|grep '\.c'|awk '{print $2}'))
	local cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${arrays[i]} " >>$am
		else
			echo "	${arrays[i]} \\" >>$am
		fi
	done
	echo "\$(shell cp .libs/libcmopensrc.so /usr/local/lib)" >> $am
	echo "\$(shell cp .libs/libcmopensrc.so.0 /usr/local/lib)" >> $am
	echo "\$(shell cp .libs/libcmopensrc.so.0.0.0 /usr/local/lib)" >> $am
	cd $BUILD_DIR
}

function make_am_base()
{
	local am=$BASE_DIR"Makefile.am"
	echo "noinst_LTLIBRARIES = libcmbase.la">$am
	
	make_am_include "base" $am

	echo "libcmbase_la_SOURCES = \\">>$am
	cd $BASE_DIR
	local arrays=($(du -a|grep '\.c'|awk '{print $2}'))
	local cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${arrays[i]} " >>$am
		else
			echo "	${arrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR
}

function make_am_cli()
{
	local am=$CLI_DIR"Makefile.am"
	echo "bin_PROGRAMS = ceres_cli">$am
	
	make_am_include "cli" $am

	echo "ceres_cli_SOURCES = \\">>$am
	cd $CLI_DIR
	local arrays=($(du -a|grep '\.c'|awk '{print $2}'))
	local cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${arrays[i]} " >>$am
		else
			echo "	${arrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR


cat >> $am <<EOF
ceres_cli_LDADD = \\
	\$(top)/config/libcfg.la \\
	\$(top)/base/libcmbase.la
ceres_cli_CFLAGS = -std=c99 -lpthread -ldl -lcmopensrc -Wl,-rpath=\$(prefix)/lib

EOF
}

function make_am_cmd()
{
	local am=$CMD_DIR"Makefile.am"
	echo "bin_PROGRAMS = ceres_cmd">$am
	
	make_am_include "cmd" $am

	echo "ceres_cmd_SOURCES = \\">>$am
	cd $CMD_DIR
	local arrays=($(du -a|grep '\.c'|awk '{print $2}'))
	local cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${arrays[i]} " >>$am
		else
			echo "	${arrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR


cat >> $am <<EOF
ceres_cmd_LDADD = \\
	\$(top)/base/libcmbase.la
ceres_cmd_CFLAGS = -std=c99 -lpthread -ldl -lcmopensrc -Wl,-rpath=\$(prefix)/lib

EOF

}

function make_am_cfg()
{
	local am=$CFG_DIR"Makefile.am"
	echo "noinst_LTLIBRARIES = libcfg.la">$am
	
	make_am_include "config" $am
	
	echo "libcfg_la_SOURCES = \\">>$am
	cd $CFG_DIR
	local arrays=($(du -a|grep '\.c'|awk '{print $2}'))
	local cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${arrays[i]} " >>$am
		else
			echo "	${arrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR
}

function make_am_exec()
{
	local am=$EXEC_DIR"Makefile.am"
	echo "bin_PROGRAMS = ceres_exec">$am
	
	make_am_include "exec" $am

	echo "ceres_exec_SOURCES = \\">>$am
	cd $EXEC_DIR
	local arrays=($(du -a|grep '\.c'|awk '{print $2}'))
	local cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${arrays[i]} " >>$am
		else
			echo "	${arrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR


cat >> $am <<EOF
ceres_exec_LDADD = \\
	\$(top)/base/libcmbase.la
ceres_exec_CFLAGS = -std=c99 -lpthread -ldl -lcmopensrc -Wl,-rpath=\$(prefix)/lib

EOF

}

function make_am_cm()
{
	local am=$CM_DIR"Makefile.am"
	echo "bin_PROGRAMS = ceres_cm">$am
	
	make_am_include "main" $am

	echo "ceres_cm_SOURCES = \\">>$am
	cd $CM_DIR
	local arrays=($(du -a|grep '\.c'|awk '{print $2}'))
	local cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${arrays[i]} " >>$am
		else
			echo "	${arrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR


cat >> $am <<EOF
ceres_cm_LDADD = \\
	\$(top)/config/libcfg.la \\
	\$(top)/base/libcmbase.la
ceres_cm_CFLAGS = -std=c99 -lpthread -ldl -lcmopensrc -Wl,-rpath=\$(prefix)/lib

EOF

}

function make_am_jni()
{
	local am=$JNI_DIR"Makefile.am"
	echo "lib_LTLIBRARIES = libcmjni.la">$am
	
	make_am_include "jni" $am
	
	echo "libcmjni_la_SOURCES = \\">>$am
	cd $JNI_DIR
	local arrays=($(du -a|grep '\.c'|awk '{print $2}'))
	local cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${arrays[i]} " >>$am
		else
			echo "	${arrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR
	
	arrays=($(du -a ../opensrc/json/|grep '\.c'|awk '{print $2}'))
	cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${arrays[i]} " >>$am
		else
			echo "	${arrays[i]} \\" >>$am
		fi
	done
	
cat >> $am <<EOF
libcmjni_LDADD = \\
	\$(top)/config/libcfg.la \\
	\$(top)/base/libcmbase.la

EOF

}

for project in $projects
do
	make_am_$project
done


