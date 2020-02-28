/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cli.c
 * author     : wbn
 * create date: 2017年11月6日
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_cfg_global.h"
#include "cm_base.h"

#include "cm_cfg_cnm.h"
#include "cm_cfg_omi.h"
#include "cm_xml.h"


#define CM_CLI_NO_REQ CM_OFF
#define CM_ALARM_RES_FILE CM_CONFIG_DIR"res_en/alarm_desc.xml"
#define CM_CLI_ERROR_RES_FILE CM_CONFIG_DIR"res_en/error_code.xml"

static cm_tree_node_t *g_cm_cli_permission = NULL;
static uint32 g_cm_cli_permission_cnt = 0;
static uint32 g_cm_cli_level = 0;
extern const cm_omi_map_enum_t CmOmiMapEnumAdminLevelType;
extern const cm_omi_map_cfg_t CmOmiMapCmds[CM_OMI_CMD_BUTT];
static sint32 cm_cli_help_check_object_permission(uint32 obj)
{
    cm_tree_node_t *node = NULL;

    if((0 == g_cm_cli_level)
        || (CM_OK == cm_omi_check_permission_obj(obj)))
    {
        return CM_OK;
    }
    
    if((0 == g_cm_cli_permission_cnt)
       || (NULL == g_cm_cli_permission))
    {
        return CM_ERR_NO_PERMISSION;
    }

    node = cm_tree_node_find(g_cm_cli_permission, obj);

    if(NULL == node)
    {
        return CM_ERR_NO_PERMISSION;
    }

    return CM_OK;
}

static sint32 cm_cli_help_check_cmd_permission(uint32 obj, uint32 cmd)
{
    cm_tree_node_t *node = NULL;

    if((0 == g_cm_cli_level)
        || (CM_OK == cm_omi_check_permission(obj,cmd)))
    {
        return CM_OK;
    }
    
    if((0 == g_cm_cli_permission_cnt)
       || (NULL == g_cm_cli_permission))
    {
        return CM_ERR_NO_PERMISSION;
    }

    node = cm_tree_node_find(g_cm_cli_permission, obj);

    if(NULL == node)
    {
        return CM_ERR_NO_PERMISSION;
    }

    node = cm_tree_node_find(node, cmd);

    if(NULL == node)
    {
        return CM_ERR_NO_PERMISSION;
    }

    return CM_OK;
}


void cm_cli_help_object_cmd(const sint8* name,
                            const cm_omi_map_object_cmd_t *pCmdCfg)
{
    uint32 iloop = 0;
    const cm_omi_map_object_field_t *pField = NULL;
    printf("%s", name);

    for(iloop = 0; iloop < pCmdCfg->param_num; iloop++)
    {
        pField = pCmdCfg->params[iloop];
        printf(" <%s>", pField->field.pname);
    }

    for(iloop = 0; iloop < pCmdCfg->opt_param_num; iloop++)
    {
        pField = pCmdCfg->opt_params[iloop];
        printf(" [%s %s]", pField->short_name, pField->field.pname);
    }

    printf("\n");
    return;
}


void cm_cli_help_object(const sint8* name, const cm_omi_map_object_t* pCfg)
{
    uint32 iloop = 0;
    const cm_omi_map_object_cmd_t *pCmd = pCfg->cmds;

    if(CM_OK != cm_cli_help_check_object_permission(pCfg->obj.code))
    {
        return;
    }

    printf("%s view\n", name);

    while(iloop < pCfg->cmd_num)
    {
        if(CM_OK == cm_cli_help_check_cmd_permission(pCfg->obj.code, pCmd->pcmd->code))
        {
            printf("%s %s [...]\n", name, pCmd->pcmd->pname);
        }

        iloop++;
        pCmd++;
    }
}

void cm_cli_help(const sint8* name)
{
    const cm_omi_map_object_t * *pObj = CmOmiMap;
    uint32 iloop = 1;

    pObj++;

    while(iloop < CM_OMI_OBJECT_BUTT)
    {
        if((CM_OMI_OBJECT_SESSION != iloop)
           && (CM_OK == cm_cli_help_check_object_permission(iloop)))
        {
            printf("%s %s [...]\n", name, (*pObj)->obj.pname);
        }

        iloop++;
        pObj++;
    }

    return;
}

const cm_omi_map_object_t* cm_cli_object_get(const sint8* obj)
{
    const cm_omi_map_object_t * *pObj = CmOmiMap;
    uint32 iloop = 1;

    pObj++;

    while(iloop < CM_OMI_OBJECT_BUTT)
    {
        if(0 == strcmp((*pObj)->obj.pname, obj))
        {
            return *pObj;
        }

        iloop++;
        pObj++;
    }

    return NULL;
}

const cm_omi_map_cfg_t* cm_cli_object_get_field(
    const cm_omi_map_object_t* pObj, const sint8 *pname)
{
    uint32 iloop = 0;

    while(iloop < pObj->field_num)
    {
        if(0 == strcmp(pObj->fields[iloop].field.pname, pname))
        {
            return &pObj->fields[iloop].field;
        }

        iloop++;
    }

    return NULL;
}

const cm_omi_map_object_cmd_t* cm_cli_object_get_cmd(
    const cm_omi_map_object_t* pObj, const sint8 *pname)
{
    const cm_omi_map_object_cmd_t* pCmd = pObj->cmds;
    uint32 iloop = 0;

    while(iloop < pObj->cmd_num)
    {
        if(0 == strcmp(pCmd->pcmd->pname, pname))
        {
            return pCmd;
        }

        iloop++;
        pCmd++;
    }

    return NULL;
}

