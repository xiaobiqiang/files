#ifndef _DISK_DB_H_
#define _DISK_DB_H_

#define CM_OK 0
#define CM_FAIL 1
#define CM_PARAM_ERR   2
#define CM_ERR_TIMEOUT 3
#define CM_ERR_NOT_SUPPORT 4
#define CM_ERR_BUSY 5
#define CM_ERR_MSG_TOO_LONG 6
#define CM_ERR_NO_MASTER 7
#define CM_ERR_REDIRECT_MASTER 8
#define CM_ERR_CONN_NONE 9
#define CM_NEED_LOGIN 10

#define CM_STRING_32 32
#define CM_STRING_64 64
#define CM_STRING_256 256
#define CM_STRING_512 512
#define CM_STRING_1K 1024
#define CM_STRING_2K 2048
#define CM_STRING_4K 4096
#define CM_STRING_8K 8092



typedef struct
{
    int *pcount;
    int is_set;
}disk_db_count_t;


typedef void* disk_db_handle_t;
typedef struct sqlite3 sqlite3;
typedef int (*sqlite3_callback)(void* arg,int col_cnt, char **col_vals, char **col_names);

typedef int (*disk_db_record_cbk_t)(
    void *arg, int col_cnt, char **col_vals, char **col_names);

extern int sqlite3_close(sqlite3* handle);
extern void sqlite3_free(void* p);
extern int sqlite3_open(const char *filename, sqlite3 **ppDb);
extern int sqlite3_exec(sqlite3* handle, const char *sql,
	sqlite3_callback callback, void *arg, char **errmsg);


extern disk_db_handle_t disk_db_init(void);
extern disk_db_handle_t disk_db_get(void);
extern int disk_db_open(const char* pname, disk_db_handle_t *phandle);
extern int disk_db_open_ext(const char* pname, disk_db_handle_t *phandle);
extern int disk_db_close(disk_db_handle_t handle);

extern int disk_db_exec(
    disk_db_handle_t handle,
    disk_db_record_cbk_t cbk,
    void *arg,
    const char* sqlformat,...);

#define disk_db_exec_ext(handle,...) \
    disk_db_exec(handle, NULL,NULL,__VA_ARGS__)

extern int disk_db_exec_get_count(disk_db_handle_t handle, int *pcnt,
    const char* sqlformat,...);

extern int disk_db_exec_get(disk_db_handle_t handle, disk_db_record_cbk_t cbk, void *pbuff, int 
max_rows, int each_size, const char* sqlformat,...);  

extern int disk_exec_cmd(char *out, unsigned sz, const char *fmt, ...);

#endif /* _DISK_DB_H_ */

