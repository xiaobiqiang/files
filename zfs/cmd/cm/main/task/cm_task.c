#include "cm_task.h"
#include "cm_db.h"
#include "cm_sync.h"
#include "cm_log.h"
#include "cm_cmt.h"
#include "cm_node.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>


#define CM_TASK_MAX_TASK_NUM 64

/*
 * every wait time when this node is not master.
 *
 */
#define CM_TASK_WAIT_TIME       5

/*
 * timeout of request to to_nid.
 *
 */
#define CM_TASK_REQUEST_TMOUT   20

#define CM_TASK_SHARED_KEY_DIR  "/var/cm/static/task/"
#define CM_TASK_SHARED_KEY_PATH CM_TASK_SHARED_KEY_DIR"shared.flag"

#define CM_TASK_SHM_KEY         0xFF

static cm_mutex_t g_cm_task_lock;
static uint64 g_cm_task_id = 0;

/*
 * handle of task module's db.
 *
 */
static cm_db_handle_t g_cm_task_db_handle = NULL;

/*
 * all task types's global configuration.
 *
 */
extern const cm_task_type_cfg_t g_cm_task_cfg[CM_TASK_TYPE_BUTT];

static sint32 g_mmapshmid = 0;
static uint32 g_mmapshm_next_index = 0;
/*
 * used by /<cm_task_update>/ for distinguish
 * type of /<pdata>/.
 *
 */
typedef enum
{
    CM_TASK_UPDATE_ADD = 0,
    CM_TASK_UPDATE_ONLY,
    CM_TASK_UPDATE_DELETE
} cm_task_update_obj_t;

typedef cm_task_info_t  cm_task_sync_insert_msg_t;

typedef uint64          cm_task_sync_delete_msg_t;

typedef struct
{
    uint32 task_type;
    sint8 task_cmd[CM_TASK_CMD_LEN];
    sint8 task_param[CM_TASK_DESC_LEN];
} cm_task_send_msg_t;

static sint32 cm_task_exec(cm_task_type_cfg_t *pCfg, uint64 count);

static sint32 cm_task_exec_row(cm_task_info_t *pinfo, uint32 tmout);
static sint32 cm_task_get_info(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names);
static sint32 cm_task_cbk_state
(cm_task_info_t *info);
static sint32 cm_task_update
(uint64 task_id, uint32 update_obj, void *pdata, uint32 len);

static sint32 cm_task_exec_cmd(sint8 *cmd, sint32 mmapshmid, uint32 offset, uint32 *cpid);

static sint32 cm_task_get_info(
    void *arg, sint32 col_cnt, sint8 **col_vals, sint8 **col_names)
{
    cm_task_info_t *pinfo = (cm_task_info_t *)arg;

    if(col_cnt < 10)
        return CM_FAIL;

    pinfo->task_id = atoll(col_vals[0]);
    pinfo->work_id = atoi(col_vals[1]);
    pinfo->to_nid = atoi(col_vals[2]);
    pinfo->task_type = atoi(col_vals[3]);
    pinfo->task_state = atoi(col_vals[4]);
    pinfo->task_prog = atoi(col_vals[5]);
    pinfo->task_start = atoll(col_vals[6]);
    pinfo->task_end = atoll(col_vals[7]);
    CM_MEM_CPY(pinfo->task_cmd, CM_TASK_CMD_LEN, col_vals[8], strlen(col_vals[8]) + 1);
    CM_MEM_CPY(pinfo->task_desc, CM_TASK_DESC_LEN, col_vals[9], strlen(col_vals[9]) + 1);
    return CM_OK;
}

static sint32 cm_task_exec_row(cm_task_info_t *pinfo, uint32 tmout)
{
    sint32 iRet;
    uint32 echo_len = 0;
    cm_task_cmt_echo_t *echo_msg = NULL;
    cm_task_send_msg_t snd_msg;

    snd_msg.task_type = pinfo->task_type;
    CM_MEM_CPY(snd_msg.task_cmd, CM_TASK_CMD_LEN,
               pinfo->task_cmd, CM_TASK_CMD_LEN);
    CM_MEM_CPY(snd_msg.task_param, CM_TASK_DESC_LEN,
               pinfo->task_desc, CM_TASK_DESC_LEN);
    snd_msg.task_cmd[CM_TASK_CMD_LEN - 1] = '\0';
    snd_msg.task_param[CM_TASK_DESC_LEN - 1] = '\0';

    iRet = cm_cmt_request(pinfo->to_nid, CM_CMT_MSG_TASK_EXEC,
                          (void *)&snd_msg, sizeof(cm_task_send_msg_t),
                          (void **)&echo_msg, &echo_len,
                          CM_TASK_REQUEST_TMOUT);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_TASK, "cmt_request fail[%d]", iRet);

        if(NULL != echo_msg)
            CM_FREE(echo_msg);

        return CM_FAIL;
    }

    (void)cm_task_update(pinfo->task_id,
                         CM_TASK_UPDATE_ONLY,
                         (void *)echo_msg, echo_len);

    CM_FREE(echo_msg);
    return CM_OK;
}

