import hashlib
import pathlib
import struct
import sys
import unittest
import zlib
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from text_banks import Archive,TextBank,Invalid,substitutions

def authored_bank():
    blocks=[]
    for strings in (('Hello','Press \x02',''),('Bonjour','Appuyer \x02','')):
        keys=bytearray(112)
        for index,(name,count,offset) in enumerate((('dialogue',2,0),('empty',1,24))):
            at=index*56;keys[at:at+len(name)]=name.encode();keys[at+49]=count;struct.pack_into('<I',keys,at+52,offset)
        pool=bytearray();meta=bytearray()
        for i,s in enumerate(strings):
            meta.extend(struct.pack('<3I',len(pool),0 if i==0 else 0xffffffff,2 if i==0 else 0));pool.extend(s.encode('latin1')+b'\0')
        blocks.append(keys+meta+pool+bytes(64-len(pool))+b'\xfe\x81')
    return struct.pack('<HH5I',2,32,112,36,64,2,2)+b''.join(blocks)

def authored_archive(text_last=False,duplicate=False):
    payloads=[('authored.exttxt',authored_bank()),('texture.bin',bytes(range(32)))]
    if text_last:payloads.reverse()
    if duplicate:payloads.insert(1,('texture.bin',b'other authored bytes'))
    records=bytearray();compressed=[zlib.compress(data,9) for name,data in payloads]
    size=4+sum(2+len(name)+1+16 for name,_ in payloads);offset=196+size+48
    for i,((name,data),packed) in enumerate(zip(payloads,compressed)):
        records.extend(struct.pack('<H',len(name)+1)+name.encode()+b'\0'+struct.pack('<4I',offset,len(data),len(packed),i));offset+=len(packed)
    prefix=bytearray(192);struct.pack_into('<I',prefix,0,28)
    data=prefix+struct.pack('<II',size,len(payloads))+records+bytes([0xcd])*48+b''.join(compressed)
    data.extend(bytes((-len(data))%2048))
    for at in range(24,44,4):struct.pack_into('<I',data,at,len(data)//2048)
    return bytes(data)+hashlib.md5(data).digest()

class TextBankTests(unittest.TestCase):
    def test_captions_keep_identity_variants_controls_and_auxiliary_bytes(self):
        bank=TextBank(authored_bank());doc=bank.template();doc['entries'][0]['translation']='Olá, '+('mundo '*30);doc['entries'][1]['translation']='Aperte \x02'
        output=bank.translate(doc);changed=TextBank(output)
        self.assertEqual(changed.blocks[0]['rows'][0]['text'],doc['entries'][0]['translation'])
        self.assertEqual(changed.blocks[0]['rows'][1]['text'],'Aperte \x02')
        self.assertEqual(changed.blocks[1]['rows'],bank.blocks[1]['rows'])
        self.assertGreater(changed.text_size,bank.text_size)
        for old,new in zip(bank.blocks,changed.blocks):
            self.assertEqual(bank.data[old['start']:old['meta']],output[new['start']:new['meta']])
            self.assertEqual(bank.data[old['text']+bank.text_size:old['start']+bank.stride],output[new['text']+changed.text_size:new['start']+changed.stride])
            for offset in range(0,bank.meta_size,12):self.assertEqual(bank.data[old['meta']+offset+4:old['meta']+offset+12],output[new['meta']+offset+4:new['meta']+offset+12])

    def test_archive_rebuild_preserves_other_resources_and_validates_trailer(self):
        archive=Archive(authored_archive());bank=TextBank(archive.unpack('authored.exttxt'));doc=bank.template();doc['entries'][0]['translation']='Olá'
        patched=archive.rebuild({'authored.exttxt':bank.translate(doc)});decoded=Archive(patched)
        self.assertEqual(decoded.unpack('texture.bin'),bytes(range(32)))
        self.assertEqual(TextBank(decoded.unpack('authored.exttxt')).blocks[0]['rows'][0]['text'],'Olá')
        self.assertEqual(hashlib.md5(patched[:-16]).digest(),patched[-16:])
        self.assertEqual(patched[196+struct.unpack_from('<I',patched,192)[0]:decoded.ordered[0]['offset']],bytes([0xcd])*48)

    def test_revision_unknown_id_control_loss_and_unrepresentable_character_fail(self):
        bank=TextBank(authored_bank())
        for change in ('hash','id','control','unicode'):
            doc=bank.template();doc['entries'][0]['translation']='Olá'
            if change=='hash':doc['source_sha256']='0'*64
            if change=='id':doc['entries'][0]['key']='absent'
            if change=='control':doc['entries'][1]['translation']='Aperte'
            if change=='unicode':doc['entries'][0]['translation']='🎮'
            with self.subTest(change=change),self.assertRaises(Invalid):bank.translate(doc)

    def test_duplicate_names_preserved_but_ambiguous_unpack_rejected(self):
        archive=Archive(authored_archive(text_last=True,duplicate=True));bank=TextBank(archive.unpack('authored.exttxt'));doc=bank.template();doc['entries'][0]['translation']='Olá'
        output=Archive(archive.rebuild({'authored.exttxt':bank.translate(doc)}))
        self.assertEqual(len(output.by_name['texture.bin']),2)
        for a,b in zip(archive.by_name['texture.bin'],output.by_name['texture.bin']):self.assertEqual(archive.data[a['offset']:a['offset']+a['packed']],output.data[b['offset']:b['offset']+b['packed']])
        with self.assertRaises(Invalid):output.unpack('texture.bin')

    def test_format_placeholders_and_literal_percentage_labels(self):
        self.assertEqual(substitutions('Name %s count %03d %%'),['%s','%03d','%%'])
        self.assertEqual(substitutions('% Accuracy'),[])

    def test_empty_translation_is_identity(self):
        bank=TextBank(authored_bank());self.assertEqual(bank.translate(bank.template()),bank.data)

    def test_invalid_extents_duplicate_descriptors_and_trailers_fail(self):
        for mutate in ('pool','count','offset','descriptor'):
            data=bytearray(authored_bank())
            if mutate=='pool':struct.pack_into('<I',data,12,0xffffffff)
            if mutate=='count':data[24+49]=0
            if mutate=='offset':struct.pack_into('<I',data,24+52,0xfffffffc)
            if mutate=='descriptor':struct.pack_into('<I',data,24+56+52,0)
            with self.subTest(mutate=mutate),self.assertRaises(Invalid):TextBank(bytes(data))
        data=bytearray(authored_archive());data[-1]^=1
        with self.assertRaises(Invalid):Archive(bytes(data))

if __name__=='__main__':unittest.main()