const cm_omi_map_object_field_t* cm_cli_object_get_optparam(
    const cm_omi_map_object_cmd_t* pCmd, const sint8* shortname)
{
    uint32 iloop = pCmd->opt_param_num;

    while(iloop > 0)
    {
        iloop--;

        if(0 == strcmp(pCmd->opt_params[iloop]->short_name, shortname))
        {
            return pCmd->opt_params[iloop];
        }
    }

    return NULL;
}

sint32 cm_cli_check_enum(const sint8* val, const cm_omi_map_enum_t *pEnum, uint32 *num)
{
    uint32 iloop = pEnum->num;
    const cm_omi_map_cfg_t *pCfg = pEnum->value;

    while(iloop > 0)
    {
        if(0 == strcmp(val, pCfg->pname))
        {
            *num = pCfg->code;
            return CM_OK;
        }

        iloop--;
        pCfg++;
    }

    return CM_FAIL;
}

sint32 cm_cli_check_objeck_cmd_fields(
    const cm_omi_map_object_t* pObj, const sint8* val, cm_omi_obj_t OmiObjParam)
{
    sint8 buf[CM_STRING_256] = {0};
    sint8 *pfield = NULL;
    sint8 setval[CM_STRING_256] = {0};
    sint32 len = 0;
    const cm_omi_map_cfg_t *pcfg = NULL;

    CM_MEM_CPY(buf, sizeof(buf), val, strlen(val));

    pfield = strtok(buf, ",");

    while(NULL != pfield)
    {
        pcfg = cm_cli_object_get_field(pObj, pfield);

        if(NULL == pcfg)
        {
            printf("field:%s invaild!\n", pfield);
            return CM_PARAM_ERR;
        }

        if(0 == len)
        {
            len = CM_VSPRINTF(setval, sizeof(setval), "%u", pcfg->code);
        }
        else
        {
            len += CM_VSPRINTF(setval + len, sizeof(setval) - len, ",%u", pcfg->code);
        }

        pfield = strtok(NULL, ",");
    }

    return cm_omi_obj_key_set_str_ex(OmiObjParam, CM_OMI_FIELD_FIELDS, setval);
}

static uint32 cm_cli_get_enum(const cm_omi_map_enum_t *pEnum, const sint8* 
    pName, uint32 default_val)
{
    uint32 iloop = pEnum->num;
    const cm_omi_map_cfg_t *pCfg = pEnum->value;
    while(iloop> 0)
    {
        if(0 == strcasecmp(pName, pCfg->pname))
        {
            return pCfg->code;
        }
        iloop--;
        pCfg++;
    }
    return default_val;
}
static sint32 cm_cli_check_objeck_cmd_levels(
    uint32 id, const sint8* val, cm_omi_obj_t OmiObjParam)
{
    sint8 buf[CM_STRING_256] = {0};
    sint8 *pfield = NULL;
    sint32 lvl = 0;
    uint32 setval = 0;

    CM_MEM_CPY(buf, sizeof(buf), val, strlen(val));

    pfield = strtok(buf, ",");

    while(NULL != pfield)
    {
        lvl = cm_cli_get_enum(&CmOmiMapEnumAdminLevelType, pfield,0);

        if(0 == lvl)
        {
            printf("field:%s invaild!\n", pfield);
            return CM_PARAM_ERR;
        }

        setval |= 1<<(lvl-1);

        pfield = strtok(NULL, ",");
    }

    return cm_omi_obj_key_set_u32_ex(OmiObjParam, id, setval);
}

sint32 cm_cli_check_object_cmd_param(
    const cm_omi_map_object_t* pObj,
    const cm_omi_map_object_field_t* pCfg,
    const sint8* val, cm_omi_obj_t OmiObjParam)
{
    sint32 iRet = CM_FAIL;
    uint32 Num = 0;
    sint8 pwd[CM_STRING_256] = {0};
    cm_time_t value;
    
    if((pCfg->data.type != CM_OMI_DATA_ENUM) 
        && (NULL != pCfg->data.check.pregex))
    {
        iRet = cm_regex_check(val, pCfg->data.check.pregex);

        if(CM_OK != iRet)
        {
            printf("param:%s, pattern:%s, match fail!\n", val, pCfg->data.check.pregex);
            return CM_PARAM_ERR;
        }
    }

    switch(pCfg->data.type)
    {
        case CM_OMI_DATA_STRING:
        case CM_OMI_DATA_PWD:
            if(strlen(val) >= pCfg->data.size)
            {
                printf("param:%s, length err!\n", val);
                return CM_PARAM_ERR;
            }           

            if(CM_OMI_FIELD_FIELDS == pCfg->field.code)
            {
                return cm_cli_check_objeck_cmd_fields(pObj, val, OmiObjParam);
            }

            break;

        case CM_OMI_DATA_INT:
            iRet = cm_regex_check_num(val, pCfg->data.size);

            if(CM_OK != iRet)
            {
                printf("param:%s, size:%u, match fail!\n", val, pCfg->data.size);
                return CM_PARAM_ERR;
            }

            break;

        case CM_OMI_DATA_ENUM:
            iRet = cm_cli_check_enum(val, pCfg->data.check.penum, &Num);

            if(CM_OK != iRet)
            {
                printf("param:%s match fail!\n", val);
                return CM_PARAM_ERR;
            }

            return cm_omi_obj_key_set_u32_ex(OmiObjParam, pCfg->field.code, Num);

        case CM_OMI_DATA_TIME:
        {
            value = cm_mktime(val);

            if(value != 0)
            {
                CM_LOG_INFO(CM_MOD_CNM, "[%ld]", iRet);
                return cm_omi_obj_key_set_u32_ex(OmiObjParam, pCfg->field.code, cm_mktime(val));
            }
            else
            {
                printf("param:%s match fail!\n", val);
                return CM_PARAM_ERR;
            }
        }

        case CM_OMI_DATA_ENUM_OBJ:
        {
            const cm_omi_map_object_t* pObjx = cm_cli_object_get(val);

            if(NULL == pObjx)
            {
                printf("param:%s match fail!\n", val);
                return CM_PARAM_ERR;
            }

            iRet = cm_cli_help_check_object_permission(pObjx->obj.code);

            if(CM_OK != iRet)
            {
                return iRet;
            }

            return cm_omi_obj_key_set_u32_ex(OmiObjParam, pCfg->field.code, pObjx->obj.code);
        }

        case CM_OMI_DATA_BITS_LEVELS:
        {
            return cm_cli_check_objeck_cmd_levels(pCfg->field.code,val,OmiObjParam);
        }

        default:
            return CM_PARAM_ERR;
    }

    if(pCfg->data.type == CM_OMI_DATA_PWD)
    {
        (void)cm_get_md5sum(val, pwd, sizeof(pwd));
        return cm_omi_obj_key_set_str_ex(OmiObjParam, pCfg->field.code, pwd);
    }

    return cm_omi_obj_key_set_str_ex(OmiObjParam, pCfg->field.code, val);
}

