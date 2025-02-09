/*
 * fikus/arch/m68k/sun3/dvma.c
 *
 * Written by Sam Creasey
 *
 * Sun3 IOMMU routines used for dvma accesses.
 *
 */

#include <fikus/kernel.h>
#include <fikus/mm.h>
#include <fikus/bootmem.h>
#include <fikus/list.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/sun3mmu.h>
#include <asm/dvma.h>


static unsigned long ptelist[120];

static unsigned long dvma_page(unsigned long kaddr, unsigned long vaddr)
{
	unsigned long pte;
	unsigned long j;
	pte_t ptep;

	j = *(volatile unsigned long *)kaddr;
	*(volatile unsigned long *)kaddr = j;

	ptep = pfn_pte(virt_to_pfn(kaddr), PAGE_KERNEL);
	pte = pte_val(ptep);
//		printk("dvma_remap: addr %lx -> %lx pte %08lx len %x\n",
//		       kaddr, vaddr, pte, len);
	if(ptelist[(vaddr & 0xff000) >> PAGE_SHIFT] != pte) {
		sun3_put_pte(vaddr, pte);
		ptelist[(vaddr & 0xff000) >> PAGE_SHIFT] = pte;
	}

	return (vaddr + (kaddr & ~PAGE_MASK));

}

int dvma_map_iommu(unsigned long kaddr, unsigned long baddr,
			      int len)
{

	unsigned long end;
	unsigned long vaddr;

	vaddr = dvma_btov(baddr);

	end = vaddr + len;

	while(vaddr < end) {
		dvma_page(kaddr, vaddr);
		kaddr += PAGE_SIZE;
		vaddr += PAGE_SIZE;
	}

	return 0;

}

void sun3_dvma_init(void)
{

	memset(ptelist, 0, sizeof(ptelist));


}
