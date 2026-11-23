#!/usr/bin/env python3
"""Fuse private generated single-owner tail fragments; retain dynamic entry mappings.
Only straight-entry fragments without resume cases or exception scopes qualify.
Original RET checks inherit the caller's real frame, preserving nonlocal transfers.
"""
import re,hashlib
FUNC=re.compile(r'(X86_FUNC\(__imp__sub_([0-9A-F]{8})\) \{\n)(.*?)(\n\})\nX86_WEAK_FUNC',re.S)
TAIL=re.compile(r'\{ X86_SAVE_FLAGS\(\); sub_([0-9A-F]{8})\(ctx, base\); return; \}')
def fuse(text):
 original=text;count=0
 for iteration in range(128):
  fs={m[2]:m for m in FUNC.finditer(text)};changes=[]
  for addr,m in fs.items():
   body=m[3]
   if 'x86_icall(' in body or 'x86_kernel(' in body or re.search(r'\bsub_[0-9A-F]+\(ctx, base\)',body):continue
   if 'try {' in body or 'catch (' in body or re.search(r'case 0x[0-9A-Fa-f]+u:',body):continue
   # Retain load flags and arithmetic temporaries, strip only the entry/continuation contract.
   begin=body.find('\tconst uint32_t x86_expected_slot = ctx.esp;')
   end=body.find('\n\t// ',begin)
   if begin<0 or end<0 or body[begin:end].count('switch (x86_resume_pc)')!=1:continue
   if body[begin:end].count('case ')!=1:continue
   # A real CALL must keep the separate function and its complete return contract.
   calls=list(re.finditer(r'\bsub_'+addr+r'\(ctx, base\)',text))
   tails=[t for t in TAIL.finditer(text) if t[1]==addr]
   if len(calls)!=1 or len(tails)!=1:continue
   tail=tails[0];owner=next((f for f in fs.values() if f.start()<tail.start()<f.end()),None)
   if owner is None or owner[2]==addr:continue
   payload=body[:begin]+body[end:]
   changes.append((tail.start(),tail.end(),'{ X86_SAVE_FLAGS(); {\n'+payload+'\n} }'))
  if not changes:break
  # Each replacement is disjoint; preserve function entry mappings and original target bodies.
  for a,b,s in sorted(changes,reverse=True):text=text[:a]+s+text[b:]
  count+=len(changes)
 else:raise ValueError('Tail fusion did not converge')
 return text,{'fused_edges':count,'source_sha256':hashlib.sha256(original.encode()).hexdigest(),'output_sha256':hashlib.sha256(text.encode()).hexdigest()}
