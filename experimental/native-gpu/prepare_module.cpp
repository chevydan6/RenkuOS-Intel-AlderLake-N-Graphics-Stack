// SPDX-License-Identifier: MIT
#include <KernelExport.h>
#include <IntelMemoryOwner.h>
#include <IntelN100PpgttTree.h>
#include <native_page_bridge.h>
#include "PrepareModule.h"
#include "RequestImage.h"

// The resident record owns every allocation and callback until CPU preparation
// cleanup completes. There is no engine/register publication operation here.
static intel_memory_mapping_module_info* sMapping;
static sem_id sLock = -1;
static IntelN100PpgttTree sTree;
static intel_memory_mapping_set sSet;
static bool sHeld, sRetained;
static status_t sPageError;
static bool Hold(void*) { if (sHeld) return false; sHeld = true; return true; }
static void Drop(void*) { sHeld = false; }
static bool Allocate(void*, IntelN100PpgttTree::Page& page)
{
	void* cpu = NULL;
	uint64_t physical = 0;
	int32_t area = renku_vm_page_run_create(4096, &cpu, &physical);
	if (area < B_OK) { sPageError = area; return false; }
	// CPU physical identity only. GPU DMA admission is required at publication.
	page = {uint64_t(uint32_t(area)) + 1, (uint64_t*)cpu, physical};
	return true;
}
static bool Free(void*, uint64_t token)
{
	status_t status = renku_vm_page_run_delete(int32_t(uint32_t(token - 1)));
	if (status != B_OK) sPageError = status;
	return status == B_OK;
}
static uint32 Hash(const uint32* words, unsigned count)
{
	uint32 hash = 2166136261u;
	for (unsigned i = 0; i < count; i++) hash = (hash ^ words[i]) * 16777619u;
	return hash;
}
static status_t Prepare(native_gpu_prepare_report* report)
{
	if (!report) return B_BAD_VALUE;
	status_t status = acquire_sem(sLock);
	if (status != B_OK) return status;
	*report = {};
	report->version = 1;
	report->mapping = report->tables = report->image = report->cleanup = B_ERROR;
	if (sRetained) { release_sem(sLock); return B_BUSY; }
	sPageError = B_OK;
	report->mapping = sMapping->begin_set(&sSet);
	if (report->mapping == B_OK) {
		IntelN100PpgttTree::Backend backend = {NULL, Hold, Drop, Allocate, Free};
		bool built = sTree.Build(backend, 0x200000, 1, 0, 0, IntelN100DirectoryCached);
		report->pages = sTree.RecordedPages();
		report->tables = built ? B_OK : sPageError != B_OK ? sPageError : B_ERROR;
		uint64_t root;
		if (built && sTree.Root(root)) {
			NativeGpu::ImageInput input = {0x46d18086, 0, {}, root,
				uint32_t(system_time()) | 1u, 1, 1};
			if (input.nonce == NativeGpu::kPoison || ~input.nonce == NativeGpu::kPoison)
				input.nonce ^= 0x915;
			for (unsigned i = 0; i < 4; i++) input.ggtt[i] = sSet.buffers[i].ggtt_offset;
			NativeGpu::ImageBuffers cpu = {(uint32*)sSet.buffers[0].cpu_aperture,
				(uint32*)sSet.buffers[1].cpu_aperture, (uint32*)sSet.buffers[2].cpu_aperture,
				(uint32*)sSet.buffers[3].cpu_aperture};
			NativeGpu::ImageOutput image;
			if (NativeGpu::BuildRequestImage(input, cpu, image)) {
				asm volatile("mfence" ::: "memory"); // Order WC stores before CPU readback.
				report->ring_tail = image.tail;
				report->workaround_tail = image.workaroundTail;
				report->ring_checksum = Hash(cpu.ring, image.workaroundTail / 4);
				report->context_checksum = Hash(cpu.context, 16384 / 4);
				report->result_poisoned = cpu.result[0] == NativeGpu::kPoison
					&& cpu.result[1] == NativeGpu::kPoison && cpu.status[0x40] == NativeGpu::kPoison;
				report->image = report->result_poisoned ? B_OK : B_BAD_DATA;
			}
		}
		// These objects are unpublished; this is not post-DMA retirement.
		if (sTree.Release()) {
			report->cleanup = sMapping->end_set(sSet.token);
			if (report->cleanup == B_OK) sSet = {};
		}
		sRetained = report->cleanup != B_OK;
	}
	status = report->mapping == B_OK && report->tables == B_OK
		&& report->image == B_OK && report->cleanup == B_OK ? B_OK : B_ERROR;
	release_sem(sLock);
	return status;
}
static status_t StandardOperations(int32 operation, ...)
{
	if (operation == B_MODULE_INIT) {
		status_t status = get_module(INTEL_MEMORY_MAPPING_MODULE_NAME, (module_info**)&sMapping);
		if (status != B_OK) return status;
		sLock = create_sem(1, "intel native preparation");
		if (sLock >= B_OK) return B_OK;
		put_module(INTEL_MEMORY_MAPPING_MODULE_NAME);
		return sLock;
	}
	if (operation == B_MODULE_UNINIT) return B_BUSY;
	return B_BAD_VALUE;
}
static native_gpu_prepare_module sModule = {
	{NATIVE_GPU_PREPARE_MODULE, B_KEEP_LOADED, StandardOperations}, Prepare
};
extern "C" { module_info* modules[] = {(module_info*)&sModule, NULL}; }