sint32 cm_cli_check_object_cmd(
    const cm_omi_map_object_t* pObj,
    sint8* FormatHead, uint32 Size, sint32 argc, sint8 **argv,
    const cm_omi_map_object_cmd_t* pCmd, cm_omi_obj_t OmiObj)
{
    cm_omi_obj_t OmiObjParam = NULL;
    uint32 iloop = 0;
    sint32 iRet = CM_OK;
    const cm_omi_map_object_field_t* pField = NULL;

    if(CM_OK != cm_omi_obj_key_set_u32(OmiObj, CM_OMI_KEY_CMD, pCmd->pcmd->code))
    {
        CM_LOG_ERR(CM_MOD_NONE, "set key cmd fail");
        return CM_FAIL;
    }

    if(argc < pCmd->param_num)
    {
        cm_cli_help_object_cmd(FormatHead, pCmd);
        return CM_FAIL;
    }

    if(argc > 0)
    {
        OmiObjParam = cm_omi_obj_new();

        if(NULL == OmiObjParam)
        {
            CM_LOG_ERR(CM_MOD_NONE, "new param fail");
            return CM_FAIL;
        }
    }

    for(iloop = 0; iloop < pCmd->param_num; iloop++)
    {
        iRet = cm_cli_check_object_cmd_param(pObj,
                                             pCmd->params[iloop], argv[iloop], OmiObjParam);

        if(CM_OK != iRet)
        {
            cm_omi_obj_delete(OmiObjParam);
            cm_cli_help_object_cmd(FormatHead, pCmd);
            return iRet;
        }
    }

    argc -= pCmd->param_num;
    argv += pCmd->param_num;

    if((argc & 1) || ((argc >> 1) > pCmd->opt_param_num))
    {
        cm_omi_obj_delete(OmiObjParam);
        cm_cli_help_object_cmd(FormatHead, pCmd);
        return CM_FAIL;
    }

    for(iloop = 0; iloop < argc; iloop += 2)
    {
        pField = cm_cli_object_get_optparam(pCmd, argv[iloop]);

        if(NULL == pField)
        {
            cm_omi_obj_delete(OmiObjParam);
            cm_cli_help_object_cmd(FormatHead, pCmd);
            return CM_FAIL;
        }

        iRet = cm_cli_check_object_cmd_param(pObj, pField, argv[iloop + 1], OmiObjParam);

        if(CM_OK != iRet)
        {
            cm_omi_obj_delete(OmiObjParam);
            cm_cli_help_object_cmd(FormatHead, pCmd);
            return iRet;
        }
    }

    if(NULL != OmiObjParam)
    {
        iRet = cm_omi_obj_key_add(OmiObj, CM_OMI_KEY_PARAM, OmiObjParam);

        if(CM_OK != iRet)
        {
            cm_omi_obj_delete(OmiObjParam);
            return CM_FAIL;
        }
    }

    return CM_OK;
}

void cm_cli_print_ack_count(cm_omi_obj_t AckObj, const sint8* name)
{
    cm_omi_obj_t item = NULL;
    cm_omi_obj_t items = cm_omi_obj_key_find(AckObj, CM_OMI_KEY_ITEMS);
    uint32 size = 0;
    uint64 count = 0;
    uint32 key=CM_OMI_FIELD_COUNT;
    if(NULL == items)
    {
        return;
    }

    size = cm_omi_obj_array_size(items);

    if(0 == size)
    {
        return;
    }

    item = cm_omi_obj_array_idx(items, 0);

    if(NULL == item)
    {
        return;
    }
    if(0 != strcmp(name, "count"))
    {
        key = CM_OMI_FIELD_TASK_ID;
    }

    if(CM_OK != cm_omi_obj_key_get_u64_ex(item, key, &count))
    {
        return;
    }

    printf("%s : %llu\n", name,count);
    return;
}

void cm_cli_print_ack_errormsg(cm_omi_obj_t AckObj)
{
    cm_omi_obj_t item = NULL;
    cm_omi_obj_t items = cm_omi_obj_key_find(AckObj, CM_OMI_KEY_ITEMS);
    sint32 size;
    sint8 errmsg[CM_STRING_1K] = {0};
    if(NULL == items)
    {
        return;
    }

    size = cm_omi_obj_array_size(items);

    if(0 == size)
    {
        return;
    }

    item = cm_omi_obj_array_idx(items, 0);

    if(NULL == item)
    {
        return;
    }

    if(CM_OK == cm_omi_obj_key_get_str_ex(item,CM_OMI_FIELD_ERROR_MSG,errmsg,sizeof(errmsg)))
    {
        printf("Error: %s\n", errmsg);
    }
    return;
}


