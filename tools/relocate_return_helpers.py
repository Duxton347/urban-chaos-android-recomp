#!/usr/bin/env python3
"""Reviewed post-generation optimization for helpers that relocate their own return slot.
Only a matching return PC may use the native return. Other functions, continuation
entries, callback root checks and nonlocal returns keep the original contract.
"""
import argparse,hashlib,json,pathlib,re,os
GUARD='if (!x86_resuming) throw X86Transfer{ctx.return_target, ctx.esp};'
FAST='if (!x86_resuming && ctx.return_target != x86_expected_return) throw X86Transfer{ctx.return_target, ctx.esp};'
def transform(source,helpers):
 for address,digest in helpers.items():
  name='__imp__sub_'+address.upper().zfill(8)
  pattern=r'(X86_FUNC\('+name+r'\) \{.*?\n\})\nX86_WEAK_FUNC'
  matches=list(re.finditer(pattern,source,re.S))
  if len(matches)!=1:raise ValueError('Helper boundary missing or duplicated')
  m=matches[0];body=m.group(1)
  if hashlib.sha256(body.encode()).hexdigest()!=digest or body.count(GUARD)!=1:raise ValueError('Reviewed helper changed')
  source=source[:m.start(1)]+body.replace(GUARD,FAST)+source[m.end(1):]
 return source

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);p.add_argument('review',type=pathlib.Path);a=p.parse_args()
 root=pathlib.Path(__file__).resolve().parents[1]
 for path in (a.source,a.output):
  if not any((root/x) in path.resolve().parents for x in ('build','private')):raise ValueError('Generated code must remain private')
 review=json.loads(a.review.read_text());text=a.source.read_text()
 if hashlib.sha256(text.encode()).hexdigest()!=review['source_sha256']:raise ValueError('Wrong generated source')
 a.output.write_text(transform(text,review['helpers']))
if __name__=='__main__':main()
