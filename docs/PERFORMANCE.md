# Performance evidence

No GPU performance results exist. Hook-dispatch tests and native compilation
measure neither desktop acceleration nor GPU throughput. Linux glxinfo confirms
Linux acceleration only.

After validated GPU execution: compare CPU and GPU copies/fills at fixed surface
sizes, pitches and overlap patterns; verify every output, warm up, then record
median/p95 latency and bytes/s, CPU time, peak pinned memory and failures across
repeated runs. Include submission and synchronization costs. Compare identical
resolution/refresh/power settings and note UMA memory pressure. Save machine-
readable results keyed by source and binary hashes, and compare with the previous
validated run. Never treat an emulated VGA workload as an Intel GPU benchmark.
