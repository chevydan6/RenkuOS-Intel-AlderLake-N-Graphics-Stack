// SPDX-License-Identifier: MIT
// Compile/link check only. No module entry point; never installed or invoked.
#include "IntelN100KernelPages.h"
extern "C" bool
intel_kernel_pages_compile(IntelN100KernelPages* provider, IntelN100PpgttTree* tree)
{
	return provider && tree
		&& tree->Build(provider->Backend(), 0, 1, 0, 3, IntelN100DirectoryCached)
		&& tree->Release();
}
