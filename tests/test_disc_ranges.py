import io
import pathlib
import sys
import tempfile
import unittest
R = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(R/'tools'))
from inspect_disc import RangeImage, LocalImage, index_image, resolve_drive, MAX_READ
from inspect_xbe import Invalid
from test_tools import xiso

class Response(io.BytesIO):
    def __init__(self, data=b'x', status=206, content_range='bytes 0-0/100', etag='v1'):
        super().__init__(data)
        self.status=status
        self.headers={'Content-Range':content_range,'ETag':etag}
    def geturl(self):return 'https://drive.usercontent.google.com/download?private-token'

class Ranges(unittest.TestCase):
    def opener(self, responses):
        values=iter(responses)
        return lambda *args, **kwargs:next(values)
    def test_exact_range(self):
        reader=RangeImage('https://example.test/image',self.opener([Response(),Response(b'abcd',content_range='bytes 20-23/100')]))
        self.assertEqual(reader.read_at(20,4),b'abcd');self.assertEqual(reader.bytes_read,5);self.assertEqual(reader.size,100)
    def test_non_range_response_is_not_consumed(self):
        class NoRead(Response):
            def read(self,*args):raise AssertionError('Must reject before reading large body')
        with self.assertRaisesRegex(Invalid,'exact bounded'):RangeImage('https://example.test/image',self.opener([NoRead(status=200)]))
    def test_wrong_content_range(self):
        with self.assertRaisesRegex(Invalid,'Mismatched'):RangeImage('https://example.test/image',self.opener([Response(content_range='bytes 1-1/100')]))
    def test_truncated_response(self):
        with self.assertRaisesRegex(Invalid,'Truncated'):RangeImage('https://example.test/image',self.opener([Response(b'')]))
    def test_changed_size(self):
        reader=RangeImage('https://example.test/image',self.opener([Response(),Response(content_range='bytes 1-1/101')]))
        with self.assertRaisesRegex(Invalid,'size changed'):reader.read_at(1,1)
    def test_changed_etag(self):
        reader=RangeImage('https://example.test/image',self.opener([Response(),Response(content_range='bytes 1-1/100',etag='v2')]))
        with self.assertRaisesRegex(Invalid,'ETag changed'):reader.read_at(1,1)
    def test_boundaries_without_network(self):
        reader=RangeImage('https://example.test/image',self.opener([Response()]))
        for offset,size in [(-1,1),(0,MAX_READ+1),(99,2)]:
            with self.subTest(offset=offset),self.assertRaises(Invalid):reader.read_at(offset,size)
        self.assertEqual(reader.read_at(0,0),b'');self.assertEqual(reader.requests,1)
    def test_drive_confirmation_stays_on_expected_host(self):
        html=b'<form action="https://evil.example/download"><input name="id" value="test-id"></form>'
        with self.assertRaisesRegex(Invalid,'not directly downloadable'):resolve_drive('https://drive.google.com/file/d/test-id/view',self.opener([Response(html,status=200)]))
    def test_drive_confirmation_matches_file(self):
        html=b'<form action="https://drive.usercontent.google.com/download"><input name="id" value="other-id"></form>'
        with self.assertRaisesRegex(Invalid,'another file'):resolve_drive('https://drive.google.com/file/d/test-id/view',self.opener([Response(html,status=200)]))

class DiscIndex(unittest.TestCase):
    def test_xdvdfs_finds_executable(self):
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/'disc.iso';p.write_bytes(xiso());reader=LocalImage(p);report,files=index_image(reader)
            self.assertEqual(report['format'],'XDVDFS');self.assertEqual(files[0]['path'],'Default.xbe');self.assertEqual(len(files),1)
    def test_path_escape_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/'disc.iso';p.write_bytes(xiso(b'../escape'))
            with self.assertRaisesRegex(Invalid,'Caminho inseguro'):index_image(LocalImage(p))
    def test_ps2_message(self):
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/'disc.iso';p.write_bytes(b'SYSTEM.CNF PLAYSTATION')
            with self.assertRaisesRegex(Invalid,'PS2 detectada'):index_image(LocalImage(p))
if __name__=='__main__':unittest.main(verbosity=2)
