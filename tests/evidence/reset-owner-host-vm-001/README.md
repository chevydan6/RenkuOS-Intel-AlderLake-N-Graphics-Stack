# A33 reset-only owner integration — software evidence

Working source: `build/a33-reset-development-001/source`, copied from A32 and
extended with a fixed v7 reset-only endpoint. Not installed or yet packaged as
patch0095. Exact source/header/test identities are in `host-result.json` and
`inputs.sha256`; input checks precede native compilation.

* Host ASan/UBSan: 113 actual-owner cases, exit 0.
* Local RenkuOS VM: the same 113 cases, exit 0.
* Native kernel owner compilation/link: success with warnings-as-errors.
* Owner SHA-256: `702c8d40111fcafa2ee6604f8d499f4d74cd2402cdcac2d19bcad4018dd31b83`.
* ELF64/x86-64 DYN, sole NEEDED `_KERNEL_`, `modules` export verified.

The VM job used its transaction-lock helper; archive transfer separately held
the same lock and its hash was verified before extraction. The guest has no N100
GPU passthrough. A33 has not performed a physical engine reset. Physical A32/v6
remains installed and its recovery files are preserved.

Coverage and next integration steps are in
`research/n100-submission/BCS-RESET-DESIGN.md`.
