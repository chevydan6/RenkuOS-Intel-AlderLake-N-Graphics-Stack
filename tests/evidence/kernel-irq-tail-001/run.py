# SPDX-License-Identifier: MIT
# Fresh disposable image only; all resource locks held until VM cleanup.
import fcntl, hashlib, json, pathlib, re, shutil, socket, stat, subprocess, time
B = pathlib.Path('/home/dan/renkuos-development/installation/private/kernel-irq-tail-001')
ROOT = pathlib.Path('/home/dan/renkuos-development')
SRC = ROOT / 'source/gpu-kernel-validation'
BASE = ROOT / 'installation/private/kernel-boot-001'
RECOVERY = ROOT / 'installation/private/gpu-hdmi-build-040'
IMAGE = 'ghcr.io/renkuos/cross-compiler@sha256:4500c502c2d5298a74ac79a593b82da59b6e14badbb4095c104a110db718d969'
locks = []
for name in ('host-config', 'haiku-vm', 'build'):
    f = open('/home/dan/haiku-development/coordination/locks/' + name + '.lock', 'a')
    fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
    locks.append(f)
assert subprocess.run(['pgrep', '-f', '^qemu-system-x86_64 '], capture_output=True).returncode != 0
assert not (B / 'disk.img').exists(), 'Never reuse an existing experiment disk'
build = json.loads((ROOT / 'gpu-driver/tests/evidence/kernel-irq-tail-build-002.json').read_text())
baseline = json.loads((BASE / 'baseline.json').read_text())
def sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''): h.update(block)
    return h.hexdigest()
def save_json(name, data):
    (B / name).write_text(json.dumps(data, indent=2) + '\n')
def run(args, log, **kwargs):
    with open(B / log, 'w') as f:
        subprocess.run(args, check=True, stdout=f, stderr=subprocess.STDOUT, **kwargs)
for name, artifact in build['artifacts'].items():
    assert sha(artifact['path']) == artifact['sha256'], name
assert sha(BASE / 'original-system.hpkg') == baseline['package_sha256']
shutil.copy2(BASE / 'original-system.hpkg', B / 'candidate-system.hpkg')
shutil.copy2(BASE / 'known-hosts', B / 'known-hosts')
shutil.copy2(RECOVERY / 'vars.fd', B / 'vars.fd')
subprocess.run(['cp', '--reflink=auto', '--sparse=always', str(RECOVERY / 'disk.img'), str(B / 'disk.img')], check=True)
for name in ('bridge', 'client'):
    shutil.copy2(build['artifacts'][name]['path'], B / ('irq_drain_' + name))
container = ['podman', 'run', '--rm', '-i', '--network=none', '-v', str(SRC) + ':/src', '-v', str(B) + ':/evidence', IMAGE]
pkg = '/src/generated.kernel/objects/linux/x86_64/release/tools/package/package'
(B / 'candidate-extracted').mkdir()
run(container + [pkg, 'add', '-f', '-1', '-C', '/src/generated.kernel/objects/haiku/x86_64/release/system/kernel/revisioned', '/evidence/candidate-system.hpkg', 'kernel_x86_64'], 'package-update.log', timeout=120)
run(container + [pkg, 'extract', '-C', '/evidence/candidate-extracted', '/evidence/candidate-system.hpkg'], 'package-extract.log', timeout=180)
run(container + [pkg, 'list', '-i', '/evidence/candidate-system.hpkg'], 'package-info.txt', timeout=30)
run(container + [pkg, 'list', '-a', '/evidence/candidate-system.hpkg'], 'package-toc.txt', timeout=30)
def entries(directory):
    result = {}
    for p in directory.rglob('*'):
        info = p.lstat()
        key = str(p.relative_to(directory))
        result[key] = [stat.S_IFMT(info.st_mode), stat.S_IMODE(info.st_mode),
            str(p.readlink()) if p.is_symlink() else sha(p) if p.is_file() else None]
    return result