#define CM_TASK_LIST_SIZE 5
static void * cm_task_exec_thread(void *arg)
{
    cm_db_handle_t handle = (cm_db_handle_t)arg;
    uint32 myid = cm_node_get_id();
    uint32 master = CM_NODE_ID_NONE;
    sint32 iRet = CM_FAIL;
    uint32 tmp = 0;
    uint64 count = 0;
    cm_task_info_t runlist[CM_TASK_LIST_SIZE];
    uint32 runcnt = 0;
    uint32 offset = 0;
    while(1)
    {
        master = cm_node_get_master();

        if(myid != master)
        {
            CM_SLEEP_S(CM_TASK_WAIT_TIME);
            continue;
        }

        (void)cm_db_exec_get_count(handle, &g_cm_task_id,
                                   "SELECT MAX(id) FROM record_t");

        for(tmp = 0; tmp < CM_TASK_TYPE_BUTT; tmp++)
        {
            count = 0;
            iRet = cm_db_exec_get_count(handle, &count,
                                        "SELECT COUNT(id) FROM record_t WHERE state = %u AND type = %u",
                                        CM_TASK_STATE_READY, tmp);

            if(iRet != CM_OK || count <= 0L)
            {
                continue;
            }
            else
            {
                (void)cm_task_exec(&g_cm_task_cfg[tmp], count);
            }
        }

        //check tasks's state
        offset = 0;
        do
        {
            CM_MEM_ZERO(runlist,sizeof(runlist));
            runcnt = cm_db_exec_get(handle,cm_task_get_info,runlist,CM_TASK_LIST_SIZE,sizeof(cm_task_info_t),
                "SELECT * FROM record_t WHERE state = %d ORDER BY id LIMIT %u,%u",
                CM_TASK_STATE_RUNNING,offset,CM_TASK_LIST_SIZE);
            offset += runcnt;
            for(tmp = 0; tmp < runcnt; tmp++)
            {
                (void)cm_task_cbk_state(&runlist[tmp]);
            }
        }while(runcnt==CM_TASK_LIST_SIZE);
        CM_SLEEP_S(CM_TASK_WAIT_TIME);
    }

    return NULL;
}

//assert that count > 0
static sint32 cm_task_exec(cm_task_type_cfg_t *pCfg, uint64 count)
{
    uint32 i = 0;
    uint64 run_cnt = 0;
    sint32 iRet = CM_FAIL;
    uint32 info_len = sizeof(cm_task_info_t);
    cm_task_info_t *pinfo = NULL;

    if(!pCfg->is_paral)
    {
        iRet = cm_db_exec_get_count(g_cm_task_db_handle, &run_cnt,
                                    "SELECT COUNT(id) FROM record_t WHERE type = %d AND state = %d",
                                    pCfg->task_type, CM_TASK_STATE_RUNNING);

        if(CM_OK != iRet)
        {
            return CM_FAIL;
        }

        if(0L == run_cnt)
        {
            pinfo = CM_MALLOC(info_len);

            if(NULL == pinfo)
            {
                return CM_FAIL;
            }

            iRet = cm_db_exec(g_cm_task_db_handle, cm_task_get_info, pinfo,
                              "SELECT * FROM record_t WHERE type = %d AND state = %d ORDER BY id LIMIT 1",
                              pCfg->task_type, CM_TASK_STATE_READY);

            if(CM_OK != iRet)
            {
                CM_FREE(pinfo);
                return CM_FAIL;
            }

            iRet = cm_task_exec_row(pinfo, CM_TASK_REQUEST_TMOUT);

            if(CM_OK != iRet)
            {
                CM_FREE(pinfo);
                return CM_FAIL;
            }

            CM_FREE(pinfo);
        }
    }
    else
    {
        pinfo = CM_MALLOC(info_len * count);

        if(NULL == pinfo)
        {
            return CM_FAIL;
        }

        count = cm_db_exec_get(g_cm_task_db_handle, cm_task_get_info, pinfo,
                               count, info_len,
                               "SELECT * FROM record_t WHERE type = %d AND state = %d ORDER BY id",
                               pCfg->task_type, CM_TASK_STATE_READY);

        for(i = 0; i < count; i++)
        {
            iRet = cm_task_exec_row(pinfo + i, CM_TASK_REQUEST_TMOUT);

            if(CM_OK != iRet)
            {
                CM_LOG_ERR(CM_MOD_TASK, "task exec row fail[%d]", iRet);
            }
        }

        CM_FREE(pinfo);
    }

    return CM_OK;
}

