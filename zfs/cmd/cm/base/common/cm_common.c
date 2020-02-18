 /******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_common.c
 * author     : wbn
 * create date: 2017年10月23日
 * description: TODO:
 *
 *****************************************************************************/
#include <sys/time.h>
#include <wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <regex.h>
#include <stdarg.h>

#include "cm_common.h"
#include "cm_log.h"

cm_time_t cm_get_time(void)
{
    struct timeval tv;

    (void)gettimeofday(&tv, NULL);
    return (cm_time_t)tv.tv_sec;
}

sint32 cm_set_time(cm_time_t tval)
{
    struct timeval tv;

    (void)gettimeofday(&tv, NULL);
    tv.tv_sec = tval;
    return settimeofday(&tv,NULL);
}

void cm_get_datetime(struct tm *tin)
{
    struct timeval tv;
    struct tm *t;
    
    (void)gettimeofday(&tv, NULL);
    t = localtime(&tv.tv_sec); 

    memcpy(tin, t, sizeof(struct tm));
    tin->tm_year += 1900;
    tin->tm_mon += 1;
    return;
}

void cm_get_datetime_ext(struct tm *tin,cm_time_t utc)
{
    struct tm *t;
    
    t = localtime(&utc); 

    memcpy(tin, t, sizeof(struct tm));
    tin->tm_year += 1900;
    tin->tm_mon += 1;
    return;
}

cm_time_t cm_get_time_hour_floor(cm_time_t utc)
{
    struct tm tin;
    memcpy(&tin, localtime(&utc), sizeof(struct tm));
    tin.tm_min = 0;
    tin.tm_sec = 0;
    return mktime(&tin);
}


void cm_utc_to_str(cm_time_t utc, sint8 *str, uint32 len)
{
    struct tm tin;
    cm_get_datetime_ext(&tin,utc);
    CM_VSPRINTF(str,len,"%04d%02d%02d%02d%02d%02d",
        tin.tm_year,tin.tm_mon,tin.tm_mday,
        tin.tm_hour,tin.tm_min,tin.tm_sec);
    return;
}

cm_time_t cm_mktime(const sint8 *strtime)
{
    struct tm tin;
    sint32 cnt = 0;
    cm_get_datetime(&tin);
    tin.tm_year = 1970;
    tin.tm_mon = 1;
    tin.tm_mday = 1;
    tin.tm_hour = 0;
    tin.tm_min = 0;
    tin.tm_sec = 0;
    cnt = sscanf(strtime, "%04d%02d%02d%02d%02d%02d",
        &tin.tm_year,&tin.tm_mon,&tin.tm_mday,
        &tin.tm_hour,&tin.tm_min,&tin.tm_sec);
    if((cnt > 0) &&
        (tin.tm_year >= 1900 && tin.tm_year<=2038) && 
        (tin.tm_mon >= 1 && tin.tm_mon <= 12) && 
        (tin.tm_mday >= 1 && tin.tm_mday <= 31) && 
        (tin.tm_hour >= 0 && tin.tm_hour <= 23) && 
        (tin.tm_min >= 0 && tin.tm_min <= 59) && 
        (tin.tm_sec >= 0 && tin.tm_sec <= 59))
    {
        tin.tm_year -= 1900;
        tin.tm_mon -= 1;
        return mktime(&tin);
    }
    return 0;
}


#define CM_ERR_TIMEOUT_INTER 2313254
#define CM_ERR_TIMEOUT_IRET 100

uint32 cm_exec_cmd_for_num(const sint8 *cmd)
{
    sint8 Buff[24] = {0};
    sint32 iRet = cm_exec_cmd_for_str(cmd, Buff, sizeof(Buff),2);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_NONE, "Exec:%s, iRet:%d",cmd,iRet);
        return 0;
    }
    return (uint32)atol(Buff);

}

uint32 cm_ipaddr_to_nid(const sint8 *pAddr)
{
    uint32 Nid = 0;
    inet_pton(AF_INET, pAddr,&Nid);
    Nid = htonl(Nid);
    Nid = ((Nid&0xFF00)<<1) + (Nid&0xFF);
    return Nid;
}


sint32 cm_get_local_nid(uint32 *pNid)
{
    /*const sint8 *pCfg = CM_NODE_CFG_FILE;*/
    *pNid = (uint32)cm_exec_int("/var/cm/script/cm_shell_exec.sh  cm_get_localcmid");
    return CM_OK;
}

uint32 cm_get_local_nid_x(void)
{
    static uint32 Nid = 0;

    if(0 != Nid)
    {
        return Nid;
    }
    (void)cm_get_local_nid(&Nid);
    return Nid;
}


