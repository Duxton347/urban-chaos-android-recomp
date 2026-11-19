"""Authored native-loader integration fixtures; no original executable, music or assets."""
import hashlib,json,pathlib,struct,subprocess,sys,tempfile,unittest
from test_tools import xbe,xiso
BINARY=pathlib.Path(sys.argv[1]).resolve()

def executable():
    data=xbe()
    struct.pack_into('<I',data,0x180+0xAC,3)
    data[0x360+36:0x360+56]=hashlib.sha1(struct.pack('<I',0x10)+data[0x400:0x410]).digest()
    return data

class NativeSource(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=pathlib.Path(self.temp.name)
        self.xbe=executable();self.sha=hashlib.sha256(self.xbe).hexdigest()
    def tearDown(self):self.temp.cleanup()
    def write(self,name,data):p=self.root/name;p.write_bytes(data);return p
    def probe(self,path,*flags,sha=None,title='12345678',entry='11000'):
        p=subprocess.run([str(BINARY),str(path),sha or self.sha,title,entry,*flags],capture_output=True,text=True,timeout=10)
        self.assertEqual(p.stderr,'');return p.returncode,json.loads(p.stdout)
    def image(self,data=None):return xiso(payload=self.xbe if data is None else data)
    def test_text_override_size_parallel_reads_and_original_image_unchanged(self):
        iso=self.write('text.iso',xiso(name=b'12-01.ent',payload=b'authored caption archive'));before=iso.read_bytes()
        subprocess.run([str(BINARY),'--text-override',str(iso)],check=True,capture_output=True)
        self.assertEqual(iso.read_bytes(),before)
    def test_sha256_vectors_and_padding_boundaries(self):
        for size in [0,1,3,55,56,63,64,65,127,128,129,1000000]:
            data=b'a'*size;p=self.write('hash.bin',data)
            actual=subprocess.check_output([str(BINARY),'--hash',str(p)],text=True,timeout=10).strip()
            self.assertEqual(actual,hashlib.sha256(data).hexdigest(),size)
    def test_mount_xiso_and_mixed_case_folder(self):
        iso=self.write('game.iso',self.image());before=iso.stat()
        code,report=self.probe(iso);self.assertEqual(code,0);self.assertTrue(report['iso']);self.assertFalse(report['experimental']);self.assertEqual(report['version'],3)
        self.assertEqual((iso.stat().st_mtime_ns,iso.stat().st_size),(before.st_mtime_ns,before.st_size))
        folder=self.root/'folder';folder.mkdir();(folder/'dEfAuLt.XbE').write_bytes(self.xbe)
        code,report=self.probe(folder);self.assertEqual(code,0);self.assertFalse(report['iso']);self.assertEqual(report['sha256'],self.sha)
    def test_redump_partition_offset(self):
        p=self.root/'redump.iso'
        with p.open('wb') as f:f.seek(0x18300000);f.write(self.image())
        code,report=self.probe(p);self.assertEqual(code,0);self.assertEqual(report['sha256'],self.sha)
    def test_revision_requires_choice_and_entry_guard_survives_override(self):
        other=bytearray(self.xbe);other[0x184]=1
        p=self.write('revision.iso',self.image(other));code,report=self.probe(p)
        self.assertEqual(code,3);self.assertEqual(report['status'],'needs_confirmation')
        code,report=self.probe(p,'--allow-other');self.assertEqual(code,0);self.assertTrue(report['experimental']);self.assertFalse(report['gameplay_validated'])
        struct.pack_into('<I',other,0x128,0x11001^0xA8FC57AB)
        code,report=self.probe(self.write('entry.iso',self.image(other)),'--allow-other');self.assertEqual(code,2);self.assertEqual(report['code'],'entry_mismatch')
    def test_cancelled_revision_preserves_the_previous_mount(self):
        known=self.write('known.iso',self.image());other=bytearray(self.xbe);other[0x184]=1
        unknown=self.write('unknown.iso',self.image(other));code,report=self.probe(known,'--preserve-on-reject',str(unknown))
        self.assertEqual(code,0);self.assertTrue(report['mount_preserved'])
    def test_oversized_executable_is_rejected_before_loading(self):
        image=self.image();struct.pack_into('<I',image,33*2048+8,257<<20)
        path=self.root/'large.iso'
        with path.open('wb') as f:f.write(image);f.truncate(300<<20)
        code,report=self.probe(path);self.assertEqual(code,2);self.assertEqual(report['code'],'invalid_xbe')
    def test_wrong_game_and_bad_section_never_bypass(self):
        code,report=self.probe(self.write('valid.iso',self.image()),'--allow-other',title='534300f7');self.assertEqual(code,2);self.assertEqual(report['code'],'wrong_title')
        broken=bytearray(self.xbe);broken[0x400]^=1
        code,report=self.probe(self.write('digest.iso',self.image(broken)),'--allow-other');self.assertEqual(code,2);self.assertEqual(report['code'],'section_integrity')
    def test_ps2_compressed_and_truncated(self):
        for name,data,expected in [('ps2.iso',b'PLAYSTATION SYSTEM.CNF','ps2_image'),('zip.iso',b'PK\x03\x04'+bytes(100),'compressed_image'),('truncated.iso',self.image()[:33000],'invalid_disc')]:
            code,report=self.probe(self.write(name,data));self.assertEqual(code,2);self.assertEqual(report['code'],expected)
    def test_directory_cycles_bounds_and_escaping_names(self):
        for name in [b'../Default.xbe',b'bad\\name',b'bad\x00name',b'bad:name']:
            code,report=self.probe(self.write('escape.iso',xiso(name=name,payload=self.xbe)));self.assertEqual(code,2);self.assertEqual(report['code'],'invalid_disc')
        malformed=self.image();struct.pack_into('<I',malformed,33*2048+4,0xFFFFFFFF)
        self.assertEqual(self.probe(self.write('extent.iso',malformed))[0],2)
        malformed=self.image();struct.pack_into('<H',malformed,33*2048,8)
        struct.pack_into('<HHIIBB',malformed,33*2048+32,8,0,34,1,0,1);malformed[33*2048+46]=ord('a')
        self.assertEqual(self.probe(self.write('cycle.iso',malformed))[0],2)
        malformed=self.image();struct.pack_into('<H',malformed,33*2048,1)
        self.assertEqual(self.probe(self.write('overlap.iso',malformed))[0],2)
    def test_case_collisions_and_symlinks(self):
        folder=self.root/'folder';folder.mkdir();(folder/'Default.xbe').write_bytes(self.xbe);(folder/'default.xbe').write_bytes(self.xbe)
        self.assertEqual(self.probe(folder)[0],2)
        iso=self.write('actual.iso',self.image());link=self.root/'linked.iso';link.symlink_to(iso)
        self.assertEqual(self.probe(link)[0],2)
    def test_executable_bounds(self):
        for mutate in [lambda b:struct.pack_into('<I',b,0x11C,0xFFFFFFFF),lambda b:struct.pack_into('<I',b,0x118,0xFFFFFFF0),lambda b:struct.pack_into('<I',b,0x10C,0xFFFFFFFF),lambda b:struct.pack_into('<I',b,0x164,0xFFFFFF00)]:
            data=bytearray(self.xbe);mutate(data)
            self.assertEqual(self.probe(self.write('bounds.iso',self.image(data)))[0],2)
    def test_nested_parallel_reads_and_eof(self):
        data=self.image();payload=bytes(range(256))*19
        # Root entry DATA is to the left of DEFAULT.XBE; its own table has TRACK.BIN.
        struct.pack_into('<H',data,33*2048,8)
        struct.pack_into('<HHIIBB',data,33*2048+32,0,0,35,128,0x10,4);data[33*2048+46:33*2048+50]=b'DATA'
        data.extend(bytes(4*2048))
        struct.pack_into('<HHIIBB',data,35*2048,0,0,36,len(payload),0,9);data[35*2048+14:35*2048+23]=b'TRACK.BIN'
        data[36*2048:36*2048+len(payload)]=payload
        code,report=self.probe(self.write('nested.iso',data),'--read','data\\track.bin')
        self.assertEqual(code,0);self.assertEqual(report['resource_sha256'],hashlib.sha256(payload).hexdigest())

if __name__=='__main__':unittest.main(argv=[sys.argv[0]],verbosity=2)
