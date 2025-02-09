#ifndef _ASM_POWERPC_PGALLOC_32_H
#define _ASM_POWERPC_PGALLOC_32_H

#include <fikus/threads.h>

/* For 32-bit, all levels of page tables are just drawn from get_free_page() */
#define MAX_PGTABLE_INDEX_SIZE	0

extern void __bad_pte(pmd_t *pmd);

extern pgd_t *pgd_alloc(struct mm_struct *mm);
extern void pgd_free(struct mm_struct *mm, pgd_t *pgd);

/*
 * We don't have any real pmd's, and this code never triggers because
 * the pgd will always be present..
 */
/* #define pmd_alloc_one(mm,address)       ({ BUG(); ((pmd_t *)2); }) */
#define pmd_free(mm, x) 		do { } while (0)
#define __pmd_free_tlb(tlb,x,a)		do { } while (0)
/* #define pgd_populate(mm, pmd, pte)      BUG() */

#ifndef CONFIG_BOOKE
#define pmd_populate_kernel(mm, pmd, pte)	\
		(pmd_val(*(pmd)) = __pa(pte) | _PMD_PRESENT)
#define pmd_populate(mm, pmd, pte)	\
		(pmd_val(*(pmd)) = (page_to_pfn(pte) << PAGE_SHIFT) | _PMD_PRESENT)
#define pmd_pgtable(pmd) pmd_page(pmd)
#else
#define pmd_populate_kernel(mm, pmd, pte)	\
		(pmd_val(*(pmd)) = (unsigned long)pte | _PMD_PRESENT)
#define pmd_populate(mm, pmd, pte)	\
		(pmd_val(*(pmd)) = (unsigned long)lowmem_page_address(pte) | _PMD_PRESENT)
#define pmd_pgtable(pmd) pmd_page(pmd)
#endif

extern pte_t *pte_alloc_one_kernel(struct mm_struct *mm, unsigned long addr);
extern pgtable_t pte_alloc_one(struct mm_struct *mm, unsigned long addr);

static inline void pte_free_kernel(struct mm_struct *mm, pte_t *pte)
{
	free_page((unsigned long)pte);
}

static inline void pte_free(struct mm_struct *mm, pgtable_t ptepage)
{
	pgtable_page_dtor(ptepage);
	__free_page(ptepage);
}

static inline void pgtable_free(void *table, unsigned index_size)
{
	BUG_ON(index_size); /* 32-bit doesn't use this */
	free_page((unsigned long)table);
}

#define check_pgt_cache()	do { } while (0)

#ifdef CONFIG_SMP
static inline void pgtable_free_tlb(struct mmu_gather *tlb,
				    void *table, int shift)
{
	unsigned long pgf = (unsigned long)table;
	BUG_ON(shift > MAX_PGTABLE_INDEX_SIZE);
	pgf |= shift;
	tlb_remove_table(tlb, (void *)pgf);
}

static inline void __tlb_remove_table(void *_table)
{
	void *table = (void *)((unsigned long)_table & ~MAX_PGTABLE_INDEX_SIZE);
	unsigned shift = (unsigned long)_table & MAX_PGTABLE_INDEX_SIZE;

	pgtable_free(table, shift);
}
#else
static inline void pgtable_free_tlb(struct mmu_gather *tlb,
				    void *table, int shift)
{
	pgtable_free(table, shift);
}
#endif

static inline void __pte_free_tlb(struct mmu_gather *tlb, pgtable_t table,
				  unsigned long address)
{
	tlb_flush_pgtable(tlb, address);
	pgtable_page_dtor(table);
	pgtable_free_tlb(tlb, page_address(table), 0);
}
#endif /* _ASM_POWERPC_PGALLOC_32_H */
