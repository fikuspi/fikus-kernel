/*
 * Copyright IBM Corp. 2012
 *
 * Author(s):
 *   Jan Glauber <jang@fikus.vnet.ibm.com>
 */

#include <fikus/kernel.h>
#include <fikus/slab.h>
#include <fikus/export.h>
#include <fikus/iommu-helper.h>
#include <fikus/dma-mapping.h>
#include <fikus/vmalloc.h>
#include <fikus/pci.h>
#include <asm/pci_dma.h>

static struct kmem_cache *dma_region_table_cache;
static struct kmem_cache *dma_page_table_cache;

static unsigned long *dma_alloc_cpu_table(void)
{
	unsigned long *table, *entry;

	table = kmem_cache_alloc(dma_region_table_cache, GFP_ATOMIC);
	if (!table)
		return NULL;

	for (entry = table; entry < table + ZPCI_TABLE_ENTRIES; entry++)
		*entry = ZPCI_TABLE_INVALID | ZPCI_TABLE_PROTECTED;
	return table;
}

static void dma_free_cpu_table(void *table)
{
	kmem_cache_free(dma_region_table_cache, table);
}

static unsigned long *dma_alloc_page_table(void)
{
	unsigned long *table, *entry;

	table = kmem_cache_alloc(dma_page_table_cache, GFP_ATOMIC);
	if (!table)
		return NULL;

	for (entry = table; entry < table + ZPCI_PT_ENTRIES; entry++)
		*entry = ZPCI_PTE_INVALID | ZPCI_TABLE_PROTECTED;
	return table;
}

static void dma_free_page_table(void *table)
{
	kmem_cache_free(dma_page_table_cache, table);
}

static unsigned long *dma_get_seg_table_origin(unsigned long *entry)
{
	unsigned long *sto;

	if (reg_entry_isvalid(*entry))
		sto = get_rt_sto(*entry);
	else {
		sto = dma_alloc_cpu_table();
		if (!sto)
			return NULL;

		set_rt_sto(entry, sto);
		validate_rt_entry(entry);
		entry_clr_protected(entry);
	}
	return sto;
}

static unsigned long *dma_get_page_table_origin(unsigned long *entry)
{
	unsigned long *pto;

	if (reg_entry_isvalid(*entry))
		pto = get_st_pto(*entry);
	else {
		pto = dma_alloc_page_table();
		if (!pto)
			return NULL;
		set_st_pto(entry, pto);
		validate_st_entry(entry);
		entry_clr_protected(entry);
	}
	return pto;
}

static unsigned long *dma_walk_cpu_trans(unsigned long *rto, dma_addr_t dma_addr)
{
	unsigned long *sto, *pto;
	unsigned int rtx, sx, px;

	rtx = calc_rtx(dma_addr);
	sto = dma_get_seg_table_origin(&rto[rtx]);
	if (!sto)
		return NULL;

	sx = calc_sx(dma_addr);
	pto = dma_get_page_table_origin(&sto[sx]);
	if (!pto)
		return NULL;

	px = calc_px(dma_addr);
	return &pto[px];
}

static void dma_update_cpu_trans(struct zpci_dev *zdev, void *page_addr,
				 dma_addr_t dma_addr, int flags)
{
	unsigned long *entry;

	entry = dma_walk_cpu_trans(zdev->dma_table, dma_addr);
	if (!entry) {
		WARN_ON_ONCE(1);
		return;
	}

	if (flags & ZPCI_PTE_INVALID) {
		invalidate_pt_entry(entry);
		return;
	} else {
		set_pt_pfaa(entry, page_addr);
		validate_pt_entry(entry);
	}

	if (flags & ZPCI_TABLE_PROTECTED)
		entry_set_protected(entry);
	else
		entry_clr_protected(entry);
}

