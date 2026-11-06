#!/usr/bin/env python3
"""Host-only lock regression; no kernel or GPU access. SPDX-License-Identifier: MIT"""
from pathlib import Path
import argparse, subprocess, tempfile
parser=argparse.ArgumentParser()
parser.add_argument("--source-root",type=Path)
parser.add_argument("--expect-fixed",action="store_true")
args=parser.parse_args()
root=Path(__file__).parent
source_path=args.source_root/'src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp' if args.source_root else root/'evidence/renku__src__add-ons__kernel__bus_managers__agp_gart__agp_gart.cpp'
source=source_path.read_text()
def extract(signature):
 start=source.index(signature); brace=source.index('{',start); depth=1; end=brace+1
 while depth:
  depth += (source[end]=='{')-(source[end]=='}'); end+=1
 return source[start:end]
helper=extract('Aperture *\nget_aperture(')
callback=extract('static status_t\nget_aperture_info(')
header_path=args.source_root/'headers/private/graphics/intel_extreme/lock.h' if args.source_root else root/'evidence/renku__headers__private__graphics__intel_extreme__lock.h'
header=header_path.read_text()
stub='''#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
using int32=int32_t; using sem_id=int32; using status_t=int32; using aperture_id=int32;
constexpr int32 B_OK=0, B_ENTRY_NOT_FOUND=-1;
inline int32 create_sem(int32,const char*) {return 1;}
inline int32 delete_sem(int32) {return 0;}
inline int32 atomic_add(int32* p,int32 v) {int32 old=*p;*p+=v;return old;}
inline int32 acquire_sem(int32) {std::puts("BLOCK: same thread enters acquire_sem before releasing outer lock");std::exit(42);}
inline int32 release_sem(int32) {return 0;}
'''
setup='''#include "lock.h"
struct aperture_info {};
struct Aperture {struct lock lock{}; struct lock& Lock(){return lock;} int GetInfo(aperture_info*){return B_OK;}};
static struct lock sLock{};
static Aperture aperture;
struct Registry {Aperture* Lookup(int id){return id==1?&aperture:nullptr;}};
static Registry sApertureHashTable;
'''
with tempfile.TemporaryDirectory() as tmp:
 d=Path(tmp); (d/'OS.h').write_text(stub); (d/'lock.h').write_text(header)
 for name,body,expected in [('production',helper,0 if args.expect_fixed else 42),('control-single-acquisition',helper.replace('Autolock _(sLock);',''),0)]:
  (d/'test.cpp').write_text(setup+body+'\n'+callback+'\nint main(){init_lock(&sLock,"manager");init_lock(&aperture.lock,"aperture");aperture_info info;return get_aperture_info(1,&info);}\n')
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-I',tmp,str(d/'test.cpp'),'-o',str(d/'test')],check=True)
  run=subprocess.run([str(d/'test')],capture_output=True,text=True,timeout=3)
  print(f'{name}: exit={run.returncode}, expected={expected}; {run.stdout.strip()}')
  assert run.returncode==expected
print('PASS: expected production lock behavior and single-acquisition control verified. Host model only.')
