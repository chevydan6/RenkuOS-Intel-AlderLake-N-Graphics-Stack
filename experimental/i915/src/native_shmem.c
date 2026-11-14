/* SPDX-License-Identifier: MIT */
#include <linux/shmem_fs.h>
#include <linux/preempt.h>
#include <native_pages.h>

/* Interface ownership only. These are native adapter objects; no Linux private
 * VFS/VM binary layout, global page cache or backing-device state is guessed. */
struct native_shmem {
	struct file file;
	struct inode inode;
	struct address_space mapping;
};
static void ReleaseFile(void *cookie)
{
	struct native_shmem *owner = cookie;
	BUG_ON(refcount_read(&owner->file.renku_references) != 0);
	free(owner);
}
struct file *shmem_file_setup(const char *name, loff_t size, unsigned long flags)
{
	if (!name || size < 0 || (flags & ~VM_NORESERVE)) return ERR_PTR(-EINVAL);
	if (!preemptible()) return ERR_PTR(-EOPNOTSUPP);
	/* Logical EOF is independent of the page-aligned native reservation. Empty
 * files keep one untouched anonymous page; they expose no readable page. */
	/* An individual native virtual object must also fit pointer differences;
	 * reject before asking the VM to round/map a signed-address-size span. */
	if ((uint64_t)size > PTRDIFF_MAX - (PAGE_SIZE - 1)) return ERR_PTR(-EFBIG);
	size_t rounded = ((size_t)size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	if (!rounded) rounded = PAGE_SIZE;
	struct native_shmem *owner = calloc(1, sizeof(*owner));
	if (!owner) return ERR_PTR(-ENOMEM);
	struct renku_page_store *store = renku_linux_page_store_create_owned(rounded,
		(flags & VM_NORESERVE) != 0, ReleaseFile, owner);
	if (!store) { free(owner); return ERR_PTR(-ENOMEM); }
	owner->inode.i_size = size;
	owner->inode.i_mapping = &owner->mapping;
	owner->mapping.host = &owner->inode;
	owner->mapping.gfp_mask = GFP_KERNEL;
	owner->mapping.renku_store = store;
	owner->file.f_inode = &owner->inode;
	owner->file.f_mapping = &owner->mapping;
	refcount_set(&owner->file.renku_references, 1);
	return &owner->file;
}
struct file *get_file(struct file *file)
{
	BUG_ON(!file || !refcount_inc_not_zero(&file->renku_references));
	return file;
}
void fput(struct file *file)
{
	BUG_ON(!file);
	if (refcount_dec_and_test(&file->renku_references))
		renku_linux_page_store_put(file->f_mapping->renku_store);
}
struct folio *shmem_read_folio_gfp(struct address_space *mapping, pgoff_t index, gfp_t gfp)
{
	if (!mapping || !mapping->host || !mapping->renku_store) return ERR_PTR(-EINVAL);
	loff_t size = i_size_read(mapping->host);
	if (size < 0 || index >= ((uint64_t)size + PAGE_SIZE - 1) >> PAGE_SHIFT)
		return ERR_PTR(-EINVAL);
	/* Native lock_memory_etc may fault and block. It cannot currently honor
 * Linux no-reclaim, zone, retry or NOFS/NOIO contracts. Preserve constraints
 * by rejecting them, rather than quietly allocating with a broader policy. */
	if (!preemptible() || gfp != GFP_KERNEL) return ERR_PTR(-EOPNOTSUPP);
	struct folio *folio = renku_linux_page_store_read(mapping->renku_store, index);
	return folio ? folio : ERR_PTR(-ENOMEM);
}
struct page *shmem_read_mapping_page_gfp(struct address_space *mapping, pgoff_t index, gfp_t gfp)
{
	struct folio *folio = shmem_read_folio_gfp(mapping, index, gfp);
	return IS_ERR(folio) ? ERR_CAST(folio) : &folio->page;
}
