/*
 * xtensa mmu stuff
 *
 * Extracted from init.c
 */
#include <fikus/percpu.h>
#include <fikus/init.h>
#include <fikus/string.h>
#include <fikus/slab.h>
#include <fikus/cache.h>

#include <asm/tlb.h>
#include <asm/tlbflush.h>
#include <asm/mmu_context.h>
#include <asm/page.h>

void __init paging_init(void)
{
	memset(swapper_pg_dir, 0, PAGE_SIZE);
}

/*
 * Flush the mmu and reset associated register to default values.
 */
void __init init_mmu(void)
{
#if !(XCHAL_HAVE_PTP_MMU && XCHAL_HAVE_SPANNING_WAY)
	/*
	 * Writing zeros to the instruction and data TLBCFG special
	 * registers ensure that valid values exist in the register.
	 *
	 * For existing PGSZID<w> fields, zero selects the first element
	 * of the page-size array.  For nonexistent PGSZID<w> fields,
	 * zero is the best value to write.  Also, when changing PGSZID<w>
	 * fields, the corresponding TLB must be flushed.
	 */
	set_itlbcfg_register(0);
	set_dtlbcfg_register(0);
#endif
	flush_tlb_all();

	/* Set rasid register to a known value. */

	set_rasid_register(ASID_INSERT(ASID_USER_FIRST));

	/* Set PTEVADDR special register to the start of the page
	 * table, which is in kernel mappable space (ie. not
	 * statically mapped).  This register's value is undefined on
	 * reset.
	 */
	set_ptevaddr_register(PGTABLE_START);
}

struct kmem_cache *pgtable_cache __read_mostly;

static void pgd_ctor(void *addr)
{
	pte_t *ptep = (pte_t *)addr;
	int i;

	for (i = 0; i < 1024; i++, ptep++)
		pte_clear(NULL, 0, ptep);

}

void __init pgtable_cache_init(void)
{
	pgtable_cache = kmem_cache_create("pgd",
			PAGE_SIZE, PAGE_SIZE,
			SLAB_HWCACHE_ALIGN,
			pgd_ctor);
}
