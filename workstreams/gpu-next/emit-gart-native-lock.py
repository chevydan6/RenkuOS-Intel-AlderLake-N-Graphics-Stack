# SPDX-License-Identifier: MIT
"""Emit a native query regression using production lookup, callback and lock.h."""
import pathlib,re,sys
source=pathlib.Path(sys.argv[1]).read_text()
helper=re.search(r'^Aperture \*\nget_aperture\([^)]*\)\n\{.*?^}',source,re.M|re.S).group()
query=re.search(r'^static status_t\nget_aperture_info\([^)]*\)\n\{.*?^}',source,re.M|re.S).group()
prefix='''// SPDX-License-Identifier: MIT
#include "lock.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
using aperture_id = int32;
struct aperture_info { int value; };
struct Aperture {
 struct lock mutex;
 struct lock& Lock() { return mutex; }
 status_t GetInfo(aperture_info* info) { info->value=73; return B_OK; }
};
static struct lock sLock;
static Aperture aperture;
struct Registry { Aperture* Lookup(aperture_id id) { return id==1?&aperture:NULL; } };
static Registry sApertureHashTable;
'''
suffix='''
int main() {
 alarm(5);
 assert(init_lock(&sLock, "test manager") == B_OK);
 assert(init_lock(&aperture.mutex, "test aperture") == B_OK);
 aperture_info info{};
 assert(get_aperture_info(1, &info) == B_OK && info.value == 73);
 assert(get_aperture_info(2, &info) == B_ENTRY_NOT_FOUND);
 assert(sLock.count == 0 && aperture.mutex.count == 0);
 uninit_lock(&aperture.mutex); uninit_lock(&sLock);
 alarm(0);
 puts("2 native production lookup/query cases passed with real semaphore locks");
}
'''
pathlib.Path(sys.argv[2]).write_text(prefix+helper+'\n'+query+suffix)
