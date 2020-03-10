#!/bin/bash

#==========================  config  ===========================
MODULE_CLI_DIRS='../base/ ../config/ ../cli/'
MODULE_CM_DIRS='../base/ ../config/ ../main/'
MODULE_JSON_DIRS='../opensrc/json/'
MODULE_INIT_DIRS='../opensrc/iniparser/'
MODULE_SQLITE_DIRS='../sqlite3/'
MODULE_CMD_DIRS='../base/ ../cmd/'
MODULE_JNI_DIRS='../base/ ../config/ ../jni/ ../opensrc/json/ /usr/jdk/instances/jdk1.6.0/include/'
MODULE_OPENSRC_DIRS='../opensrc/'
MODULE_EXEC_DIRS='../base/ ../exec/'
MODULE_BASE_DIRS='../base/'
#==========================  config  ===========================

#===============================================================
# Make_SubDirs() <SrcDir> <FileName>
#===============================================================
function Make_SubDirs()
{
    SrcDir=$1
    FileName=$2
    SubDirs=(`find $SrcDir -type d`)
    
    for((j=0;j<${#SubDirs[*]};j++))
    do
        SubDir=${SubDirs[$j]}
        if [ X"$SubDir" != X"" ]; then
            cnt=`ls -l $SubDir | grep ^'-' | grep .h$ | wc -l`
            #echo "$SubDir : HeaderFileCount $cnt"
            if [ $cnt -gt 0 ]; then
                echo "		-I"$SubDir" \\" >> $FileName
            fi
        fi
    done
}
#===============================================================
# Make_IndDirs() <ConfigFileName> <ModuleName> <srcdir1> \
#                    [srcdir2] [...]
#===============================================================
function Make_IndDirs()
{
    local FileName=$1
    local ModuleName=$2
    local Param=($@)
    local ParamNum=$#
    
    echo $ModuleName"_INC_DIRS=\\" >> $FileName
    for((i=2;i<$ParamNum;i++))
    do
        SrcDir=${Param[$i]}
        Make_SubDirs "$SrcDir" "$FileName"
    done
    SrcDir="../opensrc/"
    Make_SubDirs "$SrcDir" "$FileName"
    echo "" >> $FileName
}

#===============================================================
# Make_SrcFiles() <ConfigFileName> <ModuleName> <srcdir1> \
#                    [srcdir2] [...]
#===============================================================
function Make_SrcFiles()
{
    local FileName=$1
    local ModuleName=$2
    local Param=($@)
    local ParamNum=$#
    local Src=$ModuleName"_SRC_FILES"
    
    echo $Src"=\\" >> $FileName
    for((i=2;i<$ParamNum;i++))
    do
        SrcDir=${Param[$i]}
        Files=(`find $SrcDir -name "*.c"`)
        for((j=0;j<${#Files[*]};j++))
        do
            FileOne=${Files[$j]}
            if [ X"$FileOne" != X"" ]; then
                echo "		"$FileOne" \\" >> $FileName
            fi
        done
    done
    echo "" >> $FileName
}

#===============================================================
# Make_ObjFiles() <ConfigFileName> <ModuleName> <srcdir1> \
#                    [srcdir2] [...]
#===============================================================
function Make_ObjFiles()
{
    local FileName=$1
    local ModuleName=$2
    local Param=($@)
    local ParamNum=$#
    local Obj=$ModuleName"_OBJX"
    
    echo $Obj"=\\" >> $FileName
    for((i=2;i<$ParamNum;i++))
    do
        SrcDir=${Param[$i]}
        Files=(`find $SrcDir -name "*.c"`)
        
        for((j=0;j<${#Files[*]};j++))
        do
            FileOne=${Files[$j]}
            if [ X"$FileOne" != X"" ]; then
                FileOne=$(basename $FileOne)
                FileOne=${FileOne%%.*}
                echo "		"$FileOne".o \\" >> $FileName
            fi
        done
    done
    echo "" >> $FileName
}

#===============================================================
# Make_HeadleFiles() <ConfigFileName> <ModuleName> <srcdir1> \
#                    [srcdir2] [...]
#===============================================================
function Make_HeadleFiles()
{
    local FileName=$1
    local ModuleName=$2
    local Param=($@)
    local ParamNum=$#
    
    echo $ModuleName"_HEADER_FILES=\\" >> $FileName
    for((i=2;i<$ParamNum;i++))
    do
        SrcDir=${Param[$i]}
        Files=(`find $SrcDir -name "*.h"`)
        
        for((j=0;j<${#Files[*]};j++))
        do
            FileOne=${Files[$j]}
            if [ X"$FileOne" != X"" ]; then
                echo "		"$FileOne" \\" >> $FileName
            fi
        done
    done
    echo "" >> $FileName
}

#===============================================================
# Make_Target() <ConfigFileName> <ModuleName>
#===============================================================
function Make_Target()
{
    local FileName=$1
    local ModuleName=$2
    local Obj=$ModuleName"_OBJ"
    local Src=$ModuleName"_SRC_FILES"
    local Headers=$ModuleName"_HEADER_FILES"
    local IncDirs=$ModuleName"_INC_DIRS"
    local Cflags=$ModuleName"_CFLAGS"
    #local Cflags='-O2 -Wall $('$Headers')'
    
    echo $Obj'=$('$Src':%.c=%.o)' >> $FileName
    echo "" >> $FileName
    
    echo $Cflags'= -O2 -Wall -std=gnu99 $('$IncDirs')' >> $FileName
    echo "" >> $FileName
    
    echo $ModuleName':$('$Obj')' >> $FileName
    echo '	$(CC) -o buildresult/'$ModuleName' $('$Obj') $(LIBSPATH) $(LIBS)' >> $FileName
    echo "" >> $FileName
    
    echo '%.o:%.c $('$Headers')' >> $FileName
    echo '	$(CC) $('$Cflags') -c $< -o $@' >> $FileName
    echo "" >> $FileName
    
    echo "clean_"$ModuleName":" >> $FileName
    echo '	@rm -rf $('$Obj')' >> $FileName
    echo "" >> $FileName
}
#===============================================================
# Make_Lib <type> <ConfigFileName> <ModuleName>
#===============================================================
function Make_Lib()
{
    local lib=$1
    local FileName=$2
    local ModuleName=$3
    local Obj=$ModuleName"_OBJ"
    local Src=$ModuleName"_SRC_FILES"
    local Headers=$ModuleName"_HEADER_FILES"
    local IncDirs=$ModuleName"_INC_DIRS"
    local Cflags=$ModuleName"_CFLAGS"
    local Libname="lib"$ModuleName".$lib"
    
    #local Cflags='-O2 -Wall $('$Headers')'
    
    echo $Obj'=$('$Src':%.c=%.o)' >> $FileName
    echo "" >> $FileName
    echo $Cflags'= -O2 -Wall -std=gnu99 $('$IncDirs')' >> $FileName
    
    echo "" >> $FileName
    
    echo $Libname':$('$Obj')' >> $FileName
    echo '	rm -f $@' >> $FileName
    
    if [ $lib = "a" ]; then
        echo '	ar cr $@ $('$Obj')' >> $FileName
    else
        echo '	$(CC) -shared -o $@ $('$Obj')' >> $FileName
    fi
    echo "	cp -f $Libname ./buildresult/" >> $FileName
    echo "	cp -f $Libname ../libs" >> $FileName
    #echo '	rm -f $('$Obj')' >> $FileName
    echo "" >> $FileName
    
    echo '%.o:%.c $('$Headers')' >> $FileName
    if [ $lib = "a" ]; then
        echo '	$(CC) $('$Cflags') -c $< -o $@' >> $FileName
    else
        echo '	$(CC) $('$Cflags') -fpic -c $< -o $@' >> $FileName
    fi 
    echo "" >> $FileName
    
    echo "clean_"$ModuleName":" >> $FileName
    echo '	rm -rf $('$Obj') buildresult/'$Libname >> $FileName
    echo "" >> $FileName
}

#===============================================================
# Make_Conf() <ModuleName> <srcdir1> [srcdir2] [...]
#===============================================================
function Make_Conf()
{
    local ModuleName=$1
    local SrcDir=''
    local Param=$@
    local ParamNum=$#
    local FileName="Makefile."$ModuleName
    
    #echo "ModuleName=$ModuleName, Param=$Param $ParamNum"
    echo "#makefile for moudle $ModuleName" > $FileName
    Make_IndDirs $FileName $Param
    Make_SrcFiles $FileName $Param
    #Make_ObjFiles $FileName $Param
    Make_HeadleFiles $FileName $Param
    
    if [ "$1" = "jsonc" -o "$1" = "iniparser" -o "$1" = "cmjni" -o "$1" = "cmopensrc" -o "$1" = "cmbase" ]; then
        Make_Lib "so" $FileName $1
        echo "make $1"
    else
        Make_Target $FileName $1
    fi
    echo "include Makefile."$ModuleName >> "Makefile"
    echo "MODULES += "$ModuleName >> "Makefile"
    echo "CLEANS += clean_"$ModuleName"" >> "Makefile"
    echo "" >> "Makefile"
}


#===============================================================
make clean
if [ "$1" = "clean" ]; then
	exit 0
fi


echo "CC=gcc" > "Makefile"
echo "MODULES =" >> "Makefile"
echo "CLEANS =" >> "Makefile"
if [ ! -d ../libs/ ]; then
	mkdir -p ../libs/

fi
echo "LIBSPATH = -L../libs" >> "Makefile"
echo "LIBS = -lsocket -lnsl" >> "Makefile"

if [ "$1" = "cm" ]; then
    echo "LIBS += -lcmopensrc" >> "Makefile"
	Make_Conf "ceres_cm" $MODULE_CM_DIRS
elif [ "$1" = "cli" ]; then
    echo "LIBS += -lcmopensrc" >> "Makefile"
	Make_Conf "ceres_cli" $MODULE_CLI_DIRS
elif [ "$1" = "cmd" ]; then
    echo "LIBS += -lcmopensrc" >> "Makefile"
	Make_Conf "ceres_cmd" $MODULE_CMD_DIRS
elif [ "$1" = "exec" ]; then
    echo "LIBS += -lcmopensrc" >> "Makefile"
	Make_Conf "ceres_exec" $MODULE_EXEC_DIRS
elif [ "$1" = "json" ]; then
	Make_Conf "jsonc" $MODULE_JSON_DIRS
elif [ "$1" = "ini" ]; then
	Make_Conf "iniparser" $MODULE_INIT_DIRS
elif [ "$1" = "sqlite" ]; then
    echo "LIBS += -lcmopensrc" >> "Makefile"
	Make_Conf "sqlite3" $MODULE_SQLITE_DIRS
elif [ "$1" = "jni" ]; then
	Make_Conf "cmjni" $MODULE_JNI_DIRS
elif [ "$1" = "open" ]; then
    Make_Conf "cmopensrc" $MODULE_OPENSRC_DIRS
elif [ "$1" = "base" ]; then
    echo "LIBS += -lcmopensrc" >> "Makefile"
    Make_Conf "cmbase" $MODULE_BASE_DIRS
else
	echo "none"
fi
#add modules
echo 'all:$(MODULES)' >> "Makefile"
echo "" >> "Makefile"
echo 'clean:$(CLEANS)' >> "Makefile"
echo "" >> "Makefile"

make

