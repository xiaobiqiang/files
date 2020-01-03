/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_phys.c
 * author     : wbn
 * create date: 2018年3月16日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

const cm_omi_map_cfg_t CmOmiMapPortStateType[] =
{
    {"unkown",CM_PORT_STATE_UNKNOW},
    {"up",CM_PORT_STATE_UP},
    {"down",CM_PORT_STATE_DOWN},
};

const cm_omi_map_enum_t CmOmiMapEnumPortStateType =
{
    sizeof(CmOmiMapPortStateType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapPortStateType
};

const cm_omi_map_cfg_t CmOmiMapPortDuplexType[] =
{
    {"unkown",CM_PORT_DUPLEX_UNKNOW},
    {"full",CM_PORT_DUPLEX_FULL},
    {"half",CM_PORT_DUPLEX_HALF},
};

const cm_omi_map_enum_t CmOmiMapEnumPortDuplexType =
{
    sizeof(CmOmiMapPortDuplexType)/sizeof(cm_omi_map_cfg_t),
    CmOmiMapPortDuplexType
};

const cm_omi_map_object_field_t CmOmiMapPhysFields[] =
{    
    {{"name", CM_OMI_FIELD_PHYS_NAME}, "-n",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_PHYS_NID}, "-nid",{CM_OMI_DATA_STRING, 32, {"[0-9]{1,20}"}}},
    {{"state", CM_OMI_FIELD_PHYS_STATE}, "-state",{CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPortStateType}}},
    {{"speed", CM_OMI_FIELD_PHYS_SPEED}, "-speed",{CM_OMI_DATA_STRING, CM_SN_LEN, {NULL}}},
    {{"deplex", CM_OMI_FIELD_PHYS_DUPLEX},"-deplex", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumPortDuplexType}}},
    {{"mtu", CM_OMI_FIELD_PHYS_MTU},"-mtu", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"mac", CM_OMI_FIELD_PHYS_MAC},"-mac", {CM_OMI_DATA_STRING, CM_SN_LEN, {NULL}}},
};

const cm_omi_map_object_field_t* CmOmiMapPhysCmdParamsGet[]=
{
    &CmOmiMapPhysFields[CM_OMI_FIELD_PHYS_NID], /*node id*/
    &CmOmiMapPhysFields[CM_OMI_FIELD_PHYS_NAME],
};

const cm_omi_map_object_field_t* CmOmiMapPhysCmdParamsGetBatchOpt[]=
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapCommFields[0], /*field*/
    &CmOmiMapPhysFields[CM_OMI_FIELD_PHYS_NID],
};

const cm_omi_map_object_field_t* CmOmiMapPhysCmdParamsUpdate[]=
{
    &CmOmiMapPhysFields[CM_OMI_FIELD_PHYS_NID], /*node id*/
    &CmOmiMapPhysFields[CM_OMI_FIELD_PHYS_NAME],
    &CmOmiMapPhysFields[CM_OMI_FIELD_PHYS_MTU]
};


const cm_omi_map_object_cmd_t CmOmiMapPhysCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        4,
        CmOmiMapPhysCmdParamsGetBatchOpt
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        2,
        CmOmiMapPhysCmdParamsGet,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        1,
        CmOmiMapPhysCmdParamsGet,
        0,
        NULL,
    },
     /* update */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        3,
        CmOmiMapPhysCmdParamsUpdate,
        0,
        NULL,
    },
};