sint32 cm_regex_check(const sint8* str, const sint8* pattern)
{
    sint32 iRet = CM_OK;
    regex_t reg;
    regmatch_t pmatch[1];

    iRet = regcomp(&reg,pattern, REG_EXTENDED);
    if(CM_OK != iRet)
    {
        //CM_LOG_ERR(CM_MOD_NONE,"regcomp:%s fail\n",pattern);
        return CM_FAIL;
    }

    iRet = regexec(&reg, str, 1,pmatch,0);
    if(REG_NOMATCH == iRet)
    {
        ;//CM_LOG_ERR(CM_MOD_NONE,"str:%s pattern:%s match none\n",str,pattern);
    }
    else if(CM_OK == iRet)
    {
        if((pmatch[0].rm_so != 0)
            || (pmatch[0].rm_eo != strlen(str)))
        {
            //CM_LOG_ERR(CM_MOD_NONE,"str:%s pattern:%s match:%d-%d\n",
            //    str,pattern,pmatch[0].rm_so,pmatch[0].rm_eo);
            iRet = CM_FAIL;
        }
    }
    regfree(&reg);
    return iRet;
}

sint32 cm_regex_check_num(const sint8* strnum,uint8 Bytes)
{
    sint8 Pattern[16] = {0};

    switch(Bytes)
    {
        case 1:
            Bytes = 3; /* 0-255 */
            break;
        case 2: /* 0-65535 */
            Bytes = 5;
            break;
        case 4: /* 0-4294967295 */
            Bytes = 10;
            break;
        case 8: /* 0-9223,37203,68547,75807 */
            Bytes = 20;
            break;
        default:
            return CM_FAIL;
    }
    CM_VSPRINTF(Pattern,sizeof(Pattern)-1,"[0-9]{1,%u}",Bytes);
    return cm_regex_check(strnum, Pattern);
}

#if(CM_MEM_DEBUG == 1)
void* cm_mem_malloc(int size, const char* func, int line)
{
    void *p = malloc(size);
    cm_log_print(CM_MOD_NONE,CM_LOG_LVL_WARNING,func,line,"malloc(%d) p=%p",size,p);
    return p;
}
void cm_mem_free(void *p,const char* func, int line)
{
    cm_log_print(CM_MOD_NONE,CM_LOG_LVL_WARNING,func,line,"free p=%p",p);
    free(p);
}
#endif


static sint32 cm_exec_in(const sint8* cmd, sint8 *buff, sint32 size)
{
    FILE *handle = popen(cmd, "r");

    if(NULL == handle)
    {
        return CM_FAIL;
    }

    if(NULL != buff && 0 != size)
    {
        size = fread(buff,1,size-1,handle);
        buff[size] = '\0';
    }    
    return pclose(handle)/256;
}

static double cm_exec_double_in(const sint8* cmd)
{
    sint8 buff[CM_STRING_256] = {0};
    double val = 0;
    sint32 iRet = CM_OK;
    cm_hrtime_t hrt = CM_GET_HRTIME();
    iRet = cm_exec_in(cmd, buff,sizeof(buff));
    hrt = cm_hrtime_delta(hrt,CM_GET_HRTIME());
    if(hrt >= CM_EXEC_LOG_TIME)
    {
        CM_LOG_WARNING(CM_MOD_NONE,"[%s] cost %llus iret %d",
            cmd,CM_HRTIME_TO_S(hrt),iRet);
    }
    if(CM_OK != iRet)
    {
        return 0;
    }
    val = atof(buff);    
    return val;
}

double cm_exec_double(const sint8* cmdforamt,...)
{
    va_list args;
    sint8 cmdbuf[CM_STRING_512] = {0};
    sint32 len=CM_VSPRINTF(cmdbuf,sizeof(cmdbuf),"timeout 10 ");
    
    va_start(args, cmdforamt);
    (void)vsnprintf(cmdbuf+len,CM_STRING_512-len,cmdforamt,args);
    va_end(args);
    return cm_exec_double_in(cmdbuf);
}

sint32 cm_exec_int(const sint8* cmdforamt,...)
{
    va_list args;
    sint8 cmdbuf[CM_STRING_512] = {0};
    sint32 len=CM_VSPRINTF(cmdbuf,sizeof(cmdbuf),"timeout 10 ");
    
    va_start(args, cmdforamt);
    (void)vsnprintf(cmdbuf+len,CM_STRING_512-len,cmdforamt,args);
    va_end(args);
    return (sint32)cm_exec_double_in(cmdbuf);
}

