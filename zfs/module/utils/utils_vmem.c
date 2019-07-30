#include <sys/utils_vmem.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <linux/gfp.h>
#include <sys/atomic.h>
#include <linux/bitops.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>

#define BITS_PER_PAGE		(1UL << (PAGE_SHIFT + 3))
#define BITS_PER_PAGE_MASK	(BITS_PER_PAGE - 1)
#define ROUNDUP(addr, aligned) ((addr + ((aligned)-1)) & ~((aligned)-1))
#define VM_END_BITS			(-1UL)
/*
 * scale: [base, top] or base:quantum:top
 */
struct vmem {
	char name[VM_NAME_LEN];
	kmutex_t mtx;
	kcondvar_t cv;
	ulong_t base;
	ulong_t bits;
	ulong_t top;
	ulong_t quantum;
	uint_t vmflag;
	uint_t mpcnt;
	struct page **bitmp;

	/* protect by atomic */
	uint_t refcnt;
};

static int
vmem_alloc_bm_pages(vmem_t *vm, uint want, struct page ***pgpp)
{
	int i, j, rc= -ENOMEM;
	boolean_t vmalloced = B_FALSE;
	struct page *pg;
	struct page **pages;
	void *addr;
	
	pages = kzalloc(want * sizeof(struct page *),
		GFP_NOIO | __GFP_NOWARN);
	if (!pages) {
		pages = __vmalloc(want * sizeof(struct page *),
				GFP_NOIO | __GFP_HIGHMEM | __GFP_ZERO,
				PAGE_KERNEL);
		if (!pages)
			goto failed;
		vmalloced = 1;
	}

	for (i=0; i<want; i++) {
		pg = alloc_page(GFP_NOIO | __GFP_HIGHMEM);
		if (!pg) 
			goto free_pg;
		addr = kmap_atomic(pg);
		bzero(addr, PAGE_SIZE);
		kunmap_atomic(addr);
		pages[i] = pg;
	}

	if (vmalloced)
		vm->vmflag |= VM_VMALLOCED;
	else
		vm->vmflag &= ~VM_VMALLOCED;
	*pgpp = pages;
	return 0;
	
free_pg:
	for (j=0; j<i; j++) {
		if (pages[j])
			__free_page(pages[j]);
	}
	if (vmalloced)
                vfree(pages);
        else
                kfree(pages);
failed:
	return rc;
}
static int 
vmem_init_single(vmem_t *vm, const char *name, 
	void *base, size_t size, size_t quantum, int vmflag)
{
	int rc;
	
	mutex_init(&vm->mtx, NULL, MUTEX_DRIVER, NULL);
	cv_init(&vm->cv, NULL, CV_DRIVER, NULL);
	atomic_inc_32(&vm->refcnt);
	(void) snprintf(vm->name, VM_NAME_LEN, "%s", name);
	vm->name[VM_NAME_LEN-1] = '\0';
	vm->base = base;
	vm->top = base + (size-1)*quantum;
	vm->quantum = quantum;
	vm->vmflag = vmflag;
	vm->bits = size;
	
	vm->mpcnt = (size+BITS_PER_PAGE_MASK)/BITS_PER_PAGE;
	if ((rc = vmem_alloc_bm_pages(vm, vm->mpcnt, &vm->bitmp)) != 0)
		goto failed;

	vm->vmflag |= VM_INITIALIZED;
	return 0;

failed:
	atomic_dec_32(&vm->refcnt);
	mutex_destroy(&vm->mtx);
	return rc;
}
static int
vmem_create_common(vmem_t *vm, const char *name, 
	void *base, size_t size, size_t quantum, int vmflag)
{
	return vmem_init_single(
		vm, name, base, size, quantum, vmflag);
}

vmem_t *
vmem_create(const char *name, void *base, size_t size, size_t quantum,
	vmem_alloc_t *afunc, vmem_free_t *ffunc, vmem_t *source,
	size_t qcache_max, int vmflag)
{
	vmem_t *vm = NULL;
	
	if (strlen(name) > (VM_NAME_LEN-1))
		goto failed_out;

	if (!(vmflag & VM_IDENTIFIER))
		goto failed_out;
	
	if ((vm = kzalloc(sizeof(vmem_t), GFP_KERNEL)) == NULL)
		goto failed_out;

	if (vmem_create_common(vm, name, base, size, quantum, vmflag))
		goto failed_create;

	return vm;

failed_create:
	if (vm)
		kfree(vm);
failed_out:
	return NULL;
}
EXPORT_SYMBOL(vmem_create);

/*
 * must held vmp->mtx.
 */
