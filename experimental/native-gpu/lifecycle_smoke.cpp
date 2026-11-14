// SPDX-License-Identifier: MIT
#include "RequestLifecycle.h"
#include <assert.h>
#include <stdio.h>

int main()
{
 using L=NativeGpu::RequestLifecycle;
 L device;
 assert(device.Current()==L::Cold);
 assert(device.Begin()==L::Accepted);
 assert(device.Begin()==L::Busy);
 device.Finish(true);
 assert(device.Current()==L::Ready && device.Completed()==1);
 assert(device.Begin()==L::Accepted);
 device.Finish(false);
 assert(device.Current()==L::Lost && device.Failed()==1);
 assert(device.Begin()==L::Unavailable);

 L recovered;
 assert(recovered.Begin()==L::Accepted);
 recovered.Finish(false,true);
 assert(recovered.Current()==L::Ready && recovered.Failed()==1
  && recovered.Completed()==0 && recovered.Generation()==2);
 assert(recovered.Begin()==L::Accepted);
 assert(recovered.Freeze(L::Suspended));
 recovered.Finish(false,true);
 assert(recovered.Current()==L::Suspended && recovered.Failed()==2);

 L power;
 assert(power.Begin()==L::Accepted);
 assert(power.Freeze(L::Suspended));
 assert(power.Current()==L::Quiescing);
 assert(power.Begin()==L::Unavailable);
 power.Finish(true);
 assert(power.Current()==L::Suspended);
 assert(!power.Resume(false));
 assert(power.Current()==L::Lost);

 L resume;
 assert(resume.Freeze(L::Suspended));
 assert(resume.Current()==L::Suspended);
 assert(resume.Resume(true));
 assert(resume.Generation()==2 && resume.Begin()==L::Accepted);
 assert(resume.Freeze(L::Shutdown));
 resume.Finish(false);
 assert(resume.Current()==L::Lost); // failed drain must not hide uncertainty
 assert(!resume.Resume(true));

 L shutdown;
 assert(shutdown.Freeze(L::Shutdown));
 assert(shutdown.Begin()==L::Unavailable);
 assert(!shutdown.Resume(true));
 puts("Request admission, uncertain lifetime and power drain: PASS");
}
