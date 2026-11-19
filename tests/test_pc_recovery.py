import hashlib,importlib.util,json,pathlib,stat,sys,tempfile,unittest,zipfile
R=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(R/'scripts'));sys.path.insert(0,str(R/'tools'))
from attempt_pc_build import extract_public_source,classify
from pc_recovery_inventory import GENERATED
class Recovery(unittest.TestCase):
 def setUp(self):self.temp=tempfile.TemporaryDirectory();self.root=pathlib.Path(self.temp.name)
 def tearDown(self):self.temp.cleanup()
 def archive(self,entries):
  p=self.root/'source.zip'
  with zipfile.ZipFile(p,'w') as z:
   for name,content in entries:z.writestr(name,content)
  return p,hashlib.sha256(p.read_bytes()).hexdigest()
 def test_source_is_preserved_and_reuse_is_idempotent(self):
  p,sha=self.archive([('CMakeLists.txt','project(A)'),('scripts/original.py','print(123)')]);dest=self.root/'source'
  extract_public_source(p,dest,sha);extract_public_source(p,dest,sha)
  self.assertEqual((dest/'scripts/original.py').read_text(),'print(123)');self.assertFalse((dest/'build/xbe-full-integration-20261014/integration-status.json').exists())
 def test_modified_staged_source_is_not_called_original(self):
  p,sha=self.archive([('CMakeLists.txt','project(A)')]);dest=self.root/'source';extract_public_source(p,dest,sha);(dest/'CMakeLists.txt').write_text('changed')
  with self.assertRaisesRegex(ValueError,'modified'):extract_public_source(p,dest,sha)
 def test_wrong_hash_leaves_destination_absent(self):
  p,sha=self.archive([('CMakeLists.txt','project(A)')]);dest=self.root/'source'
  with self.assertRaisesRegex(ValueError,'checksum'):extract_public_source(p,dest,'0'*64)
  self.assertFalse(dest.exists())
 def test_title_input_rejected_before_extracting(self):
  for name in ['Default.xbe','own.iso','ci/default.xbe.enc','owner.keystore']:
   p,sha=self.archive([('CMakeLists.txt','project(A)'),(name,b'not a real game')]);dest=self.root/'source'
   with self.subTest(name=name),self.assertRaisesRegex(ValueError,'Title input'):extract_public_source(p,dest,sha)
   self.assertFalse(dest.exists())
 def test_archive_traversal_rejected(self):
  for name in ['../escape','/absolute','C:/escape','a\\b']:
   p,sha=self.archive([('CMakeLists.txt','project(A)'),(name,'bad')]);dest=self.root/'source'
   with self.subTest(name=name),self.assertRaisesRegex(ValueError,'Unsafe'):extract_public_source(p,dest,sha)
   self.assertFalse(dest.exists())
 def test_case_collisions_rejected(self):
  p,sha=self.archive([('CMakeLists.txt','project(A)'),('README.md','a'),('readme.md','b')])
  with self.assertRaisesRegex(ValueError,'colliding'):extract_public_source(p,self.root/'source',sha)
 def test_source_without_root_cmake_is_not_build_kit(self):
  p,sha=self.archive([('README.md','source')])
  with self.assertRaisesRegex(ValueError,'CMakeLists'):extract_public_source(p,self.root/'source',sha)
 def test_symlink_rejected(self):
  p=self.root/'source.zip'
  with zipfile.ZipFile(p,'w') as z:
   z.writestr('CMakeLists.txt','project(A)');i=zipfile.ZipInfo('link');i.create_system=3;i.external_attr=(stat.S_IFLNK|0o777)<<16;z.writestr(i,'/outside')
  with self.assertRaisesRegex(ValueError,'Symlink'):extract_public_source(p,self.root/'source',hashlib.sha256(p.read_bytes()).hexdigest())
 def test_existing_unverified_directory_is_preserved(self):
  p,sha=self.archive([('CMakeLists.txt','project(A)')]);dest=self.root/'source';dest.mkdir();(dest/'user-file').write_text('keep')
  with self.assertRaisesRegex(ValueError,'overwrite'):extract_public_source(p,dest,sha)
  self.assertEqual((dest/'user-file').read_text(),'keep')
 def test_failed_build_is_not_misclassified_as_success(self):
  self.assertEqual(classify("FileNotFoundError: integration-status.json"),'MISSING_QUALIFIED_TITLE_INPUTS')
  self.assertEqual(classify('a compiler error'),'UPSTREAM_BUILD_FAILED')
 def test_generated_unit_inventory_targets_real_units(self):
  for name in ['src/recomp/gen/recomp_0007.c','generated/game/x86_recomp.1.cpp','generated/game/x86_func_mapping.cpp']:
   self.assertIsNotNone(GENERATED.search(name))
  self.assertIsNone(GENERATED.search('scripts/prepare_xbe_input.py'))
if __name__=='__main__':unittest.main(verbosity=2)
