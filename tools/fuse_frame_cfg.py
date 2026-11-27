#!/usr/bin/env python3
"""Join reviewed intraprocedural tail entries, retaining real calls and dynamic mappings.
The caller supplies an independently reviewed original function extent. Only its
entry changes; external entries/continuations stay available for nonlocal resumes.
"""
import re,hashlib
from fuse_tail_fragments import FUNC,TAIL

def fuse_frame(source,root,end,max_blocks=128):
 original=source;matches=list(FUNC.finditer(source));functions={m[2]:m for m in matches}
 if len(functions)!=len(matches):raise ValueError('Duplicate function entries')
 if root not in functions or int(root,16)>=end:raise ValueError('Invalid reviewed function extent')
 nodes={};pending=[root]
 catch='\n\t} catch (const X86Resume& r) {'
 while pending:
  address=pending.pop()
  if address in nodes:continue
  if address not in functions or len(nodes)>=max_blocks:raise ValueError('Missing frame fragment or graph exceeds bound')
  body=functions[address][3];start=body.find('\n\t// ')
  if start<0 or body[:start].count('switch (x86_resume_pc)')!=1 or 'const uint32_t x86_expected_slot = ctx.esp;' not in body[:start]:raise ValueError('Unknown fragment entry contract')
  at=body.find(catch);suffix=body[at:] if at>=0 else '';payload=body[start:at if at>=0 else len(body)]
  if ('try {' in payload or 'catch (' in payload or 'x86_resume(' in payload):raise ValueError('Nested exception scope requires separate review')
  addresses=[int(a,16) for a in re.findall(r'// ([0-9A-F]{8}) ',payload)]
  if not addresses or any(a<int(root,16) or a>=end for a in addresses):raise ValueError('Fragment escapes reviewed machine-function extent')
  if not re.search(r'\breturn;',payload):raise ValueError('Fragment may fall through into another block')
  edges=[m[1] for m in TAIL.finditer(payload) if int(root,16)<=int(m[1],16)<end];pending.extend(edges)
  nodes[address]=(body[:start],payload,suffix,edges)
 if len(nodes)<2:raise ValueError('No intraprocedural tail graph')
 prefix='urban_frame_'+root+'_'
 def labels(text,address):return re.sub(r'\bloc_[0-9A-Fa-f]+\b',lambda m:prefix+address+'_'+m[0],text)
 blocks=[]
 for address,(entry,payload,_,_) in nodes.items():
  payload=labels(payload,address)
  payload=TAIL.sub(lambda m:'{ X86_SAVE_FLAGS(); goto '+prefix+m[1]+'; }' if m[1] in nodes else m[0],payload)
  reload='' if address==root else 'cf=ctx.cf;zf=ctx.zf;sf=ctx.sf;of=ctx.of;pf=ctx.pf;ca=cb=0;\n'
  if address!=root:
   expected=re.search(r'const uint32_t x86_expected_return\s*=\s*([^;]+);',entry)
   if not expected:raise ValueError('Unknown original fragment return value')
   reload+='const uint32_t urban_fragment_slot=ctx.esp, urban_fragment_return='+re.sub(r'\bx86_resuming\b','false',expected[1])+';\n'
   # A removed transfer used to serialize flags before unwinding. Preserve that
   # context write even when the enclosing machine function can now return locally.
   guard=r'if\s*\(\s*x86_resuming\s*\|\|\s*ctx\.return_target\s*!=\s*x86_expected_return\s*\|\|\s*ctx\.return_slot\s*!=\s*x86_expected_slot\s*\)'
   payload=re.sub(guard,lambda m:'if(ctx.return_target!=urban_fragment_return||ctx.return_slot!=urban_fragment_slot){X86_SAVE_FLAGS();}\n'+m[0],payload)
  blocks.append(prefix+address+': {\n'+reload+payload+'\nreturn;\n}')
 head,payload,suffix,_=nodes[root];rewritten=labels(head,root)+'\n'+'\n'.join(blocks)+suffix
 m=functions[root];source=source[:m.start(3)]+rewritten+source[m.end(3):]
 return source,{'source_sha256':hashlib.sha256(original.encode()).hexdigest(),'output_sha256':hashlib.sha256(source.encode()).hexdigest(),'root':root,'end':f'{end:08X}','blocks':len(nodes),'edges':sum(len(n[3]) for n in nodes.values()),'external_mappings_unchanged':True,'real_calls_preserved':True}