static sint32 cm_task_cbk_state
(cm_task_info_t *info)
{
    sint32 iRet;
    uint32 ackLen = 0;
    cm_task_cmt_echo_t *pAckData = NULL;
    cm_task_send_state_t sndMsg;

    sndMsg.task_id = (uint32)info->task_id;
    sndMsg.task_type = info->task_type;
    sndMsg.task_pid = info->work_id;
    CM_MEM_CPY(sndMsg.task_param, CM_TASK_DESC_LEN,
               info->task_desc, CM_TASK_DESC_LEN);
    iRet = cm_cmt_request(info->to_nid, CM_CMT_MSG_TASK_GET_STATE,
                          (void *)&sndMsg, sizeof(cm_task_send_state_t),
                          (void **)&pAckData, &ackLen,
                          CM_TASK_REQUEST_TMOUT);

    if((CM_OK != iRet) || (NULL == pAckData)
       || (sizeof(cm_task_cmt_echo_t) != ackLen))
    {
        CM_LOG_ERR(CM_MOD_TASK, "get ack data fail[%u]", ackLen);

        if(NULL != pAckData)
        { 
            CM_FREE(pAckData);
        }
        /*start  for 00006382 by wbn */
        if((CM_OK != iRet) && (CM_ERR_TIMEOUT != iRet))
        {
            cm_task_cmt_echo_t data;
            CM_LOG_ERR(CM_MOD_TASK, "node[%u],task[%llu],pid[%u],iRet[%d]", 
                info->to_nid,info->task_id,info->work_id,iRet);
            data.task_pid = info->work_id;
            data.task_prog = 100;
            data.task_state = CM_TASK_STATE_WRONG;
            data.task_end = cm_get_time();
            (void)cm_task_update(info->task_id, CM_TASK_UPDATE_ONLY,
                          &data, sizeof(cm_task_cmt_echo_t));
        }
        /*end  for 00006382 by wbn */
        return CM_FAIL;
    }

    if((pAckData->task_state == info->task_state)
       && (pAckData->task_prog == info->task_prog))
    {
        CM_FREE(pAckData);
        return CM_OK;
    }

    iRet = cm_task_update(info->task_id, CM_TASK_UPDATE_ONLY,
                          pAckData, sizeof(cm_task_cmt_echo_t));

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_TASK, "update fail[%d]", iRet);
    }

    CM_FREE(pAckData);
    return CM_OK;
}

static sint32 cm_task_init_mmapshm_next_index
(sint32 mmapshmid, sint32 *p_mmapshm_next_index)
{
    cm_task_cmt_echo_t *pTrav = NULL;
    void *mmapshmaddr = shmat(mmapshmid, NULL, 0);

    for(sint32 i = 0; i < CM_TASK_MAX_TASK_NUM; i++)
    {
        pTrav = mmapshmaddr + sizeof(cm_task_cmt_echo_t) * i;

        //?a??12?í?é?1??óD±?·???
        if(pTrav->task_pid == 0)
        {
            *p_mmapshm_next_index = i;
            (void)shmdt(mmapshmaddr);
            return CM_OK;
        }
    }

    (void)shmdt(mmapshmaddr);
    return CM_FAIL;
}