sint32 cm_system_in(uint32 tmout, const sint8* cmdforamt,...)
{
    va_list args;
    sint8 cmdbuf[CM_STRING_1K] = {0};
    cm_hrtime_t hrt = CM_GET_HRTIME();
    sint8 *pcmddata=cmdbuf;
    sint32 iret = 0;

    if(tmout > CM_CMT_REQ_TMOUT_NEVER)
    {
        iret=CM_VSPRINTF(cmdbuf,sizeof(cmdbuf),"timeout %u ",tmout);
        pcmddata=cmdbuf+iret;
    }
    
    va_start(args, cmdforamt);
    (void)vsnprintf(pcmddata,CM_STRING_1K-iret,cmdforamt,args);
    va_end(args);

    iret = system(cmdbuf);
    hrt = cm_hrtime_delta(hrt,CM_GET_HRTIME());
    if(hrt >= CM_EXEC_LOG_TIME)
    {
        CM_LOG_WARNING(CM_MOD_NONE,"[%s] cost %llus iret %d",
            pcmddata,CM_HRTIME_TO_S(hrt),iret);
    }
    /* system函数的返回值会乘以256 */
    iret /= 256;
    if(iret != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_NONE,"[%s] fail[%d]",pcmddata,iret);
        if(iret > CM_ERR_TIMEOUT_IRET)
        {
            iret = CM_FAIL;
        }        
    }
    return iret;
}


bool_t cm_file_exist(const sint8* filename)
{
    return (0 == access(filename,0));
}

sint32 cm_get_peer_addr(sint32 fd, sint8 *buff, sint32 size)
{
    struct sockaddr_in sa;
    socklen_t len=sizeof(sa);

    if(getpeername(fd,(struct sockaddr*)&sa,&len))
    {
        return CM_FAIL;
    }
    CM_VSPRINTF(buff,size-1,"%s",inet_ntoa(sa.sin_addr));
    return CM_OK;
}

const sint8* cm_get_peer_addr_str(sint32 fd)
{
    struct sockaddr_in sa;
    socklen_t len=sizeof(sa);

    if(getpeername(fd,(struct sockaddr*)&sa,&len))
    {
        return (const sint8*)"0.0.0.0";
    }
    return inet_ntoa(sa.sin_addr);
}


sint32 cm_get_md5sum(const sint8* str,sint8 *buff, sint32 size)
{
    return cm_exec(buff,size,"printf \'%s\' |md5sum |awk '{printf $1}'",str);
}


sint32 cm_exec_tmout(sint8 *buff, sint32 size,sint32 tmout, const sint8* cmdforamt,...)
{    
    sint32 iRet = CM_OK;
    sint8 cmd[CM_STRING_1K] = {0};
    cm_hrtime_t hrt = CM_GET_HRTIME();
    sint8 *pcmdstr = cmd;
    int len = 0;

    if(tmout > CM_CMT_REQ_TMOUT_NEVER)
    {
        len = CM_VSPRINTF(cmd,CM_STRING_1K,"timeout %u ",tmout);
        pcmdstr += len;
    }
    
    CM_ARGS_TO_BUF(pcmdstr, sizeof(cmd)-len,cmdforamt);

    iRet = cm_exec_in(cmd, buff, size);
    hrt = cm_hrtime_delta(hrt,CM_GET_HRTIME());
    if(hrt >= CM_EXEC_LOG_TIME)
    {
        CM_LOG_WARNING(CM_MOD_NONE,"[%s] cost %llus",pcmdstr,CM_HRTIME_TO_S(hrt));
    }
    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_NONE,"[%s] fail[%d]",pcmdstr,iRet);
        if(iRet > CM_ERR_TIMEOUT_IRET)
        {
            iRet = CM_FAIL;
        }        
    }
    return iRet;
}

sint32 cm_exec_get_subpid(const sint8 *cmd, uint32 *subPID)
{
    sint32 pid = 0;
    sint32 status;
    
    if(NULL == cmd)
    {
        CM_LOG_ERR(CM_MOD_TASK, "cmd is NULL");
        return CM_FAIL;
    }

    if((pid = vfork()) < 0)
    {
        CM_LOG_ERR(CM_MOD_TASK, "fork error[%d]", pid);
        return CM_FAIL;
    }
    else if(pid == 0)
    {
        if(NULL != subPID)
        {
            *subPID = getpid() + 1;
        }
        
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);

        if(NULL != subPID)
        {
            *subPID = 0;
        }

        _exit(0);
    }

    (void)waitpid(pid, &status, 0);

    if(WIFEXITED(status))
    {
        return CM_OK;
    }

    return CM_FAIL;
}

sint32 cm_thread_start(cm_thread_func_t thread, void* arg)
{
    cm_thread_t handle;
    sint32 iRet = CM_THREAD_CREATE(&handle,thread,arg);
    if(CM_OK != iRet)
    {
        return iRet;
    }
    CM_THREAD_DETACH(handle);
    return CM_OK;
}

uint32 cm_get_cols(sint8 *col_line, sint8 **cols, uint32 max_cols)
{
    uint32 cnt = 0;

    while((*col_line != '\0') 
            && (!cm_check_char(*col_line)))
    {
        col_line++;
    }
    
    while((cnt < max_cols) && (*col_line != '\0'))
    {
        *cols = col_line;
        cnt++;
        cols++;
        while(cm_check_char(*col_line))
        {
            col_line++;
        }
        while((*col_line != '\0') 
            && (!cm_check_char(*col_line)))
        {
            *col_line = '\0';
            col_line++;
        }
    }

    return cnt;
}

