import hashlib, json, pathlib, struct, subprocess, sys, tempfile, unittest
from unittest import mock
import shutil
R=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'tools'))
from inspect_xbe import inspect,Invalid
from prepare_game import prepare,image_entries
from recompile_xbe import generate

def xbe():
    b=bytearray(0x410);b[:4]=b'XBEH'
    def put(o,*v):struct.pack_into('<'+'I'*len(v),b,o,*v)
    base=0x10000
    put(0x104,base,0x400,0x2000);put(0x118,base+0x180,1,base+0x360,0,0x11000^0xa8fc57ab)
    put(0x158,(base+0x3c0)^0x5b6d40b6,0,1,base+0x3a0)
    put(0x180,0x1d0,0,0x12345678)
    put(0x360,4,0x11000,0x100,0x400,0x10,base+0x398)
    b[0x398:0x39e]=b'.text\0';struct.pack_into('<8s4H',b,0x3a0,b'XAPILIB',1,0,5849,0)
    put(0x3c0,0x80000001,0);b[0x400]=0xc3
    return b

def xiso(name=b'Default.xbe',payload=None):
    payload=bytes(xbe()) if payload is None else payload
    b=bytearray(36*2048);magic=b'MICROSOFT*XBOX*MEDIA';off=32*2048
    b[off:off+20]=magic;struct.pack_into('<II',b,off+20,33,128);b[off+2028:off+2048]=magic
    struct.pack_into('<HHIIBB',b,33*2048,0,0,34,len(payload),0,len(name));b[33*2048+14:33*2048+14+len(name)]=name
    b[34*2048:34*2048+len(payload)]=payload;return b