const cm_omi_map_object_t CmCnmPhysCfg =
{
    {"phys", CM_OMI_OBJECT_PHYS},
    sizeof(CmOmiMapPhysFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPhysFields,
    sizeof(CmOmiMapPhysCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPhysCmds
};



/*                       route(路由管理)                  */

const cm_omi_map_object_field_t CmOmiMapRouteFields[] = 
{
    {{"destination", CM_OMI_FIELD_ROUTE_DESTINATION}, "-destination", {CM_OMI_DATA_STRING, CM_NAME_LEN, {IP_FORMAT}}},
    {{"nid", CM_OMI_FIELD_ROUTE_NID}, "-nid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"netmask", CM_OMI_FIELD_ROUTE_NETMASK}, "-netmask", {CM_OMI_DATA_STRING, CM_NAME_LEN, {IP_FORMAT}}},
    {{"gateway", CM_OMI_FIELD_ROUTE_GETEWAY}, "-geteway", {CM_OMI_DATA_STRING, CM_NAME_LEN, {IP_FORMAT}}},
};

const cm_omi_map_object_field_t* CmOmiMapRouteCmdParamsGetBatch[] =
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapRouteFields[CM_OMI_FIELD_ROUTE_NID],
};


const cm_omi_map_object_field_t* CmOmiMapRouteCmdParamsInsert[]=
{
    &CmOmiMapRouteFields[CM_OMI_FIELD_ROUTE_NID],
    &CmOmiMapRouteFields[CM_OMI_FIELD_ROUTE_DESTINATION],
    &CmOmiMapRouteFields[CM_OMI_FIELD_ROUTE_NETMASK],
    &CmOmiMapRouteFields[CM_OMI_FIELD_ROUTE_GETEWAY],
};


const cm_omi_map_object_cmd_t CmOmiMapRouteCmds[] =
{
    /*getbatch*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        3,
        CmOmiMapRouteCmdParamsGetBatch
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        1,
        CmOmiMapRouteCmdParamsInsert,
        0,
        NULL
    },
    /*insert*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        4,
        CmOmiMapRouteCmdParamsInsert,
        0,
        NULL
    },
    /*delete*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        4,
        CmOmiMapRouteCmdParamsInsert,
        0,
        NULL
    },
};



const cm_omi_map_object_t CmCnmRouteCfg =
{
    {"route", CM_OMI_OBJECT_ROUTE},
    sizeof(CmOmiMapRouteFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapRouteFields,
    sizeof(CmOmiMapRouteCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapRouteCmds
};


/****************************************************
                    phys_ip
*****************************************************/

const cm_omi_map_object_field_t CmOmiMapPhys_ipFields[] = 
{
    {{"adapter", CM_OMI_FIELD_PHYS_IP_ADAPTER}, "-adapter", {CM_OMI_DATA_STRING, CM_NAME_LEN, {"((([a-z0-9A-Z_-]+)[.:/]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"nid", CM_OMI_FIELD_PHYS_IP_NID}, "-nid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"ip", CM_OMI_FIELD_PHYS_IP_IP}, "-ip", {CM_OMI_DATA_STRING, CM_NAME_LEN, {IP_FORMAT}}},
    {{"netmask", CM_OMI_FIELD_PHYS_IP_NETMASK}, "-netmask", {CM_OMI_DATA_STRING, CM_NAME_LEN, {IP_FORMAT}}},
};

const cm_omi_map_object_field_t* CmOmiMapPhys_ipCmdParamsCount[] =
{
    &CmOmiMapPhys_ipFields[CM_OMI_FIELD_PHYS_IP_NID],
};

const cm_omi_map_object_field_t* CmOmiMapPhys_ipCmdParamsInsert[] = 
{
    &CmOmiMapPhys_ipFields[CM_OMI_FIELD_PHYS_IP_NID],
    &CmOmiMapPhys_ipFields[CM_OMI_FIELD_PHYS_IP_ADAPTER],
    &CmOmiMapPhys_ipFields[CM_OMI_FIELD_PHYS_IP_IP],
};

const cm_omi_map_object_field_t* CmOmiMapPhys_ipCmdParamsInsert_ext[] = 
{
    &CmOmiMapPhys_ipFields[CM_OMI_FIELD_PHYS_IP_NETMASK],
};

const cm_omi_map_object_field_t* CmOmiMapPhys_ipCmdParamsDelete[] = 
{
    &CmOmiMapPhys_ipFields[CM_OMI_FIELD_PHYS_IP_NID],
    &CmOmiMapPhys_ipFields[CM_OMI_FIELD_PHYS_IP_IP],
};

const cm_omi_map_object_field_t* CmOmiMapPhys_ipCmdParamsDelete_opt[] = 
{
    &CmOmiMapPhys_ipFields[CM_OMI_FIELD_PHYS_IP_ADAPTER],
};
const cm_omi_map_object_field_t* CmOmiMapPhys_ipCmdParamsGetBatch[] =
{
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
    &CmOmiMapPhys_ipFields[CM_OMI_FIELD_PHYS_IP_ADAPTER],
};


const cm_omi_map_object_cmd_t CmOmiMapPhys_ipCmds[] =
{   
    /*getbatch*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        1,
        CmOmiMapPhys_ipCmdParamsCount,
        3,
        CmOmiMapPhys_ipCmdParamsGetBatch
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        1,
        CmOmiMapPhys_ipCmdParamsCount,
        0,
        NULL
    },
    /*insert*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        3,
        CmOmiMapPhys_ipCmdParamsInsert,
        1,
        CmOmiMapPhys_ipCmdParamsInsert_ext
    },
    /*delete*/
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        2,
        CmOmiMapPhys_ipCmdParamsDelete,
        1,
        CmOmiMapPhys_ipCmdParamsDelete_opt
    },
};

const cm_omi_map_object_t CmCnmPhys_ipCfg =
{
    {"phys_ip", CM_OMI_OBJECT_PHYS_IP},
    sizeof(CmOmiMapPhys_ipFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapPhys_ipFields,
    sizeof(CmOmiMapPhys_ipCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapPhys_ipCmds
};


const cm_omi_map_object_field_t CmOmiMapFcinfoFields[] =
{    
    {{"port_wwn", CM_OMI_FIELD_FCINFO_PORT_WWN}, "-wwn",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"nid", CM_OMI_FIELD_FCINFO_NID}, "-nid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"port_mode", CM_OMI_FIELD_FCINFO_PORT_MODE}, "-mode",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"driver", CM_OMI_FIELD_FCINFO_DRIVER_NAME}, "-drn",{CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"state", CM_OMI_FIELD_FCINFO_STATE},"-state", {CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"speed", CM_OMI_FIELD_FCINFO_SPEED},"-speed", {CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"cur_speed", CM_OMI_FIELD_FCINFO_CURSPEED},"-cspeed", {CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
};

const cm_omi_map_object_field_t* CmOmiMapFcinfoCmdParamsGetBatch[]=
{
    &CmOmiMapPhysFields[CM_OMI_FIELD_PHYS_NID], /*node id*/
};
const cm_omi_map_object_cmd_t CmOmiMapFcinfoCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH], 
        0,
        NULL,
        1,
        CmOmiMapFcinfoCmdParamsGetBatch   
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT], 
        0,
        NULL,
        1,
        CmOmiMapFcinfoCmdParamsGetBatch   
    },
};
const cm_omi_map_object_t CmCnmFcinfoCfg =
{
    {"fcinfo", CM_OMI_OBJECT_FCINFO},
    sizeof(CmOmiMapFcinfoFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFcinfoFields,
    sizeof(CmOmiMapFcinfoCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapFcinfoCmds
};

/******************************************************************************
                         ipfilter
*******************************************************************************/
extern const cm_omi_map_enum_t CmOmiMapIpfStatusEnumBoolType;
extern const cm_omi_map_enum_t CmOmiMapIpfopEnumBoolType;
const cm_omi_map_object_field_t CmOmiMapIpfFields[] =
{    
    {{"operate", CM_OMI_FIELD_IPF_OPERATE}, "-ope", {CM_OMI_DATA_ENUM, 2, {&CmOmiMapIpfopEnumBoolType}}},
    {{"nid", CM_OMI_FIELD_IPF_NID}, "-nid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"nic", CM_OMI_FIELD_IPF_NIC}, "-nic",{CM_OMI_DATA_STRING, 32, {"[0-9a-zA-Z_]{1,63}"}}},
    {{"ip", CM_OMI_FIELD_IPF_IP}, "-ip",{CM_OMI_DATA_STRING, 32, {NULL}}},
    {{"port", CM_OMI_FIELD_IPF_PORT},"-port", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"status", CM_OMI_FIELD_IPF_STATE},"-status", {CM_OMI_DATA_ENUM, 2, {&CmOmiMapIpfStatusEnumBoolType}}},
};

const cm_omi_map_object_field_t* CmOmiMapIpfCmdParamsGetBatch[]=
{
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_NID], /*node id*/
};

const cm_omi_map_object_field_t* CmOmiMapIpfCmdParamsDelete[]=
{
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_OPERATE], 
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_NIC],
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_IP],
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_PORT],
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_NID],
};

