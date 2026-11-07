# SPDX-License-Identifier: MIT
"""Check GART callback lock coverage and run the production query callback."""
import pathlib, re, subprocess, tempfile, sys
source=pathlib.Path(sys.argv[1]).read_text()
names=['get_aperture_info','allocate_memory','free_memory','reserve_aperture','unreserve_aperture','bind_aperture','unbind_aperture']
for name in names:
 body=re.search(r'^static status_t\n'+name+r'\([^)]*\)\n\{.*?^}',source,re.M|re.S).group()
 assert body.index('Autolock managerLocker(sLock);')<body.index('get_aperture(id)'),name
body=re.search(r'^static status_t\nget_aperture_info\([^)]*\)\n\{.*?^}',source,re.M|re.S).group()
prefix=r'''
#include <cassert>
#include <cstdio>
using status_t=int;using aperture_id=int;
constexpr int B_ENTRY_NOT_FOUND=-2;
struct aperture_info {int value;};
struct TestLock {bool held=false;};
static TestLock sLock;
struct Autolock {TestLock& lock;Autolock(TestLock& l):lock(l){assert(!l.held);l.held=true;}~Autolock(){lock.held=false;}};
struct Aperture {TestLock lock;TestLock& Lock(){assert(sLock.held);return lock;}
 int GetInfo(aperture_info* info){assert(sLock.held&&lock.held);info->value=73;return 0;}};
static Aperture aperture;
static Aperture* get_aperture(int id){assert(sLock.held);return id==1?&aperture:nullptr;}
'''
suffix=r'''
int main(){aperture_info info{};assert(get_aperture_info(1,&info)==0&&info.value==73);assert(!sLock.held&&!aperture.lock.held);assert(get_aperture_info(2,&info)==B_ENTRY_NOT_FOUND);assert(!sLock.held);puts("7 callback lock guards and 2 production query lock-lifetime cases passed");}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d);(p/'test.cpp').write_text(prefix+body+suffix)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
