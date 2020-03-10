巡检脚本使用说明
1. 上传脚本
    (1)将相关脚本上传设备；
    (2)所有脚本设置权限755；
    (3)执行巡检；巡检脚本只打印回显，不会输出文件，如果需要直接将回显重定向输出到指定文件即可
2. inspect.sh
    巡检相关操作都可以通过脚本执行
    inspect.sh help  
        ---- 列出相关子命令
    inspect.sh init
        ---- 初始化操作，启动cm_multi_server 方便跨节点执行命令, v8r5之前的版本使用，后续版本不用执行
    inspect.sh list
        ---- 列出巡检的模块名称
    inspect.sh list <modname> 
        ---- 列出改模块相关巡检项以及描述，如:./inspect.sh list cm
            Prodigy-root#./inspect.sh list cm
            cm_rootdir 根目录检查功能
            cm_rootcore 根目录下core文件检查
            cm_check_cron cron服务检查
            cm_check_mem 内存占用检查
            cm_check_cpu CPU占用检查
            cm_check_gui GUI服务检查
            cm_check_clumgt CLUMGT服务检查
            cm_check_svc CM服务检查

    inspect.sh exec
        ---- 执行所有巡检，如果执行到哪一步阻塞了，找相应的模块定位
    inspect.sh exec <modname>
        ---- 执行巡检指定模块，如:./inspect.sh exec cm
    inspect.sh exec <modname> <func>
        ---- 执行巡检指定模块的具体一个巡检项，如:./inspect.sh exec cm cm_check_svc
            Prodigy-root#./inspect.sh exec cm cm_check_svc
            [20190925160941]START: cm_check_svc        
            node count: 2
            [20190925160941]END  : cm_check_svc         RESULT:    OK

    
    巡检回显输出说明：
    [当前开始时间]START: 执行巡检项函数 (通过该项可以直接确定当前模块)
    巡检输出内容 (只输出最关键的信息)
    [当前结束时间]END: 执行巡检项函数 RESULT:  (OK :正常 / FAIL: 异常)
    例如
    Prodigy-root#./inspect.sh exec nas nas_check_log
    [20190925161140]START: nas_check_log            
    find count[1]:spa:.*os:.* OFFLINE
    The record, spa: xxxxxx, os:xxxxxxx is OFFLINE, cmd: xx, opera: xx
    find count[1]:long fid return
    long fid return
    [20190925161140]END  : nas_check_log        RESULT:  FAIL

3. inspect_common.sh
    巡检通用功能函数

4. inspect_<mod>.sh
    相关模块具体巡检项对于的功能函数实现,格式参考inspect_cm.sh
    每个功能项函数格式如下：
    #==============================================================================
    ###巡检项描述
    function <modname>_<xxxxxx>()
    {
        local result=$INSPECT_OK
        #开始
        INSPECT_START ${FUNCNAME}
        
        #执行相关检查，并设置巡检结果
        #巡检只输出最关键的信息，失踪是OK或者FAIL的巡检都没有意义
        .....
        #结束
        INSPECT_END ${FUNCNAME} $result
        return $result
    }
    
    ./inspect_<mod>.sh 
        ---- 列出该模块巡检项
    ./inspect_<mod>.sh <func>
        ---- 执行该模块指定巡检项
    ./inspect_<mod>.sh inspect_all
        ---- 执行该模块所有巡检项

5. cm_multi_*.py
    用于节点间执行命令，和clumgt类似，但是更简单可靠；
    v8r5以后的版本默认自带该服务，之前版本需要通过inspect.sh init来设置和启动
    