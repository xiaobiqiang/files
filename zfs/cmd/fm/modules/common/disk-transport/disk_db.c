#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/wait.h>
#include "disk_db.h"

#define CM_DATA_DIR "/var/cm/data/"
#define CM_DISK_DB_DIR CM_DATA_DIR
#define CM_DISK_DB_RECORD "db_disk.db"
#define CM_DISK_DB_RECORD_TMP "db_disk_tmp.db"

typedef struct
{
    disk_db_handle_t db_handle;
    uint64_t global_id;
    uint64_t cache_id;
} disk_db_data_t;

typedef struct {
	char *id;
	char *sn;
	char *vender;
	int size;
	int rpm;
	int enid;
	int slotid;
	int status;
	uint64_t global_id;
} disk_db_record_t;
	
static disk_db_data_t g_disk_db_data;


int disk_file_exist(const char* filename)
{
    return (0 == access(filename,0));
}


int disk_db_open(const char* pname, disk_db_handle_t *phandle)
{
    int iRet = CM_FAIL;
    sqlite3 *psql = NULL;

    if(0 == disk_file_exist(pname)) {
        system("/usr/bin/touch /var/cm/data/db_disk_tmp.db");
    } else {
		system("/usr/bin/rm /var/cm/data/db_disk_tmp.db");
		system("/usr/bin/touch /var/cm/data/db_disk_tmp.db");
    }
    
    iRet = sqlite3_open(pname, &psql);
    if(CM_OK != iRet)
    {
        syslog(LOG_ERR, "open:%s fail[%d]",pname,iRet);
        return iRet;
    }
    *phandle = (disk_db_handle_t)psql;
    return CM_OK;
}

int disk_db_close(disk_db_handle_t handle)
{
	int count;
	int ret;
	
	(void)disk_db_exec_get_count(handle,&count,
            "SELECT COUNT(id) FROM record_t");
    syslog(LOG_ERR, "disk_count:%d\n", count);
    if(NULL == handle)
    {
        syslog(LOG_ERR, "handle: null");
        return CM_FAIL;
    }
    ret =  sqlite3_close((sqlite3*)handle);
	system("mv /var/cm/data/db_disk_tmp.db /var/cm/data/db_disk.db");
	return ret;
}

int disk_db_exec(
    disk_db_handle_t handle,
    disk_db_record_cbk_t cbk,
    void *arg,
    const char* sqlformat,...)
{
    int iRet = CM_FAIL;
    va_list args;
    char sqldiskd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;

    va_start(args, sqlformat);
    (void)vsnprintf(sqldiskd,CM_STRING_1K,sqlformat,args);
    va_end(args);
    
    iRet = sqlite3_exec((sqlite3*)handle,sqldiskd,cbk,arg,&perrmsg);

    if(NULL != perrmsg)
    {
        syslog(LOG_ERR, "exec:%s,errmsg:%s",sqldiskd,perrmsg);
        sqlite3_free(perrmsg);
    }

    return iRet;    
}

static int disk_db_get_count(
    void *arg, int col_cnt, char **col_vals, char **col_names)
{
    disk_db_count_t *pinfo = (disk_db_count_t*)arg;

    if(NULL == pinfo->pcount)
    {
        return CM_OK;
    }
    pinfo->is_set = 1;
    if((0 == col_cnt) || (NULL == col_vals) || (NULL == col_vals[0]))
    {
        syslog(LOG_ERR, "col_cnt:0");
        return CM_OK;
    }
    *(pinfo->pcount) = (int)atoll(col_vals[0]);
    return CM_OK;    
}

int disk_db_exec_get_count(disk_db_handle_t handle,int *pcnt,
    const char* sqlformat,...)
{
    disk_db_count_t info = {pcnt,0};
    int iRet = CM_FAIL;
    va_list args;
    char sqlcmd[CM_STRING_1K+1] = {0};
    char *perrmsg = NULL;

    va_start(args, sqlformat);
    (void)vsnprintf(sqlcmd,CM_STRING_1K,sqlformat,args);
    va_end(args);

    iRet = sqlite3_exec((sqlite3*)handle,sqlcmd,disk_db_get_count,&info,&perrmsg);

    if(NULL != perrmsg)
    {
        syslog(LOG_ERR, "exec:%s,errmsg:%s",sqlcmd,perrmsg);
        sqlite3_free(perrmsg);
    }
    return iRet;
}
static int disk_init_db(disk_db_handle_t handle)
{
    const char* create_tables[]=
    {
        "CREATE TABLE IF NOT EXISTS record_t ("
            "id VARCHAR(64),"
            "sn VARCHAR(64),"
            "vendor VARCHAR(64),"
            "nblocks VARCHAR(64),"
            "blksize VARCHAR(64),"
            "gsize DOUBLE,"
            "dim VARCHAR(64),"
            "enid INT,"
            "slotid INT,"
            "rpm INT,"
            "status VARCHAR(64),"
            "led VARCHAR(32))"
    };
	
    int iloop = 0;
    int table_cnt = sizeof(create_tables)/(sizeof(char*));
    int iRet = CM_OK;
    
    while(iloop < table_cnt)
    {
        iRet = disk_db_exec_ext(handle,"%s",create_tables[iloop]);
        if(CM_OK != iRet)
        {
            return iRet;
        }
        iloop++;
    } 
    return iRet;
}

int disk_db_init(void)
{
    int iRet = CM_OK;
    disk_db_data_t *pinfo = &g_disk_db_data;
    
    memset(pinfo, 0, sizeof(disk_db_data_t));
    iRet = disk_db_open(CM_DISK_DB_DIR
        CM_DISK_DB_RECORD_TMP,&pinfo->db_handle);
    if(CM_OK != iRet)
    {
        syslog(LOG_ERR,"open %s fail", CM_DISK_DB_RECORD);
        return iRet;
    }
    iRet = disk_init_db(pinfo->db_handle);
    if(CM_OK != iRet)
    {
        syslog(LOG_ERR,"init db fail");
        return iRet;
    }

    return CM_OK;
}


disk_db_handle_t disk_db_get(void)
{
    return g_disk_db_data.db_handle;
}

/* cnt received returned if succeed, otherwise -1*/
int disk_exec_cmd(char *out, unsigned sz, const char *fmt, ...)
{
    int rdcnt = 0;
    int iRet = 0;
    pid_t pid;
    int fd[2] = {0, 0};
    char cmd[512] = {0};
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(cmd, 512, fmt, ap);
    va_end(ap);

    if( (iRet = pipe(fd)) != 0 )
        return -1;

    if( (pid = fork()) < 0 )
        return -1;
    else if(pid > 0)
    {
        close(fd[1]);
        if(out != NULL && sz > 0)
        {
            while( (iRet = read(fd[0], out+rdcnt, sz-rdcnt)) > 0 )
                rdcnt += iRet;
        }
        waitpid(pid, &iRet, 0);
        return (WIFEXITED(iRet) ? rdcnt : -1);
    }

    close(fd[0]);
    if( (iRet = dup2(fd[1], STDOUT_FILENO)) != STDOUT_FILENO )
        _exit(-1);
    _exit(execl("/usr/bin/sh", "sh", "-c", cmd, (char *)0));
}

