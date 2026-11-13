<!-- SPDX-License-Identifier: MIT -->
# Original completion/swait on native N100

Real N100, RenkuOS hrev60072+518, installed GCC13.3 and matching SDK. Payload
verification, compilation, kernel execution and complete GEM/ring compilation
attempts ran in fullscreen Terminal through NanoKVM. SSH transferred/collected
files. The completion and simple-wait-queue .c files and public headers are
unchanged Linux v6.6; a native compilation shim supplies their environment.

Four runs pass 119 checks each (476 checks, zero failures), 12 phases each and
96 total kernel waiter workers. Cases include pre-signals, counted completions,
two queued waiters receiving distinct signals, broadcast/reinitialization,
timeout, try-wait and data visibility. IRQ-disabled complete/complete_all keep
IRQs disabled. Native task events are destroyed when registered scopes exit.
Actual diagnostic unload asserts that no tasks or signal waiters remain.

Two additional actual user-thread signal cases pass: SIGUSR1 produces -512
(-ERESTARTSYS) in an interruptible wait, while a killable wait stays blocked
through SIGUSR1 and returns positive remaining ticks when completed. Its signal
handler runs after the ioctl returns. Actual kill/cancellation was not tested.
No GPU registers, GPU IRQ completion, GPU submission or user mappings are tested.

Native build uses Werror and emits no warnings. The runner removes its diagnostic
driver/link, observes actual UNINIT status 0, and compares graphics device lists.
Original display/GART/owner modules were not replaced in this checkpoint.

The full GEM/ring compile attempts also ran on RenkuOS and all returned 1; their
unaltered errors/exit files are in whole-build/. Native errors now include macro
namespace collisions (ALIGN/roundup/min/attributes), early init/might_sleep
declarations, and architecture/preemption support. No error was suppressed to
claim a complete driver build. These are later port dependencies, not failures
of the separately linked completion module.

The external source checkout was missing headers. Official Linux6.6 release
headers were verified against 6,457 regular-file Git blobs at the exact pin;
6,783 unchanged reference files were exported for native whole-source attempts.
See source-materialization.json and reference-provenance.json. The final import
manifest contains 48 unique files; a duplicate manifest entry found during this
work was removed and duplicate-path validation added.

Full payload/verification logs and compiled artifacts are retained outside Git
in the archive identified by result.json. payload-manifest.sha256 hashes the
full checksum list; adapter-sources.sha256 retains its non-reference entries.
The native payload check summary records all 6,916 successes. Host sanitizer,
mutation and component/target-probe evidence accompanies this checkpoint.