sint32 cm_task_init(void)
{
    cm_thread_t handle;
    sint32 iRet = CM_FAIL;
    const sint8* table = "CREATE TABLE IF NOT EXISTS record_t ("
                         "id BIGINT,"
                         "pid INT,"
                         "nid INT,"
                         "type INT,"
                         "state INT,"
                         "prog INT,"
                         "start BIGINT,"
                         "end BIGINT,"
                         "cmd TEXT(128),"
                         "description TEXT(256))";

    iRet = cm_db_open_ext(CM_TASK_DB_FILE, &g_cm_task_db_handle);

    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_TASK, "open %s fail", CM_TASK_DB_FILE);
        return iRet;
    }

    iRet = cm_db_exec_ext(g_cm_task_db_handle, table);

    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_TASK, "create table fail[%d]", iRet);
        return iRet;
    }

    CM_MUTEX_INIT(&g_cm_task_lock);

    (void)cm_system("mkdir -p "CM_TASK_SHARED_KEY_DIR);

    //初始化g_mmapshmid
    iRet = open(CM_TASK_SHARED_KEY_PATH, O_RDONLY | O_CREAT, 0666);

    if(iRet < 0)
    {
        CM_LOG_ERR(CM_MOD_TASK, "open %s fail[%d]", CM_TASK_SHARED_KEY_PATH, iRet);
        return iRet;
    }

    close(iRet);

    key_t key = ftok(CM_TASK_SHARED_KEY_PATH, CM_TASK_SHM_KEY);
    g_mmapshmid = shmget(key, sizeof(cm_task_cmt_echo_t) * CM_TASK_MAX_TASK_NUM, 0666);

    //?úo??1??óD′′?¨?a??12?í??
    if(g_mmapshmid < 0)
    {
        //′′?¨?a??12?í??
        g_mmapshmid = shmget(key, sizeof(cm_task_cmt_echo_t) * CM_TASK_MAX_TASK_NUM,
                             IPC_CREAT | IPC_EXCL | 0666);

        if(g_mmapshmid < 0)
        {
            CM_LOG_ERR(CM_MOD_TASK, "create shmid fail[%d]", g_mmapshmid);
            return CM_FAIL;
        }
    }
    //???°?úo?ò??-′′?¨á??a??12?í??,???′?íDèòa3?ê??ˉg_mmapshm_next_index
    else
    {
        iRet = cm_task_init_mmapshm_next_index(g_mmapshmid, &g_mmapshm_next_index);

        if(iRet != CM_OK)
        {
            CM_LOG_ERR(CM_MOD_TASK, "init g_mmapshm_next_index fail");
            return CM_FAIL;
        }
    }

    iRet = CM_THREAD_CREATE(&handle, cm_task_exec_thread, g_cm_task_db_handle);

    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_TASK, "create exec_thread fail[%d]", iRet);
        return iRet;
    }

    CM_THREAD_DETACH(handle);
    return CM_OK;
}

static uint64 cm_task_new_id(void)
{
    uint64 rs_id = 0;
    CM_MUTEX_LOCK(&g_cm_task_lock);
    rs_id = ++g_cm_task_id;
    CM_MUTEX_UNLOCK(&g_cm_task_lock);
    return rs_id;
}

static sint32 cm_task_cat_insert_sql(cm_task_info_t *pinfo, sint8 *rs_sql, uint32 len)
{
    (void)CM_SNPRINTF_ADD(rs_sql, len, "INSERT INTO record_t VALUES ");
    (void)CM_SNPRINTF_ADD(rs_sql, len, "(%llu,%u,%u,",
                          pinfo->task_id, pinfo->work_id, pinfo->to_nid);
    (void)CM_SNPRINTF_ADD(rs_sql, len, "%u,%u,%u,",
                          pinfo->task_type, pinfo->task_state, pinfo->task_prog);
    (void)CM_SNPRINTF_ADD(rs_sql, len, "%lu,%lu,'%s',",
                          pinfo->task_start, pinfo->task_end, pinfo->task_cmd);

    if((NULL == pinfo->task_desc)
       || (0 == strlen(pinfo->task_desc)))
    {
        (void)CM_SNPRINTF_ADD(rs_sql, len, "'NULL')");
    }
    else
    {
        (void)CM_SNPRINTF_ADD(rs_sql, len, "'%s')", pinfo->task_desc);
    }

    rs_sql[len - 1] = '\0';
    return CM_OK;
}


sint32 cm_task_cbk_sync_request(uint64 task_id, void *pdata, uint32 len)
{
    uint64 count = 0;
    sint32 iRet = CM_FAIL;
    uint32 tmp = 0;
    cm_task_info_t *p_task_info;
    cm_task_sync_update_msg_t *p_upd_msg;
    sint8 ins_sql[CM_STRING_512] = {0};

    iRet = cm_db_exec_get_count(g_cm_task_db_handle, &count,
                                "SELECT COUNT(id) FROM record_t WHERE id = %ld", task_id);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_TASK, "get count fail[%d]", iRet);
        return CM_FAIL;
    }

    if(count == 0L)
    {
        tmp = sizeof(cm_task_info_t);

        if(len < tmp)
        {
            CM_LOG_ERR(CM_MOD_TASK,
                       "task info len[%d], data len[%d]", tmp, len);
            return CM_FAIL;
        }

        p_task_info = (cm_task_info_t *)pdata;
        (void)cm_task_cat_insert_sql(p_task_info, ins_sql, sizeof(ins_sql));
        iRet = cm_db_exec_ext(g_cm_task_db_handle, ins_sql);
    }
    else
    {
        tmp = sizeof(cm_task_sync_update_msg_t);

        if(len < tmp)
        {
            CM_LOG_ERR(CM_MOD_TASK,
                       "update msg len[%d], data len[%d]", tmp, len);
            return CM_FAIL;
        }

        p_upd_msg = (cm_task_sync_update_msg_t *)pdata;
        iRet = cm_db_exec_ext(g_cm_task_db_handle,
                              "UPDATE record_t SET pid = %d, state = %d, prog = %d, end = %ld WHERE id = %ld",
                              p_upd_msg->task_pid, p_upd_msg->task_state, p_upd_msg->task_prog,
                              p_upd_msg->task_end, task_id);
    }

    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_TASK, "record fail[%d]", iRet);
        return CM_FAIL;
    }

    return CM_OK;
}

