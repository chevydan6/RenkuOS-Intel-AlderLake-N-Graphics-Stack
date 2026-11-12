# Cycle 94 — ownership repair required

Chain: `cycle-cc40de74-cacc-48c2-abd6-4bee5bc6ade1`

Integrator outcome: **repair** (`REPAIR_REQUIRED`).

Independent native rebuilding of both driver and accelerant passed (exit 0). The production-linked software regression suite passed **146 cases, 0 failures**; the request lifetime test passed. These results accept candidate compilation and software checks, but **live-owner submission entry lifetime remains unsafe**. Production submission safety is not established.

The patched local candidate is `driver-development/candidate-cycle-92`, based on driver commit `30dfdb32957215b4c59da3ac763ba14ab0c482d0` and RenkuOS dependency `4799bb5a188c4bb6bbef967287640cc21a061a0d`. The separately verified cycle 75 baseline remains unchanged. Independent rebuilding does not establish byte-identical reproducibility of the supplied artifacts.

Tests ran natively in the existing Haiku VM using software mocks/fixtures; they demonstrate no GPU execution. The implementation build reported the accelerant up to date; independent verification forced both targets to rebuild. Misleading-indentation warnings and implementation clock-skew warnings remain recorded. No new physical N100 reachability measurement, image deployment/boot experiment, installation, or verified GPU execution occurred. DMA safety and deployment readiness remain unproved; uncertain backing and existing hardware custody remain preserved.

`intel_submit_request` accepts a raw `intel_info&` and caller-supplied owner. `gLock` does not protect object lifetime while entry waits, and aggregate `open_count` does not prove the caller owns a live cookie. Existing race tests do not force both orderings or concurrent destruction; fixtures retained after free do not prove safe destruction. There is no supported GPU publication backend: valid requests return `B_NOT_SUPPORTED` without publication.

Next repair: route submission through a private lock-held helper reached via a live owner cookie, derive owner from that cookie, reject closed cookies, and protect its lifetime through return without recursive ioctl locking. Any other entry must acquire balanced scoped device/context references while the cookie is valid and guard free/teardown/unload. Add bounded deterministic submit-before-close and close-before-submit tests, expecting respectively unchanged-admission `B_NOT_SUPPORTED` then cancellation, or `B_NOT_ALLOWED` without publication. Force free/final teardown against protected entry with destruction/reference counters proving no early destruction and exactly-once eligible cleanup. Another open owner must not legitimize stale ownership or backing; preserve uncertain DMA on failures. Natively build both binaries and tests, then obtain configured independent verification.

Recorded SHA-256 identities (supplied candidate artifacts, not a byte-identity comparison with the independent rebuild):

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `intel_extreme.driver` | 326728 | `bff30e23a2ee0da8f7c18538dc509b23915bc8b3a246b59169ca3d36fac73fb4` |
| `intel_extreme.accelerant` | 1236184 | `366cf90865fefc466a4b88984ee5d046f385fdf99ba5d0718cb5b4b76d4d6864` |
| `cleanup-regression` | 220952 | `eab853a4f278ac1010a654b627bc1a2f9240553ef74bcc18047d57221e8a8154` |
| `request-lifetime-test` | 36208 | `eb9158e7a622153f873935b97a3bda2164eca190ea0f2169152f67357c99ff1f` |

Selected candidate source SHA-256 values from the build receipt:

- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/device.cpp`: `5b6663de67736283e63216ed96beb38dda8eb0f42f4cf4f74f288971093e1326`
- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/driver.h`: `86b666f7b7469c1c9073ca03322b8c9959a447571f9e2e38c80d7d46f0ff610f`
- `tests/cleanup-regression.cpp`: `36001f1b022810c9d36a3b29e871d6dea7b917770804187807f869b5ecc9fb6e`
- `tests/request-lifetime-test.cpp`: `7710609b109779d68bc3d69ca4855719fad58533cd190ea5de8909dea008dfc1`

Selected evidence references (workspace-relative; evidence and binaries are not included in this report-only publication):

- `evidence/native-cycle-94/publisher-handoff.json`
- `evidence/native-cycle-94/independent-verification.txt`
- `evidence/native-cycle-94/native-build.json`
- `cycle-ledger.json`