old = entries(BASE / 'original-extracted-verified')
new = entries(B / 'candidate-extracted')
assert old.keys() == new.keys()
differences = [key for key in old if old[key] != new[key]]
assert differences == ['kernel_x86_64'], differences
assert sha(B / 'candidate-extracted/kernel_x86_64') == build['artifacts']['kernel']['sha256']
package_hash = sha(B / 'candidate-system.hpkg')
save_json('package-comparison.json', {'entries': len(old), 'changed_entries': differences, 'candidate_package_sha256': package_hash, 'candidate_kernel_sha256': build['artifacts']['kernel']['sha256'], 'scope': 'files, modes, symlinks; not every attribute byte'})
parts = json.loads(subprocess.check_output(['sfdisk', '--json', str(B / 'disk.img')], text=True))['partitiontable']
bfs = [p for p in parts['partitions'] if p['type'].lower() == '42465331-3ba3-10f1-802a-4861696b7521']
assert len(bfs) == 1
start = bfs[0]['start'] * parts['sectorsize']; end = start + bfs[0]['size'] * parts['sectorsize']
save_json('partition-bounds.json', {'start': start, 'end': end})
bfs_command = container + ['/src/generated.kernel/objects/linux/x86_64/release/tools/bfs_shell/bfs_shell', '--start-offset', str(start), '--end-offset', str(end), '/evidence/disk.img']
package_path = '/myfs/system/packages/' + baseline['system_package']
def bfs_run(commands, label):
    (B / (label + '.commands')).write_text(commands)
    run(bfs_command, label + '.log', input=commands, text=True, timeout=120)