sint32 cm_task_cbk_sync_get(uint64 task_id, void **ppAck, uint32 *pLen)
{
    return cm_task_get(task_id, ppAck, pLen);
}


sint32 cm_task_cbk_sync_delete(uint64 task_id)
{

    sint32 iRet;
    iRet = cm_db_exec_ext(g_cm_task_db_handle,
                          "DELETE FROM record_t WHERE id = %ld", task_id);
    CM_LOG_INFO(CM_MOD_TASK, "task_id:%llu", task_id);
    return iRet;
}

static sint32 cm_task_echo_cmt_exec_fail(cm_task_cmt_echo_t *echo_msg)
{
    echo_msg->task_state = CM_TASK_STATE_WRONG;
    echo_msg->task_end = cm_get_time();
    echo_msg->task_prog = 0;
    echo_msg->task_pid = 0;
    return CM_OK;
}

static sint32 cm_task_get_mmapshm_offset()
{
    uint32 offset = 0;
    uint32 oldindex = 0;
    cm_task_cmt_echo_t *tmp = NULL;

    (void)shmctl(g_mmapshmid, SHM_LOCK, NULL);
    void *mmapshmaddr = shmat(g_mmapshmid, NULL, 0);
    oldindex = g_mmapshm_next_index;
    offset = sizeof(cm_task_cmt_echo_t) * g_mmapshm_next_index;

    for(sint32 i = g_mmapshm_next_index + 1; i < CM_TASK_MAX_TASK_NUM; i++)
    {
        tmp = mmapshmaddr + sizeof(cm_task_cmt_echo_t) * i;

        if(tmp->task_pid == 0)  //?1??±?·???3?è￥
        {
            g_mmapshm_next_index = i;
            break;
        }
    }

    if(oldindex == g_mmapshm_next_index)
    {
        (void)shmctl(g_mmapshmid, SHM_UNLOCK, NULL);
        return -1;
    }

    (void)shmctl(g_mmapshmid, SHM_UNLOCK, NULL);
    return offset;
}

sint32 cm_task_cbk_cmt_exec
(void *pdata, uint32 len, void **ppAckData, uint32 *acklen)
{
    sint32 iRet;
    uint32 subPID = 0;
    uint32 tmp = 0;
    cm_task_type_cfg_t *pCfg = NULL;
    cm_task_cmt_echo_t *pAckData = NULL;
    cm_task_send_msg_t *pRecvMsg = NULL;

    tmp = sizeof(cm_task_send_msg_t);
    pAckData = CM_MALLOC(tmp);

    if(NULL == pAckData)
    {
        CM_LOG_ERR(CM_MOD_TASK, "malloc fail");
        return CM_FAIL;
    }

    *acklen = sizeof(cm_task_cmt_echo_t);
    cm_task_echo_cmt_exec_fail(pAckData);
    *ppAckData = pAckData;

    if((NULL == pdata) || (len < tmp))
    {
        CM_LOG_ERR(CM_MOD_TASK,
                   "need len[%d], data len[%d]", tmp, len);
        return CM_OK;
    }

    //when return CM_FAIL, above function will throw ppAckData.
    pRecvMsg = (cm_task_send_msg_t *)pdata;
    pCfg = &g_cm_task_cfg[pRecvMsg->task_type];

    if(NULL != pCfg->cm_task_pre_exe)
    {
        iRet = pCfg->cm_task_pre_exe(pRecvMsg->task_param, CM_TASK_DESC_LEN,
                                     CM_TASK_REQUEST_TMOUT);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_TASK, "pre cmd exec fail");
            return CM_OK;
        }
    }

    tmp = cm_task_get_mmapshm_offset();

    //?a???úμ?μ?è????óáD?úá?￡?êμ?êê?12?í???úá?￡?è????′DDò2??2?μ??á1??￡
    if(tmp < 0)
    {
        CM_LOG_ERR(CM_MOD_TASK, "task queue is busy");
        return CM_OK;
    }

    iRet = cm_task_exec_cmd(pRecvMsg->task_cmd, g_mmapshmid, tmp, &subPID);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_TASK, "cmd exec fail[%d]", iRet);
        return CM_OK;
    }

    if(subPID != 0)
    {
        pAckData->task_state = CM_TASK_STATE_RUNNING;
        pAckData->task_pid = subPID;
        pAckData->task_prog = 0;
        pAckData->task_end = 0;
    }

    return CM_OK;
}

