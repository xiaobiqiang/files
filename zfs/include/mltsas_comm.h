#ifndef __MLTSAS_COMM_H
#define __MLTSAS_COMM_H

#include <sys/sysmacros.h>
#include <linux/types.h>

#ifndef _KERNEL
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
#endif

#define MLSAS_NAME			"Mlsas_dev"
#define Mlsas_Ioc_Magic		0xdeafdeaf

typedef enum Mlsas_ioc {
	Mlsas_Ioc_First,
	Mlsas_Ioc_Ensvc,
	Mlsas_Ioc_Diesvc,
	Mlsas_Ioc_Newminor,
	Mlsas_Ioc_Attach,
	Mlsas_Ioc_Failoc,
	Mlsas_Ioc_Virtinfo,
	Mlsas_Ioc_Last
} Mlsas_ioc_e;

typedef enum Mlsas_devst {
	Mlsas_Devst_Standalone,
	Mlsas_Devst_Failed,
	Mlsas_Devst_Degraded,
	Mlsas_Devst_Attached,
	Mlsas_Devst_Healthy,
	Mlsas_Devst_Last
} Mlsas_devst_e;

typedef struct Mlsas_iocdt {
	uint32_t Mlioc_magic;
	uint32_t Mlioc_nibuf;
	uint32_t Mlioc_nobuf;
	uint32_t Mlioc_nofill;
	uint64_t Mlioc_ibufptr;
	uint64_t Mlioc_obufptr;
} Mlsas_iocdt_t;

typedef struct Mlsas_virtinfo_return {
	uint64_t 		vti_hashkey;
	char 			vti_scsi_protocol[32];
	Mlsas_devst_e	vti_devst;
	uint32_t 		vti_switch;
	uint32_t		vti_error_cnt;
	uint32_t 		vti_io_npending;
	uint32_t		vti_pad;
} Mlsas_virtinfo_return_t;

#endif
