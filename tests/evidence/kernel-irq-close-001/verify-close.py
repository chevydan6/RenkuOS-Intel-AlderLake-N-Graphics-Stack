# SPDX-License-Identifier: MIT
import pathlib,json,re,hashlib
b=pathlib.Path(__file__).parent
result=json.loads((b/'result.json').read_text())
assert result['runtime_identity_verified'] and result['client_exit']==0
assert result['clean_shutdown'] and result['process_exit_zero_after_shutdown_request']
client=(b/'client.log').read_text()
assert 'NOT PASS' not in client
m=re.search(r'^PASS close client entered=0 close=0 dispatch=1 reopen=-1 errno=(-?\d+) expected_denial=(-?\d+); serial overlap proof required$',client,re.M)
assert m and m[1]==m[2]
manifest=json.loads((b/'manifest.json').read_text())
serial_path=pathlib.Path(manifest['private_evidence'])/'serial.log'
assert hashlib.sha256(serial_path.read_bytes()).hexdigest()==manifest['files']['serial.log']
serial=serial_path.read_text(errors='replace')
pattern=r'^IRQ_CLOSE_RESULT run=(\d+) vector=(\d+) status=(-?\d+) cpu=(-?\d+) close_cpu=(-?\d+) inflight=(\d+) closed=(\d+) retained=(-?\d+) read=(-?\d+) observed=(\d+) callbacks=(\d+) timeout=(\d+) invalid=(\d+) events=(\d+),(\d+),(\d+)$'
rows=re.findall(pattern,serial,re.M)
assert len(rows)==1,rows
run,vector,status,cpu,close_cpu,inflight,closed,retained,read,observed,callbacks,timeout,invalid,enter,close,leave=map(int,rows[0])
assert run==1 and vector>=16 and status==1 and cpu==0 and close_cpu==2
assert inflight==0 and closed==1 and retained==0 and read==0
assert observed==1 and callbacks==1 and timeout==0 and invalid==0
assert 0<enter<close<leave
marker='IRQ_BRIDGE_DISPATCH_RETURN status=1 copy=0'
assert serial.count(marker)==1 and serial.count('IRQ_BRIDGE_FREE')==1
assert serial.index('IRQ_CLOSE_RESULT') < serial.index(marker) < serial.index('IRQ_BRIDGE_FREE')
events=json.loads((b/'qmp-events.json').read_text())
assert any(e.get('event')=='SHUTDOWN' and e.get('data',{}).get('guest') is True and e.get('data',{}).get('reason')=='guest-shutdown' for e in events)
verified={'passed':True,'strict_close_overlap':True,'events':[enter,close,leave],
 'retained_handler_verified':True,'bridge_free_after_dispatch_tail':True,
 'reopen_denied':True,'guest_shutdown':True,'physical_gpu_execution':False,
 'scope':'controlled close of instrumented resident callback; no team death or physical IRQ proof',
 'client_sha256':hashlib.sha256((b/'client.log').read_bytes()).hexdigest(),
 'serial_sha256':hashlib.sha256(serial_path.read_bytes()).hexdigest()}
(b/'close-verification.json').write_text(json.dumps(verified,indent=2)+'\n')
print(json.dumps(verified))