void cm_cli_print_ack_field(cm_omi_obj_t field, const cm_omi_map_object_field_t *pFieldCfg)
{
    const cm_omi_map_enum_t *pEnum = NULL;
    const sint8* pData = cm_omi_obj_value(field);
    const cm_omi_map_cfg_t *pMap = NULL;
    uint32 iloop = 0;
    uint32 val = 0;

    if(NULL == pData)
    {
        return;
    }

    printf("%-10s: ", pFieldCfg->field.pname);

    switch(pFieldCfg->data.type)
    {
        case CM_OMI_DATA_STRING:
        case CM_OMI_DATA_INT:
            printf("%s\n", pData);
            return;

        case CM_OMI_DATA_ENUM:
            break;

        case CM_OMI_DATA_TIME:
        {
            struct tm tin;
            cm_time_t tdata = (cm_time_t)atoi(pData);

            if(tdata > 0)
            {
                cm_get_datetime_ext(&tin, tdata);
                printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                       tin.tm_year, tin.tm_mon, tin.tm_mday,
                       tin.tm_hour, tin.tm_min, tin.tm_sec);
            }
            else
            {
                printf("--\n");
            }
        }

        return;

        case CM_OMI_DATA_BITS:
        {
            val = (uint32)atoi(pData);
            iloop = 0;

            while(val)
            {
                if(val & 1)
                {
                    printf("#%02u ", iloop);
                }

                iloop++;
                val = val >> 1;
            }

            printf("\n");
        }

        return;

        case CM_OMI_DATA_ENUM_OBJ:
        {
            val = (uint32)atoi(pData);
            if(val < CM_OMI_OBJECT_BUTT)
            {
                printf("%s\n", CmOmiMap[val]->obj.pname);
            }
            return;
        }

        case CM_OMI_DATA_BITS_LEVELS:
        {
            val = (uint32)atoi(pData);
            iloop = 1;
            while(val)
            {
                if((val & 1) && (iloop < CmOmiMapEnumAdminLevelType.num))
                {
                    printf("%s ", CmOmiMapEnumAdminLevelType.value[iloop].pname);
                }

                iloop++;
                val = val >> 1;
            }
            printf("\n");
            return;
            
        }
        
        default:
            printf("%s\n", pData);
            return;
    }

    pEnum = pFieldCfg->data.check.penum;
    val = (uint32)atoi(pData);
    pMap = pEnum->value;

    while(iloop < pEnum->num)
    {
        if(pMap->code == val)
        {
            printf("%s\n", pMap->pname);
            return;
        }

        iloop++;
        pMap++;
    }

    printf("\n");
    return;
}

static void cm_cli_print_ack_item_default(const cm_omi_map_object_t* pObj, cm_omi_obj_t item)
{
    uint32 index = 0;
    cm_omi_obj_t field = NULL;
    const cm_omi_map_object_field_t *pFieldCfg = NULL;

    for(index = 0, pFieldCfg = pObj->fields;
        index < pObj->field_num;
        index++, pFieldCfg++)
    {
        field = cm_omi_obj_key_find_ex(item, pFieldCfg->field.code);

        if(NULL == field)
        {
            continue;
        }

        cm_cli_print_ack_field(field, pFieldCfg);
    }

    return;
}

static void cm_cli_print_alarm_info(const sint8 *almid, const sint8 *paramlist, bool_t is_list)
{
    sint8 buff[CM_STRING_1K] = {0};
    sint8 *pfield[CM_STRING_256] = {NULL};
    const sint8* names[] = {"title", "desc", "suggest"};
    uint32 cnt = (is_list == CM_TRUE) ? 1 : 3;
    uint32 iloop = 0;
    sint8 *ptmp = NULL;
    uint8 param_index = 0;
    const sint8* ptemname = NULL;

    (void)cm_omi_get_fields(paramlist, pfield, CM_STRING_256);

    while(iloop < cnt)
    {
        buff[0] = '\0';
        printf("%-10s: ", names[iloop]);
        (void)cm_exec(buff, sizeof(buff) - 1, "grep 'id=\"%s\"' "CM_ALARM_RES_FILE
                      " |sed 's/.*%s=\"\\(.*\\)/\\1/g' |awk -F'\"' '{printf $1}'", almid, names[iloop]);
        ptmp = buff;

        while(*ptmp != '\0')
        {
            if(('#' == *ptmp) && ('0' <= *(ptmp + 1)) && (*(ptmp + 1) <= '9'))
            {
                ptmp++;
                param_index = (uint8)atoi(ptmp);

                if(NULL == pfield[param_index])
                {
                    printf("--");
                }
                else
                {
                    ptemname = pfield[param_index];

                    while((*ptemname != '\0') && (*ptemname != ','))
                    {
                        printf("%c", *ptemname);
                        ptemname++;
                    }
                }

                while(('0' <= *ptmp) && (*ptmp <= '9'))
                {
                    ptmp++;
                }
            }
            else
            {
                printf("%c", *ptmp);
                ptmp++;
            }
        }

        printf("\n");
        iloop++;
    }

    return;
}

static void cm_cli_print_ack_item_alarm(const cm_omi_map_object_t* pObj,
                                        cm_omi_obj_t item, bool_t is_list)
{
    uint32 index = 0;
    cm_omi_obj_t field = NULL;
    const cm_omi_map_object_field_t *pFieldCfg = NULL;
    const sint8 *almid = NULL;
    const sint8 *paramlist = NULL;

    for(index = 0, pFieldCfg = pObj->fields;
        index < pObj->field_num;
        index++, pFieldCfg++)
    {
        field = cm_omi_obj_key_find_ex(item, pFieldCfg->field.code);

        if(NULL == field)
        {
            continue;
        }

        switch(pFieldCfg->field.code)
        {
            case CM_OMI_FIELD_ALARM_PARENT_ID:
                almid = cm_omi_obj_value(field);
                break;

            case CM_OMI_FIELD_ALARM_PARAM:
                paramlist = cm_omi_obj_value(field);
                break;

            default:
                cm_cli_print_ack_field(field, pFieldCfg);
                break;
        }
    }

    if((NULL != almid) && (NULL != paramlist))
    {
        cm_cli_print_alarm_info(almid, paramlist, is_list);
    }

    return;
}

