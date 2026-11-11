#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <cassert>
#define CONCURRENT_TEST
static std::recursive_mutex moduleMutex;
static thread_local int depth;
static std::atomic<int> nextThread{0};
int thread_get_current_thread_id(){static thread_local int id=++nextThread;return id;}
int recursive_lock_get_recursion(int*){return depth?depth:-1;}
void recursive_lock_lock(int*){moduleMutex.lock();++depth;}
void recursive_lock_unlock(int*){assert(depth>0);--depth;moduleMutex.unlock();}
struct RecursiveLocker {int* lock;explicit RecursiveLocker(int& l):lock(&l){recursive_lock_lock(lock);}~RecursiveLocker(){recursive_lock_unlock(lock);}};
#define main sequential_main
#include "harness.cpp"
#undef main
static std::mutex gate;
static std::condition_variable changed;
static int paused,completed;
static void pauseAt(int stage){
 assert(depth==0);std::unique_lock<std::mutex> lock(gate);
 paused=stage;changed.notify_all();changed.wait(lock,[=]{return completed>=stage;});
}
static int rootOps(int op){assert(op==B_MODULE_INIT);pauseAt(2);return B_OK;}
static int childOps(int op){assert(op==B_MODULE_INIT);pauseAt(1);return B_OK;}
static int otherOps(int op){assert(op==B_MODULE_INIT&&depth==0);return B_OK;}
static int releaseCalls;
static int releaseOps(int op){assert(op==B_MODULE_UNINIT&&depth==0);++releaseCalls;pauseAt(3);assert(put_module("b")==B_OK);return B_OK;}
int main(){
 module_info parent{rootOps},child{childOps},other{otherOps};
 module_info* infos[]={&parent,&other};module_info* slot=nullptr;
 module_dependency deps[]={{"a",&slot},{nullptr,nullptr}};
 module_image image{};image.info=infos;image.dependencies=deps;
 module root{"root",MODULE_LOADED,0,0,&parent,&image,0},core{"core",MODULE_LOADED,0,0,&other,&image,1};
 module a{"a",MODULE_LOADED,B_BUILT_IN_MODULE,0,&child,nullptr,0},b{"b",MODULE_LOADED,B_BUILT_IN_MODULE,0,&other,nullptr,0};
 entries[0]=&root;entries[1]=&core;entries[2]=&a;entries[3]=&b;
 std::thread contender([&]{for(int stage=1;stage<=2;++stage){
  {std::unique_lock<std::mutex> lock(gate);changed.wait(lock,[=]{return paused==stage;});}
  assert(depth==0);module_info* out=nullptr;
  {RecursiveLocker lock(sModulesLock);assert(current_module_entry()==nullptr);
   int refs=root.ref_count,pins=image.ref_count;
   assert(get_module("root",&out)==B_BUSY&&depth==1&&!out);
   assert(root.ref_count==refs&&image.ref_count==pins);
  }
  assert(get_module("root",&out)==B_BUSY&&!out);
  assert(put_module("root")==B_BUSY&&root.ref_count==0);
  if(stage==1){assert(get_module("core",&out)==B_BUSY&&!out);assert(core.ref_count==0&&image.dependency_leases==0&&slot==nullptr);}
  else {assert(get_module("core",&out)==B_OK&&out==&other);assert(image.dependency_leases==2&&slot==&child);}
  out=nullptr;assert(get_module("b",&out)==B_OK&&out==&other);
  {RecursiveLocker lock(sModulesLock);assert(current_module_entry()==nullptr);}
  {std::lock_guard<std::mutex> lock(gate);completed=stage;}changed.notify_all();
 }});
 module_info* out=nullptr;assert(get_module("root",&out)==B_OK&&out==&parent);contender.join();
 assert(depth==0&&sModuleEntries==nullptr&&root.ref_count==1&&core.ref_count==1&&a.ref_count==1&&b.ref_count==2);
 assert(image.ref_count==2&&image.dependency_leases==2&&image.dependency_acquired==1&&!root.transition_active&&image.transition_pins==0);

 root.info=&parent;parent.std_ops=releaseOps;
 std::thread putter([&]{
  {std::unique_lock<std::mutex> lock(gate);changed.wait(lock,[]{return paused==3;});}
  module_info* out=nullptr;
  auto second=put_module_internal("root");assert(second.status==B_BUSY&&second.consumption==NotConsumed);
  assert(get_module("root",&out)==B_BUSY&&!out);
  assert(root.ref_count==0&&root.transition_active&&image.transition_pins==1);
  assert(get_module("b",&out)==B_OK);
  {std::lock_guard<std::mutex> lock(gate);completed=3;}changed.notify_all();
 });
 auto released=put_module_internal("root");putter.join();
 assert(released.status==B_OK&&released.consumption==ConsumedComplete);
 assert(releaseCalls==1&&root.ref_count==0&&!root.transition_active&&image.transition_pins==0);
 assert(image.dependency_leases==1&&depth==0&&sModuleEntries==nullptr);
 std::free(image.dependency_records);puts("two real threads: child/INIT pauses, owner-aware recursion, target/Building busy, unrelated progress, shared leases and context isolation passed");
}
