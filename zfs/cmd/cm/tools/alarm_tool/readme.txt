1.修改cm\static\comm\alarm_cfg.xml文件
2.修改alarm_tool.sh可执行权限
3.执行./alarm_tool.sh
4.检查是否执行成功，如果执行失败，排查alarm_cfg.xml配置
5.将在cm\static\comm\中生成cm_alarm_cfg.db文件

注意: sqlite3为编译的sqlite3的二进制文件，
    不同平台需要使用cm\opensrc\sqlite3\中的shell.c进行编译
