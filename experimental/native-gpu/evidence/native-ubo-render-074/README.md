# Native N100 UBO draw, checkpoint 074

## Result

The physical N100 executed two version-16 native RCS draws using the exact
Mesa 26.3.0-generated 336-byte fragment program. Binding-table entry 1
addressed a separate 16-byte uniform resource. The first draw wrote
`ffff4000`, the second `ff00ff80`; each matched 777/777 checked pixels and
the resource readback matched the returned image. Both completed with matching
GPU sequence numbers and no reported fault. See `physical-ubo.log`.

The version-16 request initially failed before submission because the old
execution bridge routed it without its resource session. The matching owner
and bridge pair resolved this; temporary validation instrumentation was
removed from the canonical source before this checkpoint. The physical hashes
in `physical-hashes.log` match the VM build outputs. The previous owner and
bridge are retained under `/boot/home/native-ubo-074/rollback` on the N100.

The existing physical Vulkan regression scripts passed 124 + 28 + 16 + 30 +
24 = 222 draws. The two UBO draws bring this checkpoint's observed total to
224. Their per-case output is in `regression/*.log`.

This proves native owner-side uniform binding and GPU shader reads. Vulkan
descriptor-set support is a subsequent userspace increment.

## Reproduce

The VM source bundle is `/boot/home/test-work/owner-ubo-074`. Its
`build-execution.sh` passed; see `owner-vm-build.log`. The physical probe is
`/boot/home/native-ubo-074/gpu_render_uniform`:

```sh
ssh renkuos-n100 'LIBRARY_PATH=/boot/home/config/lib:/boot/system/lib timeout 20 /boot/home/native-ubo-074/gpu_render_uniform'
```

The exact shader header has 84 little-endian words and SHA-256
`03179b37bcd77cf5399f298622c91e9a46d55dc8f79e1a518fb85fc729636f92`.

## Source closure

`stack-manifest.json` combines the pinned compiler file list, this owner
build's depfiles, and the previous Vulkan runtime manifest. Its selected
canonical runtime source union, deduplicated by content and excluding SDK,
generated files, and probes, is **12,648,796 bytes** (12.65 MB decimal).
The previous 071 measurement was 12,637,741 bytes, so this increment adds
11,055 bytes under that measure. This is not a complete system source total:
the manifest lists unmeasured kernel/build inputs. The development model's
tokenizer is unavailable here, so no model-token count is claimed.
