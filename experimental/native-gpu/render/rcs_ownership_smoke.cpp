// SPDX-License-Identifier: MIT
#include "RcsOwnership.h"
#include <assert.h>
#include <stdio.h>
using NativeGpu::RcsOwnership;
static const RcsOwnership::Profile profile = RcsOwnership::N100DisplayOnlyV1;
int main()
{
 RcsOwnership p;
 assert(p.Current() == RcsOwnership::Legacy && !p.HoldsBacking());
 assert(p.Begin(1, 1, 1) == RcsOwnership::Unavailable);
 assert(!p.DisplayOpened(0));
 assert(!p.AcknowledgeHandoff(1, 1, profile));
 assert(p.DisplayOpened(7));
 assert(!p.DisplayOpened(7) && !p.DisplayOpened(6));
 // An idle legacy ring, allocation readiness or an unknown profile is no grant.
 assert(p.Begin(7, 1, 1) == RcsOwnership::Unavailable);
 assert(!p.AcknowledgeHandoff(7, 0, profile));
 assert(!p.AcknowledgeHandoff(6, 1, profile));
 assert(!p.AcknowledgeHandoff(7, 1, RcsOwnership::UnknownProfile));
 assert(p.AcknowledgeHandoff(7, 10, profile));
 assert(!p.AcknowledgeHandoff(7, 11, profile));
 assert(p.Begin(6, 10, 1) == RcsOwnership::Unavailable);
 assert(p.Begin(7, 9, 1) == RcsOwnership::Unavailable);
 assert(p.Begin(7, 10, 1) == RcsOwnership::Accepted && p.HoldsBacking());
 assert(p.Begin(7, 10, 1) == RcsOwnership::Busy);
 assert(!p.DisplayOpened(8));
 assert(!p.Finish(6, 10, 1, false) && !p.Finish(7, 9, 1, false));
 assert(!p.Withdraw(6, 10) && !p.Withdraw(7, 9));
 assert(p.Current() == RcsOwnership::Granted && p.HoldsBacking());
 assert(p.Finish(7, 10, 1, true));
 assert(!p.HoldsBacking() && p.Current() == RcsOwnership::Granted);
 assert(!p.Finish(7, 10, 1, false));
 assert(p.Begin(7, 10, 1) == RcsOwnership::Unavailable);
 assert(p.Begin(7, 10, 2) == RcsOwnership::Accepted);
 assert(!p.Finish(7, 10, 1, true));
 assert(p.Withdraw(7, 10));
 assert(p.Current() == RcsOwnership::Draining && p.HoldsBacking());
 assert(p.Begin(7, 10, 1) == RcsOwnership::Unavailable);
 assert(!p.AcknowledgeHandoff(7, 11, profile));
 assert(p.Finish(7, 10, 2, true));
 assert(p.Current() == RcsOwnership::Legacy && !p.HoldsBacking());
 assert(p.Begin(7, 10, 1) == RcsOwnership::Unavailable);
 assert(!p.AcknowledgeHandoff(7, 10, profile)); // revoked token cannot replay
 assert(p.AcknowledgeHandoff(7, 11, profile));
 assert(p.Withdraw(7, 11) && !p.HoldsBacking()); // idle withdrawal
 assert(p.DisplayOpened(8));
 assert(!p.Withdraw(7, 11));
 assert(!p.AcknowledgeHandoff(8, 11, profile));
 assert(p.AcknowledgeHandoff(8, 12, profile));
 assert(!p.Withdraw(7, 12) && !p.Finish(7, 12, 1, false));
 assert(p.Begin(8, 12, 0) == RcsOwnership::Unavailable);
 assert(p.Begin(8, 12, 3) == RcsOwnership::Accepted);
 assert(p.Withdraw(8, 12));
 assert(p.Finish(8, 12, 3, false)); // uncertain revoked work retains the hold
 assert(p.Current() == RcsOwnership::Lost && p.HoldsBacking());
 assert(!p.Finish(8, 12, 3, true));
 assert(!p.DisplayOpened(9) && !p.AcknowledgeHandoff(8, 13, profile));
 assert(!p.Withdraw(8, 12));
 assert(p.Begin(8, 12, 1) == RcsOwnership::Unavailable);
 RcsOwnership lostGranted;
 assert(lostGranted.DisplayOpened(1));
 assert(lostGranted.AcknowledgeHandoff(1, 1, profile));
 assert(lostGranted.Begin(1, 1, 1) == RcsOwnership::Accepted);
 assert(lostGranted.Finish(1, 1, 1, false));
 assert(lostGranted.Current() == RcsOwnership::Lost && lostGranted.HoldsBacking());
 puts("Native RCS ownership smoke: PASS (CPU policy only)");
}
