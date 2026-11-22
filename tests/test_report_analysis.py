import json,pathlib,sys,tempfile,unittest,zipfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from analyze_report import analyze,Invalid
class ReportAnalysisTests(unittest.TestCase):
 def test_progress_summary_excludes_messages_and_distinguishes_unknown_locale(self):
  with tempfile.TemporaryDirectory() as temporary:
   path=pathlib.Path(temporary)/'report.zip';rows=[]
   for ms,code,detail in [(10,'UC-GPU-001',{'frame':0}),(20,'UC-DISC-002',{'bytes':10}),(30,'UC-DISC-002',{'bytes':20}),(40,'UC-GPU-001',{'frame':60})]:rows.append({'schema':1,'elapsed_ms':ms,'utc_ms':1000+ms,'code':code,'detail':detail,'template':'PRIVATE_SENTINEL','reason':'PRIVATE_SENTINEL'})
   with zipfile.ZipFile(path,'w') as z:z.writestr('sessions/authored/native/runtime.0.jsonl','\n'.join(map(json.dumps,rows)))
   result=analyze(path);self.assertEqual(result['last_frame'],60);self.assertTrue(result['reads_progressed_in_last_io_interval']);self.assertIsNone(result['last_locale_state']);self.assertFalse(result['mission_playable_verified']);self.assertNotIn('PRIVATE_SENTINEL',json.dumps(result))
 def test_invalid_event_rejected(self):
  with tempfile.TemporaryDirectory() as temporary:
   path=pathlib.Path(temporary)/'bad.zip'
   with zipfile.ZipFile(path,'w') as z:z.writestr('sessions/authored/native/runtime.0.jsonl','{}')
   with self.assertRaises(Invalid):analyze(path)
