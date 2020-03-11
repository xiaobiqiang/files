#!/bin/bash

OPEN_DIR='../opensrc/'
BASE_DIR='../base/'
CLI_DIR='../cli/'
CM_DIR='../main/'
CMD_DIR='../cmd/'
EXEC_DIR='../exec/'
JNI_DIR='../jni/'
BUILD_DIR='../build/'
CFG_DIR='../config/'
CM_DIR='../main/'
JNI_DIR='../jni/'
SCRIPT_DIR='../script_linux/'
PYTHON_DIR='../script_py2/'
projects="open base cli cfg cmd exec cm jni script python"

function make_am_head()
{

	local amdir=$1

cat << EOF >$amdir
include \$(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += \\
	-I\$(top_srcdir)/include \\
	-I\$(top_srcdir)/lib/libspl/include

EOF
}

function make_am_include()
{
	local symbol=$1
	local amdir=$2
	
cat << EOF >>$amdir
AM_CPPFLAGS += \\
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
		find /usr/lib/jvm -name "*jni*" | while read line
		do
			jnidir=`echo $line|sed "s/jni.h//g"|sed "s/jni_md.h//g"`
			if [ -d $jnidir ]; then
				echo "	-I$jnidir \\">>$am
			fi
		done
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
	make_am_head $am
	echo "lib_LTLIBRARIES = libcmopensrc.la">>$am
	
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
    
	echo "EXTRA_DIST = \\" >> $am
	local harrays=($(du -a|grep '\.h'|awk '{print $2}'))
	local cols=${#harrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${harrays[i]} " >>$am
		else
			echo "	${harrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR
    
cat >> $am <<EOF
libcmopensrc_la_CFLAGS = -std=gnu99 -lpthread -ldl
EOF
}

function make_am_base()
{
	local am=$BASE_DIR"Makefile.am"
	make_am_head $am
	echo "noinst_LTLIBRARIES = libcmbase.la">>$am
	
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
    
	echo "EXTRA_DIST = \\" >> $am
	local harrays=($(du -a|grep '\.h'|awk '{print $2}'))
	local cols=${#harrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${harrays[i]} " >>$am
		else
			echo "	${harrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR
}

function make_am_cli()
{
	local am=$CLI_DIR"Makefile.am"
	make_am_head $am
	echo "bin_PROGRAMS = ceres_cli">>$am
	
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

	echo "EXTRA_DIST = \\" >> $am
	local harrays=($(du -a|grep '\.h'|awk '{print $2}'))
	local cols=${#harrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${harrays[i]} " >>$am
		else
			echo "	${harrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR


cat >> $am <<EOF
ceres_cli_LDADD = \\
	\$(top)/config/libcfg.la \\
	\$(top)/base/libcmbase.la
ceres_cli_CFLAGS = -std=gnu99 -lpthread -ldl -L../opensrc -lcmopensrc -Wl,-rpath=\$(prefix)/lib

EOF
}

function make_am_cmd()
{
	local am=$CMD_DIR"Makefile.am"
	make_am_head $am
	echo "bin_PROGRAMS = ceres_cmd">>$am
	
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
    
	echo "EXTRA_DIST = \\" >> $am
	local harrays=($(du -a|grep '\.h'|awk '{print $2}'))
	local cols=${#harrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${harrays[i]} " >>$am
		else
			echo "	${harrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR


cat >> $am <<EOF
ceres_cmd_LDADD = \\
	\$(top)/base/libcmbase.la
ceres_cmd_CFLAGS = -std=gnu99 -lpthread -ldl -L../opensrc -lcmopensrc -Wl,-rpath=\$(prefix)/lib

EOF

}

function make_am_cfg()
{
	local am=$CFG_DIR"Makefile.am"
	make_am_head $am
	echo "noinst_LTLIBRARIES = libcfg.la">>$am
	
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
    
	echo "EXTRA_DIST = \\" >> $am
	local harrays=($(du -a|grep '\.h'|awk '{print $2}'))
	local cols=${#harrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${harrays[i]} " >>$am
		else
			echo "	${harrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR
}

function make_am_exec()
{
	local am=$EXEC_DIR"Makefile.am"
	make_am_head $am
	echo "bin_PROGRAMS = ceres_exec">>$am
	
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
    
	echo "EXTRA_DIST = \\" >> $am
	local harrays=($(du -a|grep '\.h'|awk '{print $2}'))
	local cols=${#harrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${harrays[i]} " >>$am
		else
			echo "	${harrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR


cat >> $am <<EOF
ceres_exec_LDADD = \\
	\$(top)/base/libcmbase.la
ceres_exec_CFLAGS = -std=gnu99 -lpthread -ldl -L../opensrc -lcmopensrc -Wl,-rpath=\$(prefix)/lib

EOF

}

function make_am_cm()
{
	local am=$CM_DIR"Makefile.am"
	make_am_head $am
	echo "bin_PROGRAMS = ceres_cm">>$am
	
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
    
	echo "EXTRA_DIST = \\" >> $am
	local harrays=($(du -a|grep '\.h'|awk '{print $2}'))
	local cols=${#harrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${harrays[i]} " >>$am
		else
			echo "	${harrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR


cat >> $am <<EOF
ceres_cm_LDADD = \\
	\$(top)/config/libcfg.la \\
	\$(top)/base/libcmbase.la
ceres_cm_CFLAGS = -std=gnu99 -lpthread -ldl -L../opensrc -lcmopensrc -Wl,-rpath=\$(prefix)/lib

EOF

}

function make_am_jni()
{
	local am=$JNI_DIR"Makefile.am"
	make_am_head $am
	echo "lib_LTLIBRARIES = libcmjni.la">>$am
	
	make_am_include "jni" $am
	
	echo "libcmjni_la_SOURCES = \\">>$am
	cd $JNI_DIR
	local arrays=($(du -a|grep '\.c'|awk '{print $2}'))
	local cols=${#arrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		echo "	${arrays[i]} \\" >>$am
	done
	cd $BUILD_DIR
	
	carrays=($(du -a ../config|grep '\.c'|awk '{print $2}'))
	barrays=($(du -a ../base|grep '\.c'|awk '{print $2}'))
	arrays=($(du -a ../opensrc/json/|grep '\.c'|awk '{print $2}'))
	arrays=(${barrays[*]} ${carrays[*]} ${arrays[*]})
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

	cd $JNI_DIR
	echo "EXTRA_DIST = \\" >> $am
	local harrays=($(du -a|grep '\.h'|awk '{print $2}'))
	local cols=${#harrays[@]}
	((col=$cols-1))
	for (( i=0; i<$cols; i=i+1 ))
	do
		if [ $i -eq $col ]; then
			echo "	${harrays[i]} " >>$am
		else
			echo "	${harrays[i]} \\" >>$am
		fi
	done
	cd $BUILD_DIR
cat >> $am <<EOF
libcmjni_la_CFLAGS = -std=gnu99 -lpthread -ldl
EOF

}

function make_am_script()
{
    local am=$SCRIPT_DIR"Makefile.am"
    
    cd $SCRIPT_DIR
    echo "cmscriptdir = \${prefix}/cm/script" > $am
    echo "dist_cmscript_SCRIPTS = \\" >> $am

    local harrays=($(du -a|grep '\.sh'|awk '{print $2}'))
    local cols=${#harrays[@]}
    ((col=$cols-1))
    for (( i=0; i<$cols; i=i+1 ))
    do
        if [ $i -eq $col ]; then
            echo "	${harrays[i]} " >>$am
        else
            echo "	${harrays[i]} \\" >>$am
        fi
    done
    cd $BUILD_DIR
}

function make_am_python()
{
    local am=$PYTHON_DIR"Makefile.am"

    cd $PYTHON_DIR
    echo "cmpythondir = \${prefix}/cm/script" > $am
    echo "dist_cmpython_SCRIPTS = \\" >> $am
    local harrays=($(du -a|grep '\.py'|awk '{print $2}'))
    local cols=${#harrays[@]}
    ((col=$cols-1))
    for (( i=0; i<$cols; i=i+1 ))
    do
        if [ $i -eq $col ]; then
            echo "	${harrays[i]} " >>$am
        else
            echo "	${harrays[i]} \\" >>$am
        fi
    done
    cd $BUILD_DIR
}

for project in $projects
do
	make_am_$project
done


