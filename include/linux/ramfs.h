#ifndef _FIKUS_RAMFS_H
#define _FIKUS_RAMFS_H

struct inode *ramfs_get_inode(struct super_block *sb, const struct inode *dir,
	 umode_t mode, dev_t dev);
extern struct dentry *ramfs_mount(struct file_system_type *fs_type,
	 int flags, const char *dev_name, void *data);

#ifdef CONFIG_MMU
static inline int
ramfs_nommu_expand_for_mapping(struct inode *inode, size_t newsize)
{
	return 0;
}
#else
extern int ramfs_nommu_expand_for_mapping(struct inode *inode, size_t newsize);
extern unsigned long ramfs_nommu_get_unmapped_area(struct file *file,
						   unsigned long addr,
						   unsigned long len,
						   unsigned long pgoff,
						   unsigned long flags);

extern int ramfs_nommu_mmap(struct file *file, struct vm_area_struct *vma);
#endif

extern const struct file_operations ramfs_file_operations;
extern const struct vm_operations_struct generic_file_vm_ops;
extern int __init init_ramfs_fs(void);

int ramfs_fill_super(struct super_block *sb, void *data, int silent);

#endif
