import unittest,sys,pathlib,tempfile,json,zipfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from analyze_report import analyze
class ThreadCpuReport(unittest.TestCase):
 def test_cpu_time_is_separate_from_wall_time(self):
  with tempfile.TemporaryDirectory() as folder:
   p=pathlib.Path(folder)/'report.zip';rows=[]
   for ms,ns in [(1000,100000000),(11000,9100000000)]:
    for slot,valid in [(1,1),(2,0)]:rows.append({'schema':1,'elapsed_ms':ms,'utc_ms':ms,'code':'UC-CPU-003','detail':{'slot':slot,'kind':1,'cpu_valid':valid,'cpu_ns':ns,'transfers':ms//1000,'resume_steps':ms//500}})
   with zipfile.ZipFile(p,'w') as z:z.writestr('sessions/authored/native/runtime.0.jsonl','\n'.join(json.dumps(r) for r in rows))
   result=analyze(p)['last_thread_cpu'];self.assertEqual(len(result),1);self.assertEqual(result[0]['cpu_percent_of_one_core'],90);self.assertEqual(result[0]['slot'],1)
   windows=analyze(p)['thread_cpu_windows'];self.assertEqual(len(windows),1);self.assertEqual(windows[0]['median_cpu_percent_of_one_core'],90)
   total=analyze(p)['sampled_thread_cpu'][0];self.assertEqual(total['cpu_seconds'],9);self.assertEqual(total['cpu_percent_of_one_core'],90);self.assertEqual(total['samples'],2)
   self.assertEqual(analyze(p)['last_thread_execution'],[{'slot':1,'transfers':11,'resume_steps':22}])
