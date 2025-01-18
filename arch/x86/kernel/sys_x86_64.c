#include <fikus/errno.h>
#include <fikus/sched.h>
#include <fikus/syscalls.h>
#include <fikus/mm.h>
#include <fikus/fs.h>
#include <fikus/smp.h>
#include <fikus/sem.h>
#include <fikus/msg.h>
#include <fikus/shm.h>
#include <fikus/stat.h>
#include <fikus/mman.h>
#include <fikus/file.h>
#include <fikus/utsname.h>
#include <fikus/personality.h>
#include <fikus/random.h>
#include <fikus/uaccess.h>
#include <fikus/elf.h>

#include <asm/ia32.h>
#include <asm/syscalls.h>

/*
 * Align a virtual address to avoid aliasing in the I$ on AMD F15h.
 */
static unsigned long get_align_mask(void)
{
	/* handle 32- and 64-bit case with a single conditional */
	if (va_align.flags < 0 || !(va_align.flags & (2 - mmap_is_ia32())))
		return 0;

	if (!(current->flags & PF_RANDOMIZE))
		return 0;

	return va_align.mask;
}

unsigned long align_vdso_addr(unsigned long addr)
{
	unsigned long align_mask = get_align_mask();
	return (addr + align_mask) & ~align_mask;
}

static int __init control_va_addr_alignment(char *str)
{
	/* guard against enabling this on other CPU families */
	if (va_align.flags < 0)
		return 1;

	if (*str == 0)
		return 1;

	if (*str == '=')
		str++;

	if (!strcmp(str, "32"))
		va_align.flags = ALIGN_VA_32;
	else if (!strcmp(str, "64"))
		va_align.flags = ALIGN_VA_64;
	else if (!strcmp(str, "off"))
		va_align.flags = 0;
	else if (!strcmp(str, "on"))
		va_align.flags = ALIGN_VA_32 | ALIGN_VA_64;
	else
		return 0;

	return 1;
}
__setup("align_va_addr", control_va_addr_alignment);

SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
		unsigned long, prot, unsigned long, flags,
		unsigned long, fd, unsigned long, off)
{
	long error;
	error = -EINVAL;
	if (off & ~PAGE_MASK)
		goto out;

	error = sys_mmap_pgoff(addr, len, prot, flags, fd, off >> PAGE_SHIFT);
out:
	return error;
}

static void find_start_end(unsigned long flags, unsigned long *begin,
			   unsigned long *end)
{
	if (!test_thread_flag(TIF_ADDR32) && (flags & MAP_32BIT)) {
		unsigned long new_begin;
		/* This is usually used needed to map code in small
		   model, so it needs to be in the first 31bit. Limit
		   it to that.  This means we need to move the
		   unmapped base down for this case. This can give
		   conflicts with the heap, but we assume that glibc
		   malloc knows how to fall back to mmap. Give it 1GB
		   of playground for now. -AK */
		*begin = 0x40000000;
		*end = 0x80000000;
		if (current->flags & PF_RANDOMIZE) {
			new_begin = randomize_range(*begin, *begin + 0x02000000, 0);
			if (new_begin)
				*begin = new_begin;
		}
	} else {
		*begin = current->mm->mmap_legacy_base;
		*end = TASK_SIZE;
	}
}

unsigned long
arch_get_unmapped_area(struct file *filp, unsigned long addr,
		unsigned long len, unsigned long pgoff, unsigned long flags)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	struct vm_unmapped_area_info info;
	unsigned long begin, end;

	if (flags & MAP_FIXED)
		return addr;

	find_start_end(flags, &begin, &end);

	if (len > end)
		return -ENOMEM;

	if (addr) {
		addr = PAGE_ALIGN(addr);
		vma = find_vma(mm, addr);
		if (end - len >= addr &&
		    (!vma || addr + len <= vma->vm_start))
			return addr;
	}

	info.flags = 0;
	info.length = len;
	info.low_limit = begin;
	info.high_limit = end;
	info.align_mask = filp ? get_align_mask() : 0;
	info.align_offset = pgoff << PAGE_SHIFT;
	return vm_unmapped_area(&info);
}

unsigned long
arch_get_unmapped_area_topdown(struct file *filp, const unsigned long addr0,
			  const unsigned long len, const unsigned long pgoff,
			  const unsigned long flags)
{
	struct vm_area_struct *vma;
	struct mm_struct *mm = current->mm;
	unsigned long addr = addr0;
	struct vm_unmapped_area_info info;

	/* requested length too big for entire address space */
	if (len > TASK_SIZE)
		return -ENOMEM;

	if (flags & MAP_FIXED)
		return addr;

	/* for MAP_32BIT mappings we force the legacy mmap base */
	if (!test_thread_flag(TIF_ADDR32) && (flags & MAP_32BIT))
		goto bottomup;

	/* requesting a specific address */
	if (addr) {
		addr = PAGE_ALIGN(addr);
		vma = find_vma(mm, addr);
		if (TASK_SIZE - len >= addr &&
				(!vma || addr + len <= vma->vm_start))
			return addr;
	}

	info.flags = VM_UNMAPPED_AREA_TOPDOWN;
	info.length = len;
	info.low_limit = PAGE_SIZE;
	info.high_limit = mm->mmap_base;
	info.align_mask = filp ? get_align_mask() : 0;
	info.align_offset = pgoff << PAGE_SHIFT;
	addr = vm_unmapped_area(&info);
	if (!(addr & ~PAGE_MASK))
		return addr;
	VM_BUG_ON(addr != -ENOMEM);

bottomup:
	/*
	 * A failed mmap() very likely causes application failure,
	 * so fall back to the bottom-up function here. This scenario
	 * can happen with large stack limits and large mmap()
	 * allocations.
	 */
	return arch_get_unmapped_area(filp, addr0, len, pgoff, flags);
}
