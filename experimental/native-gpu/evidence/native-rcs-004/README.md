# RCS lifecycle boundaries and PCI diagnosis — candidate004

Native SDK build/export link and17 focused fixtures passed. The exact installed
hashes and source manifest are included. RCS bootstrap/draw, BCS copies,
quiesce, suspended DRAW rejection, resume, explicit bootstrap requirement,
rebootstrap/draw and later BCS copies all passed. Resume advanced generation1to2
without consuming a GPU sequence; the final state was Ready, sequence10,
completed10, failed0. See the runnable `rcs-boundary-004.sh` and separate logs.

The bounded window reproduced the candidate003 initial-admission failure after
55GPUframes: sequence66, stage1, PCI_UNSTABLE9, before forcewake/publication. It
latched independent CPU fallback, then closed normally and joined its worker.
The complete42-read trace in `pci-failure-trace.log` shows precisely one changing
field: PCIStatus at offset6 changed0x10to0x18. Identity, revision, command, BARs,
capability headers and D0 PMCSR were unchanged. Bit3 is PCI_STATUS_INTERRUPT,
defined by pinned Linuxv6.6 `include/uapi/linux/pci_regs.h:55`; it is volatile and
cannot serve as an immutable admission snapshot bit.

The subsequent RCS-only correction normalizes exactly that interrupt-status bit
for the two full initial captures, preserving raw all-ones failure and equality
of every other field/bit. It does not write PCI config, retry failures, change
BCS or alter Intel GPU command/init logic. Candidate004 itself intentionally
retained the strict check for diagnosis and is not the corrected build.

Full archive on host/N100: `native-rcs-boundary-evidence-004.tar.gz`, SHA256
`fd4c3be233f9a994000de32f7967be464210e58bd725985333fe43c2040045bc`.