static sint32 cm_task_update(uint64 task_id, uint32 update_obj, void *pdata, uint32 len)
{
    uint32 upd_msg_len = sizeof(cm_task_sync_update_msg_t);
    uint32 ins_msg_len = sizeof(cm_task_sync_insert_msg_t);

    if(CM_TASK_UPDATE_DELETE == update_obj)
    {
        return cm_sync_request(CM_SYNC_OBJ_TASK, task_id, NULL, 0);
    }

    if(CM_TASK_UPDATE_ADD == update_obj)
    {
        if(len != ins_msg_len)
        {
            CM_LOG_ERR(CM_MOD_TASK,
                       "ins_msg_len[%lu], param_len[%lu]", ins_msg_len, len);
            return CM_FAIL;
        }

        return cm_sync_request(CM_SYNC_OBJ_TASK, task_id, pdata, len);
    }

    if(CM_TASK_UPDATE_ONLY == update_obj)
    {
        if(len != upd_msg_len)
        {
            CM_LOG_ERR(CM_MOD_TASK,
                       "upd_msg_len[%d], param_len[%d]", upd_msg_len, len);
            return CM_FAIL;
        }

        if(NULL == pdata)
        {
            CM_LOG_ERR(CM_MOD_TASK, "update data is NULL");
            return CM_FAIL;
        }

        return cm_sync_request(CM_SYNC_OBJ_TASK, task_id, pdata, len);
    }

    return CM_FAIL;
}

static sint32 cm_task_trav_mmapshm
(sint32 mmapshmid, uint32 pid, cm_task_cmt_echo_t **ppAck)
{
    cm_task_cmt_echo_t *pAck = NULL;
    cm_task_cmt_echo_t *pTrav = NULL;
    uint32 eachsize = sizeof(cm_task_cmt_echo_t);

    (void)shmctl(mmapshmid, SHM_LOCK, NULL);
    void *mmapshmaddr = shmat(mmapshmid, NULL, 0);

    for(sint32 i = 0; i < CM_TASK_MAX_TASK_NUM; i++)
    {
        pTrav = mmapshmaddr + eachsize * i;

        if(pTrav->task_pid == pid)
        {
            pAck = CM_MALLOC(eachsize);

            if(NULL == pAck)
            {
                (void)shmdt(mmapshmaddr);
                (void)shmctl(mmapshmid, SHM_UNLOCK, NULL);
                return CM_FAIL;
            }

            CM_MEM_CPY(pAck, eachsize, pTrav, eachsize);
            CM_MEM_ZERO(pTrav, eachsize);
            break;
        }
    }

    (void)shmdt(mmapshmaddr);
    (void)shmctl(mmapshmid, SHM_UNLOCK, NULL);

    if(NULL != pAck)
    {
        *ppAck = pAck;
    }

    return CM_OK;
}

sint32 cm_task_cbk_cmt_state
(void *arg, uint32 len, void **ppAckData, uint32 *acklen)
{
    sint32 iRet;
    cm_task_cmt_echo_t *pAckData = NULL;
    cm_task_send_state_t *pMsg = (cm_task_send_state_t *)arg;
    cm_task_type_cfg_t *pCfg = &g_cm_task_cfg[pMsg->task_type];

    (void)cm_task_trav_mmapshm(g_mmapshmid, pMsg->task_pid, &pAckData);

    //由system函数决定命令最终的状态
    if(pCfg->repo_by == CM_TASK_REPORT_BY_SYS)
    {
        if(NULL == pAckData)
        {
            
        }
    }

    switch(pCfg->repo_by)
    {
        case CM_TASK_REPORT_BY_CALL:
        {
            if(NULL != pAckData)
            {
                CM_FREE(pAckData);
                pAckData = NULL;
            }
        }
        case CM_TASK_REPORT_BY_SYS:
        {
            if(NULL != pAckData)
            {
                break;
            }
            
            iRet = pCfg->cm_task_report_exe(pMsg, &pAckData);
            if(iRet != CM_OK)
            {
                CM_LOG_ERR(CM_MOD_TASK, "report prog[%u] fail[%d]", pMsg->task_pid, iRet);
                return CM_FAIL;
            }
            break;
        }
        default:
        {
            CM_LOG_ERR(CM_MOD_TASK, "out of the range of report authority");
            return CM_FAIL;
        }
    }

    *ppAckData = pAckData;
    *acklen = sizeof(cm_task_cmt_echo_t);

    if((pAckData->task_prog == 100) && (NULL != pCfg->cm_task_aft_exe))
    {
        iRet = pCfg->cm_task_aft_exe(pMsg->task_param,
                                     CM_TASK_DESC_LEN,
                                     CM_TASK_REQUEST_TMOUT);

        if(CM_OK != iRet)
        {
            CM_LOG_WARNING(CM_MOD_TASK, "exec task_aft function fail");
        }
    }

    return CM_OK;
}