static void cm_cli_print_ack_item_alarmeach(const cm_omi_map_object_t* pObj, cm_omi_obj_t item)
{
    cm_cli_print_ack_item_alarm(pObj, item, CM_FALSE);
}

static void cm_cli_print_ack_item_alarmlist(const cm_omi_map_object_t* pObj, cm_omi_obj_t item)
{
    cm_cli_print_ack_item_alarm(pObj, item, CM_TRUE);
}

static void cm_cli_print_ack_field_task_desc
(cm_omi_obj_t item, cm_omi_obj_t param_item, const cm_omi_map_object_field_t * p_field_cfg)
{
    sint32 iRet;
    sint32 tmp = 0;
    sint32 desc_len = 0;
    sint8 desc[CM_STRING_256] = {0};
    sint8 rs[CM_STRING_256] = {0};
    sint8 tmp_line[CM_STRING_256] = {0};
    sint8 *param = cm_omi_obj_value(param_item); 
    cm_omi_obj_t type_item = cm_omi_obj_key_find_ex(item, CM_OMI_FIELD_TASK_MANAGER_TYPE);   
    sint8 *type_str = cm_omi_obj_value(type_item);
    
    iRet = cm_exec_tmout(desc, sizeof(desc), 2, "awk '/type=\"%s\"/' "
                         "/var/cm/static/task/task_desc.xml "
                         "| awk -F'=' '{printf $4}'", type_str);
    if(iRet != CM_OK || strlen(desc) == 0)
    {
        printf("%s:-\n", p_field_cfg->field.pname);
        return ;
    }
    
    desc[strlen(desc)-4] = '\0';
    uint32 isshtops = CM_TRUE;  //是否应该把tmp_line追加到rs
    uint32 isopshda = CM_FALSE; //是否一句话含有有数据参数
    uint32 next_index = 0;
    desc_len = strlen(desc);
    for(sint32 i=0; i<desc_len+1; i++)
    {
        if(desc[i] == ',')
        {
            if(isshtops == CM_TRUE)
            {
                CM_SNPRINTF_ADD(rs, sizeof(rs), "%s,", tmp_line);
                CM_MEM_ZERO(tmp_line, sizeof(tmp_line));
            }
            else
            {
                isshtops = CM_TRUE;
            }
            next_index = 0;
            isopshda = CM_FALSE;
        }
        else if(desc[i] == '$')
        {
            tmp = desc[i+1] - 48;
            sint8 tmps[CM_STRING_128] = {0};
            (void)cm_exec_tmout(tmps, sizeof(tmps), 2, "echo '%s' | awk -F'|' '{printf $%d}'", param, tmp);
            CM_SNPRINTF_ADD(tmp_line, sizeof(tmp_line), "%s", tmps);
            next_index += strlen(tmps);
            i++;
            isopshda = CM_TRUE;
            isshtops = CM_TRUE;
        }
        else if(desc[i] == '#')
        {
            tmp = desc[i+1] - 48;
            sint8 tmps[CM_STRING_128] = {0};
            (void)cm_exec_tmout(tmps, sizeof(tmps), 2, "echo '%s' | awk -F'|' '{printf $%d}'", param, tmp);
            uint32 len = strlen(tmps);
            if(len > 0)
            {
                CM_SNPRINTF_ADD(tmp_line, sizeof(tmp_line), "%s", tmps);
                next_index += len;
                isopshda = CM_TRUE;
                isshtops = CM_TRUE;
            }   
            else if(isopshda == CM_FALSE)
            {
                isshtops = CM_FALSE;
            }
            else
            {
                tmp_line[next_index++] = '-';
            }
            i++;
        }
        else
        {
            tmp_line[next_index++] = desc[i];
        }
    }
    if(isshtops == CM_TRUE)
    {
        CM_SNPRINTF_ADD(rs, sizeof(rs), "%s", tmp_line);
    }
    printf("%-10s: %s\n", p_field_cfg->field.pname, rs);
}

static void cm_cli_print_ack_item_task(const cm_omi_map_object_t* pObj, cm_omi_obj_t item)
{
    cm_omi_obj_t f_val = NULL;
    const cm_omi_map_object_field_t *pFcfg = pObj->fields;

    for(uint32 i = 0; i < pObj->field_num; i++, pFcfg++)
    {
        f_val = cm_omi_obj_key_find_ex(item, pFcfg->field.code);

        if(NULL == f_val)
        {
            continue;
        }

        if(pFcfg->field.code != CM_OMI_FIELD_TASK_MANAGER_DESC)
        {
            cm_cli_print_ack_field(f_val, pFcfg);
        }
        else
        {
            cm_cli_print_ack_field_task_desc(item, f_val, pFcfg);
        }
    }

}

typedef void (*cm_cli_print_ack_item_cbk_t)(const cm_omi_map_object_t* pObj, cm_omi_obj_t item);

void cm_cli_print_ack_item(const cm_omi_map_object_t* pObj,
                           cm_omi_obj_t items, cm_cli_print_ack_item_cbk_t cbk)
{
    uint32 items_size = 0;
    uint32 iloop = 0;
    cm_omi_obj_t item = NULL;

    if(NULL == items)
    {
        return;
    }

    items_size = cm_omi_obj_array_size(items);

    for(iloop = 0; iloop < items_size; iloop++)
    {
        item = cm_omi_obj_array_idx(items, iloop);

        if(NULL == item)
        {
            continue;
        }

        printf("--------------------------------------\n");
        cbk(pObj, item);
    }
}