static int dma_update_trans(struct zpci_dev *zdev, unsigned long pa,
			    dma_addr_t dma_addr, size_t size, int flags)
{
	unsigned int nr_pages = PAGE_ALIGN(size) >> PAGE_SHIFT;
	u8 *page_addr = (u8 *) (pa & PAGE_MASK);
	dma_addr_t start_dma_addr = dma_addr;
	unsigned long irq_flags;
	int i, rc = 0;

	if (!nr_pages)
		return -EINVAL;

	spin_lock_irqsave(&zdev->dma_table_lock, irq_flags);
	if (!zdev->dma_table) {
		dev_err(&zdev->pdev->dev, "Missing DMA table\n");
		goto no_refresh;
	}

	for (i = 0; i < nr_pages; i++) {
		dma_update_cpu_trans(zdev, page_addr, dma_addr, flags);
		page_addr += PAGE_SIZE;
		dma_addr += PAGE_SIZE;
	}

	/*
	 * rpcit is not required to establish new translations when previously
	 * invalid translation-table entries are validated, however it is
	 * required when altering previously valid entries.
	 */
	if (!zdev->tlb_refresh &&
	    ((flags & ZPCI_PTE_VALID_MASK) == ZPCI_PTE_VALID))
		/*
		 * TODO: also need to check that the old entry is indeed INVALID
		 * and not only for one page but for the whole range...
		 * -> now we WARN_ON in that case but with lazy unmap that
		 * needs to be redone!
		 */
		goto no_refresh;

	rc = zpci_refresh_trans((u64) zdev->fh << 32, start_dma_addr,
				nr_pages * PAGE_SIZE);

no_refresh:
	spin_unlock_irqrestore(&zdev->dma_table_lock, irq_flags);
	return rc;
}

static void dma_free_seg_table(unsigned long entry)
{
	unsigned long *sto = get_rt_sto(entry);
	int sx;

	for (sx = 0; sx < ZPCI_TABLE_ENTRIES; sx++)
		if (reg_entry_isvalid(sto[sx]))
			dma_free_page_table(get_st_pto(sto[sx]));

	dma_free_cpu_table(sto);
}

static void dma_cleanup_tables(struct zpci_dev *zdev)
{
	unsigned long *table;
	int rtx;

	if (!zdev || !zdev->dma_table)
		return;

	table = zdev->dma_table;
	for (rtx = 0; rtx < ZPCI_TABLE_ENTRIES; rtx++)
		if (reg_entry_isvalid(table[rtx]))
			dma_free_seg_table(table[rtx]);

	dma_free_cpu_table(table);
	zdev->dma_table = NULL;
}

static unsigned long __dma_alloc_iommu(struct zpci_dev *zdev, unsigned long start,
				   int size)
{
	unsigned long boundary_size = 0x1000000;

	return iommu_area_alloc(zdev->iommu_bitmap, zdev->iommu_pages,
				start, size, 0, boundary_size, 0);
}

static unsigned long dma_alloc_iommu(struct zpci_dev *zdev, int size)
{
	unsigned long offset, flags;

	spin_lock_irqsave(&zdev->iommu_bitmap_lock, flags);
	offset = __dma_alloc_iommu(zdev, zdev->next_bit, size);
	if (offset == -1)
		offset = __dma_alloc_iommu(zdev, 0, size);

	if (offset != -1) {
		zdev->next_bit = offset + size;
		if (zdev->next_bit >= zdev->iommu_pages)
			zdev->next_bit = 0;
	}
	spin_unlock_irqrestore(&zdev->iommu_bitmap_lock, flags);
	return offset;
}

static void dma_free_iommu(struct zpci_dev *zdev, unsigned long offset, int size)
{
	unsigned long flags;

	spin_lock_irqsave(&zdev->iommu_bitmap_lock, flags);
	if (!zdev->iommu_bitmap)
		goto out;
	bitmap_clear(zdev->iommu_bitmap, offset, size);
	if (offset >= zdev->next_bit)
		zdev->next_bit = offset + size;
out:
	spin_unlock_irqrestore(&zdev->iommu_bitmap_lock, flags);
}

int dma_set_mask(struct device *dev, u64 mask)
{
	if (!dev->dma_mask || !dma_supported(dev, mask))
		return -EIO;

	*dev->dma_mask = mask;
	return 0;
}
EXPORT_SYMBOL_GPL(dma_set_mask);

