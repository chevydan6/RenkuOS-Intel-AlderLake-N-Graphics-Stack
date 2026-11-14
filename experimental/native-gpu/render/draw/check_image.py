#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Independently decode packed words through the pinned Mesa XML, not pack macros."""
from pathlib import Path
import argparse, hashlib, json, re, struct, sys
p=argparse.ArgumentParser();p.add_argument('mesa_source',type=Path);p.add_argument('--header',type=Path,default=Path(__file__).with_name('RcsDrawGenerated.h'));a=p.parse_args()
sys.path.insert(0,str(a.mesa_source/'src/intel/genxml'))
from intel_genxml import GenXml
xml=GenXml(a.mesa_source/'src/intel/genxml/gen120.xml',import_xml=True).et.getroot()
nodes={n.attrib['name']:n for n in xml if 'name' in n.attrib}
def fields(name):return [f for f in nodes[name] if f.tag=='field']
def bounds(f):
 hi,lo=map(int,f.attrib['bits'].split(':'));return int(f.attrib['dword'])*32+lo,hi-lo+1,lo
def get(name,words,field):
 f=next(f for f in fields(name) if f.attrib['name']==field);start,width,lo=bounds(f)
 value=(sum(w<<(32*i) for i,w in enumerate(words))>>start)&((1<<width)-1)
 return value<<lo if f.attrib['type'] in ['address','offset'] else value
def match(n,w):
 for f in fields(n):
  if 'default' not in f.attrib or f.attrib['name']=='DWord Length':continue
  if int(f.attrib.get('dword','-1'))!=0:continue
  if get(n,[w],f.attrib['name'])!=int(f.attrib['default'],0):return False
 return True
s=a.header.read_text();arrays={name:[int(w,16) for w in re.findall(r'0x([0-9a-f]+)U',body)] for name,body in re.findall(r'uint32_t (\w+)\[[^]]*\] = \{(.*?)\};',s,re.S)}
inst=[n.attrib['name'] for n in xml if n.tag=='instruction']
batch=arrays['Batch']; cmds={}; pos=0
while pos<len(batch):
 candidates=[n for n in inst if match(n,batch[pos])]
 assert len(candidates)==1,(pos,hex(batch[pos]),candidates)
 name=candidates[0];node=nodes[name]
 length=int(node.attrib.get('length',0))
 if any(f.attrib['name']=='DWord Length' for f in fields(name)):
  length=get(name,[batch[pos]],'DWord Length')+int(node.attrib['bias'])
 assert length>0 and pos+length<=len(batch),(name,pos,length)
 cmds.setdefault(name,[]).append(batch[pos:pos+length]);pos+=length
assert list(cmds)[-1]=='MI_BATCH_BUFFER_END' and batch[-1]==0x05000000
assert len(cmds['3DPRIMITIVE'])==1

def check(n,f,value):
 words=cmds[n][0];actual=get(n,words,f);assert actual==value,(n,f,actual,value)
check('3DSTATE_VS','Enable',1);check('3DSTATE_VS','SIMD8 Dispatch Enable',1)
check('3DSTATE_VS','Kernel Start Pointer',0x4000);check('3DSTATE_VS','Vertex URB Entry Read Length',0)
check('3DSTATE_VS','Dispatch GRF Start Register For URB Data',2)
check('3DSTATE_PS_EXTRA','Pixel Shader Valid',1)
check('3DSTATE_PS','8 Pixel Dispatch Enable',1);check('3DSTATE_PS','16 Pixel Dispatch Enable',1)
check('3DSTATE_PS','32 Pixel Dispatch Enable',0)
check('3DSTATE_PS','Kernel Start Pointer 0',0x5000);check('3DSTATE_PS','Kernel Start Pointer 2',0x5080)
check('3DSTATE_PS','Binding Table Entry Count',1);check('3DSTATE_PS','Sampler Count',0)
check('3DSTATE_PS','Render Target Fast Clear Enable',0)
check('3DSTATE_VF_TOPOLOGY','Primitive Topology Type',4)
check('3DPRIMITIVE','Primitive Topology Type',4);check('3DPRIMITIVE','Vertex Count Per Instance',3);check('3DPRIMITIVE','Instance Count',1)
for stage in ['VS','PS']:
 check('3DSTATE_'+stage,'Scratch Space Base Pointer',0)
 check('3DSTATE_'+stage,'Per-Thread Scratch Space' if stage=='VS' else 'Per Thread Scratch Space',0)
check('STATE_BASE_ADDRESS','Dynamic State Base Address',0x401000)
check('STATE_BASE_ADDRESS','Surface State Base Address',0x402000)
check('STATE_BASE_ADDRESS','Instruction Base Address',0x400000)
check('3DSTATE_SF','Viewport Transform Enable',1)
check('3DSTATE_CLIP','Clip Enable',1)
check('3DSTATE_SBE','Number of SF Output Attributes',0)
check('3DSTATE_SBE','Vertex URB Entry Read Offset',0)
check('3DSTATE_SBE','Vertex URB Entry Read Length',0)
check('3DSTATE_VS','Maximum Number of Threads',545)
check('3DSTATE_PS','Maximum Number of Threads Per PSD',63)
for stage in ['HS','DS','GS']: check('3DSTATE_'+stage,'Enable',0)
check('3DSTATE_URB_ALLOC_VS','VS Number of URB Entries Slice0',3576)
check('3DSTATE_URB_ALLOC_VS','VS URB Starting Address Slice0',4)
check('3DSTATE_URB_ALLOC_VS','VS URB Entry Allocation Size',0)
check('3DSTATE_WM_DEPTH_STENCIL','Depth Test Enable',0)
check('3DSTATE_PS_BLEND','Color Buffer Blend Enable',0)
check('3DSTATE_PS_BLEND','Has Writeable RT',1)
ss=arrays['Surface'][16:32]
for f,value in [('Surface Type',1),('Surface Format',0xc0),('Width',31),('Height',31),('Surface Pitch',127),('Surface Base Address',0x406000),('Auxiliary Surface Mode',0),('Memory Compression Enable',0),('Tile Mode',0)]:
 assert get('RENDER_SURFACE_STATE',ss,f)==value,f
assert arrays['Surface'][0]==64
vp=arrays['Dynamic'][:16]
for field,value in [('Viewport Matrix Element m00',0x41800000),('Viewport Matrix Element m11',0x41800000),('Viewport Matrix Element m30',0x41800000),('Viewport Matrix Element m31',0x41800000)]:
 assert get('SF_CLIP_VIEWPORT',vp,field)==value,field
vb=cmds['3DSTATE_VERTEX_BUFFERS'][0][1:]
assert get('VERTEX_BUFFER_STATE',vb,'Buffer Starting Address')==0x403000
assert get('VERTEX_BUFFER_STATE',vb,'Buffer Size')==48
assert get('VERTEX_BUFFER_STATE',vb,'Buffer Pitch')==16
assert len(batch)<=1024 and len(arrays['Vs'])*4==128 and len(arrays['Fs'])*4==208
for name in ['vs','fs']:
 assert struct.pack('<'+'I'*len(arrays[name.title()]),*arrays[name.title()])==(a.header.parent.parent/'compiler/mesa-26.3.0/artifacts'/f'{name}.bin').read_bytes()
print(f'XML decoded {sum(map(len,cmds.values()))} commands / {len(batch)} dwords: enabled VS+PS, TRILIST, fixed bounded resources; CPU checks only')
