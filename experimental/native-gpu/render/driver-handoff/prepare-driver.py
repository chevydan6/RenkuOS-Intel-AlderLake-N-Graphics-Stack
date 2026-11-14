#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Prepare a new directory containing the private N100 graphics-driver candidate."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import shutil

SOURCE_SHA256 = 'e32ce739f36af66dea0f2c2023453549be5b875c32a81c70a62dabfbb47adcb1'
INCLUDE = '#include "intel_extreme.h"\n'
FINISH = '\t\tinfo.memory_owner_ready = true;\n'
HANDOFF = '''		// Optional private render handoff: display remains usable on failure.
		if (info.device_type.InGroup(INTEL_GROUP_ALD)) {
			native_gpu_render_module* render = NULL;
			status_t renderStatus = get_module(NATIVE_GPU_RENDER_MODULE,
				(module_info**)&render);
			if (renderStatus == B_OK) {
				if (render->acknowledge_display_profile != NULL) {
					renderStatus = render->acknowledge_display_profile(
						info.memory_owner_lease,
						NATIVE_GPU_RENDER_PROFILE_N100_DISPLAY_ONLY_V1);
				} else
					renderStatus = B_NOT_SUPPORTED;
				put_module(NATIVE_GPU_RENDER_MODULE);
			}
			if (renderStatus != B_OK) {
				dprintf("intel_extreme: native render handoff unavailable (%"
					B_PRId32 "); display remains available\\n", renderStatus);
			}
		}
'''


def transform(source):
    if hashlib.sha256(source.encode()).hexdigest() != SOURCE_SHA256:
        raise ValueError('intel_extreme.cpp SHA256 mismatch; refusing to patch')
    if source.count(INCLUDE) != 1 or source.count(FINISH) != 1:
        raise ValueError('source insertion anchors are not unique')
    return source.replace(INCLUDE, INCLUDE + '#include "RenderModule.h"\n', 1).replace(FINISH, FINISH + HANDOFF, 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    source_bytes = args.source.read_bytes()
    if hashlib.sha256(source_bytes).hexdigest() != SOURCE_SHA256:
        parser.error('intel_extreme.cpp SHA256 mismatch; refusing to patch')
    source = source_bytes.decode('utf-8')
    candidate = transform(source)
    here = Path(__file__).resolve().parent
    native = here.parents[1]
    for name in ('RenderModule.h', 'RenderReport.h'):
        if not (native / name).is_file():
            parser.error(f'missing private header: {native / name}')
    # Fail if any output already exists; never overwrite a baseline or candidate.
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output / 'intel_extreme.cpp').write_text(candidate)
    (args.output / 'intel_extreme.original.cpp').write_bytes(source_bytes)
    for name in ('RenderModule.h', 'RenderReport.h'):
        shutil.copyfile(native / name, args.output / name)
    shutil.copyfile(here / 'build-driver.sh', args.output / 'build-driver.sh')
    (args.output / 'handoff.patch').write_text(''.join(difflib.unified_diff(
        source.splitlines(True), candidate.splitlines(True),
        fromfile='intel_extreme.cpp.original', tofile='intel_extreme.cpp')))
    paths = ['intel_extreme.cpp', 'intel_extreme.original.cpp', 'RenderModule.h', 'RenderReport.h', 'build-driver.sh']
    manifest = {name: hashlib.sha256((args.output / name).read_bytes()).hexdigest() for name in paths}
    (args.output / 'candidate.sha256').write_text(''.join(f'{digest}  {name}\n' for name, digest in manifest.items()))
    (args.output / 'manifest.json').write_text(json.dumps({'source_sha256': SOURCE_SHA256, 'files': manifest}, indent=2) + '\n')
    print(f'DRIVER_HANDOFF_PREPARED {args.output}')


if __name__ == '__main__':
    main()
