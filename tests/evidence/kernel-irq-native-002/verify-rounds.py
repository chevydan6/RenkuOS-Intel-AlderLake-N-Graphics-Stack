# SPDX-License-Identifier: MIT
import json, pathlib, re, hashlib
base = pathlib.Path(__file__).parent
result = json.loads((base / 'result.json').read_text())
assert result['runtime_identity_verified'] and result['native_irq_test_started']
assert result['client_exit'] == 0 and result['process_exit_zero_after_shutdown_request']
text = (base / 'client.log').read_text()
pattern = re.compile(r"^round run=(\d+) control=(\d+) finish=(-?\d+) passed=(\d+) vector=(\d+) cpus=(-?\d+),(-?\d+),hook:(-?\d+) callbacks=(\d+) timeout=(\d+) events=(\d+),(\d+),(\d+),(\d+) debug=(\d+),(\d+),(\d+)$", re.M)
rows = [list(map(int, match)) for match in pattern.findall(text)]
assert len(rows) == 11
for expected, row in enumerate(rows, 1):
    run, control, finish, passed, vector, dispatch_cpu, remove_cpu, hook_cpu, callbacks, timeout, enter, contention, leave, removed, debug_spin, debug_cont, debug_irq = row
    assert run == expected and control == (run == 1)
    assert finish == 0 and passed == 1 and vector >= 16
    assert (dispatch_cpu, remove_cpu, hook_cpu) == (0, 1, 1)
    assert callbacks == 1 and timeout == 0
    assert (debug_spin, debug_cont, debug_irq) == (1, 0, 1)
    if control:
        assert contention == 0 and 0 < enter < leave < removed
    else:
        assert 0 < enter < contention < leave < removed
assert 'NOT PASS' not in text and 'INCONCLUSIVE' not in text
assert 'PASS native IRQ drain fixture; software dispatch, no GPU execution' in text
verified = {'rounds': len(rows), 'sequential_controls': 1, 'observed_contention_rounds': 10,
    'all_event_orders_verified': True, 'physical_gpu_execution': False,
    'client_log_sha256': hashlib.sha256((base / 'client.log').read_bytes()).hexdigest()}
(base / 'round-verification.json').write_text(json.dumps(verified, indent=2) + '\n')
print(json.dumps(verified))