sint32 cm_cli_print_ack(const cm_omi_map_object_t* pObj, const cm_omi_map_object_cmd_t* pCmd, cm_omi_obj_t AckObj)
{
    sint32 iRetx = CM_FAIL;
    sint32 iRet = cm_omi_obj_key_get_s32(AckObj, CM_OMI_KEY_RESULT, &iRetx);
    cm_cli_print_ack_item_cbk_t cbk = cm_cli_print_ack_item_default;

    if((CM_OK != iRet) ||(CM_OK != iRetx))
    {
        cm_cli_print_ack_errormsg(AckObj);
        return iRet;
    }

    switch(pCmd->pcmd->code)
    {
        case CM_OMI_CMD_GET_BATCH:
        case CM_OMI_CMD_GET:
        case CM_OMI_CMD_SCAN:
            break;
        case CM_OMI_CMD_ADD:
        case CM_OMI_CMD_MODIFY:
        case CM_OMI_CMD_DELETE:
            cm_cli_print_ack_count(AckObj,"taskid");
            return CM_OK;
        case CM_OMI_CMD_COUNT:
            cm_cli_print_ack_count(AckObj,"count");
            return CM_OK;

        default:
            return CM_OK;
    }

    if((pObj->obj.code == CM_OMI_OBJECT_ALARM_CURRENT)
       || (pObj->obj.code == CM_OMI_OBJECT_ALARM_HISTORY))
    {
        if(pCmd->pcmd->code == CM_OMI_CMD_GET_BATCH)
        {
            cbk = cm_cli_print_ack_item_alarmlist;
        }
        else
        {
            cbk = cm_cli_print_ack_item_alarmeach;
        }
    }

    if(pObj->obj.code == CM_OMI_OBJECT_TASK_MANAGER)
    {
        cbk = cm_cli_print_ack_item_task;
    }

    cm_cli_print_ack_item(pObj, cm_omi_obj_key_find(AckObj, CM_OMI_KEY_ITEMS), cbk);

    return CM_OK;
}

static void cm_cli_object_view_objects(void)
{
    const cm_omi_map_object_t * *pObj = CmOmiMap;
    uint32 iloop = 1;
    uint32 cnt = 0;
    pObj++;

    for(; iloop < CM_OMI_OBJECT_BUTT; iloop++, pObj++)
    {
        if(CM_OMI_OBJECT_SESSION == iloop)
        {
            continue;
        }

        if(CM_OK != cm_cli_help_check_object_permission(iloop))
        {
            continue;
        }

        if(cnt == 5)
        {
            printf("\n%-30s"," ");
            cnt = 0;
        }

        printf("%-20s ", (*pObj)->obj.pname);
        cnt++;
    }
    printf("\n");
    return;
}

static void cm_cli_object_view_levels(void)
{
    const cm_omi_map_cfg_t *cfg = CmOmiMapEnumAdminLevelType.value;
    uint32 iloop = 0;
    uint32 cnt = 0;

    for(; iloop < CmOmiMapEnumAdminLevelType.num; iloop++, cfg++)
    {
        if(CM_OMI_OBJECT_SESSION == iloop)
        {
            continue;
        }

        if(cnt == 5)
        {
            printf("\n%-30s"," ");
            cnt = 0;
        }

        printf("%-20s ", cfg->pname);
        cnt++;
    }
    printf("\n");
    return;
}


void cm_cli_object_view(const cm_omi_map_object_t* pObj)
{
    const cm_omi_map_object_field_t *field = pObj->fields;
    uint32 iloop = 0;
    uint32 jloop = 0;
    const sint8* dtype[] = {"string", "integer", "enum", "double", "time", "bits", "password", "enum", "enum", "enum"};

    printf("%-15s %-15s %-15s\n", (const sint8*)"name", (const sint8*)"type", (const sint8*)"value");

    while(iloop < pObj->field_num)
    {
        printf("%-15s %-15s ", field->field.pname, dtype[field->data.type]);

        switch(field->data.type)
        {
            case CM_OMI_DATA_STRING:
                if(NULL != field->data.check.pregex)
                {
                    printf("%s\n", field->data.check.pregex);
                }
                else
                {
                    printf("--\n");
                }

                break;

            case CM_OMI_DATA_ENUM:
                jloop = 0;

                while(jloop < field->data.check.penum->num)
                {
                    printf("%s ", field->data.check.penum->value[jloop].pname);
                    jloop++;
                }

                printf("\n");
                break;

            case CM_OMI_DATA_ENUM_OBJ:
                cm_cli_object_view_objects();
                break;

            case CM_OMI_DATA_BITS_LEVELS:
                cm_cli_object_view_levels();
                break;

            default:
                printf("--\n");
                break;
        }

        iloop++;
        field++;
    }

    return;
}

typedef sint32 (*cm_cli_ack_cbk_t)(const cm_omi_map_object_t* pObj, const cm_omi_map_object_cmd_t* pCmd, cm_omi_obj_t AckObj);

static sint8 g_cm_cli_session[CM_STRING_64] = {0};

