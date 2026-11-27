import json,pathlib,sys,tempfile,unittest
sys.path[:0]=[str(pathlib.Path(__file__).resolve().parents[1]/'tools')]
from build_locale_pack import build
from test_text_banks import authored_archive
from text_banks import Archive,TextBank,Invalid
class LocalePackTests(unittest.TestCase):
 def test_required_caption_coverage_and_no_original_strings_in_dictionary(self):
  with tempfile.TemporaryDirectory() as temporary:
   folder=pathlib.Path(temporary);archive=authored_archive(text_last=True,text_name='12-01.exttxt');source=folder/'archive';source.write_bytes(archive)
   bank=TextBank(Archive(archive).unpack('12-01.exttxt'));document=bank.template();translation=folder/'translation.json'
   row={'name':'12-01.ent','archive':str(source),'translations':str(translation),'required_keys':['dialogue']}
   translation.write_text(json.dumps(document))
   with self.assertRaises(Invalid):build([row])
   document['entries'][0]['translation']='Olá';translation.write_text(json.dumps(document));pack=build([row])
   self.assertEqual(pack[:8],b'UCPTBR01');self.assertNotIn(b'Hello',pack);self.assertIn('Olá'.encode(),pack)
   row['required_keys']=['missing-caption']
   with self.assertRaises(Invalid):build([row])
