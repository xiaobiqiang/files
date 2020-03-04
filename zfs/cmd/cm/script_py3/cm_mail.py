#!/usr/bin/python
# -*- coding:utf-8 -*-
import smtplib
import xml.dom.minidom
import os
import subprocess
import sys

CM_DATA_FILE = '/var/cm/data/cm_mail.db'
CM_ALARM_FILE = '/var/cm/data/cm_alarm.db'
CM_OK = 0
CM_FAIL = 1
CM_ARARM_LOG_TYPE=2

##封装命令执行函数
def cm_exec_cmd(cmd):
    (status, data) = subprocess.getstatusoutput(cmd)
    return [status,data]

##封装打印函数
def cm_print(str_val):
    print(str_val)

##检查连通性
def cm_check_ping(addr):
    info = cm_exec_cmd("ping "+addr+" 1 2>/dev/null")
    return info[0]

##获取全局配置信息
def cm_report_config():
    info = cm_exec_cmd('sqlite3 '+CM_DATA_FILE+' "SELECT * FROM server_t LIMIT 1"')
    cfg = info[1]
    return cfg.split('|')

##获取告警配置信息
def cm_alarm_config(alarm_id):
    info = cm_exec_cmd('sqlite3 '+CM_ALARM_FILE+' "SELECT lvl,type,is_disable FROM config_t WHERE alarm_id='+str(alarm_id)+'"')
    cfg = info[1]
    if cfg == '' :
        cm_print("alarmid not config")
        return ''
    return cfg.split('|')

##获取满足条件的收件人邮箱
def cm_report_recvers(lvl):
    info = cm_exec_cmd('sqlite3 '+CM_DATA_FILE+' "SELECT receiver FROM receiver_t WHERE level>='+str(lvl)+'"')
    receivers = info[1]
    if receivers == '' :
        cm_print("no recerver")
        return ''
    return receivers.split('\n')

##获取告警描述信息
def cm_alarm_desc(language, alarm_id,param):
    if language == 0:
        dom = xml.dom.minidom.parse('/var/cm/static/res_ch/alarm_desc.xml')
    else :
        dom = xml.dom.minidom.parse('/var/cm/static/res_en/alarm_desc.xml')
    root = dom.documentElement
    alarm_list = root.getElementsByTagName('alarm')
    alarm_len = len(alarm_list)
    for i in range(0,alarm_len):
        info = alarm_list[i]
        if alarm_id == info.getAttribute('id'):
            param_list = param.split(',')
            param_len = len(param_list)
            title = info.getAttribute('title')
            desc = info.getAttribute('desc')
            for j in range(0,param_len):
                num = '(#' + str(j) + ')'
                title = title.replace(num,param_list[j])
                desc = desc.replace(num,param_list[j])
            title = title.encode('utf-8')
            desc = desc.encode('utf-8')
            return [title,desc]
    return ''

##链接邮件服务器
def cm_server_conn(cfg_server, cfg_port, cfg_useron = 0, user='',pwd=''):
    #链接邮件服务器
    iret = cm_check_ping(cfg_server)
    if iret != CM_OK:
        cm_print("ping "+cfg_server+" fail")
        return ''
    try:
        if cfg_port == 465:
            smtpobj = smtplib.SMTP_SSL(cfg_server,cfg_port)
        else:
            smtpobj = smtplib.SMTP(cfg_server,cfg_port)
        smtpobj.ehlo()
        
        if cfg_useron == 1:
            smtpobj.login(user,pwd)
        return smtpobj
    except smtplib.SMTPException:
        cm_print("connect "+cfg_server+" fail")
        return ''

#发送邮件
def cm_email_send(obj, sender, recver, title, content):
    message = 'From:' + sender + '\nTo:' + recver + '\nSubject:' + title + '\n' + '\n' + content
    message = message.decode('utf-8').encode('gbk')
    
    try:
        obj.sendmail(sender,recver, message)
        return CM_OK
    except smtplib.SMTPException:
        cm_print("send to "+recver+" fail")
        return CM_FAIL
        