sint32 cm_cli_check_object_in(sint8* FormatHead, uint32 Size, sint32 argc, sint8
                              **argv, cm_cli_ack_cbk_t cbk)
{
    sint32 iRet = CM_FAIL;
    const cm_omi_map_object_t* pObj = NULL;
    const cm_omi_map_object_cmd_t* pCmd = NULL;
    cm_omi_obj_t OmiObj = NULL;
    cm_omi_obj_t AckObj = NULL;

    if(argc < 1)
    {
        cm_cli_help(FormatHead);
        return CM_FAIL;
    }

    pObj = cm_cli_object_get(argv[0]);

    if(NULL == pObj)
    {
        cm_cli_help(FormatHead);
        return CM_FAIL;
    }

    iRet = cm_cli_help_check_object_permission(pObj->obj.code);

    if(CM_OK != iRet)
    {
        printf("%s no permission!\n",argv[0]);
        return iRet;
    }

    CM_VSPRINTF(FormatHead + strlen(FormatHead), Size - strlen(FormatHead), "%s", argv[0]);

    if(argc < 2)
    {
        cm_cli_help_object(FormatHead, pObj);
        return CM_FAIL;
    }

    if(0 == strcmp("view", argv[1]))
    {
        cm_cli_object_view(pObj);
        return CM_OK;
    }

    pCmd = cm_cli_object_get_cmd(pObj, argv[1]);

    if(NULL == pCmd)
    {
        cm_cli_help_object(FormatHead, pObj);
        return CM_FAIL;
    }

    iRet = cm_cli_help_check_cmd_permission(pObj->obj.code, pCmd->pcmd->code);

    if(CM_OK != iRet)
    {
        printf("%s %s no permission!\n",argv[0],argv[1]);
        return iRet;
    }

    CM_VSPRINTF(FormatHead + strlen(FormatHead), Size - strlen(FormatHead), " %s", argv[1]);

    OmiObj = cm_omi_obj_new();

    if(NULL == OmiObj)
    {
        CM_LOG_ERR(CM_MOD_NONE, "new obj fail");
        return CM_FAIL;
    }

    if((0 != g_cm_cli_session[0])
       && (CM_OK != cm_omi_obj_key_set_str(OmiObj, CM_OMI_KEY_SESSION, g_cm_cli_session)))
    {
        CM_LOG_ERR(CM_MOD_NONE, "set session fail");
        cm_omi_obj_delete(OmiObj);
        return CM_FAIL;
    }

    if(CM_OK != cm_omi_obj_key_set_u32(OmiObj, CM_OMI_KEY_OBJ, pObj->obj.code))
    {
        CM_LOG_ERR(CM_MOD_NONE, "set key fail");
        cm_omi_obj_delete(OmiObj);
        return CM_FAIL;
    }

    iRet = cm_cli_check_object_cmd(pObj, FormatHead, Size, argc - 2, &argv[2], pCmd, OmiObj);

    if(CM_OK == iRet)
    {
        //printf("Req:%s\n",cm_omi_obj_tostr(OmiObj));
#if(CM_CLI_NO_REQ != CM_ON)
        iRet = cm_omi_request(OmiObj, &AckObj, 30);
#endif
    }

    cm_omi_obj_delete(OmiObj);

    if(NULL != AckObj)
    {
        //printf("Ack:%s\n",cm_omi_obj_tostr(AckObj));
        iRet = cbk(pObj, pCmd, AckObj);
        cm_omi_obj_delete(AckObj);
    }

    if(CM_OK != iRet)
    {
        sint8 buff[CM_STRING_512] = {0};
        (void)cm_exec(buff, sizeof(buff) - 1, "grep 'id=\"%d\"' "CM_CLI_ERROR_RES_FILE
                      " |sed 's/.*desc=\"\\(.*\\)/\\1/g' |awk -F'\"' '{printf $1}'", iRet);
        printf("error[%d]:%s\n", iRet, buff);
    }

    return iRet;
}


sint32 cm_cli_check_object(sint8* FormatHead, uint32 Size, sint32 argc, sint8 **argv)
{
    return cm_cli_check_object_in(FormatHead, Size, argc, argv, cm_cli_print_ack);
}

sint32 cm_cli_init(void)
{
    const cm_base_config_t cfg=
    {
        CM_LOG_DIR \
        CM_LOG_FILE, CM_LOG_MODE,CM_LOG_LVL,

        CM_TRUE, CM_FALSE, CM_TRUE, CM_FALSE,

        0,NULL,
        
        CM_OMI_OBJECT_SESSION, CM_OMI_OBJECT_BUTT, 
        g_CmOmiObjCmdNoCheckPtr, CmOmiMap, NULL,

        NULL,NULL
    };
    
    sint32 iRet = cm_base_init(&cfg);

    if(CM_OK != iRet)
    {
        printf("cm_base_init fail\n");
        return CM_FAIL;
    }

    return CM_OK;
}

static sint32 cm_cli_login(const cm_omi_map_object_t* pObj, const cm_omi_map_object_cmd_t* pCmd, cm_omi_obj_t AckObj)
{
    sint32 iRetx = CM_FAIL;
    sint32 iRet = cm_omi_obj_key_get_s32(AckObj, CM_OMI_KEY_RESULT, &iRetx);
    cm_omi_obj_t AckItem = NULL;

    if(CM_OK != iRet)
    {
        return iRet;
    }

    if(CM_OK != iRetx)
    {
        return iRetx;
    }

    AckItem = cm_omi_obj_key_find(AckObj, CM_OMI_KEY_ITEMS);

    if(NULL == AckItem)
    {
        return CM_FAIL;
    }

    AckItem = cm_omi_obj_array_idx(AckItem, 0);

    if(NULL == AckItem)
    {
        return CM_FAIL;
    }

    iRet = cm_omi_obj_key_get_str_ex(AckItem, CM_OMI_FIELD_SESSION_ID,
                                     g_cm_cli_session,
                                     sizeof(g_cm_cli_session));

    if(CM_OK != iRet)
    {

        return iRet;
    }

    iRet = cm_omi_obj_key_get_u32_ex(AckItem, CM_OMI_FIELD_SESSION_LEVEL, &g_cm_cli_level);
    return iRet;
}

