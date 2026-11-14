# Native RCS cooperative ownership foundation

This is a CPU policy and matching-source audit for part of Task3. It does not
establish live hardware exclusion, implement the native handoff, initialize RCS,
allocate render resources, or complete Task3. No hardware permission is supplied
by this component; no changes are installed. The existing display source and its
Intel sequences are unchanged.

## Matching display source

The audit uses the actual matching native display source copied to
`/tmp/rcs-native-display-001/`, rather than assuming the older pinned upstream
accelerant has the same behavior. Exact SHA256 identities:

| File | SHA256 |
| --- | --- |
| accelerant.cpp | `9ccd8fac29cd1d7c04846cf86d889e74536bbe1708e30175362d69037b6a1ce5` |
| engine.cpp | `c5485ca4f129a96ff174049f9f1d5113882af4c05dc45621c63786034e52f30e` |
| hooks.cpp | `656c6895042713bdfc82371a12ba3c2cbfd1864f16ba89d0d4f66824126e57e3` |
| intel_extreme.cpp | `e32ce739f36af66dea0f2c2023453549be5b875c32a81c70a62dabfbb47adcb1` |
| intel_extreme.h | `c1837851a8dc5ed527a91b42368cf9d2e575e2a8fc7eec285a724dbe585725d3` |

The legacy primary ring register base is `0x2030` (`intel_extreme.h:742`),
with tail/start/control offsets `0/8/0xc` (lines746–749). The potential legacy
producers remain in the generic source, but the matching Alder Lake profile
already excludes them through these guards:

- `accelerant.cpp:617` calls `setup_ring_buffer` during primary initialization.
  `engine.cpp:169–174` returns for ALD before lock setup or the tail/start/control
  writes at189–193. Thus the call itself is not an ALD ring write.
- `accelerant.cpp:732–736` calls `uninit_ring_buffer` on primary teardown.
  `engine.cpp:158–164` returns for ALD before its control-register write.
- `intel_extreme.cpp:852–862` excludes ALD from primary-ring aperture allocation
  and register-base initialization. Lines958–959 also guard legacy overlay/status
  allocation. Memory allocation readiness alone conveys no engine ownership.
- `hooks.cpp:17–38` denies ALD acquire/release, idle, get/sync-token and every
  listed overlay hook. `engine.cpp:202–208` reports zero engines for ALD.
- `hooks.cpp:130–147` denies Lake-family 2D blit/fill/invert and fill-span hooks.
  These gates matter: generic `QueueCommands` publishes a tail at
  `engine.cpp:40–54`, and generic blit/fill/invert paths use it at309/329/348/373.
  A sync return is not proof of completed GPU work: the generic wait returns
  early for Lake/SOC0 at247–249, and sync returns success at294–298.

The matching profile already skips legacy ring allocation, initialization and
teardown and denies its public producers. No additional accelerant patch follows
from this audit. A future adapter must validate that the running display driver
and accelerant actually match this profile, including primary/clone lifecycle
and any private entry points. These five files do not prove that firmware,
other drivers or other GT users have transferred ownership.

## Native adapter still required

`RcsOwnership::N100DisplayOnlyV1` names the above semantic profile. Its numeric
value is not authentication. Only a trusted kernel-private graphics-driver to
resident-owner path may call `AcknowledgeHandoff` after verifying the live device,
matching display profile and current display lease/generation. A userspace grant
flag, an idle register snapshot, successful GART allocation or the existing
memory-owner readiness indication cannot substitute for that acknowledgment.

The minimal remaining handoff is generation-aware grant/withdraw through the
existing trusted display lease and resident owner. All current ALD legacy skip
and hook guards must remain effective throughout the grant. A primary/clone
reopen, teardown, suspend or incompatible profile transition must close admission
under the common owner lock, drain any active request with bounded polling, and
check that the backing hold has ended before allowing conflicting activity.
A later implementation should reuse the already-guarded N100 paths; it must not
reintroduce legacy initialization or teardown writes while native RCS owns it.
Native adapter integration is absent here and supplies no implicit grant.

Every policy operation, query and associated hardware transition requires caller
serialization under the device-wide owner lock. RCS engine exclusion does not
permit independent shared GT changes: PAT/MOCS, MCR steering, general workarounds,
power and fences require coordinated compatible configuration across display,
BCS and RCS. The display initialization's `disable_fences` call
(`accelerant.cpp:584–597,641`) illustrates shared state outside the legacy ring.
Do not independently restore shared state while another engine uses it. This
foundation adds no register programming and changes no Intel hardware logic.

## CPU policy contract

`DisplayOpened(generation)` requires a nonzero increasing display generation and
no active hold; it closes admission in `Legacy`. `AcknowledgeHandoff(generation,
handoff, profile)` accepts only the current generation, the exact known profile
and a nonzero increasing handoff id. `Begin(generation, handoff, request)` accepts
one request under that grant and requires a nonzero increasing request id.
Generation, handoff and request counters span the resident owner's boot lifetime
and must not wrap. A stale completion cannot retire a later request.

`Withdraw(generation, handoff)` closes admission before draining. With an active
request it enters `Draining`; otherwise it returns to `Legacy`. A proven
`Finish(generation, handoff, request, true)` drops the backing hold but cannot
regrant withdrawn ownership. A revoked handoff id cannot be replayed. An
uncertain finish enters `Lost` retaining the hold, and no finish, grant, open or
withdraw can recover it. Reboot is the only recovery represented here. The
resident policy object must survive for the boot, including display reopen.

These ownership transitions are separate from `RequestLifecycle`'s retirement
and administrative power policy. `Finish(..., true)` must be supplied only after
the eventual engine-specific completion, cache visibility and safety checks;
this class does not infer them from ownership or a sequence value. The native
adapter must also retain referenced allocations whenever `HoldsBacking()` is
true, including after an uncertain completion.

Focused host check (no GPU access):

```sh
c++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  experimental/native-gpu/render/rcs_ownership_smoke.cpp -o /tmp/rcs-ownership-smoke
/tmp/rcs-ownership-smoke
```

The fixture covers missing/stale/profile-mismatched handoff denial, one active
request, request-id replay rejection, withdrawal before drain, revoked completion
without regrant, newer display generation isolation, and permanent lost holds.
Passing it proves CPU policy transitions only.