sint32 cm_task_cbk_ask_master
(void *arg, uint32 len, void **ppAckData, uint32 *acklen)
{
    cm_task_info_t *pinfo = NULL;

    if(len != sizeof(cm_task_info_t))
    {
        CM_LOG_ERR(CM_MOD_TASK,
                   "info len[%d], arg len[%d]",
                   sizeof(cm_task_info_t), len);
        return CM_FAIL;

    }

    pinfo = (cm_task_info_t *)arg;
    return cm_task_update(pinfo->task_id, CM_TASK_UPDATE_ADD,
                          (void *)pinfo, len);
}

static sint32 cm_task_new_task(uint32 toNid, uint32 type,
                               const sint8 *cmd, const sint8 *desc, cm_task_info_t **ppAck)
{
    cm_task_info_t *pAck = CM_MALLOC(sizeof(cm_task_info_t));

    if(NULL == pAck)
    {
        CM_LOG_ERR(CM_MOD_TASK, "get new info fail");
        return CM_FAIL;
    }

    CM_MEM_ZERO(pAck, sizeof(cm_task_info_t));
    pAck->task_id = cm_task_new_id();
    pAck->to_nid = toNid;
    pAck->task_type = type;
    pAck->work_id = 0;
    pAck->task_state = CM_TASK_STATE_READY;
    pAck->task_prog = 0;
    pAck->task_start = cm_get_time();
    pAck->task_end = 0;
    strncpy(pAck->task_cmd, cmd, CM_TASK_CMD_LEN);

    if(NULL != desc)
    {
        strncpy(pAck->task_desc, desc, CM_TASK_DESC_LEN);
    }

    *ppAck = pAck;
    return CM_OK;
}


sint32 cm_task_add
(uint32 toNid, uint32 type, const sint8 *cmd, const sint8 *desc)
{
    sint32 iRet;
    uint32 myid;
    uint32 master;
    cm_task_info_t *pinfo = NULL;
    cm_task_type_cfg_t *pCfg = NULL;

    if(type >= CM_TASK_TYPE_BUTT)
    {
        CM_LOG_ERR(CM_MOD_TASK, "not supported task_type[%u]", type);
        return CM_FAIL;
    }

    pCfg = &g_cm_task_cfg[type];

    if(NULL == pCfg->cm_task_report_exe)
    {
        CM_LOG_ERR(CM_MOD_TASK, "NULL report function");
        return CM_FAIL;
    }

    iRet = cm_task_new_task(toNid, type, cmd, desc, &pinfo);

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_TASK, "get new task fail");

        if(NULL != pinfo)
        {
            CM_FREE(pinfo);
        }

        return CM_FAIL;
    }

    myid = cm_node_get_id();
    master = cm_node_get_master();

    if(myid != master)
    {
        iRet = cm_cmt_request_master(CM_CMT_MSG_TASK_ASK_MASTER,
                                     (void *)pinfo, sizeof(cm_task_info_t),
                                     NULL, NULL, CM_TASK_REQUEST_TMOUT);

        if(CM_OK != iRet)
        {
            CM_LOG_ERR(CM_MOD_TASK, "request master fail[%d]", iRet);
            return CM_FAIL;
        }

        CM_FREE(pinfo);
        return CM_OK;
    }

    iRet = cm_task_update(pinfo->task_id, CM_TASK_UPDATE_ADD,
                          (void *)pinfo, sizeof(cm_task_info_t));
    CM_FREE(pinfo);
    return iRet;
}

sint32 cm_task_delete(uint64 task_id)
{
    sint32 iRet;
    iRet = cm_task_update(task_id, CM_TASK_UPDATE_DELETE, NULL, 0);
    return iRet;
}


