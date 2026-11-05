#!/usr/bin/env python3
"""Test the proposed GPT geometry on a new sparse regular file, never a disk."""
import json,os,pathlib,stat,subprocess,sys
if len(sys.argv)!=5:raise SystemExit('usage: gpt-rehearsal.py PRIMARY SECONDARY DISK_BYTES NEW_OUTPUT_DIRECTORY')
primary=pathlib.Path(sys.argv[1]);secondary=pathlib.Path(sys.argv[2]);size=int(sys.argv[3]);out=pathlib.Path(sys.argv[4]).resolve()
root=pathlib.Path('/home/dan/renkuos-development/installation/private').resolve()
if out.parent!=root or out.exists() or size<=0 or size%512:raise SystemExit('use a fresh direct child of the private installation directory')
a=primary.read_bytes();b=secondary.read_bytes()
if len(a)!=34*512 or len(b)!=33*512:raise SystemExit('unexpected GPT backup lengths')
out.mkdir(mode=0o700);image=out/'disk.img'
with image.open('xb') as f:
 f.truncate(size);f.seek(0);f.write(a);f.seek(size-len(b));f.write(b)
if not stat.S_ISREG(image.lstat().st_mode):raise SystemExit('target is not a regular file')
def inspect():
 d=json.loads(subprocess.check_output(['sfdisk','--json',str(image)],text=True,timeout=20))['partitiontable']
 if d['label']!='gpt' or d['sectorsize']!=512 or len(d['partitions']) not in (3,4):raise RuntimeError('unexpected GPT')
 return d
def same(x,y,keys):return all(x.get(k)==y.get(k) for k in keys)
before=inspect();old=before['partitions'];p3=old[2];shrink=8*1024**3//512
if p3['size']<=shrink or (p3['start']+p3['size'])%2048:raise RuntimeError('unexpected p3 geometry')
newsize=p3['size']-shrink;newend=p3['start']+newsize-1;newstart=newend+1;oldend=p3['start']+p3['size']-1
if newstart%2048:raise RuntimeError('new partition is not 1 MiB aligned')
subprocess.run(['parted','---pretend-input-tty',str(image),'unit','s','resizepart','3',str(newend)+'s'],input='Yes\n',text=True,check=True,timeout=30)
resized=inspect()['partitions'];keys=('start','size','type','uuid','name')
if any(not same(old[i],resized[i],keys) for i in (0,1)):raise RuntimeError('earlier partition changed')
if not same(old[2],resized[2],('start','type','uuid','name')) or resized[2]['size']!=newsize:raise RuntimeError('p3 identity changed')
subprocess.run(['sgdisk',f'--new=4:{newstart}:{oldend}','--typecode=4:eb00','--change-name=4:RenkuOS',str(image)],check=True,timeout=30)
subprocess.run(['sgdisk','--verify',str(image)],check=True,timeout=30)
after=inspect()['partitions']
if len(after)!=4 or any(not same(resized[i],after[i],keys) for i in range(3)):raise RuntimeError('existing partition changed')
if after[3]['start']!=newstart or after[3]['size']!=shrink or after[3]['type']!='42465331-3BA3-10F1-802A-4861696B7521':raise RuntimeError('new BFS partition geometry wrong')
record={'gpt_only':True,'btrfs_payload_present':False,'btrfs_shrink_tested':False,'source_disk_changed':False,'old_p3_sectors':p3['size'],'new_p3_sectors':newsize,'new_partition_start':newstart,'new_partition_sectors':shrink,'existing_partition_identities_preserved':True}
(out/'result.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record,indent=2))
