#ifndef _FIKUS_HUGETLB_INLINE_H
#define _FIKUS_HUGETLB_INLINE_H

#ifdef CONFIG_HUGETLB_PAGE

#include <fikus/mm.h>

static inline int is_vm_hugetlb_page(struct vm_area_struct *vma)
{
	return !!(vma->vm_flags & VM_HUGETLB);
}

#else

static inline int is_vm_hugetlb_page(struct vm_area_struct *vma)
{
	return 0;
}

#endif

#endif
