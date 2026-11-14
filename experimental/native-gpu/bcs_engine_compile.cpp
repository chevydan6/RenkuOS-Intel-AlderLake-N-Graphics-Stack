// SPDX-License-Identifier: MIT
#include <KernelExport.h>
#include "BcsEngine.h"
// Compile both paths with the actual kernel headers/options. No module entry,
// load or hardware invocation; this object only checks target compilation.
bool native_bcs_engine_compile(NativeGpu::BcsEngine& engine,
	const NativeGpu::BcsEngine::IO& io, uint64 status, uint64 descriptor, uint64 deadline)
{
	return engine.Initialize(io, 0x46d18086, 0, status, deadline)
		&& engine.Publish(io, descriptor);
}
