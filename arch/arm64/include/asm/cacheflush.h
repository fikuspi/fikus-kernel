/*
 * Based on arch/arm/include/asm/cacheflush.h
 *
 * Copyright (C) 1999-2002 Russell King.
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_CACHEFLUSH_H
#define __ASM_CACHEFLUSH_H

#include <fikus/mm.h>

/*
 * This flag is used to indicate that the page pointed to by a pte is clean
 * and does not require cleaning before returning it to the user.
 */
#define PG_dcache_clean PG_arch_1

/*
 *	MM Cache Management
 *	===================
 *
 *	The arch/arm64/mm/cache.S implements these methods.
 *
 *	Start addresses are inclusive and end addresses are exclusive; start
 *	addresses should be rounded down, end addresses up.
 *
 *	See Documentation/cachetlb.txt for more information. Please note that
 *	the implementation assumes non-aliasing VIPT D-cache and (aliasing)
 *	VIPT or ASID-tagged VIVT I-cache.
 *
 *	flush_cache_all()
 *
 *		Unconditionally clean and invalidate the entire cache.
 *
 *	flush_cache_mm(mm)
 *
 *		Clean and invalidate all user space cache entries
 *		before a change of page tables.
 *
 *	flush_icache_range(start, end)
 *
 *		Ensure coherency between the I-cache and the D-cache in the
 *		region described by start, end.
 *		- start  - virtual start address
 *		- end    - virtual end address
 *
 *	__flush_cache_user_range(start, end)
 *
 *		Ensure coherency between the I-cache and the D-cache in the
 *		region described by start, end.
 *		- start  - virtual start address
 *		- end    - virtual end address
 *
 *	__flush_dcache_area(kaddr, size)
 *
 *		Ensure that the data held in page is written back.
 *		- kaddr  - page address
 *		- size   - region size
 */
extern void flush_cache_all(void);
extern void flush_cache_range(struct vm_area_struct *vma, unsigned long start, unsigned long end);
extern void flush_icache_range(unsigned long start, unsigned long end);
extern void __flush_dcache_area(void *addr, size_t len);
extern void __flush_cache_user_range(unsigned long start, unsigned long end);

static inline void flush_cache_mm(struct mm_struct *mm)
{
}

static inline void flush_cache_page(struct vm_area_struct *vma,
				    unsigned long user_addr, unsigned long pfn)
{
}

/*
 * Copy user data from/to a page which is mapped into a different
 * processes address space.  Really, we want to allow our "user
 * space" model to handle this.
 */
extern void copy_to_user_page(struct vm_area_struct *, struct page *,
	unsigned long, void *, const void *, unsigned long);
#define copy_from_user_page(vma, page, vaddr, dst, src, len) \
	do {							\
		memcpy(dst, src, len);				\
	} while (0)

#define flush_cache_dup_mm(mm) flush_cache_mm(mm)

/*
 * flush_dcache_page is used when the kernel has written to the page
 * cache page at virtual address page->virtual.
 *
 * If this page isn't mapped (ie, page_mapping == NULL), or it might
 * have userspace mappings, then we _must_ always clean + invalidate
 * the dcache entries associated with the kernel mapping.
 *
 * Otherwise we can defer the operation, and clean the cache when we are
 * about to change to user space.  This is the same method as used on SPARC64.
 * See update_mmu_cache for the user space part.
 */
#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 1
extern void flush_dcache_page(struct page *);

static inline void __flush_icache_all(void)
{
	asm("ic	ialluis");
}

#define flush_dcache_mmap_lock(mapping) \
	spin_lock_irq(&(mapping)->tree_lock)
#define flush_dcache_mmap_unlock(mapping) \
	spin_unlock_irq(&(mapping)->tree_lock)

/*
 * We don't appear to need to do anything here.  In fact, if we did, we'd
 * duplicate cache flushing elsewhere performed by flush_dcache_page().
 */
#define flush_icache_page(vma,page)	do { } while (0)

/*
 * flush_cache_vmap() is used when creating mappings (eg, via vmap,
 * vmalloc, ioremap etc) in kernel space for pages.  On non-VIPT
 * caches, since the direct-mappings of these pages may contain cached
 * data, we need to do a full cache flush to ensure that writebacks
 * don't corrupt data placed into these pages via the new mappings.
 */
static inline void flush_cache_vmap(unsigned long start, unsigned long end)
{
	/*
	 * set_pte_at() called from vmap_pte_range() does not
	 * have a DSB after cleaning the cache line.
	 */
	dsb();
}

static inline void flush_cache_vunmap(unsigned long start, unsigned long end)
{
}

#endif
