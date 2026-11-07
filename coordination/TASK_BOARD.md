# Task board

Consult before new work. One owner per task. Astra owns board edits; Terra reports transitions in its status/findings. Shared files are communication records, not hardware locks. Existing canonical resource locks remain authoritative for local operations.

| ID | Task | Owner | Priority | Dependencies | Status | Acceptance |
|---|---|---|---|---|---|---|
| T1 | Independent GART ownership/locking review | Terra | P1 | frozen snapshot | initial review delivered; corrections under validation | concrete defects or evidence-backed review, missing test cases, hardware procedure review |
| A1 | DMA/module/shared-aperture ownership design | Astra | P1 | kernel source inspection | active | supported lifetime mechanism and interfaces; Terra review before implementation |
| T2 | N100 submission/firmware research | Terra | P2 | T1 initial findings | delivered, integrated cd65120 | pinned sources, exact N100 implications, unknowns, first-workload validation specification |
| A2 | Resolve T1 defects and validate | Astra | P1 | T1 | R1–R4 source/host reviewed; native passed; physical stress pending | meaningful regression and native compilation; Terra validates |
| A3 | Controlled physical memory validation | Astra | P1 | A2, verified recovery connectivity | first physical30-cycle test passed; independent review/repeat pending | logs, identity, matched artifacts, display preserved, cleanup evidence |
| T3 | Review submission architecture | Terra | P2 | A1, T2 | memory-only implementation design accepted; GPU publication remains gated | recorded recommendation and unresolved gates |
| A4 | Minimal engine/context/submission implementation | Astra | P3 | A3,T3 | waiting | fixed workload, bounded timeout/reset, explicit ownership |
| T4 | Independently verify physical execution evidence | Terra | P3 | A4 | waiting | result excludes CPU fallback, repeatable physical logs |

| A5 | Kernel owner/RAM clone fixture | Astra | P1 | 0085 | native086 passed; Terra review pending | actual module admission, clone survives original deletion, clean shutdown |
| A6 | Interrupt acquisition/unwind | Astra | P1 | source review | 0086–0088 focused fixes tested; native drain passed22 rounds/two boots; close/team-exit and full teardown incomplete | actual init fault tests, explicit resource ownership, safe teardown, native build |

| A7 | Review/fix vector allocator exhaustion prerequisite | Astra | P1 | Terra IRQ source review | host independently reviewed; full kernel compiled and VM boot verified; native locking drain passed; saturation pending | no overlapping allocation on exhaustion; kernel build and native evidence |

| A8 | Reproduce retrace owner exit/exec behavior | Astra | P1 | semaphore source/API review | native2 cases passed; notification fix remains separate | blocked waiter observed before cleanup, stale operations checked, scope reviewed |

| A9 | Native close/callback overlap control | Astra | P1 | reviewed native drain fixture | one close case +11 normal regression rounds passed; team death separate | strict event order, retention, actual reopen rejection and bridge lifetime evidence |

| A10 | Establish production IRQ removal lifetime contract | Astra | P1 | A6,A9, Terra review | host no-lock defect reproduced; cleanup withheld; design options under review | admitted callback and dispatcher-tail retirement, bounded failure/retention, native review before integration |

| A11 | Protect locked dispatcher accounting against vector free | Astra | P1 | A10 review | isolated5ccc8871; host matrix and11 native regression rounds passed | independent evidence review; no late/no-lock or physical claim |

| A12 | Physical concurrent-client validation | Astra | P1 | physical-memory001/002, Terra design review | 10 physical paired rounds passed; evidence review pending | two owned live ranges, bounded join/error handling, app_server preserved, physical logs |

| A13 | Paired owner retirement coverage | Astra | P1 | A1 | host/native state tests passed; review pending | both return orders, sticky failures, stale leases and negative controls |

| A14 | Actual owner adapter lifetime harness | Astra | P1 | A13 |12 host/native modeled-backend cases passed; review pending | close during backend call, forwarding, stale rejection, failure retention |