bfs_run('cp ' + package_path + ' :/evidence/offline-original.hpkg\ncp /myfs/system/packages/administrative/activated-packages :/evidence/activated-before.txt\nquit\n', 'offline-before')
assert sha(B / 'offline-original.hpkg') == baseline['package_sha256']
bfs_run('cp -f :/evidence/candidate-system.hpkg ' + package_path + '\nquit\n', 'offline-edit')
bfs_run('cp ' + package_path + ' :/evidence/offline-candidate.hpkg\ncp /myfs/system/packages/administrative/activated-packages :/evidence/activated-after.txt\nquit\n', 'offline-after')
assert sha(B / 'offline-candidate.hpkg') == package_hash
assert (B / 'activated-before.txt').read_bytes() == (B / 'activated-after.txt').read_bytes()
print('Offline candidate package verified; starting four-CPU disposable VM.', flush=True)
qmp_socket=None
qmp_file=None
qmp_events=[]
options = ['-o', 'BatchMode=yes', '-o', 'StrictHostKeyChecking=yes', '-o', 'HostKeyAlias=renku-install-rehearsal', '-o', 'UserKnownHostsFile=' + str(B / 'known-hosts'), '-o', 'ConnectTimeout=3', '-i', '/home/dan/.ssh/id_ed25519']
ssh = ['ssh', '-p', '10524', *options, 'user@127.0.0.1']
scp = ['scp', '-P', '10524', *options]
qmp = B / 'qmp.sock'
cmd = ['qemu-system-x86_64', '-name', 'renku-irq-close-001', '-enable-kvm', '-cpu', 'host', '-smp', '4', '-m', '1024', '-drive', 'if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd', '-drive', f'if=pflash,format=raw,file={B}/vars.fd', '-drive', f'file={B}/disk.img,format=raw,if=ide,index=0', '-netdev', 'user,id=net,hostfwd=tcp:127.0.0.1:10524-:22', '-device', 'e1000,netdev=net,addr=0x3', '-vga', 'none', '-device', 'VGA,addr=0x2,vgamem_mb=64,xres=1280,yres=800', '-display', 'none', '-qmp', f'unix:{qmp},server=on,wait=off', '-serial', f'file:{B}/serial.log']
result = {'source_commit': build['source_commit'], 'physical_gpu_execution': False, 'native_irq_test_started': False, 'native_irq_test_passed': False, 'clean_shutdown': False}
vm = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=open(B / 'qemu.log', 'w'))
try:
    for n in range(80):
        if vm.poll() is not None: raise RuntimeError('VM exited during boot')
        probe = subprocess.run(ssh + ['uname -a'], capture_output=True, text=True, timeout=10)
        if probe.returncode == 0: break
        time.sleep(2)
    else: raise RuntimeError('VM SSH deadline exceeded')
    def remote(command, timeout=30):
        return subprocess.check_output(ssh + [command], text=True, timeout=timeout)
    result['uname'] = remote('uname -a')
    assert 'Haiku revision: ' + build['revision_label'] + ', debug level:' in (B / 'serial.log').read_text(errors='replace')
    assert remote('sha256sum /boot/system/kernel_x86_64').split()[0] == build['artifacts']['kernel']['sha256']
    assert remote('sha256sum /boot/system/packages/' + baseline['system_package']).split()[0] == package_hash
    assert remote('id -u').strip() == '0'
    result['runtime_identity_verified'] = True
    save_json('result.json', result)
    remote('mkdir /boot/home/irq-drain-test')
    for name in ('bridge', 'client'):
        subprocess.run(scp + [str(B / ('irq_drain_' + name)), 'user@127.0.0.1:/boot/home/irq-drain-test/'], check=True, timeout=30)
        assert remote('sha256sum /boot/home/irq-drain-test/irq_drain_' + name).split()[0] == build['artifacts'][name]['sha256']
    install = 'set -e; base=/boot/system/non-packaged/add-ons/kernel/drivers; test ! -e "$base/bin/irq_drain_bridge"; test ! -e "$base/dev/misc/irq_drain_test"; mkdir -p "$base/bin" "$base/dev/misc"; cp /boot/home/irq-drain-test/irq_drain_bridge "$base/bin/irq_drain_bridge"; chmod 600 "$base/bin/irq_drain_bridge"; ln -s ../../bin/irq_drain_bridge "$base/dev/misc/irq_drain_test"; chmod 700 /boot/home/irq-drain-test/irq_drain_client'
    remote(install)
    qmp_socket=socket.socket(socket.AF_UNIX)
    qmp_socket.settimeout(5);qmp_socket.connect(str(qmp))
    qmp_file=qmp_socket.makefile('rwb',buffering=0)
    qmp_file.readline();qmp_file.write(b'{"execute":"qmp_capabilities"}\n')
    while True:
        message=json.loads(qmp_file.readline())
        if 'event' in message:qmp_events.append(message)
        if 'return' in message:break
    print('Kernel/bridge/client identities verified; executing native software-dispatch fixture.', flush=True)
    result['native_irq_test_started'] = True; save_json('result.json', result)
    try:
        test = subprocess.run(ssh + ['/boot/home/irq-drain-test/irq_drain_client'], capture_output=True, text=True, timeout=35)
    except subprocess.TimeoutExpired as exc:
        def decode(value):
            return value.decode(errors='replace') if isinstance(value, bytes) else value or ''
        (B/'client.log').write_text(decode(exc.stdout)+decode(exc.stderr))
        result['client_timeout']=True;save_json('result.json',result)
        raise
    (B / 'client.log').write_text(test.stdout + test.stderr)
    result['client_exit'] = test.returncode
    result['native_irq_test_passed'] = False  # Requires post-shutdown serial verifier.
    save_json('result.json', result)
    print(test.stdout + test.stderr, flush=True)
    # Even a failed/inconclusive fixture gets a controlled shutdown if responsive.
    shutdown = subprocess.run(ssh + ['sync; shutdown -s'], timeout=15, capture_output=True, text=True)
    result['shutdown_ssh_status']=shutdown.returncode
    (B/'shutdown-ssh.log').write_text(shutdown.stdout+shutdown.stderr)
    deadline=time.monotonic()+60
    while time.monotonic()<deadline:
        try:
            line=qmp_file.readline()
            if not line:break
            message=json.loads(line)
            if 'event' in message:qmp_events.append(message)
            if message.get('event')=='SHUTDOWN':break
        except socket.timeout:
            break
    vm.wait(timeout=60)
    save_json('qmp-events.json',qmp_events)
    result['process_exit_zero_after_shutdown_request']=vm.returncode==0
    result['clean_shutdown']=vm.returncode==0 and any(event.get('event')=='SHUTDOWN' and event.get('data',{}).get('guest') is True and event.get('data',{}).get('reason')=='guest-shutdown' for event in qmp_events)
    save_json('result.json', result)
    subprocess.run(['python3',str(B/'verify-rounds.py')],check=True,timeout=10)
    result['native_irq_test_passed']=True
    save_json('result.json',result)
    assert result['clean_shutdown']
finally:
    if vm.poll() is None:
        try:
            if qmp_file is not None:
                qmp_file.write(b'{"execute":"quit"}\n')
            else:
                with socket.socket(socket.AF_UNIX) as cleanup:
                    cleanup.settimeout(5);cleanup.connect(str(qmp));f=cleanup.makefile('rwb',buffering=0)
                    f.readline();f.write(b'{"execute":"qmp_capabilities"}\n');f.readline();f.write(b'{"execute":"quit"}\n')
            vm.wait(timeout=10)
        except (OSError,subprocess.TimeoutExpired):
            vm.kill();vm.wait(timeout=10)
    if qmp_file is not None:qmp_file.close()
    if qmp_socket is not None:qmp_socket.close()
