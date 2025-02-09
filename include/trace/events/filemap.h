#undef TRACE_SYSTEM
#define TRACE_SYSTEM filemap

#if !defined(_TRACE_FILEMAP_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_FILEMAP_H

#include <fikus/types.h>
#include <fikus/tracepoint.h>
#include <fikus/mm.h>
#include <fikus/memcontrol.h>
#include <fikus/device.h>
#include <fikus/kdev_t.h>

DECLARE_EVENT_CLASS(mm_filemap_op_page_cache,

	TP_PROTO(struct page *page),

	TP_ARGS(page),

	TP_STRUCT__entry(
		__field(struct page *, page)
		__field(unsigned long, i_ino)
		__field(unsigned long, index)
		__field(dev_t, s_dev)
	),

	TP_fast_assign(
		__entry->page = page;
		__entry->i_ino = page->mapping->host->i_ino;
		__entry->index = page->index;
		if (page->mapping->host->i_sb)
			__entry->s_dev = page->mapping->host->i_sb->s_dev;
		else
			__entry->s_dev = page->mapping->host->i_rdev;
	),

	TP_printk("dev %d:%d ino %lx page=%p pfn=%lu ofs=%lu",
		MAJOR(__entry->s_dev), MINOR(__entry->s_dev),
		__entry->i_ino,
		__entry->page,
		page_to_pfn(__entry->page),
		__entry->index << PAGE_SHIFT)
);

DEFINE_EVENT(mm_filemap_op_page_cache, mm_filemap_delete_from_page_cache,
	TP_PROTO(struct page *page),
	TP_ARGS(page)
	);

DEFINE_EVENT(mm_filemap_op_page_cache, mm_filemap_add_to_page_cache,
	TP_PROTO(struct page *page),
	TP_ARGS(page)
	);

#endif /* _TRACE_FILEMAP_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