| A15 | Physical allocation copyout-failure probe | Astra | P1 | physical matched089 | expected error and follow-up alloc/free passed; review pending | protected output unchanged, return/log evidence, display preserved |

| A16 | Offline N100 private-page-table encoding | Astra | P2 | pinned i915, Terra design review | offline candidate independently reviewed; host/native2150pass | checked39-bit DMA/48-bit GPU VA, explicit PAT index, no MMIO/publication, host/native boundary tests |

| A17 | Owned offline private-page-table construction | Astra | P2 | A16, Terra design | offline candidate reviewed;69host/native cases passed | owned backing, context lifetime, alias rejection, allocation/release faults, independent tree walks; no GPU publication |

| A18 | Kernel wired-page provider | Astra | P2 | A17, VM API review |35host mocks and native kernel compile/link passed;runtime pending | retained ownership,fault coverage,real kernel runtime before integration |

| A19 | Resident kernel page/tree integration fixture | Astra | P2 | A18,Terra review | three physical kernel cases/42areas passed;independently reviewed | three real kernel cases,cached result,retention,observed evidence |

| A20 | First GGTT nonce payload encoder | Astra | P2 | pinned packets,Terra review |32host cases independently passed;offline only | golden packet,bounds,nonmutation,no publication |

| A21 | N100 GT forcewake design | Astra | P2 | pinned i915,Terra research | exact fields/sequence independently reviewed;implementation pending | ownership,power,bounded fallback and register allowlist before implementation |
| A22 | CPU physical to GPU DMA audit | Astra | P2 | A18,fullkernel PCI/VM audit | existing PCIhelper unsuitable as DMAproof;live state pending | explicit identity/translation evidence before publication |

| A23 | Read-only DMAR snapshot/inventory | Astra | P2 | A22,ACPI lifecycle review | physical136byte snapshot independently reviewed;96host/native parser cases passed | bounded cached table,allDRHD/scopes,unknown preserved,no controller access |

| A24 | Read-only VT-d status observer | Astra | P2 | A23,Intel spec,mapping review | physicaltwo-unitTES0observations passed;79helper+3module mocks;independently reviewed | explicitUC/readonly,narrow offsets,cached observations,no writes,no identity inference |

| A25 | Enforce DMAR reserved-range exclusions | Astra | P1 | A23/A24,Terra review |physical42exclusionchecks/cleanup passed;independently reviewed | mandatorypolicy,fullpageexclusion,retention,independentphysicaloracle |

| A26 | Checked PCI power preflight and retained-owner observation session | Astra | P2 | A21,A25,Terra design review | native compiled, six archived physical sessions +20 alloc/free passed; Terra accepted | fail-closed PCI checks, existing v0 preserved, shared singleton, race tests, native build; no GT writes |

| A27 | Owner-only GT ACK observation with explicit UC mapping | Astra | P2 | A26,Terra cache/read review | native/26 mocks and eight physical samples passed; Terra accepted | v0/v1 preserved, explicit UC single owner map at boot, bounded fixed-offset reads, PCI brackets, no GT writes, host/native/physical evidence |

| A28 | Bounded owner forcewake handshake state machine | Astra | P2 | A27,Terra ownership/power review | single physical primary handshake passed;67+37 mocks and Terra evidence accepted | exact bit authority, write/posting order, definite/uncertain cleanup, deadlines, tests before standalone physical handshake; no DMA |

| A29 | Scoped engine inventory and execution initialization design | Astra | P2 | A28,Terra engine/reset research | host-tested/reviewed; native/physical blocked on laptop reset | allowlisted powered engine reads, explicit held-access lifetime, context/reset/DMA prerequisites before command publication |


| PUBLISH-01 | Publish milestone history, current evidence and remaining execution gates to private GitHub | Astra | P1 | A28 evidence, A29 host review | published to gpu/native-memory; remote SHA verified; issue #1 | preserve history; explicit host/native/physical distinctions; remote readback |