static void
__vmem_destroy_locked(vmem_t *vmp)
{
	int i;

	mutex_enter(&vmp->mtx);
	if (vmp->mpcnt) {
		for (i=0; i<vmp->mpcnt; i++) {
			if(vmp->bitmp[i])
				__free_page(vmp->bitmp[i]);
		}
	}

	if (vmp->vmflag & VM_VMALLOCED)
		vfree(vmp->bitmp);
	else
		kfree(vmp->bitmp);

	mutex_exit(&vmp->mtx);
	mutex_destroy(&vmp->mtx);
	cv_destroy(&vmp->cv);
	kfree(vmp);
}

void
vmem_destroy(vmem_t *vm)
{
	atomic_dec_32_nv(&vm->refcnt);	
	mutex_enter(&vm->mtx);
	vm->vmflag |= VM_DESTROYING;
	// broadcast all sleeping task for allocing.
	cv_broadcast(&vm->cv);
	mutex_exit(&vm->mtx);

	__vmem_destroy_locked(vm);
}
EXPORT_SYMBOL(vmem_destroy);

static ulong_t 
__vmem_find_next_zero(struct page *pg, ulong_t off)
{
	ulong_t rv;
	void *pg_addr = NULL;
	uint_t 		pg_ulong_off;
	int 		ulong_bit_off;
	
	pg_addr = kmap_atomic(pg);
	rv = find_next_zero_bit_le(pg_addr, BITS_PER_PAGE, off);
	if (rv < BITS_PER_PAGE) {
		pg_ulong_off = rv / (sizeof(ulong_t) * 8);
		ulong_bit_off = rv & (sizeof(ulong_t) * 8 - 1);
		test_and_set_bit_le(ulong_bit_off, pg_addr+pg_ulong_off);
	}
	kunmap_atomic(pg_addr);

	return (rv >= BITS_PER_PAGE ? VM_END_BITS : rv);
}

static void *
__vmem_alloc_unlocked(vmem_t *vmp, uint_t bits, uint_t flags)
{
	uint_t i;
	ulong_t zero_idx;
	void *rv = NULL;
	
	VERIFY(bits == 1);

	for (i=0; i<vmp->mpcnt; i++) {
		if (vmp->bitmp[i]) {
			zero_idx = __vmem_find_next_zero(
				vmp->bitmp[i], 0);
			if (zero_idx == VM_END_BITS)
				continue;

			if ((i < vmp->mpcnt-1) ||
				(zero_idx + i*BITS_PER_PAGE) <=
				(vmp->bits - 1))
				goto found;

			goto out;
		}
	}	
	goto out;
	
found:
	rv = (void *)(uintptr_t)(vmp->base + 
		(i*BITS_PER_PAGE+zero_idx)*vmp->quantum);
out:
	return rv;
}


void *
vmem_alloc(vmem_t *vmp, size_t size, int vmflag)
{
	void *rv = NULL;
	uint_t bits = 0;
	
	mutex_enter(&vmp->mtx);
	if (vmp->vmflag & VM_DESTROYING|| 
		size != vmp->quantum) {
		mutex_exit(&vmp->mtx);
		goto out;
	}

	bits = ROUNDUP(size, vmp->quantum) / vmp->quantum;

	rv = __vmem_alloc_unlocked(vmp, bits, vmflag);
	mutex_exit(&vmp->mtx);
out:
	return rv;
}
EXPORT_SYMBOL(vmem_alloc);

static int
__vmem_set_n_zero_bit(vmem_t *vmp, ulong_t n)
{
	uint_t 		pg_idx = n >> (PAGE_SHIFT + 3);
	struct page *pg = vmp->bitmp[pg_idx];
	uint_t 		pg_off = n & BITS_PER_PAGE_MASK;
	uint_t 		pg_ulong_off = pg_off / (sizeof(ulong_t) * 8);
	int 		ulong_bit_off = pg_off & (sizeof(ulong_t) * 8 - 1);
	ulong_t 	*pg_addr, ulong_off_addr;
	
	if (!pg)
		return -EINVAL;

	pg_addr = kmap_atomic(pg);
	ulong_off_addr = pg_addr + pg_ulong_off;
	test_and_clear_bit_le(ulong_bit_off, ulong_off_addr);
	kunmap_atomic(pg_addr);
	return 0;
}


void
vmem_free(vmem_t *vmp, void *vaddr, size_t size)
{
	ulong_t bit, addr = (uintptr_t)vaddr;
	 
	mutex_enter(&vmp->mtx);
	if (vaddr < vmp->base || vaddr > vmp->top ||
		size != vmp->quantum) {
		mutex_exit(&vmp->mtx);
		return ;
	}

	bit = (addr - vmp->base) / vmp->quantum;

	(void) __vmem_set_n_zero_bit(vmp, bit);
	mutex_exit(&vmp->mtx);
}
EXPORT_SYMBOL(vmem_free);

