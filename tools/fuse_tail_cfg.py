#!/usr/bin/env python3
"""Fuse a reviewed, closed, call-free guest tail CFG into its original entry.
Keep every independent dynamic entry and every RET guard. No title addresses are
built into this tool. Generated inputs/outputs must remain private at the caller.
"""
import hashlib,re
from fuse_tail_fragments import FUNC,TAIL

def fuse_cfg(source,roots,max_blocks=64):
 original=source;proof=[]
 for root in roots:
  matches=list(FUNC.finditer(source));functions={m[2]:m for m in matches}
  if len(functions)!=len(matches) or root not in functions:raise ValueError('Missing or duplicate CFG entry')
  nodes={};pending=[root]
  while pending:
   address=pending.pop()
   if address in nodes:continue
   if address not in functions or len(nodes)>=max_blocks:raise ValueError('CFG is not closed or exceeds bound')
   m=functions[address];body=m[3]
   if 'urban_cfg_' in body:raise ValueError('Overlapping or already fused CFG')
   begin=body.find('\tconst uint32_t x86_expected_slot = ctx.esp;');end=body.find('\n\t// ',begin)
   if begin<0 or end<0:raise ValueError('Unknown generated entry contract')
   contract=body[begin:end]
   if contract.count('switch (x86_resume_pc)')!=1 or contract.count('case ')!=1:raise ValueError('Resume entries require independent execution')
   payload=body[:begin]+body[end:]
   remaining=TAIL.sub('',payload)
   if any(token in remaining for token in ('x86_icall(','x86_kernel(','try {','catch (','x86_trap(')) or re.search(r'\bsub_[0-9A-F]+\(ctx, base\)',remaining):raise ValueError('CFG contains a real call or unsupported scope')
   edges=[tail[1] for tail in TAIL.finditer(payload)];pending.extend(edges)
   nodes[address]=(m,payload,end)
  if len(nodes)<2:raise ValueError('No tail CFG to fuse')
  prefix='urban_cfg_'+root+'_'
  blocks=[]
  for address,(_,payload,_) in nodes.items():
   payload=re.sub(r'\bloc_[0-9A-Fa-f]+\b',lambda label:prefix+address+'_'+label[0],payload)
   payload=TAIL.sub(lambda tail:'{ X86_SAVE_FLAGS(); goto '+prefix+tail[1]+'; }',payload)
   blocks.append(prefix+address+': {\n'+payload+'\n}')
  entry,_,end=nodes[root]
  body=entry[3][:end]+'\n'+ '\n'.join(blocks)
  source=source[:entry.start(3)]+body+source[entry.end(3):]
  proof.append({'root':root,'blocks':len(nodes),'edges':sum(len(list(TAIL.finditer(p))) for _,p,_ in nodes.values())})
 return source,{'source_sha256':hashlib.sha256(original.encode()).hexdigest(),'output_sha256':hashlib.sha256(source.encode()).hexdigest(),'graphs':proof}