static sint32 cm_cli_get_permission_cnt(const cm_omi_map_object_t* pObj,
                                        const cm_omi_map_object_cmd_t* pCmd, cm_omi_obj_t AckObj)
{
    sint32 iRetx = CM_FAIL;
    sint32 iRet = cm_omi_obj_key_get_s32(AckObj, CM_OMI_KEY_RESULT, &iRetx);
    cm_omi_obj_t AckItem = NULL;

    if(CM_OK != iRet)
    {
        return iRet;
    }

    if(CM_OK != iRetx)
    {
        return iRetx;
    }

    AckItem = cm_omi_obj_key_find(AckObj, CM_OMI_KEY_ITEMS);

    if(NULL == AckItem)
    {
        return CM_FAIL;
    }

    AckItem = cm_omi_obj_array_idx(AckItem, 0);

    if(NULL == AckItem)
    {
        return CM_FAIL;
    }

    return cm_omi_obj_key_get_u32_ex(AckItem, CM_OMI_FIELD_COUNT, &g_cm_cli_permission_cnt);;
}

static sint32 cm_cli_get_permission(const cm_omi_map_object_t* pObj,
                                    const cm_omi_map_object_cmd_t* pCmd, cm_omi_obj_t AckObj)
{
    sint32 iRetx = CM_FAIL;
    sint32 iRet = cm_omi_obj_key_get_s32(AckObj, CM_OMI_KEY_RESULT, &iRetx);
    cm_omi_obj_t AckItems = NULL;
    cm_omi_obj_t AckItem = NULL;
    uint32 cnt = 0;
    uint32 iloop = 0;
    uint32 obj = 0;
    uint32 cmd = 0;
    cm_tree_node_t *pObjTree = NULL;

    if(CM_OK != iRet)
    {
        return iRet;
    }

    if(CM_OK != iRetx)
    {
        return iRetx;
    }

    AckItems = cm_omi_obj_key_find(AckObj, CM_OMI_KEY_ITEMS);

    if(NULL == AckItems)
    {
        return CM_FAIL;
    }

    cnt = cm_omi_obj_array_size(AckItems);

    for(iloop = 0; iloop < cnt; iloop++)
    {
        AckItem = cm_omi_obj_array_idx(AckItems, iloop);

        if(NULL == AckItem)
        {
            return CM_FAIL;
        }

        iRet = cm_omi_obj_key_get_u32_ex(AckItem, CM_OMI_FIELD_PERMISSION_OBJ, &obj);
        iRetx = cm_omi_obj_key_get_u32_ex(AckItem, CM_OMI_FIELD_PERMISSION_CMD, &cmd);

        if(CM_OK != (iRet + iRetx))
        {
            return CM_FAIL;
        }

        pObjTree = cm_tree_node_addnew(g_cm_cli_permission, obj);

        if(NULL != pObjTree)
        {
            cm_tree_node_addnew(pObjTree, cmd);
        }
    }

    return CM_OK;
}

sint32 cm_cli_check_user(const sint8* name, const sint8* pwd)
{
    sint8 buf[512] = {0};
    sint8 ip[CM_IP_LEN] = {0};
    sint8* argv[] = {"session", "get", name, pwd, ip};
    sint8 offset_str[32] = {0};
    sint8* argv_permission[] =
    {"permission", "getbatch", "-d", "obj,cmd", "-levels", NULL, "-t", "100", "-f", offset_str};
    sint8* argv_permission_cnt[] =
    {"permission", "count", "-levels", NULL};
    sint32 iRet = CM_OK;
    uint32 iloop = 0;

    if(CM_OK != cm_exec(ip, sizeof(ip), "who am i |sed -e 's/[()]//g' |awk '{printf $NF}'"))
    {
        return CM_FAIL;
    }

    iRet = cm_cli_check_object_in(buf, sizeof(buf), sizeof(argv) / sizeof(sint8*), argv, cm_cli_login);

    if(CM_OK != iRet)
    {
        return iRet;
    }

    if(0 == g_cm_cli_level)
    {
        return CM_OK;
    }

    if(g_cm_cli_level >= CmOmiMapEnumAdminLevelType.num)
    {
        printf("level [%u]\n", g_cm_cli_level);
        return CM_ERR_NOT_SUPPORT;
    }

    if(NULL == g_cm_cli_permission)
    {
        g_cm_cli_permission = cm_tree_node_new(0);

        if(NULL == g_cm_cli_permission)
        {
            printf("new tree fail\n");
            return CM_FAIL;
        }
    }

    argv_permission[5] = CmOmiMapEnumAdminLevelType.value[g_cm_cli_level].pname;
    argv_permission_cnt[3] = argv_permission[5];

    buf[0] = '\0';
    iRet = cm_cli_check_object_in(buf, sizeof(buf),
                                  sizeof(argv_permission_cnt) / sizeof(sint8*), argv_permission_cnt,
                                  cm_cli_get_permission_cnt);

    if(iRet != CM_OK)
    {
        printf("get cnt fail\n");
        return CM_FAIL;
    }
    iloop = 0;
    //printf("g_cm_cli_permission_cnt=%u\n",g_cm_cli_permission_cnt);
    while(iloop < g_cm_cli_permission_cnt)
    {
        buf[0] = '\0';
        CM_VSPRINTF(offset_str, sizeof(offset_str), "%u", iloop);
        iRet = cm_cli_check_object_in(buf, sizeof(buf),
                                      sizeof(argv_permission) / sizeof(sint8*), argv_permission,
                                      cm_cli_get_permission);

        if(iRet != CM_OK)
        {
            break;
        }

        iloop += 100;
    }

    return CM_OK;
}

sint32 cm_cli_logout(void)
{
    sint8 buf[512] = {0};
    sint8* argv[] = {"session", "delete", g_cm_cli_session};

    if('\0' != g_cm_cli_session)
    {
        return cm_cli_check_object_in(buf, sizeof(buf), 3, argv, cm_cli_print_ack);
    }

    if(NULL != g_cm_cli_permission)
    {
        cm_tree_destory(g_cm_cli_permission);
        g_cm_cli_permission = NULL;
    }

    return CM_OK;
}