class Tools(unittest.TestCase):
 def setUp(self):self.t=tempfile.TemporaryDirectory();self.p=pathlib.Path(self.t.name)
 def tearDown(self):self.t.cleanup()
 def file(self,name,data):p=self.p/name;p.write_bytes(data);return p
 def test_metadata(self):
    r=inspect(self.file('test.xbe',xbe()));self.assertEqual(r['kernel_import_ordinals'],[1]);self.assertEqual(r['certificate']['title_id'],'12345678');self.assertEqual(r['libraries'][0]['build'],5849);self.assertFalse(r['candidate_match']);self.assertNotIn('payload',json.dumps(r))
 def test_invalid_and_truncated(self):
    for data in (b'',b'XBEH',bytes(xbe())[:-1],b'PS2'):
     with self.subTest(size=len(data)),self.assertRaises(Invalid):inspect(self.file('bad.xbe',data))
 def test_section_integrity_digest(self):
    b=xbe();b[0x360+36:0x360+56]=hashlib.sha1(struct.pack('<I',0x10)+b[0x400:0x410]).digest()
    path=self.file('integrity.xbe',b);self.assertTrue(inspect(path)['sections'][0]['digest_match'])
    b[0x400]=0x90;path.write_bytes(b);self.assertFalse(inspect(path)['sections'][0]['digest_match'])
 def test_bad_hash(self):
    with self.assertRaisesRegex(Invalid,'SHA-256'):inspect(self.file('a.xbe',xbe()),'0'*64)
 def test_unmapped_thunk(self):
    b=xbe();struct.pack_into('<I',b,0x158,0x11500^0x5b6d40b6)
    with self.assertRaises(Invalid):inspect(self.file('a.xbe',b))
 def test_unterminated_names(self):
    b=xbe();struct.pack_into('<I',b,0x374,0x103ff);b[0x3ff]=65
    with self.assertRaises(Invalid):inspect(self.file('a.xbe',b))
 def test_folder_import_and_existing_destination(self):
    s=self.p/'folder';s.mkdir();(s/'Default.xbe').write_bytes(xbe());dest=self.p/'installed'
    self.assertEqual(prepare(s,dest)['files'],1);self.assertTrue((dest/'Default.xbe').exists())
    with self.assertRaises(Invalid):prepare(s,dest)
 def test_folder_symlink(self):
    s=self.p/'folder';s.mkdir();(s/'Default.xbe').symlink_to('/etc/passwd')
    with self.assertRaises(Invalid):prepare(s,self.p/'installed')
 def test_case_collision(self):
    s=self.p/'folder';s.mkdir();(s/'Default.xbe').write_bytes(xbe());(s/'default.xbe').write_bytes(xbe())
    with self.assertRaises(Invalid):prepare(s,self.p/'installed')
 def test_ps2_specific(self):
    with self.assertRaisesRegex(Invalid,'PS2 detectada'):prepare(self.file('ps2.iso',b'PLAYSTATION SYSTEM.CNF'),self.p/'installed')
 def test_safe_xiso_extraction(self):
    prepare(self.file('xbox.iso',xiso()),self.p/'installed');self.assertEqual((self.p/'installed/Default.xbe').read_bytes(),xbe())
 def test_redump_xgd1_offset(self):
    image=self.p/'redump.iso';base=0x18300000
    with image.open('wb') as f:f.seek(base);f.write(xiso())
    prepare(image,self.p/'installed');self.assertEqual((self.p/'installed/Default.xbe').read_bytes(),xbe())
 def test_certificate_version(self):
    b=xbe();struct.pack_into('<II',b,0x180+0xa8,2,7)
    r=inspect(self.file('version.xbe',b));self.assertEqual(r['certificate']['disc_number'],2);self.assertEqual(r['certificate']['version'],7)
 def test_xiso_path_escape(self):
    with self.assertRaisesRegex(Invalid,'Caminho inseguro'):image_entries(self.file('bad.iso',xiso(b'../bad')))
 def test_xiso_truncation(self):
    with self.assertRaises(Invalid):image_entries(self.file('bad.iso',xiso()[:34*2048+4]))
 def test_xiso_bad_descriptor(self):
    b=xiso();b[32*2048+2028]=0
    with self.assertRaises(Invalid):image_entries(self.file('bad.iso',b))
 def test_bad_hash_leaves_no_installation(self):
    s=self.p/'folder';s.mkdir();(s/'Default.xbe').write_bytes(xbe());dest=self.p/'installed'
    with self.assertRaises(Invalid):prepare(s,dest,'0'*64)
    self.assertFalse(dest.exists());self.assertEqual(list(self.p.glob('.import-*')),[])
 def test_no_space_keeps_installation_inactive(self):
    s=self.p/'folder';s.mkdir();(s/'Default.xbe').write_bytes(xbe());dest=self.p/'installed'
    with mock.patch('prepare_game.shutil.disk_usage',return_value=shutil._ntuple_diskusage(1024,1024,0)):
     with self.assertRaisesRegex(Invalid,'Espaço insuficiente'):prepare(s,dest)
    self.assertFalse(dest.exists());self.assertEqual(list(self.p.glob('.import-*')),[])
 def test_source_truncated_during_import_rolls_back(self):
    s=self.p/'folder';s.mkdir();source=s/'Default.xbe';source.write_bytes(xbe());dest=self.p/'installed';usage=shutil.disk_usage(self.p)
    def truncate(_):source.write_bytes(b'XBEH');return usage
    with mock.patch('prepare_game.shutil.disk_usage',side_effect=truncate):
     with self.assertRaisesRegex(Invalid,'interrompida/truncada'):prepare(s,dest)
    self.assertFalse(dest.exists());self.assertEqual(list(self.p.glob('.import-*')),[])
 def test_recompiler_rejects_output_outside_ignored_build(self):
    with self.assertRaisesRegex(Invalid,'build/'):generate(self.p/'fake',self.p/'config',self.p/'output')
 @unittest.skipUnless((R/'build/host/xbe_recomp').exists(),'Build host first')
 def test_real_host_recomp_deterministic(self):
    image=self.file('fixture.xbe',xbe());cfg=self.p/'config.toml';cfg.write_text('[xbe]\ncode_sections=[".text"]\npartitions=1\ngap_fill=false\n[functions]\n"0x11000"="start"\n')
    for n in (1,2):
     r=subprocess.run([str(R/'build/host/xbe_recomp'),str(cfg),str(image),str(self.p/f'out{n}')],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
    def inventory(d):return {str(f.relative_to(d)):hashlib.sha256(f.read_bytes()).hexdigest() for f in d.rglob('*') if f.is_file()}
    self.assertEqual(inventory(self.p/'out1'),inventory(self.p/'out2'));self.assertTrue(inventory(self.p/'out1'));self.assertIn('X86_FUNC(__imp__sub_00011000)',(self.p/'out1/x86_recomp.0.cpp').read_text())
 @unittest.skipUnless((R/'build/host/xbe_recomp').exists(),'Build host first')
 def test_explicit_research_variant_never_qualifies_game(self):
    (R/'build').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=R/'build') as temp:
     root=pathlib.Path(temp);image=root/'fixture.xbe';image.write_bytes(xbe());cfg=root/'config.toml';cfg.write_text('[xbe]\ncode_sections=[".text"]\npartitions=1\ngap_fill=false\n[functions]\n"0x11000"="start"\n')
     report=generate(image,cfg,root/'output',hashlib.sha256(image.read_bytes()).hexdigest())
     self.assertFalse(report['g1_qualified']);self.assertFalse(report['reference_candidate_match']);self.assertEqual(report['source_profile'],'EXPLICIT_RESEARCH_VARIANT');self.assertTrue(report['deterministic'])
 @unittest.skipUnless((R/'build/host/xbe_recomp').exists(),'Build host first')
 def test_unresolved_abi_symbol_fails(self):
    image=self.file('fixture.xbe',xbe());cfg=self.p/'config.toml';cfg.write_text('[xbe]\npartitions=1\n[abi]\nmissing=["esi"]\n')
    r=subprocess.run([str(R/'build/host/xbe_recomp'),str(cfg),str(image),str(self.p/'output')],capture_output=True,text=True);self.assertNotEqual(r.returncode,0);self.assertIn('not a [symbols] name',r.stderr);self.assertFalse((self.p/'output').exists())
if __name__=='__main__':unittest.main(verbosity=2)
