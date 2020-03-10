/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cli.h
 * author     : wbn
 * create date: 2017Äê11ÔÂ6ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef CLI_CM_CLI_H_
#define CLI_CM_CLI_H_

#include "cm_common.h"

extern sint32 cm_cli_init(void);

extern void cm_cli_help(const sint8* name);

extern sint32 cm_cli_check_user(const sint8* name,const sint8* pwd);

extern sint32 cm_cli_check_object(sint8* FormatHead, uint32 Size, sint32 argc, sint8 **argv);

extern sint32 cm_cli_logout(void);

#endif /* CLI_CM_CLI_H_ */