static dma_addr_t s390_dma_map_pages(struct device *dev, struct page *page,
				     unsigned long offset, size_t size,
				     enum dma_data_direction direction,
				     struct dma_attrs *attrs)
{
	struct zpci_dev *zdev = get_zdev(to_pci_dev(dev));
	unsigned long nr_pages, iommu_page_index;
	unsigned long pa = page_to_phys(page) + offset;
	int flags = ZPCI_PTE_VALID;
	dma_addr_t dma_addr;

	/* This rounds up number of pages based on size and offset */
	nr_pages = iommu_num_pages(pa, size, PAGE_SIZE);
	iommu_page_index = dma_alloc_iommu(zdev, nr_pages);
	if (iommu_page_index == -1)
		goto out_err;

	/* Use rounded up size */
	size = nr_pages * PAGE_SIZE;

	dma_addr = zdev->start_dma + iommu_page_index * PAGE_SIZE;
	if (dma_addr + size > zdev->end_dma) {
		dev_err(dev, "(dma_addr: 0x%16.16LX + size: 0x%16.16lx) > end_dma: 0x%16.16Lx\n",
			 dma_addr, size, zdev->end_dma);
		goto out_free;
	}

	if (direction == DMA_NONE || direction == DMA_TO_DEVICE)
		flags |= ZPCI_TABLE_PROTECTED;

	if (!dma_update_trans(zdev, pa, dma_addr, size, flags)) {
		atomic64_add(nr_pages, (atomic64_t *) &zdev->fmb->mapped_pages);
		return dma_addr + (offset & ~PAGE_MASK);
	}

out_free:
	dma_free_iommu(zdev, iommu_page_index, nr_pages);
out_err:
	dev_err(dev, "Failed to map addr: %lx\n", pa);
	return DMA_ERROR_CODE;
}

static void s390_dma_unmap_pages(struct device *dev, dma_addr_t dma_addr,
				 size_t size, enum dma_data_direction direction,
				 struct dma_attrs *attrs)
{
	struct zpci_dev *zdev = get_zdev(to_pci_dev(dev));
	unsigned long iommu_page_index;
	int npages;

	npages = iommu_num_pages(dma_addr, size, PAGE_SIZE);
	dma_addr = dma_addr & PAGE_MASK;
	if (dma_update_trans(zdev, 0, dma_addr, npages * PAGE_SIZE,
			     ZPCI_TABLE_PROTECTED | ZPCI_PTE_INVALID))
		dev_err(dev, "Failed to unmap addr: %Lx\n", dma_addr);

	atomic64_add(npages, (atomic64_t *) &zdev->fmb->unmapped_pages);
	iommu_page_index = (dma_addr - zdev->start_dma) >> PAGE_SHIFT;
	dma_free_iommu(zdev, iommu_page_index, npages);
}

static void *s390_dma_alloc(struct device *dev, size_t size,
			    dma_addr_t *dma_handle, gfp_t flag,
			    struct dma_attrs *attrs)
{
	struct zpci_dev *zdev = get_zdev(to_pci_dev(dev));
	struct page *page;
	unsigned long pa;
	dma_addr_t map;

	size = PAGE_ALIGN(size);
	page = alloc_pages(flag, get_order(size));
	if (!page)
		return NULL;

	atomic64_add(size / PAGE_SIZE, (atomic64_t *) &zdev->fmb->allocated_pages);
	pa = page_to_phys(page);
	memset((void *) pa, 0, size);

	map = s390_dma_map_pages(dev, page, pa % PAGE_SIZE,
				 size, DMA_BIDIRECTIONAL, NULL);
	if (dma_mapping_error(dev, map)) {
		free_pages(pa, get_order(size));
		return NULL;
	}

	if (dma_handle)
		*dma_handle = map;
	return (void *) pa;
}

static void s390_dma_free(struct device *dev, size_t size,
			  void *pa, dma_addr_t dma_handle,
			  struct dma_attrs *attrs)
{
	s390_dma_unmap_pages(dev, dma_handle, PAGE_ALIGN(size),
			     DMA_BIDIRECTIONAL, NULL);
	free_pages((unsigned long) pa, get_order(size));
}