const cm_omi_map_object_field_t* CmOmiMapIpfCmdParamsInsert[]=
{
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_OPERATE], 
};

const cm_omi_map_object_field_t* CmOmiMapIpfCmdParamsInsert_opt[]=
{
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_NIC],
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_IP],
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_PORT],
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_NID],
};

const cm_omi_map_object_field_t* CmOmiMapIpfCmdParamsUpdate[]=
{
    &CmOmiMapIpfFields[CM_OMI_FIELD_IPF_STATE], 
};
const cm_omi_map_object_cmd_t CmOmiMapIpfCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH], 
        0,
        NULL,
        1,
        CmOmiMapIpfCmdParamsGetBatch   
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT], 
        0,
        NULL,
        1,
        CmOmiMapIpfCmdParamsGetBatch   
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE], 
        5,
        CmOmiMapIpfCmdParamsDelete,
        0,
        NULL
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY], 
        1,
        CmOmiMapIpfCmdParamsUpdate,
        1,
        CmOmiMapIpfCmdParamsGetBatch  
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD], 
        1,
        CmOmiMapIpfCmdParamsInsert,
        4,
        CmOmiMapIpfCmdParamsInsert_opt
    },
};
const cm_omi_map_object_t CmCnmIpfCfg =
{
    {"ipfilter", CM_OMI_OBJECT_IPF},
    sizeof(CmOmiMapIpfFields)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapIpfFields,
    sizeof(CmOmiMapIpfCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapIpfCmds
};
