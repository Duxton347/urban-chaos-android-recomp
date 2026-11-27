#!/usr/bin/env python3
"""Build a private, revision-bound translation dictionary, never original archive bytes/text."""
import argparse,hashlib,json,pathlib,struct,sys
from text_banks import Archive,TextBank,require,ROOT
NAMES={'globallanguage.lst':'globals.exttxt','99-01.ent':'99-01.exttxt','12-01.ent':'12-01.exttxt'}
def build(spec):
    result=bytearray(b'UCPTBR01')
    def integer(n):result.extend(struct.pack('>I',n))
    def text(s):
        encoded=s.encode('utf8');require(len(encoded)<=65536,'Pack string too long');integer(len(encoded));result.extend(encoded)
    require(1<=len(spec)<=3,'Invalid archive count');integer(len(spec));seen=set()
    for row in spec:
        name=row['name'];require(name in NAMES and name not in seen,'Unsupported/duplicate archive');seen.add(name)
        raw=pathlib.Path(row['archive']).read_bytes();archive=Archive(raw);bank=TextBank(archive.unpack(NAMES[name]));doc=json.loads(pathlib.Path(row['translations']).read_text());bank.translate(doc)
        entries=[r for r in doc['entries'] if r.get('translation') and r['translation']!=r['source']]
        translated_keys={entry['key'] for entry in entries}
        require(set(row.get('required_keys',[]))<=translated_keys,'Required caption translations missing')
        text(name);text(NAMES[name]);text(hashlib.sha256(raw).hexdigest());text(hashlib.sha256(bank.data).hexdigest());integer(len(entries))
        for entry in entries:text(entry['key']);integer(entry['variant']);text(entry['translation'])
    require(len(result)<=2*1024*1024,'Pack too large');return bytes(result)
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('manifest');parser.add_argument('--out',required=True);args=parser.parse_args()
    output=pathlib.Path(args.out).resolve();require(any((ROOT/name) in output.parents for name in ('build','private')),'Private output required')
    output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(build(json.loads(pathlib.Path(args.manifest).read_text())))
    print('Private translation dictionary built; no archive payload bundled')
