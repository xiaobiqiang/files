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
	Mlsas_Ioc_Aggr_Virt,
	Mlsas_Ioc_Last
} Mlsas_ioc_e;

typedef struct Mlsas_iocdt {
	uint32_t Mlioc_magic;
	uint32_t Mlioc_nibuf;
	uint32_t Mlioc_nobuf;
	uint32_t Mlioc_nofill;
	uint64_t Mlioc_ibufptr;
	uint64_t Mlioc_obufptr;
} Mlsas_iocdt_t;

#endif