static int s390_dma_map_sg(struct device *dev, struct scatterlist *sg,
			   int nr_elements, enum dma_data_direction dir,
			   struct dma_attrs *attrs)
{
	int mapped_elements = 0;
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nr_elements, i) {
		struct page *page = sg_page(s);
		s->dma_address = s390_dma_map_pages(dev, page, s->offset,
						    s->length, dir, NULL);
		if (!dma_mapping_error(dev, s->dma_address)) {
			s->dma_length = s->length;
			mapped_elements++;
		} else
			goto unmap;
	}
out:
	return mapped_elements;

unmap:
	for_each_sg(sg, s, mapped_elements, i) {
		if (s->dma_address)
			s390_dma_unmap_pages(dev, s->dma_address, s->dma_length,
					     dir, NULL);
		s->dma_address = 0;
		s->dma_length = 0;
	}
	mapped_elements = 0;
	goto out;
}

static void s390_dma_unmap_sg(struct device *dev, struct scatterlist *sg,
			      int nr_elements, enum dma_data_direction dir,
			      struct dma_attrs *attrs)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nr_elements, i) {
		s390_dma_unmap_pages(dev, s->dma_address, s->dma_length, dir, NULL);
		s->dma_address = 0;
		s->dma_length = 0;
	}
}

int zpci_dma_init_device(struct zpci_dev *zdev)
{
	int rc;

	spin_lock_init(&zdev->iommu_bitmap_lock);
	spin_lock_init(&zdev->dma_table_lock);

	zdev->dma_table = dma_alloc_cpu_table();
	if (!zdev->dma_table) {
		rc = -ENOMEM;
		goto out_clean;
	}

	zdev->iommu_size = (unsigned long) high_memory - PAGE_OFFSET;
	zdev->iommu_pages = zdev->iommu_size >> PAGE_SHIFT;
	zdev->iommu_bitmap = vzalloc(zdev->iommu_pages / 8);
	if (!zdev->iommu_bitmap) {
		rc = -ENOMEM;
		goto out_reg;
	}

	rc = zpci_register_ioat(zdev,
				0,
				zdev->start_dma + PAGE_OFFSET,
				zdev->start_dma + zdev->iommu_size - 1,
				(u64) zdev->dma_table);
	if (rc)
		goto out_reg;
	return 0;

out_reg:
	dma_free_cpu_table(zdev->dma_table);
out_clean:
	return rc;
}

void zpci_dma_exit_device(struct zpci_dev *zdev)
{
	zpci_unregister_ioat(zdev, 0);
	dma_cleanup_tables(zdev);
	vfree(zdev->iommu_bitmap);
	zdev->iommu_bitmap = NULL;
	zdev->next_bit = 0;
}

static int __init dma_alloc_cpu_table_caches(void)
{
	dma_region_table_cache = kmem_cache_create("PCI_DMA_region_tables",
					ZPCI_TABLE_SIZE, ZPCI_TABLE_ALIGN,
					0, NULL);
	if (!dma_region_table_cache)
		return -ENOMEM;

	dma_page_table_cache = kmem_cache_create("PCI_DMA_page_tables",
					ZPCI_PT_SIZE, ZPCI_PT_ALIGN,
					0, NULL);
	if (!dma_page_table_cache) {
		kmem_cache_destroy(dma_region_table_cache);
		return -ENOMEM;
	}
	return 0;
}

int __init zpci_dma_init(void)
{
	return dma_alloc_cpu_table_caches();
}

void zpci_dma_exit(void)
{
	kmem_cache_destroy(dma_page_table_cache);
	kmem_cache_destroy(dma_region_table_cache);
}

#define PREALLOC_DMA_DEBUG_ENTRIES	(1 << 16)

static int __init dma_debug_do_init(void)
{
	dma_debug_init(PREALLOC_DMA_DEBUG_ENTRIES);
	return 0;
}
fs_initcall(dma_debug_do_init);

struct dma_map_ops s390_dma_ops = {
	.alloc		= s390_dma_alloc,
	.free		= s390_dma_free,
	.map_sg		= s390_dma_map_sg,
	.unmap_sg	= s390_dma_unmap_sg,
	.map_page	= s390_dma_map_pages,
	.unmap_page	= s390_dma_unmap_pages,
	/* if we support direct DMA this must be conditional */
	.is_phys	= 0,
	/* dma_supported is unconditionally true without a callback */
};
EXPORT_SYMBOL_GPL(s390_dma_ops);
