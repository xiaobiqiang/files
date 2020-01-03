/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_main_cli.c
 * author     : wbn
 * create date: 2017年10月25日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_cli.h"
#include "cm_db.h"
#include "cm_log.h"
#include "cm_ini.h"
#include "cm_omi.h"

static int cm_main_get_param_val(int argc, char **argv,const sint8* param,sint8 *
buf,sint32 siz)
{
    if(1 >= argc)
    {
        return CM_FAIL;
    }
    while(argc > 0)
    {
        argc--;
        if(0==strcmp(*argv,param))
        {
            argv++;
            break;
        }
        argv++;
    }
    if(0 == argc)
    {
        return CM_FAIL;
    }
    snprintf(buf,siz-1,"%s",*argv);
    return CM_OK;
}

static void cm_main_scanf(const sint8* name, sint8 *buf, sint32 siz)
{
    sint8 *gchr = buf;
    int len = siz-1;
    printf("%s ",name);
    while(len>0)
    {
        *gchr = getchar();
        if(*gchr == '\n')
        {
            *gchr = '\0';
            break;
        }
        gchr++;
        len--;            
    }
    if(len == 0)
    {
        printf("\n too long!\n");
        return;
    }
    return;
}

static void cm_get_cmd_input(const sint8* name, sint8* buf,sint32 siz, sint8
** argvs, sint32 *pcnt)
{
    sint8 *gchr = buf;
    int len = siz-1;
    int cnt = 1;
    *argvs = buf;
    printf("%s ",name);
    
    while((len>0) && (cnt < *pcnt))
    {
        *gchr = getchar();
        if(*gchr == '\n')
        {
            *gchr = '\0';
            break;
        }
        if(*gchr == ' ')
        {
            *gchr = '\0';
            if(*(*argvs) != '\0')
            {
                argvs++;
                cnt++;
                gchr++;
                len--; 
                *argvs = gchr;                
            }
            continue;
        }
        gchr++;
        len--;            
    }
    if(len == 0)
    {
        printf("\n too long!\n");
        return;
    }
    *pcnt = cnt;
    return;
}

int main(int argc, char **argv)
{
    sint8 Buf[512] = {0};
    sint8 user[64] = {0};
    sint8 pwd[64] = {0};
    sint8 cmd[1024] = {0};
    sint8* params[32] = {NULL};
    sint32 cnt = 0;
    sint32 index = 0;
    sint8* pPwd = NULL;
    sint32 iRet = CM_FAIL;
#ifdef CM_SESSION_TEST_FILE 
    bool_t testmode = cm_file_exist(CM_SESSION_TEST_FILE);
    if(CM_TRUE == testmode)
    {
        CM_VSPRINTF(Buf,sizeof(Buf)-1,"%s ",argv[0]);
    }
#endif    
    argc--;
    argv++;

    if(CM_OK != cm_cli_init())
    {
        return 1;
    }
    
#ifdef CM_SESSION_TEST_FILE
    if(CM_TRUE == testmode)
    {
        iRet=cm_cli_check_object(Buf,sizeof(Buf)-1,argc,argv);
        cm_omi_close();
        return iRet;
    }
#endif
    /* 取用户名 */
    if(CM_OK != cm_main_get_param_val(argc,argv,"-u",user,sizeof(user)))
    {
        cm_main_scanf("username:",user,sizeof(user));
    }
    /* 输入密码 */
    while(cnt<3)
    {
        pPwd = getpass("password:");
        cm_get_md5sum(pPwd,pwd,sizeof(pwd));
        if(CM_OK == cm_cli_check_user(user,pwd))
        {
            break;
        }
        cnt++;
    }
    if(cnt == 3)
    {
        cm_omi_close();
        return 1;
    }
    CM_SNPRINTF_ADD(user,sizeof(user),":>");
    while(1)
    {
        index = 0;
        cnt = sizeof(params)/sizeof(sint8*);
        cm_get_cmd_input(user,cmd,sizeof(cmd),params,&cnt);
        if(cmd[0] == '\0')
        {
            continue;
        }
        if((0 == strcmp(cmd,"exit"))
            || (0 == strcmp(cmd,"quit")))
        {
            cm_cli_logout();
            break;
        }
        Buf[0] = '\0';
        iRet = cm_cli_check_object(Buf,sizeof(Buf)-1,cnt,params);
        if(CM_NEED_LOGIN == iRet)
        {
            break;
        }
    }
    cm_omi_close();
    return iRet;
}


