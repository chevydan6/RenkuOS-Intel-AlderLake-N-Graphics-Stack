# Disposable Haiku VM compiler bootstrap

Date: host 2026-11-15 JST. Target: isolated `RenkuOS-Tests` QEMU/KVM guest
with a 16 GiB BFS workspace, no N100 PCI device. This experiment followed the
physical N100 block-cache panic recorded in
[`native-haiku-compiler-001`](../native-haiku-compiler-001/README.md). It did
not touch the physical machine, its GPU driver, or the base kernel source.

The guest booted to the desktop and opened Terminal through Team Monitor.
`sshd` exited with `PRNG is not seeded`; host SSH forwarding therefore could
not run guest jobs. Guest `wget` successfully downloaded a 46 MB tool-package
bundle from a temporary HTTP server at QEMU's `10.0.2.2`, and the guest's
SHA-256 matched the host:

```
71ba57716914915f5a589d989f779e8361b7bcd8a4b27fc540dec8f1df4911e7
```

`pkgman install -H -R -y` on the local Haiku package files did not finish.
The guest's serial log reported a `libssl.so.3` segment violation at
`SSL_CTX_set_options`, reached from `BSecureSocket::Connect` during a package
repository request. The package command's redirected log was empty. Guest
`curl` also reported `error initializing curl library`; `wget` over plain HTTP
worked. Copying the packages directly into the home package directory did not
activate their tools in the running session.

A transient QMP hotplug of `rng-random` backed by the host `/dev/urandom` and
`virtio-rng-pci` did not seed `sshd` while the guest was running. On guest
reboot, the guest entered its kernel debugger with a page fault in
`yarrow_enqueue_randomness`, called by `VirtioRNGDevice::_Enqueue`. This is a
VM random-device failure, separate from the physical N100's BFS panic and
separate from GPU execution. The VM was force-stopped, restored from its
`ready` disk/EFI checkpoint, and started again. The transient device and
package copies were discarded.

No native Mesa compiler binary or GPU result was obtained from this VM run.
The host-side SPIR-V compiler proof and physical N100 triangle evidence remain
the verified compiler and hardware checkpoints. Another native build attempt
needs a guest with working entropy/SSH and package activation, or a cross
toolchain, before running the large Mesa build again.
