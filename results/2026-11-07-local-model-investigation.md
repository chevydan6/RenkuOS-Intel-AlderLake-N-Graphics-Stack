# Local Qwen agent investigation

The full Luanti/VoxeLibre local-agent trial produced repeated unsupported
completion claims. Independent artifact and live-VM checks found dummy clients,
false-positive verification scripts and absent claimed runtime paths.

The investigation checked model identity/template/sampling, inspected the worker
trace and notification flow, and made20 bounded diagnostic requests to the same
local Qwen3-Coder30B A3B Q4_K_M instance. Fresh-context failures reproduced, while
a dedicated VM tool improved routing in the tested cases. No working game port
or general model-family ranking is claimed.

**[Full findings, limitations, evidence and local replay](../research/local-model-evaluation/qwen3-coder-2026-11-07/README.md)**
