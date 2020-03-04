/********************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ********************************************************************************
 * filename     : cm_task_manager.h
 * author       : bqxiao
 * version      : v1.0
 * create date  : 2018.07.31
 * description  : in consideration of that we need to wait for a long time
 * when we execute some tasks which need for a long time, we abstract a
 * certain module to manage those tasks especially. when we need to run
 * tasks like those, we can add them into task manager module and query
 * them through interfaces provided by task manager.
 * we also provide some basic interfaces to manage tasks added into task
 * manager like add, get, getbatch, update and delete, and it is designed
 * to real-time synchronization, so when the master node doesn't work, the
 * task manager can do the same work at the new master node. for upper modules,
 * they have no influence when the master node change.
 *
 *********************************************************************************/

#ifndef MAIN_TASK_CM_TASK_H_
#define MAIN_TASK_CM_TASK_H_

#include "cm_common.h"
#include "cm_cfg_global.h"
/*
 * file path of task module db table /<record_t>/
 *
 */
#define CM_TASK_DB_FILE         CM_DATA_DIR"cm_task.db"

/*
 * max length of command to execute.
 *
 */
#define CM_TASK_CMD_LEN         CM_STRING_256
/*
 * max length of task's description.
 *
 */
#define CM_TASK_DESC_LEN        CM_STRING_256


typedef cm_time_t cm_task_time_t;

/*
 * task information.
 *
 */
typedef struct
{
    uint64 task_id;                     /* unique id for every task */
    uint32 work_id;                     /* task PID */
    uint32 to_nid;                      /* which node the task run at */
    uint32 task_type;
    uint32 task_state;
    uint32 task_prog;                   /* progress percent, not supported now */
    cm_task_time_t  task_start;         /* task added time */
    cm_task_time_t  task_end;           /* task end time */
    sint8 task_cmd[CM_TASK_CMD_LEN];    /* compelete real-command string */
    sint8 task_desc[CM_TASK_DESC_LEN];  /* task cbk param */
} cm_task_info_t;

/*
 * corresponding with CM_TASK_UPDATE_ONLY,
 * in this case, /<pdata>/ is the pointer of
 * the structure.
 *
 */
typedef struct
{
    uint32 task_pid;
    uint32 task_prog;
    uint32 task_state;
    cm_task_time_t task_end;
} cm_task_sync_update_msg_t;

typedef cm_task_sync_update_msg_t cm_task_cmt_echo_t;

/*
 *
 *
 */
typedef struct
{
    uint32 task_id;      
    uint32 task_type;
    uint32 task_pid;
    sint8 task_param[CM_TASK_DESC_LEN];
} cm_task_send_state_t;

/*
 * 
 *
 */
typedef enum
{
    CM_TASK_REPORT_BY_SYS = 0x1,
    CM_TASK_REPORT_BY_CALL = 0x2
} cm_task_report_authority_e;

typedef sint32 (*cm_task_cbk_func_t)
(sint8 *param, uint32 len, uint32 tmout);
typedef sint32 (*cm_task_cbk_report_func_t)
(cm_task_send_state_t *pSnd, cm_task_cmt_echo_t **pproc);

/*
 * task configuration according to task type.
 *
 */
typedef struct
{
    uint32 task_type;

    /*
     * CM_TRUE if tasks with the same type
     * are allowed to execute parallelly,
     * otherwise, it's CM_FALSE.
     *
     */
    uint32 is_paral;

    uint32 repo_by;

    /*
     * executed before the real command runs.
     * it's usually used to config environment
     * and so on.
     *
     */
    cm_task_cbk_func_t cm_task_pre_exe;

    /*
     * task call back this function while preparing to 
     * check state or progress of specified task, task 
     * get its reported progress returned by this function
     * to update some properties of record_t in cm_task.db
     *
     */
    cm_task_cbk_report_func_t cm_task_report_exe;

    /*
     * be contrary to /<cm_task_pre_exe>/, it'll be
     * executed after the real-command finish or
     * occur errors. used to recover the environment
     * normally.
     *
     */
    cm_task_cbk_func_t cm_task_aft_exe;
} cm_task_type_cfg_t;



/*
 * function name: cm_task_init
 * parameters   : void(none)
 * return       : sint32
 *                CM_OK if executed successfully.
 *                otherwise CM_FAIL.
 * brief        : used to do some initial work like
 *                creating /<record_t>/ table to save all
 *                tasks's information and creating threads
 *                to send command to execute and check tasks's
 *                states.
 *
 */
extern sint32 cm_task_init(void);

