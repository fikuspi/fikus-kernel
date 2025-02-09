#ifndef PAGE_FLAGS_LAYOUT_H
#define PAGE_FLAGS_LAYOUT_H

#include <fikus/numa.h>
#include <generated/bounds.h>

/*
 * When a memory allocation must conform to specific limitations (such
 * as being suitable for DMA) the caller will pass in hints to the
 * allocator in the gfp_mask, in the zone modifier bits.  These bits
 * are used to select a priority ordered list of memory zones which
 * match the requested limits. See gfp_zone() in include/fikus/gfp.h
 */
#if MAX_NR_ZONES < 2
#define ZONES_SHIFT 0
#elif MAX_NR_ZONES <= 2
#define ZONES_SHIFT 1
#elif MAX_NR_ZONES <= 4
#define ZONES_SHIFT 2
#else
#error ZONES_SHIFT -- too many zones configured adjust calculation
#endif

#ifdef CONFIG_SPARSEMEM
#include <asm/sparsemem.h>

/* SECTION_SHIFT	#bits space required to store a section # */
#define SECTIONS_SHIFT	(MAX_PHYSMEM_BITS - SECTION_SIZE_BITS)

#endif /* CONFIG_SPARSEMEM */

/*
 * page->flags layout:
 *
 * There are five possibilities for how page->flags get laid out.  The first
 * pair is for the normal case without sparsemem. The second pair is for
 * sparsemem when there is plenty of space for node and section information.
 * The last is when there is insufficient space in page->flags and a separate
 * lookup is necessary.
 *
 * No sparsemem or sparsemem vmemmap: |       NODE     | ZONE |          ... | FLAGS |
 *         " plus space for last_nid: |       NODE     | ZONE | LAST_NID ... | FLAGS |
 * classic sparse with space for node:| SECTION | NODE | ZONE |          ... | FLAGS |
 *         " plus space for last_nid: | SECTION | NODE | ZONE | LAST_NID ... | FLAGS |
 * classic sparse no space for node:  | SECTION |     ZONE    | ... | FLAGS |
 */
#if defined(CONFIG_SPARSEMEM) && !defined(CONFIG_SPARSEMEM_VMEMMAP)
#define SECTIONS_WIDTH		SECTIONS_SHIFT
#else
#define SECTIONS_WIDTH		0
#endif

#define ZONES_WIDTH		ZONES_SHIFT

#if SECTIONS_WIDTH+ZONES_WIDTH+NODES_SHIFT <= BITS_PER_LONG - NR_PAGEFLAGS
#define NODES_WIDTH		NODES_SHIFT
#else
#ifdef CONFIG_SPARSEMEM_VMEMMAP
#error "Vmemmap: No space for nodes field in page flags"
#endif
#define NODES_WIDTH		0
#endif

#ifdef CONFIG_NUMA_BALANCING
#define LAST_NID_SHIFT NODES_SHIFT
#else
#define LAST_NID_SHIFT 0
#endif

#if SECTIONS_WIDTH+ZONES_WIDTH+NODES_SHIFT+LAST_NID_SHIFT <= BITS_PER_LONG - NR_PAGEFLAGS
#define LAST_NID_WIDTH LAST_NID_SHIFT
#else
#define LAST_NID_WIDTH 0
#endif

/*
 * We are going to use the flags for the page to node mapping if its in
 * there.  This includes the case where there is no node, so it is implicit.
 */
#if !(NODES_WIDTH > 0 || NODES_SHIFT == 0)
#define NODE_NOT_IN_PAGE_FLAGS
#endif

#if defined(CONFIG_NUMA_BALANCING) && LAST_NID_WIDTH == 0
#define LAST_NID_NOT_IN_PAGE_FLAGS
#endif

#endif /* _FIKUS_PAGE_FLAGS_LAYOUT */