// param is like 'id = xxx AND(OR) pid = xxx and so on'
//when *pCnt < total, free remainning memory
sint32 cm_task_getbatch(void *param, uint32 len,
                        uint64 offset, uint32 total,
                        cm_task_info_t **ppAck, uint32 *pCnt)
{
    uint32 count = 0;
    cm_task_info_t *pinfo = NULL;

    pinfo = CM_MALLOC(sizeof(cm_task_info_t) * total);

    if(NULL == pinfo)
    {
        CM_LOG_ERR(CM_MOD_TASK, "malloc fail");
        *ppAck = NULL;
        *pCnt = 0;
        return CM_FAIL;
    }

    if((NULL == param) || (0 == len)
       || (0 == strlen((sint8 *)param)))
    {
        count = cm_db_exec_get(g_cm_task_db_handle, cm_task_get_info, pinfo,
                               total, sizeof(cm_task_info_t),
                               "SELECT * FROM record_t ORDER BY id DESC LIMIT %llu, %lu", offset, total);
    }
    else
    {
        count = cm_db_exec_get(g_cm_task_db_handle, cm_task_get_info, pinfo,
                               total, sizeof(cm_task_info_t),
                               "SELECT * FROM record_t WHERE %s ORDER BY id DESC LIMIT %llu, %lu",
                               (sint8 *)param, offset, total);
    }

    *pCnt = count;
    *ppAck = pinfo;
    return CM_OK;
}

sint32 cm_task_get(uint64 task_id, void **ppAck, uint32 *pLen)
{
    sint32 iRet;
    cm_task_info_t *pAck;

    pAck = CM_MALLOC(sizeof(cm_task_info_t));

    if(NULL == pAck)
    {
        CM_LOG_ERR(CM_MOD_TASK, "malloc fail");
        return CM_FAIL;
    }

    CM_MEM_ZERO(pAck, sizeof(cm_task_info_t));
    iRet = cm_db_exec(g_cm_task_db_handle, cm_task_get_info, (void *)pAck,
                      "SELECT * FROM record_t WHERE id = %ld LIMIT 1", task_id);

    if(CM_OK != iRet || strlen(pAck->task_cmd) == 0)
    {
        CM_FREE(pAck);
        *ppAck = NULL;
        *pLen = 0;
        return CM_FAIL;
    }

    *ppAck = (void *)pAck;
    *pLen = sizeof(cm_task_info_t);
    return CM_OK;
}

sint32 cm_task_count(void *param, uint32 len, uint64 *pCnt)
{
    uint64 count = 0;
    sint32 iRet;

    if((NULL == param) || (0 == len)
       || (0 == strlen((sint8 *)param)))
    {
        iRet = cm_db_exec_get_count(g_cm_task_db_handle, &count,
                                    "SELECT COUNT(id) FROM record_t");
    }
    else
    {
        iRet = cm_db_exec_get_count(g_cm_task_db_handle, &count,
                                    "SELECT COUNT(id) FROM record_t WHERE %s", (sint8 *)param);
    }

    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_TASK, "get count fail[%d]", iRet);
        *pCnt = 0;
        return CM_FAIL;
    }

    *pCnt = count;
    return CM_OK;
}

sint32 cm_task_exec_cmd(sint8 *cmd, sint32 mmapshmid, uint32 offset, uint32 *cpid)
{
    pid_t pid;
    sint32 status = 0;
    sint32 iRet;
    sint32 fds[2];
    const char *command = cmd;

    //′′?¨μ?×ó????3ìμ?1üμà?￡
    iRet = pipe(fds);

    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_TASK, "create pipe fail[%d]", iRet);
        return CM_FAIL;
    }

    if((pid = fork()) < 0)
    {
        CM_LOG_ERR(CM_MOD_TASK, "fork error.");
        return CM_FAIL;
    }
    else if(pid > 0)
    {
        /* main process */
        (void)close(fds[1]);
        iRet = read(fds[0], cpid, sizeof(uint32));

        if(iRet < 0)
        {
            CM_LOG_ERR(CM_MOD_TASK, "read pid fail");
        }

        (void)waitpid(pid, &status, 0);

        if(WIFEXITED(status))
        {
            return CM_OK;
        }

        return CM_FAIL;
    }

    /* son process */
    iRet = daemon(0, 0);

    if(iRet != CM_OK)
    {
        CM_LOG_ERR(CM_MOD_TASK, "daemeonize fail[%d]", iRet);
        _exit(CM_FAIL);
    }

    /* grandson process */
    close(fds[0]);
    // system fork to exec the command
    sint32 cmdpid = getpid() + 1;
    (void)write(fds[1], &cmdpid, sizeof(sint32));

    iRet = system(command);

    (void)shmctl(mmapshmid, SHM_LOCK, NULL);
    void *mmapshmaddr = shmat(mmapshmid, NULL, 0);
    cm_task_cmt_echo_t *pShare = mmapshmaddr + offset;
    pShare->task_pid = cmdpid;
    pShare->task_prog = 100;
    pShare->task_end = cm_get_time();
    pShare->task_state = (iRet == 0 ? CM_TASK_STATE_FINISHED : CM_TASK_STATE_WRONG);
    (void)shmdt(mmapshmaddr);
    (void)shmctl(mmapshmid, SHM_UNLOCK, NULL);
    _exit(iRet);
}