sint32 cm_exec_get_list(cm_exec_cbk_col_t cbk, 
    uint32 row_offset, uint32 each_size,void **ppbuff,uint32 *row_cnt,
    const sint8 *cmdformat, ...)
{
    FILE *fp = NULL;
    int iRet = CM_FAIL;
    int rows = 0;
    sint8 cmd[CM_STRING_1K] = {0};    
    sint8 tmpbuff[CM_STRING_1K] = {0};
    int len = CM_VSPRINTF(cmd,CM_STRING_1K,"timeout %u ",CM_CMT_REQ_TMOUT);
    sint8 *ptemp = cmd+len;
    uint32 cols_num = 0;
    sint8* cols[32] = {NULL};
    cm_hrtime_t hrt = CM_GET_HRTIME();
    
    CM_ARGS_TO_BUF(ptemp, sizeof(cmd)-len,cmdformat);
    CM_SNPRINTF_ADD(cmd,sizeof(cmd)," 2>/dev/null |sed -n '%u,%up'",
        row_offset+1, row_offset + *row_cnt); 
        
    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        CM_LOG_ERR(CM_MOD_NONE,"[%s] popenfail[%d]",cmd);
        return CM_FAIL;
    }

    do{
        len = each_size * (*row_cnt);
        *ppbuff = CM_MALLOC(len);
        if(NULL == *ppbuff)
        {
            CM_LOG_ERR(CM_MOD_NONE,"malloc[%d] fail",len);
            break;
        }
        for(ptemp = *ppbuff, rows=0;rows < *row_cnt; )
        {
            if(NULL == fgets(tmpbuff,CM_STRING_1K,fp))
            {
                //CM_LOG_INFO(CM_MOD_NONE,"cmd[%s] row_offset[%u] fgets\n",cmd,row_offset);
                break;
            }
            cols_num = cm_get_cols(tmpbuff,cols,sizeof(cols)/sizeof(sint8*));
            if(0 == cols_num)
            {
                //CM_LOG_INFO(CM_MOD_NONE,"cmd[%s] row_offset[%u] null\n",cmd,row_offset);
                continue;
            }
            iRet = cbk((void*)ptemp,cols,cols_num);
            if(CM_OK != iRet)
            {
                CM_LOG_INFO(CM_MOD_NONE,"cmd[%s] row_offset[%u] iRet[%d]\n",cmd,row_offset,iRet);
                continue;
            }
            rows++;
            ptemp += each_size;
        }
    }while(0);

    iRet = pclose(fp);
    hrt = cm_hrtime_delta(hrt,CM_GET_HRTIME());
    if(hrt >= CM_EXEC_LOG_TIME)
    {
        CM_LOG_WARNING(CM_MOD_NONE,"[%s] cost %llus",cmd,CM_HRTIME_TO_S(hrt));
    }
    
    if(CM_OK != iRet) {
        CM_LOG_ERR(CM_MOD_NONE,"[%s] fail[%d]",cmd,iRet);
        if(iRet > CM_ERR_TIMEOUT_IRET)
        {
            iRet = CM_FAIL;
        }
    }
    
    if((CM_OK == iRet) && (rows > 0))
    {
        *row_cnt = rows;
    }
    else
    {
        *row_cnt = 0;
        if(NULL != *ppbuff)
        {
            CM_FREE(*ppbuff);
            *ppbuff = NULL;
        }
    }
    return iRet;
}


uint64 cm_hrtime_delta(cm_hrtime_t old, cm_hrtime_t new)
{
	uint64 del;

	if ((new >= old) && (old >= 0L))
		return (new - old);
	else {
		/*
		 * We've overflowed the positive portion of an
		 * hrtime_t.
		 */
		if (new < 0L) {
			/*
			 * The new value is negative. Handle the
			 * case where the old value is positive or
			 * negative.
			 */
			uint64 n1;
			uint64 o1;

			n1 = -new;
			if (old > 0L)
				return (n1 - old);
			else {
				o1 = -old;
				del = n1 - o1;
				return (del);
			}
		} else {
			/*
			 * Either we've just gone from being negative
			 * to positive *or* the last entry was positive
			 * and the new entry is also positive but *less*
			 * than the old entry. This implies we waited
			 * quite a few days on a very fast system between
			 * iostat displays.
			 */
			if (old < 0L) {
				uint64 o2;

				o2 = -old;
				del = CM_UINT64_MAX - o2;
			} else {
				del = CM_UINT64_MAX - old;
			}
			del += new;
			return (del);
		}
	}
}


