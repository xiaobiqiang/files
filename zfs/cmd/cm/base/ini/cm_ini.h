/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_ini.h
 * author     : wbn
 * create date: 2017��11��7��
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_INI_CM_INI_H_
#define BASE_INI_CM_INI_H_
#include "cm_common.h"

/******************************************************************************
ini file format:

[section1]
key1 = val1
key2 = val2

[section2]
key11 = val11
key12 = val12

******************************************************************************/

typedef void* cm_ini_handle_t;

/******************************************************************************
 * function     : cm_ini_open
 * description  : ��ini�ļ������ؽ����Ľṹ���
 *
 * parameter in : const sint8 *filename
 *   
 * parameter out: ��
 *
 * return type  : cm_ini_handle_t
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern cm_ini_handle_t cm_ini_open(const sint8 *filename);

/******************************************************************************
 * function     : cm_ini_new
 * description  : ������һ�����
 *
 * parameter in : ��
 *      
 * parameter out: ��
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern cm_ini_handle_t cm_ini_new(void);

/******************************************************************************
 * function     : cm_ini_save
 * description  : ���浽�ļ�
 *
 * parameter in : cm_ini_handle_t handle
 *                const sint8 *filename
 * parameter out: ��
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern sint32 cm_ini_save(cm_ini_handle_t handle, const sint8 *filename);

/******************************************************************************
 * function     : cm_ini_free
 * description  : �ͷ���Դ
 *
 * parameter in : cm_ini_handle_t handle
 *                
 * parameter out: ��
 *
 * return type  : void
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern void cm_ini_free(cm_ini_handle_t handle);

/******************************************************************************
 * function     : cm_ini_set
 * description  : �޸�����(������ʱ���)
 *
 * parameter in : cm_ini_handle_t handle
 *                const sint8 *section
 *                const sint8 *key
 *                const sint8 *val
 *
 * parameter out: ��
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern sint32 cm_ini_set(cm_ini_handle_t handle, 
    const sint8 *section, const sint8 *key, const sint8 *val);

/******************************************************************************
 * function     : cm_ini_get
 * description  : ��ѯ����
 *
 * parameter in : cm_ini_handle_t handle
 *                const sint8 *section
 *                const sint8 *key
 * parameter out: ��
 *
 * return type  : const sint8*
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern const sint8* cm_ini_get(cm_ini_handle_t handle, 
    const sint8 *section, const sint8 *key);

/******************************************************************************
 * function     : cm_ini_delete_section
 * description  : ɾ�����ö�
 *
 * parameter in : cm_ini_handle_t handle,
 *                const sint8 *section
 * parameter out: ��
 *
 * return type  : viod
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern void cm_ini_delete_section(cm_ini_handle_t handle, 
    const sint8 *section);

/******************************************************************************
 * function     : cm_ini_delete_key
 * description  : ɾ�����µ�����
 *
 * parameter in : cm_ini_handle_t handle
 *                const sint8 *section
 *                const sint8 *key
 * parameter out: ��
 *
 * return type  : void
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern void cm_ini_delete_key(cm_ini_handle_t handle, 
    const sint8 *section, const sint8 *key);

/******************************************************************************
 * function     : cm_ini_set_ext
 * description  : �޸��ļ�������
 *
 * parameter in : const sint8 *filename
 *                const sint8 *section
 *                const sint8 *key
 *                const sint8 *val
 * parameter out: ��
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern sint32 cm_ini_set_ext(const sint8 *filename,
    const sint8 *section, const sint8 *key, const sint8 *val);

/******************************************************************************
 * function     : cm_ini_get_ext
 * description  : ��ȡ�ļ�������
 *
 * parameter in : const sint8 *filename
 *                const sint8 *section
 *                const sint8 *key
 *                sint32 len
 * parameter out: sint8 *val
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern sint32 cm_ini_get_ext(const sint8 *filename, 
    const sint8 *section, const sint8 *key, sint8 *val, sint32 len);

/******************************************************************************
 * function     : cm_ini_set_ext_uint32
 * description  : �����ļ�������(uint32��ʽ)
 *
 * parameter in : const sint8 *filename
 *                const sint8 *section
 *                const sint8 *key
 *                uint32 val
 * parameter out: ��
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern sint32 cm_ini_set_ext_uint32(const sint8 *filename,
    const sint8 *section, const sint8 *key, uint32 val);

/******************************************************************************
 * function     : cm_ini_set_ext_uint32
 * description  : ��ȡ�ļ�������(uint32��ʽ)
 *
 * parameter in : const sint8 *filename
 *                const sint8 *section
 *                const sint8 *key
 *                
 * parameter out: uint32 *val
 *
 * return type  : sint32
 *
 * author       : wbn
 * create date  : 2017.11.8
 *****************************************************************************/
extern sint32 cm_ini_get_ext_uint32(const sint8 *filename, 
    const sint8 *section, const sint8 *key, uint32 *val);    


extern sint32 cm_ini_delete_key_ext(const sint8 *filename, 
    const sint8 *section, const sint8 *key);

#endif /* BASE_INI_CM_INI_H_ */
