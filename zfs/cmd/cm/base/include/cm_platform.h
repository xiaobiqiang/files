/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_platform.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_INCLUDE_CM_PLATFORM_H_
#define BASE_INCLUDE_CM_PLATFORM_H_

#include <stdio.h>
#include <stdlib.h>

typedef unsigned char uint8;
typedef signed char sint8;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned long long uint64;
typedef signed long long sint64;

typedef uint8 bool_t;

typedef long cm_time_t;


#ifndef NULL
#define NULL (void*)0
#endif


#define CM_TRUE 1
#define CM_FALSE 0

#endif /* BASE_INCLUDE_CM_PLATFORM_H_ */
