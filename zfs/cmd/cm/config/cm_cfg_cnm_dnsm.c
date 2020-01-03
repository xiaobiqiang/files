/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cfg_cnm_dnsm.c
 * author     : wbn
 * create date: 2017Äê11ÔÂ13ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_cfg_cnm.h"
     
const cm_omi_map_object_field_t CmOmiMapFieldsDnsm[] =
{
    {{"ip", CM_OMI_FIELD_DNSM_IP}, "-ip", {CM_OMI_DATA_STRING, CM_NAME_LEN, {IP_FORMAT}}},
    {{"nid", CM_OMI_FIELD_DNSM_NID}, "-nid",{CM_OMI_DATA_INT, 4, {NULL}}},
    {{"domain", CM_OMI_FIELD_DNSM_DOMAIN}, "-dom", {CM_OMI_DATA_STRING, 64, {"[0-9a-zA-Z_]{1,63}"}}},
    
};

const cm_omi_map_object_field_t* CmOmiMapDnsmCmdParamsGetBatch[]=
{
    &CmOmiMapFieldsDnsm[CM_OMI_FIELD_DNSM_NID],/*nid*/
    &CmOmiMapCommFields[0], /*fields*/
    &CmOmiMapCommFields[1], /*offset*/
    &CmOmiMapCommFields[2], /*total*/
};
const cm_omi_map_object_field_t* CmOmiMapDnsmCmdParamsCount[]=
{
    &CmOmiMapFieldsDnsm[CM_OMI_FIELD_DNSM_NID],/*nid*/   
};

const cm_omi_map_object_field_t* CmOmiMapDnsmCmdParamsDelete[]=
{
    &CmOmiMapFieldsDnsm[CM_OMI_FIELD_DNSM_DOMAIN],/*domain*/   
    &CmOmiMapFieldsDnsm[CM_OMI_FIELD_DNSM_NID],/*nid*/
};

const cm_omi_map_object_field_t* CmOmiMapDnsmCmdParamsAdd[]=
{
    
    &CmOmiMapFieldsDnsm[CM_OMI_FIELD_DNSM_IP], /*ip*/
    &CmOmiMapFieldsDnsm[CM_OMI_FIELD_DNSM_DOMAIN], /*domain*/
    &CmOmiMapFieldsDnsm[CM_OMI_FIELD_DNSM_NID],/*nid*/
};

const cm_omi_map_object_field_t* CmOmiMapDnsmCmdParamsModify[]=
{
    &CmOmiMapFieldsDnsm[CM_OMI_FIELD_DNSM_DOMAIN], /*domain*/
    &CmOmiMapFieldsDnsm[CM_OMI_FIELD_DNSM_IP], /*ip*/

    &CmOmiMapFieldsDnsm[CM_OMI_FIELD_DNSM_NID], /*nid*/
    
};
const cm_omi_map_object_cmd_t CmOmiMapDnsmCmds[] =
{
    /* getbatch */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET_BATCH],
        0,
        NULL,
        4,
        CmOmiMapDnsmCmdParamsGetBatch,
    },
    
    /* add */
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],        
        3,
        CmOmiMapDnsmCmdParamsAdd,
        0,
        NULL,
    },
    /* modify */
    {
        &CmOmiMapCmds[CM_OMI_CMD_MODIFY],        
        3,
        CmOmiMapDnsmCmdParamsModify,
        0,
        NULL,
    },
    /* count */
    {
        &CmOmiMapCmds[CM_OMI_CMD_COUNT],        
        0,
        NULL,
        1,
        CmOmiMapDnsmCmdParamsCount,
    },
    /* delete */
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],        
        2,
        CmOmiMapDnsmCmdParamsDelete,
        0,
        NULL,
    },
    /* get */
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],        
        2,
        CmOmiMapDnsmCmdParamsDelete,
        0,
        NULL,
    },
    
};

const cm_omi_map_object_t CmCnmDnsmCfg =
{
    {"dnsm", CM_OMI_OBJECT_DNSM},
    sizeof(CmOmiMapFieldsDnsm)/sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsDnsm,
    sizeof(CmOmiMapDnsmCmds)/sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDnsmCmds
}; 
const cm_omi_map_object_field_t CmOmiMapFieldsDns[] =
{
    {{"ip_master", CM_OMI_FIELD_DNS_IPMASTER}, "-ipm", {CM_OMI_DATA_STRING, CM_NAME_LEN, {IP_FORMAT}}},
    {{"ip_slave", CM_OMI_FIELD_DNS_IPSLAVE}, "-ips", {CM_OMI_DATA_STRING, CM_NAME_LEN, {IP_FORMAT}}},
};
const cm_omi_map_object_field_t* CmOmiMapDnsCmdParamsDelete[]=
{
    &CmOmiMapFieldsDns[CM_OMI_FIELD_DNS_IPMASTER],
    &CmOmiMapFieldsDns[CM_OMI_FIELD_DNS_IPSLAVE],       
};
const cm_omi_map_object_cmd_t CmOmiMapDnsCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        0,
        NULL,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        0,
        NULL,
        2,
        CmOmiMapDnsCmdParamsDelete,
    },  
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        0,
        NULL,
        2,
        CmOmiMapDnsCmdParamsDelete,
    },
};
const cm_omi_map_object_t CmCnmDnsCfg =
{
    {"dns", CM_OMI_OBJECT_DNS},
    sizeof(CmOmiMapFieldsDns) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsDns,
    sizeof(CmOmiMapDnsCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDnsCmds
};
const cm_omi_map_object_field_t CmOmiMapFieldsDomainAd[] =
{
    {{"domain", CM_OMI_FIELD_DOMAINAD_DOMIAN}, "-d", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"dc_name", CM_OMI_FIELD_DOMAINAD_DOMAINCTL}, "-dc", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"username", CM_OMI_FIELD_DOMAINAD_USERNAME}, "-n", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"userpwd", CM_OMI_FIELD_DOMAINAD_USERPWD}, "-pwd", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
    {{"state", CM_OMI_FIELD_DOMAINAD_STATE}, "-st", {CM_OMI_DATA_STRING, CM_STRING_64, {NULL}}},
};
const cm_omi_map_object_field_t* CmOmiMapDomainAdCmdParamsInsert[]=
{
    &CmOmiMapFieldsDomainAd[CM_OMI_FIELD_DOMAINAD_DOMIAN],
    &CmOmiMapFieldsDomainAd[CM_OMI_FIELD_DOMAINAD_DOMAINCTL], 
    &CmOmiMapFieldsDomainAd[CM_OMI_FIELD_DOMAINAD_USERNAME],
    &CmOmiMapFieldsDomainAd[CM_OMI_FIELD_DOMAINAD_USERPWD],  
};
const cm_omi_map_object_cmd_t CmOmiMapDomainAdCmds[] =
{
    {
        &CmOmiMapCmds[CM_OMI_CMD_GET],
        0,
        NULL,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_ADD],
        4,
        CmOmiMapDomainAdCmdParamsInsert,
        0,
        NULL,
    },
    {
        &CmOmiMapCmds[CM_OMI_CMD_DELETE],
        0,
        NULL,
        0,
        NULL,
    },
};
const cm_omi_map_object_t CmCnmDomainAdCfg =
{
    {"domain_ad", CM_OMI_OBJECT_DOMAIN_AD},
    sizeof(CmOmiMapFieldsDomainAd) / sizeof(cm_omi_map_object_field_t),
    CmOmiMapFieldsDomainAd,
    sizeof(CmOmiMapDomainAdCmds) / sizeof(cm_omi_map_object_cmd_t),
    CmOmiMapDomainAdCmds
};