def cm_alarm_report(alarmid,alarmparam,isrecory):
    ##获取全局配置信息
    cfg = cm_report_config()
    ##state INT,useron INT,sender VARCHAR(128),server VARCHAR(128),port INT,language INT,user VARCHAR(128),passwd VARCHAR(128)
    cfg_state  = int(cfg[0])
    cfg_useron = int(cfg[1])
    cfg_sender = cfg[2]
    cfg_server = cfg[3]
    cfg_port   = int(cfg[4])
    cfg_lan    = int(cfg[5])
    cfg_user   = cfg[6]
    cfg_pwd    = cfg[7]
    
    if cfg_state == 0 :
        cm_print("switch off")
        return CM_OK
    
    ##获取告警配置
    alarmcfg = cm_alarm_config(alarmid)
    if alarmcfg == '' :
        return CM_OK
    alarm_lvl    = int(alarmcfg[0])
    alarm_type   = int(alarmcfg[1])
    alarm_disable= int(alarmcfg[2])
    
    if alarm_disable == 1 :
        cm_print("alarm disable")
        return CM_OK
    if alarm_type == CM_ARARM_LOG_TYPE :
        cm_print("alarm log not report")
        return CM_OK
    
    ##获取邮件通知人
    recvs = cm_report_recvers(alarm_lvl)
    if recvs == '' :
        cm_print("no receiver")
        return CM_OK
    
    ##获取告警描述
    alarmdsc = cm_alarm_desc(cfg_lan, alarmid, alarmparam)
    if alarmdsc == '' :
        cm_print("not support")
        return CM_OK
    alarm_title = alarmdsc[0]
    alarm_desc = alarmdsc[1]
    
    #链接邮件服务器
    obj = cm_server_conn(cfg_server,cfg_port,cfg_useron,cfg_user,cfg_pwd)
    if obj == '' :
        cm_print("connect fail")
        return CM_FAIL
    if cfg_lan == 0:
        typecfg = ['故障','事件','日志']
        lvlcfg = ['紧急','严重','一般','轻微']
        reptype= ['上报','恢复']
    else:
        typecfg = ['FAULT','EVENT','LOG']
        lvlcfg = ['CRITICAL','MAJOR','MINOR','TRIVIAL']
        reptype= ['report','recovery']
    
    if isrecory > 0 :
        alarm_type = typecfg[alarm_type]+":"+reptype[1]
    else :
        alarm_type = typecfg[alarm_type]+":"+reptype[0]
    title = "["+alarm_type+"]"+alarm_title
    if cfg_lan == 0:
        content = '标题: '+ alarm_title + '\n' + '类型: ' + alarm_type + '\n' + '级别: ' + lvlcfg[alarm_lvl] + '\n' + '描述: ' + alarm_desc
    else:
        content = 'TITLE: '+ alarm_title + '\n' + 'TYPE: ' + alarm_type + '\n' + 'LEVEL: ' + lvlcfg[alarm_lvl] + '\n' + 'DESC: ' + alarm_desc
    
    for recv in recvs:
        iret = cm_email_send(obj,recv, cfg_sender, title,content)
        cm_print("send to "+recv+" iret:"+str(iret))
    obj.close()
    return CM_OK

def cm_sever_conn_test(cfg_server, cfg_port, user='',pwd=''):
    cfg_useron = 1
    if user == '': 
        cfg_useron = 0
    obj = cm_server_conn(cfg_server,cfg_port,cfg_useron,user,pwd)
    
    if obj == '':
        cm_print("connect fail")
        return CM_FAIL
    obj.close()
    cm_print("connect "+cfg_server+" ok")
    return CM_OK

def cm_mail_send_test(recv):
    cfg = cm_report_config()
    ##state INT,useron INT,sender VARCHAR(128),server VARCHAR(128),port INT,language INT,user VARCHAR(128),passwd VARCHAR(128)
    cfg_state  = int(cfg[0])
    cfg_useron = int(cfg[1])
    cfg_sender = cfg[2]
    cfg_server = cfg[3]
    cfg_port   = int(cfg[4])
    cfg_lan    = int(cfg[5])
    cfg_user   = cfg[6]
    cfg_pwd    = cfg[7]
    
    if cfg_state == 0 :
        cm_print("switch off")
        return CM_OK
    obj = cm_server_conn(cfg_server,cfg_port,cfg_useron,cfg_user,cfg_pwd)
    if obj == '' :
        cm_print("connect fail")
        return CM_FAIL
    
    iret = cm_email_send(obj, cfg_sender,recv,"This is test email title","This is test email content!")
    obj.close()
    if iret == CM_OK:
        cm_print("send to "+recv+" ok")
    return iret

def cm_mail_help(name):
    cm_print(name+" test_server <addr> <port> <user> <pwd>")
    cm_print(name+" test_send <recvemail>")
    cm_print(name+" alarm <alarmid> <param> <recovery>")
    return CM_OK

if __name__ == '__main__':
    argc = len(sys.argv)
    if argc < 2 :
        cm_mail_help(sys.argv[0])
        sys.exit(0)
    act = sys.argv[1]
    argc = argc - 2
    if act == 'test_server' :
        if argc < 4 :
            cm_mail_help(sys.argv[0])
            sys.exit(2)
        rec = cm_sever_conn_test(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5])
    elif act == 'test_send' :
        if argc < 1 :
            cm_mail_help(sys.argv[0])
            sys.exit(2)
        rec = cm_mail_send_test(sys.argv[2])
    else :
        alarmid=sys.argv[1]
        alarmparam=sys.argv[2]
        alarmtype=int(sys.argv[3])
        rec = cm_alarm_report(alarmid,alarmparam,alarmtype)
    sys.exit(rec)