/*
 * function name: cm_task_getbatch
 * parameters   :
 *                param : optional parameter,usually a string of
 *                        some constraint of sqlite3 database.
 *                len   : length of /<*param>/.
 *                offset: display the query result from the
 *                        /<offset>/ record of /<record_t>/ table.
 *                total : from /<offset>/, dislay /<total>/ records.
 *                ppAck : result array of task info, length of this array
 *                        is /<total>/.
 *                pCnt  : the real-count of query results.
 * return       : sint32
 *                CM_OK if executed successfully.
 *                otherwise CM_FAIL.
 * brief        : usually used for paging, called by upper module.
 *
 */
extern sint32 cm_task_getbatch(void *param, uint32 len,
                               uint64 offset, uint32 total,
                               cm_task_info_t **ppAck, uint32 *pCnt);

/*
 * function name: cm_task_get
 * parameters   : task_id   : id
 *
 *
 *                ppAck     : query result.
 *                pLen      : length of /<ppAck>/
 * return       : sint32
 *                CM_OK if executed successfully.
 *                otherwise CM_FAIL.
 * brief        : usually used for searching a specified task record.
 *
 */
extern sint32 cm_task_get(uint64 task_id, void **ppAckm, uint32 *pLen);

/*
 * function name: cm_task_get
 * parameters   :
 *                param     : optional parameter,usually a string of
 *                            some constraint of sqlite3 database.
 *                len       : length of /<*param>/.
 *                pCnt      : count of query result.
 * return       : sint32
 *                CM_OK if executed successfully.
 *                otherwise CM_FAIL.
 * brief        : usually used for searching a specified task record.
 *
 */
extern sint32 cm_task_count(void *param, uint32 len, uint64 *pCnt);

/*
 *
 */
extern sint32 cm_task_add
(uint32 toNid, uint32 type, const sint8 *cmd, const sint8 *desc);

/*
 *
 */
extern sint32 cm_task_delete(uint64 task_id);

/*
 * function name: cm_task_cbk_sync_request
 * parameters   : task_id   : id
 *                pdata     : when dealing with the sync request of insertion,
 *                            it's the pointer of /<cm_task_info_t>/ structure.
 *                            when the sync request is updating existed record,
 *                            it's the pointer of /<cm_task_sync_update_msg_t>/
 *                            structure.
 *                len       : length of /<*pdata>/.
 * return       : sint32
 *                CM_OK if executed successfully.
 *                otherwise CM_FAIL.
 * brief        : it's a callback function called by /<cm_sync_request_local>/.
 *                used at every local node to synchronize task record.
 *
 */
extern sint32 cm_task_cbk_sync_request(uint64 task_id, void *pdata, uint32 len);

/*
 *
 */
extern sint32 cm_task_cbk_sync_get(uint64 task_id, void **ppAck, uint32 *pLen);

/*
 *
 */
extern sint32 cm_task_cbk_sync_delete(uint64 task_id);

/*
 * function name: cm_task_cbk_cmt_exec
 * parameters   : arg       : a pointer of /<cm_task_send_msg_t>/ structure.
 *                len       : length of /<*arg>/.
 *                ppAckData : a pointer of /<cm_task_cmt_echo_t>/ structure.
 *                acklen    : length of /<**ppAckData>/.
 * return       : sint32
 *                CM_OK if executed successfully.
 *                otherwise CM_FAIL.
 * brief        : a callback function called by /<cm_cmt_request>/ and
 *                /<cm_cmt_rpc_callback>/. used to really execute the command
 *                at the specified node and echo result data to master node.
 *
 */
extern sint32 cm_task_cbk_cmt_exec
(void *arg, uint32 len, void **ppAckData, uint32 *acklen);

/*
 * function name: cm_task_cbk_cmt_state
 * parameters   : arg       : a pointer of /<cm_task_send_state_t>/ structure.
 *                len       : length of /<*arg>/.
 *                ppAckData : a pointer of /<cm_task_cmt_echo_t>/ structure.
 *                acklen    : length of /<**ppAckData>/.
 * return       : sint32
 *                CM_OK if executed successfully.
 *                otherwise CM_FAIL.
 * brief        : familiar with /<cm_task_cbk_cmt_exec>/, but it's used to deal
 *                with state checking, task manager inform /<to_nid>/ of checking
 *                task's state with specified PID, and /<to_nid>/ call
 *                /<cm_task_cbk_cmt_state>/ function to check and echo result to master.
 *
 */
extern sint32 cm_task_cbk_cmt_state
(void *arg, uint32 len, void **ppAckData, uint32 *acklen);                      //

/*
 *
 *
 */
extern sint32 cm_task_cbk_ask_master
(void *arg, uint32 len, void **ppAckData, uint32 *acklen);

#endif /* MAIN_TASK_CM_TASK_H_ */
