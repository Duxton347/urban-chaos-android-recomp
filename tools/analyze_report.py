#!/usr/bin/env python3
"""Summarize bounded exported diagnostics without extracting files or printing game text/paths."""
import argparse,collections,hashlib,json,pathlib,re,zipfile
LIMIT=32*1024*1024
class Invalid(ValueError):pass
def check(ok):
 if not ok:raise Invalid('Invalid or oversized diagnostic report')
def numbers(row):return {k:v for k,v in row.get('detail',{}).items() if type(v)==int and re.fullmatch('[a-z][a-z0-9_]{0,40}',k)}
def analyze(path):
 path=pathlib.Path(path);check(path.stat().st_size<=2*LIMIT)
 sessions=collections.defaultdict(list);builds={}
 with zipfile.ZipFile(path) as archive:
  files=archive.infolist();check(len(files)<=256 and sum(f.file_size for f in files)<=LIMIT)
  for item in files:
   name=item.filename
   if re.fullmatch(r'sessions/[^/]+/manifest.json',name):
    check(item.file_size<=4096);data=json.loads(archive.read(item));build=data.get('build','')
    if re.fullmatch(r'experimental-[0-9a-f]{16}',build):builds[name.rsplit('/',1)[0]]=build
   if not re.fullmatch(r'sessions/[^/]+/native/runtime\.[0-9]+\.jsonl',name):continue
   check(item.file_size<=2*1024*1024)
   for line in archive.read(item).splitlines():
    check(len(line)<=16384);row=json.loads(line)
    check(row.get('schema')==1 and type(row.get('elapsed_ms'))==int and type(row.get('utc_ms'))==int and isinstance(row.get('detail'),dict))
    check(isinstance(row.get('code'),str) and re.fullmatch(r'UC-[A-Z0-9-]{1,40}',row['code']))
    sessions[name.split('/native/')[0]].append(row)
 check(bool(sessions));key=max(sessions,key=lambda k:max(r['utc_ms'] for r in sessions[k]));rows=sorted(sessions[key],key=lambda r:r['elapsed_ms']);counts=collections.Counter(r['code'] for r in rows)
 def selected(code):return [r for r in rows if r['code']==code]
 def last(code):values=selected(code);return numbers(values[-1]) if values else None
 frames=selected('UC-GPU-001');visibility=selected('UC-GPU-002');io=selected('UC-DISC-002')
 result={'build':builds.get(key),'capture_elapsed_ms':rows[-1]['elapsed_ms'],'code_counts':dict(counts),'last_frame':numbers(frames[-1]).get('frame') if frames else None,'first_visibility_elapsed_ms':visibility[0]['elapsed_ms'] if visibility else None,'last_io':last('UC-DISC-002'),'last_io_completion':last('UC-DISC-003'),'last_locale_state':last('UC-LOCALE-005'),'last_gpu_synchronization':last('UC-GPU-006'),'last_input':last('UC-INPUT-002'),'mission_playable_verified':False}
 if len(frames)>=2:
  a,b=frames[-2:];delta=b['elapsed_ms']-a['elapsed_ms'];result['last_presentations_per_second']=round((numbers(b)['frame']-numbers(a)['frame'])*1000/delta,3) if delta>0 else None
 result['reads_progressed_in_last_io_interval']=len(io)>=2 and numbers(io[-1]).get('bytes',0)>numbers(io[-2]).get('bytes',0)
 result['gpu_host_timing']={str(numbers(r)['operation']):numbers(r) for r in selected('UC-GPU-005') if 'operation' in numbers(r)}
 with path.open('rb') as file:result['report_sha256']=hashlib.file_digest(file,'sha256').hexdigest()
 return result
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('report');parser.add_argument('--reference');args=parser.parse_args();result=analyze(args.report)
 if args.reference:
  reference=analyze(args.reference);first=reference['first_visibility_elapsed_ms'];result['reference_first_visibility_elapsed_ms']=first;result['reference_first_visibility_beyond_current_capture']=first is not None and first>result['capture_elapsed_ms']
 print(json.dumps(result,indent=2))
