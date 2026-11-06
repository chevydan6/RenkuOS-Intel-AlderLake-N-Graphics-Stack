#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import pathlib,re,subprocess,sys,tempfile
here=pathlib.Path(__file__).parent
text=(here/'hdmi-firmware-host.py').read_text()
header=re.search("stub.write_text\\(r'''(.*?)'''",text,re.S).group(1)
header=header.replace('enum { B_OK=0, B_BAD_VALUE=-1,','enum {')
header=header.replace('#include <cstddef>','#include <SupportDefs.h>\n#include <cstddef>')
header=header.replace('pll_info pll_info;','struct pll_info pll_info;')
header=header.replace('gpu_pci_revision;','gpu_pci_revision, display_pipe_leases, display_power_references[9], display_power_owned_requests;')
header=header.replace('struct intel_shared_info {', 'struct display_mode {unsigned virtual_width,virtual_height;};\nstruct intel_shared_info { display_mode current_mode; uint32 frame_buffer_offset,bytes_per_row,bits_per_pixel,graphics_memory_size; void* frame_buffer;void* graphics_memory;')
header=header.replace('#include <SupportDefs.h>','#include <SupportDefs.h>\nusing addr_t=uintptr_t;')
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d)
 (p/'intel_extreme.h').write_text(header)
 (p/'SupportDefs.h').write_text('#pragma once\n#include <stdint.h>\n#include <stddef.h>\ntypedef uint8_t uint8;typedef uint16_t uint16;typedef uint32_t uint32;typedef uint64_t uint64;typedef int status_t;enum {B_OK=0,B_ERROR=-1,B_BUSY=-2,B_BAD_VALUE=-3,B_TIMED_OUT=-4};\n')
 (p/'OS.h').write_text('#pragma once\ninline int snooze(int){return 0;}\n')
 b=p/'test';subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',d,'-I',sys.argv[1],str(here/'alder-lake-hdmi-context-test.cpp'),'-o',str(b)],check=True)
 subprocess.run([str(b)],check=True)
