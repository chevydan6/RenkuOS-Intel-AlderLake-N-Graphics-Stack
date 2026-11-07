# Publication checks — 2026-11-07

All31 preparation/build/test commands completed with exit0 against an export
of the Git index before publication. This report, its link and coordination-report
whitespace cleanup followed the export; implementation and workflow inputs were
unchanged.

| Check | Observed result |
| --- | --- |
| Fresh pinned-source preparation | All96 patches applied |
| `tools/check-source.py` |918 payload hashes and unpatched GART regression passed |
| DDI pipe selection | Passed |
| Actual-owner observation |140 mocked PCI/kernel cases passed |
| Reset-domain planning |4101 cases passed |
| Forcewake group |964 mocked-I/O cases passed |
| BCS reset |431 mocked-I/O cases passed |
| Context descriptor/status |2357 offline cases passed |
| CSB reader/HWSP |135 callback-injection cases passed |
| Context-image reference fetch | All7 files downloaded at pinned Linux commit and SHA-256 verified |
| Context image |715 CPU-only cases passed, including full16KiB reference comparisons |
| GGTT range/PTE/stolen | All3 compiled programs passed |
| GART range/lifecycle/allocation and kernel resources | All4 host runners passed |
| Boot framebuffer and linear copy | Both compiled programs passed |
| Python syntax | All top-level `tools/*.py` and `tests/*.py` compiled |

New standalone C++ fixtures were compiled with warnings-as-errors and ASan/UBSan;
execution used leak detection and halt-on-error. The owner/context-image runners
also completed their sanitizer builds. Existing host runners retained their own
build options. The workflow contains reproducible source and test commands.

Publication validation also checked staged whitespace (excluding original tool
whitespace in `qwen-ppgtt-vm-review-001/platform.log` and
`reset-owner-host-vm-001/artifact-review.txt`), selected-evidence SHA-256
manifests and common credential signatures. Workstation-only configuration,
generated binaries/build trees and excluded local reports/captures were preserved
locally. These checks neither install a driver nor access the physical GPU.
Historical native and physical results are identified separately in the
[progress report](../../results/2026-11-07-reset-context-progress.md).
