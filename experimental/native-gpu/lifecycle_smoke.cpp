// SPDX-License-Identifier: MIT
#include "RequestLifecycle.h"
#include <assert.h>
#include <stdio.h>

int main()
{
 using L=NativeGpu::RequestLifecycle;
 L observation;
 assert(observation.Begin()==L::Accepted);
 observation.FinishObservation(true);
 assert(observation.Current()==L::Ready && observation.Completed()==0 && observation.Failed()==0);
 assert(observation.Begin()==L::Accepted && observation.Freeze(L::Suspended));
 observation.FinishObservation(true);
 assert(observation.Current()==L::Suspended && observation.Completed()==0 && observation.Failed()==0);
 assert(observation.Resume(true) && observation.Begin()==L::Accepted);
 observation.FinishObservation(false);
 assert(observation.Current()==L::Lost && observation.Completed()==0 && observation.Failed()==0);
 L closingObservation;
 assert(closingObservation.Begin()==L::Accepted && closingObservation.Freeze(L::Shutdown));
 closingObservation.FinishObservation(true);
 assert(closingObservation.Current()==L::Shutdown && closingObservation.Completed()==0);
 // A safely rejected prepublication request drops admission without claiming
 // a GPU completion; a later genuine uncertain request must still enter Lost.
 L prepublicationBusy;
 assert(prepublicationBusy.Begin()==L::Accepted);
 prepublicationBusy.FinishObservation(true);
 assert(prepublicationBusy.Current()==L::Ready && prepublicationBusy.Completed()==0
  && prepublicationBusy.Failed()==0);
 assert(prepublicationBusy.Begin()==L::Accepted);
 prepublicationBusy.Finish(false);
 assert(prepublicationBusy.Current()==L::Lost && prepublicationBusy.Completed()==0
  && prepublicationBusy.Failed()==1);
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
