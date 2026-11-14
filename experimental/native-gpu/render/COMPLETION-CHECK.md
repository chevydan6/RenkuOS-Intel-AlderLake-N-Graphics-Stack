# Administrative RCS uncertain-completion check

The existing root-only `gpu_lifecycle test-timeout` action arms one global flag.
The next accepted BCS copy retains its existing timeout/recovery behavior. The
next accepted RCS DRAW instead consumes the flag and hides its CSB tail from the
reader, reporting the request's initial cursor throughout the bounded wait.
PROBE, BOOTSTRAP and rejected DRAW calls do not consume it. No new ioctl or user
workload control is added.

The same valid shader workload is published. Its actual HWSP, commands, target
and GPU execution remain unchanged; hiding observation prevents the owner from
proving retirement. A CSB deadline or poll limit returns `B_TIMED_OUT`, leaves
the 4 KiB output zero, and enters Lost with backing and uncertain power retained.
Later requests publish nothing. Reboot is the recovery represented by this RCS
check; BCS reset is never used as RCS recovery. This exercises missing completion
evidence, not a physically hung engine or malformed commands.

The `rcs_timeout_smoke.cpp` fixture checks one-shot consumption, PROBE/BOOTSTRAP
and rejected-DRAW preservation, a hidden completed class0 CSB reaching its time
or poll bound without reading entries, and normal visible completion. It uses
CPU policy and fake CSB data only; it supplies no hardware proof.

## Initial PCI admission diagnostics

`report.pci_error` occupies the former final reserved word (offset236); report
and command sizes remain240 and4352 bytes. It records the exact
`IntelN100PciPower::Capture` result:0 Success,1 InvalidMapping,2 ReadFailed,
3 UnsupportedDevice,4 MemoryDisabled,5 InvalidBar,6 InvalidCapabilities,
7 MissingPower,8 NotD0,9 Unstable. Zero is also the initial value before a capture
is attempted; interpret it with the stage/mapping/DMA fields. The CLI prints
`pci_error` explicitly. Existing failure status and Lost/retention behavior remain
unchanged.

A failed initial RCS capture additionally writes a kernel diagnostic containing
its exact result and up to64 ordered PCI offset/width/raw-value/read-success
records. The fixed record array is512 bytes; a total count and truncation marker
identify overflow. Successful captures produce no trace. No pointer or CPU
address is logged, and hot power checks do not use this diagnostic wrapper. The
reader delegates to the existing native PCI reader; no retry or register write
is introduced. The RCS-specific interrupt-status normalization below is applied
after the raw diagnostic record is saved. The trace can distinguish a changed Status or
PMCSR observation from an identity, BAR, capability or transport failure before
GPU publication.

## Observed interrupt-status transition

The native004 reproduction captured42 PCI reads. All values matched across the
two complete captures except offset0x06: PCI Status changed from0x0010 to0x0018.
The difference is bit3, `PCI_STATUS_INTERRUPT` (Linux v6.6
`include/uapi/linux/pci_regs.h:55`), which can change as display interrupts occur.
The unchanged full-capture helper correctly returned Unstable under its exact
snapshot comparison; no RCS publication had occurred on this failure path.

`RcsPciAdmission.h` now normalizes only this bit, only for a16-bit read at
PCI offset0x06. Raw0xffff and out-of-width values remain untouched, preserving
all-ones/transport rejection. All other status bits, identity, command, BAR,
capability and power fields retain the full helper's exact checks and the same
two complete captures. Initial RCS user admission and RCS resume both use this
adapter. BCS admission, runtime native power checks and the shared full-capture
helper remain unchanged. Raw records are retained before normalization.

The focused fixture first demonstrated the exact strict-capture rejection and
failed normalized admission before the adapter was implemented (RED). It now
accepts only the interrupt-bit transition (GREEN), verifies four anchor/status
reads across the same two full captures, rejects changes to every other status
bit and command/PM fields, and preserves raw all-ones failures. This is CPU
policy verification; native confirmation belongs to the subsequent candidate.
