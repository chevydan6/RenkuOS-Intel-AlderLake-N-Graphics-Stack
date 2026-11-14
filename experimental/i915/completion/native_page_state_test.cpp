// SPDX-License-Identifier: MIT
// Independent native oracle, linked only into the diagnostic module. Callers
// retain a writable pin on newly allocated private diagnostic backing.
#include <KernelExport.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/vm_page.h>
#include <vm/VMCache.h>
#include <native_page_bridge.h>
extern "C" status_t renku_test_vm_page_state(uint64_t physical, bool clear, uint32_t *state)
{
	vm_page *page = vm_lookup_page(physical / B_PAGE_SIZE);
	if (!page) return B_BAD_VALUE;
	VMCache *cache = vm_cache_acquire_locked_page_cache(page, false);
	if (!cache) return B_BAD_VALUE;
	status_t result = B_OK;
	if (!page->WiredCount()) result = B_BAD_VALUE;
	else {
		if (clear) vm_clear_map_flags(page, PAGE_ACCESSED | PAGE_MODIFIED);
		*state = (page->modified ? RENKU_VM_PAGE_DIRTY : 0)
			| (page->accessed ? RENKU_VM_PAGE_ACCESSED : 0);
	}
	cache->ReleaseRefAndUnlock();
	return result;
}
