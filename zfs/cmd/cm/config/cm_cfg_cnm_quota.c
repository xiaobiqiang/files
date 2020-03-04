/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_quota.c
 * author     : xar
 * create date: 2018.08.24
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"

extern const cm_omi_map_enum_t CmOmiMapCifsNameTypeCfg;

const cm_omi_map_object_field_t CmOmiMapFieldsQuota[] =
{
    {{"usertype", CM_OMI_FIELD_QUOTA_USERTYPE}, "-usertype", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapCifsNameTypeCfg}}},
    {{"nid", CM_OMI_FIELD_QUOTA_NID}, "-nid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"name", CM_OMI_FIELD_QUOTA_NAME}, "-name", {CM_OMI_DATA_STRING, CM_NAME_LEN, {"[0-9a-zA-Z_]{1,64}"}}},
    {{"filesystem", CM_OMI_FIELD_QUOTA_FILESYSTEM}, "-filesystem", {CM_OMI_DATA_STRING, 128, {"((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"hardspace", CM_OMI_FIELD_QUOTA_HARDSPACE}, "-hardspace", {CM_OMI_DATA_STRING, CM_STRING_32, {"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGPkmtgp]{1}"}}},
    {{"softspace", CM_OMI_FIELD_QUOTA_SOFTSPACE}, "-softspace", {CM_OMI_DATA_STRING, CM_STRING_32, {"[0-9]{1,10}[.]{0,1}[0-9]{0,3}[KMTGPkmtgp]{1}"}}},
    {{"used", CM_OMI_FIELD_QUOTA_USED}, "-used", {CM_OMI_DATA_STRING, CM_STRING_32, {"[0-9a-zA-Z_]{1,32}"}}},
    {{"domain", CM_OMI_FIELD_QUOTA_DOMAIN}, "-domain", {CM_OMI_DATA_ENUM, 4, {&CmOmiMapEnumDomainType}}},
};

const cm_omi_map_object_field_t* CmOmiMapQuotaCmdParamsGetBatch[] =
{    
    &CmOmiMapFieldsQuota[CM_OMI_FIELD_QUOTA_DOMAIN],
    &CmOmiMapFieldsQuota[CM_OMI_FIELD_QUOTA_USERTYPE],
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};


const cm_omi_map_object_field_t* CmOmiMapQuotaCmdParamsGet[] =
{
    &CmOmiMapFieldsQuota[CM_OMI_FIELD_QUOTA_NID],
    &CmOmiMapFieldsQuota[CM_OMI_FIELD_QUOTA_FILESYSTEM],
    &CmOmiMapFieldsQuota[CM_OMI_FIELD_QUOTA_DOMAIN],
    &CmOmiMapFieldsQuota[CM_OMI_FIELD_QUOTA_USERTYPE],
    &CmOmiMapFieldsQuota[CM_OMI_FIELD_QUOTA_NAME], 
};

const cm_omi_map_object_field_t* CmOmiMapQuotaCmdParamsModify[] =
{
    &CmOmiMapFieldsQuota[CM_OMI_FIELD_QUOTA_HARDSPACE],
    &CmOmiMapFieldsQuota[CM_OMI_FIELD_QUOTA_SOFTSPACE],
};

const cm_omi_map_object_cmd_t CmOmiMapQuotaCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        2,
        CmOmiMapQuotaCmdParamsGet,
        4,
        CmOmiMapQuotaCmdParamsGetBatch,
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        5,
        CmOmiMapQuotaCmdParamsGet,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        2,
        CmOmiMapQuotaCmdParamsGet,
        2,
        CmOmiMapQuotaCmdParamsGetBatch,
    },
    /* update */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],
        5,
        CmOmiMapQuotaCmdParamsGet,
        2,
        CmOmiMapQuotaCmdParamsModify
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        5,
        CmOmiMapQuotaCmdParamsGet,
        0,
        NULL,
    }
};



const cm_omi_map_object_t CmCnmQuotaCfg =
{
    {"quota", CM_OMI_OBJECT_QUOTA},
    sizeof(CmOmiMapFieldsQuota) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsQuota,
    sizeof(CmOmiMapQuotaCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapQuotaCmds
};

/*===================================================================
                    ipshift(ip∆Ø“∆)
=====================================================================*/

const cm_omi_map_object_field_t CmOmiMapFieldsIpshift[] =
{
    {{"filesystem", CM_OMI_FIELD_IPSHIFT_FILESYSTEM}, "-filesystem", {CM_OMI_DATA_STRING, 128, {"((([a-z0-9A-Z_-]+)[./]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"nid", CM_OMI_FIELD_IPSHIFT_NID}, "-nid", {CM_OMI_DATA_INT, 4, {NULL}}},
    {{"adapter", CM_OMI_FIELD_IPSHIFT_ADAPTER}, "-adapter", {CM_OMI_DATA_STRING, CM_NAME_LEN, {"((([a-z0-9A-Z_-]+)[.:/]{0,1})+)[a-z0-9A-Z_-]$"}}},
    {{"ip", CM_OMI_FIELD_IPSHIFT_IP}, "-ip", {CM_OMI_DATA_STRING, CM_NAME_LEN, {IP_FORMAT}}},
    {{"netmask", CM_OMI_FIELD_IPSHIFT_NETMASK}, "-netmask", {CM_OMI_DATA_STRING, CM_NAME_LEN, {IP_FORMAT}}},
};

const cm_omi_map_object_field_t* CmOmiMapIpshiftCmdParamsGet[] =
{
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_NID],
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_FILESYSTEM],
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_ADAPTER],
};


const cm_omi_map_object_field_t* CmOmiMapIpshiftCmdParamsInsert[] =
{
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_NID],
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_FILESYSTEM],
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_ADAPTER],
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_IP],
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_NETMASK],
};

const cm_omi_map_object_field_t* CmOmiMapIpshiftCmdParamsDelete[] =
{
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_NID],
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_FILESYSTEM],
    &CmOmiMapFieldsIpshift[CM_OMI_FIELD_IPSHIFT_IP],
};

const cm_omi_map_object_cmd_t CmOmiMapIpshiftCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        2,
        CmOmiMapIpshiftCmdParamsGet,
        2,
        CmOmiMapQuotaCmdParamsGetBatch,
    },
    /* get
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        3,
        CmOmiMapIpshiftCmdParamsGet,
        0,
        NULL,
    },*/
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],
        2,
        CmOmiMapIpshiftCmdParamsGet,
        0,
        NULL,
    },
    /* insert */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        5,
        CmOmiMapIpshiftCmdParamsInsert,
        0,
        NULL,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        3,
        CmOmiMapIpshiftCmdParamsDelete,
        0,
        NULL,
    }
};

const cm_omi_map_object_t CmCnmIpshiftCfg =
{
    {"ipshift", CM_OMI_OBJECT_IPSHFIT},
    sizeof(CmOmiMapFieldsIpshift) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsIpshift,
    sizeof(CmOmiMapIpshiftCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapIpshiftCmds
};


