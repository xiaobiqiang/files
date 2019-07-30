#ifndef _ISCSIT_VMEM_H
#define _ISCSIT_VMEM_H

#include <linux/types.h>

#define VM_NAME_LEN		64

enum vm_flag {
	__VM_FLAG_ST,

	/* must set now when creating */
	__VM_IDENTIFIER,	

	__VM_VMALLOCED,
	__VM_INITIALIZED,
	__VM_DESTROYING,
	/*
	 * set when allocating
	 */
	__VM_SLEEP,			/*  */
	__VM_NOSLEEP,
	
	__VM_FLAG_END
};

#define VM_IDENTIFIER 	(1 << __VM_IDENTIFIER)
#define VM_VMALLOCED	(1 << __VM_VMALLOCED)
#define VM_INITIALIZED 	(1 << __VM_INITIALIZED)
#define VM_DESTROYING	(1 << __VM_DESTROYING)
#define VM_SLEEP		(1 << __VM_SLEEP)
#define VM_NOSLEEP		(1 << __VM_NOSLEEP)

//OOP
struct vmem;
typedef struct vmem vmem_t;

typedef void *(vmem_alloc_t)(vmem_t *, size_t, int);
typedef void (vmem_free_t)(vmem_t *, void *, size_t);

extern vmem_t *
vmem_create(const char *name, void *base, size_t size, size_t quantum,
    vmem_alloc_t *afunc, vmem_free_t *ffunc, vmem_t *source,
    size_t qcache_max, int vmflag);

extern void
vmem_destroy(vmem_t *vmp);

extern void *
vmem_alloc(vmem_t *vmp, size_t size, int vmflag);

extern void
vmem_free(vmem_t *vmp, void *vaddr, size_t size);

#endif 	/* _ISCSIT_VMEM_H */