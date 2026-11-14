/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_FS_H
#define RENKU_LINUX_FS_H
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/refcount.h>
/* Native adapter-owned Linux interface views, not Linux binary VM/VFS layouts.
 * Only objects returned by this shmem adapter may use these file operations. */
typedef unsigned long pgoff_t;
struct inode;
struct renku_page_store;
struct address_space {
	struct inode *host;
	gfp_t gfp_mask;
	struct renku_page_store *renku_store;
};
struct inode {
	loff_t i_size;
	struct address_space *i_mapping;
};
struct file {
	struct address_space *f_mapping;
	struct inode *f_inode;
	refcount_t renku_references;
};
struct file *get_file(struct file *);
void fput(struct file *);
static inline long file_count(const struct file *file) { return refcount_read(&file->renku_references); }
static inline struct inode *file_inode(const struct file *file) { return file->f_inode; }
static inline loff_t i_size_read(const struct inode *inode) { return READ_ONCE(inode->i_size); }
static inline gfp_t mapping_gfp_mask(struct address_space *mapping) { return READ_ONCE(mapping->gfp_mask); }
static inline void mapping_set_gfp_mask(struct address_space *mapping, gfp_t mask) { WRITE_ONCE(mapping->gfp_mask, mask); }
static inline gfp_t mapping_gfp_constraint(struct address_space *mapping, gfp_t mask) { return mapping_gfp_mask(mapping) & mask; }
#endif
