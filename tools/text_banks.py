#!/usr/bin/env python3
"""Independent bounded readers for the user's LST/ENT and EXTTXT text banks.

Format inferred from the supplied disc's headers, zlib streams, index coverage
and MD5 trailers. No upstream title implementation or commercial text included.
"""
import argparse
import hashlib
import json
import pathlib
import re
import struct
import zlib

ROOT=pathlib.Path(__file__).resolve().parents[1]
LIMIT=128*1024*1024

class Invalid(ValueError):pass

def require(ok,message):
    if not ok:raise Invalid(message)

def u32(data,at):
    require(0<=at<=len(data)-4,'Truncated field')
    return struct.unpack_from('<I',data,at)[0]

def put32(data,at,value):struct.pack_into('<I',data,at,value)

class Archive:
    def __init__(self,data):
        require(208<=len(data)<=LIMIT and len(data)%2048==16,'Invalid archive length')
        require(hashlib.md5(data[:-16]).digest()==data[-16:],'Archive MD5 differs')
        require(u32(data,0)==28,'Unknown archive header')
        self.data=data;self.sectors=(len(data)-16)//2048
        require(all(u32(data,at)==self.sectors for at in range(24,44,4)),'Archive sector fields differ')
        table_size,count=u32(data,192),u32(data,196)
        require(0<count<=100000 and 4<=table_size<=16*1024*1024,'Invalid archive index')
        end=196+table_size;require(end<=len(data)-16,'Truncated archive index')
        at=200;self.entries=[]
        for _ in range(count):
            require(at+2<=end,'Truncated entry name')
            length=struct.unpack_from('<H',data,at)[0];at+=2
            require(1<=length<=1024 and at+length+16<=end,'Invalid entry name extent')
            raw_name=data[at:at+length];at+=length
            require(raw_name[-1]==0 and b'\0' not in raw_name[:-1],'Entry name is not terminated')
            try:name=raw_name[:-1].decode('ascii')
            except UnicodeError as error:raise Invalid('Non-ASCII resource ID') from error
            require(name,'Empty resource ID')
            offset,raw,packed,identifier=struct.unpack_from('<4I',data,at)
            require(offset>=end and packed<=len(data)-16-offset and raw<=LIMIT,'Invalid payload extent')
            self.entries.append(dict(name=name,offset=offset,raw=raw,packed=packed,identifier=identifier,record=at));at+=16
        require(at==end,'Index length differs')
        self.ordered=sorted(self.entries,key=lambda entry:entry['offset'])
        for prev,following in zip(self.ordered,self.ordered[1:]):
            require(prev['offset']+prev['packed']<=following['offset'],'Overlapping payloads')
        self.by_name={}
        for entry in self.entries:self.by_name.setdefault(entry['name'],[]).append(entry)

    def unpack(self,name):
        require(name in self.by_name,'Resource ID absent')
        require(len(self.by_name[name])==1,'Ambiguous resource ID')
        entry=self.by_name[name][0];packed=self.data[entry['offset']:entry['offset']+entry['packed']]
        decoder=zlib.decompressobj()
        try:raw=decoder.decompress(packed,entry['raw']+1)
        except zlib.error as error:raise Invalid('Invalid zlib stream') from error
        require(len(raw)==entry['raw'] and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail,'Payload size/stream differs')
        return raw

    def rebuild(self,replacements):
        require(set(replacements)<=self.by_name.keys(),'Unknown replacement resource')
        require(all(len(self.by_name[name])==1 for name in replacements),'Ambiguous replacement resource')
        if not replacements:return self.data
        output=bytearray(self.data[:self.ordered[0]['offset']])
        for index,entry in enumerate(self.ordered):
            payload=zlib.compress(replacements[entry['name']],9) if entry['name'] in replacements else self.data[entry['offset']:entry['offset']+entry['packed']]
            require(len(output)+len(payload)<=LIMIT-2048,'Rebuilt archive exceeds limit')
            put32(output,entry['record'],len(output))
            put32(output,entry['record']+4,len(replacements[entry['name']]) if entry['name'] in replacements else entry['raw'])
            put32(output,entry['record']+8,len(payload));output.extend(payload)
            if index+1<len(self.ordered):
                output.extend(self.data[entry['offset']+entry['packed']:self.ordered[index+1]['offset']])
        # The last sector padding and MD5 are regenerated, never reused as content.
        output.extend(bytes((-len(output))%2048));sectors=len(output)//2048
        for at in range(24,44,4):put32(output,at,sectors)
        output.extend(hashlib.md5(output).digest())
        checked=Archive(bytes(output))
        for name,raw in replacements.items():require(checked.unpack(name)==raw,'Replacement roundtrip differs')
        return bytes(output)

def controls(text):return [c for c in text.encode('latin1') if c<32 or 127<=c<160]
def substitutions(text):return re.findall(r'%(?:\d+\$)?[-+#0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|h|ll|l|L|z|t|j)?[diuoxXfFeEgGaAcspn%]',text)

class TextBank:
    def __init__(self,data):
        require(24<=len(data)<=LIMIT,'Invalid text bank length')
        self.data=data;self.languages,self.tag=struct.unpack_from('<HH',data)
        self.keys_size,self.meta_size,self.text_size,self.aux_size,self.count=struct.unpack_from('<5I',data,4)
        require(1<=self.languages<=16 and 0<self.count<=65535,'Invalid text bank counts')
        require(self.keys_size==self.count*56 and self.meta_size%12==0,'Unknown text bank directory shape')
        self.stride=self.keys_size+self.meta_size+self.text_size+self.aux_size
        require(self.text_size>0 and len(data)==24+self.languages*self.stride,'Text bank extents differ')
        self.blocks=[]
        for language in range(self.languages):
            start=24+language*self.stride;meta=start+self.keys_size;text=meta+self.meta_size
            keys=[];used=set();names=set()
            for index in range(self.count):
                at=start+index*56;raw_name=data[at:at+48]
                require(b'\0' in raw_name,'Unterminated text ID')
                try:name=raw_name.split(b'\0',1)[0].decode('ascii')
                except UnicodeError as error:raise Invalid('Invalid text ID') from error
                require(name and name not in names,'Duplicate text ID');names.add(name)
                variants=data[at+49];offset=u32(data,at+52)
                require(variants>0 and offset%12==0 and offset+variants*12<=self.meta_size,'Invalid variant extent')
                for variant in range(variants):
                    record=offset+variant*12;require(record not in used,'Overlapping text descriptors');used.add(record)
                    pos,aux,length=struct.unpack_from('<3I',data,meta+record)
                    require(pos<self.text_size,'Text pointer outside pool')
                    end=data.find(b'\0',text+pos,text+self.text_size);require(end>=0,'Unterminated text')
                    require(aux==0xffffffff or aux<=self.aux_size and length<=self.aux_size-aux,'Auxiliary data outside pool')
                    keys.append(dict(key=name,variant=variant,record=record,text=data[text+pos:end].decode('latin1')))
            require(len(used)*12==self.meta_size,'Unaccounted text descriptors')
            self.blocks.append(dict(start=start,meta=meta,text=text,rows=keys))

    def template(self,language=0):
        require(0<=language<self.languages,'Unknown language slot')
        return {'schema':1,'source_sha256':hashlib.sha256(self.data).hexdigest(),'language_slot':language,
                'entries':[dict(key=row['key'],variant=row['variant'],source=row['text'],translation=None) for row in self.blocks[language]['rows'] if row['text']]}

    def translate(self,document):
        require(document.get('schema')==1 and document.get('source_sha256')==hashlib.sha256(self.data).hexdigest(),'Translation source revision differs')
        language=document.get('language_slot');require(type(language)==int and 0<=language<self.languages,'Unknown language slot')
        rows=self.blocks[language]['rows'];known={(row['key'],row['variant']):row for row in rows};changes={}
        for item in document.get('entries',[]):
            identity=(item['key'],item['variant']);require(identity in known and identity not in changes,'Unknown/duplicate translation ID')
            source=known[identity]['text'];require(item.get('source')==source,'Translation source text differs')
            translated=item.get('translation')
            if translated is None:continue
            require(isinstance(translated,str) and source and translated,'Empty or invalid caption replacement')
            try:encoded=translated.encode('latin1')
            except UnicodeError as error:raise Invalid('Caption requires a character outside the original single-byte repertoire') from error
            require(b'\0' not in encoded and len(encoded)<=4096 and controls(source)==controls(translated),'Caption controls/length differ')
            require(substitutions(source)==substitutions(translated),'Caption format substitutions differ')
            changes[identity]=translated
        if not changes:return self.data
        pools={};descriptors={}
        for slot,block in enumerate(self.blocks):
            if slot!=language:continue
            pool=bytearray();dedup={};meta=bytearray(self.data[block['meta']:block['text']])
            for row in rows:
                encoded=changes.get((row['key'],row['variant']),row['text']).encode('latin1')+b'\0'
                if encoded not in dedup:dedup[encoded]=len(pool);pool.extend(encoded)
                put32(meta,row['record'],dedup[encoded])
            pools[slot]=pool;descriptors[slot]=meta
        size=max(self.text_size,max(map(len,pools.values())));require(size<=16*1024*1024,'Translation pool exceeds limit')
        output=bytearray(self.data[:24]);put32(output,12,size)
        for slot,block in enumerate(self.blocks):
            output.extend(self.data[block['start']:block['meta']]);output.extend(descriptors.get(slot,self.data[block['meta']:block['text']]))
            pool=pools.get(slot,self.data[block['text']:block['text']+self.text_size]);output.extend(pool);output.extend(bytes(size-len(pool)))
            output.extend(self.data[block['text']+self.text_size:block['start']+self.stride])
        checked=TextBank(bytes(output))
        for slot,block in enumerate(checked.blocks):
            for old,new in zip(self.blocks[slot]['rows'],block['rows']):
                expected=changes.get((old['key'],old['variant']),old['text']) if slot==language else old['text']
                require(new['text']==expected,'Translation roundtrip differs')
        return bytes(output)

def private_output(path):
    path=pathlib.Path(path).resolve()
    require(any((ROOT/name) in path.parents for name in ('build','private')),'Text exports/patches must remain in ignored build/ or private/')
    require(not path.exists(),'Output already exists');path.parent.mkdir(parents=True,exist_ok=True);return path

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('archive',type=pathlib.Path);parser.add_argument('--entry',required=True);parser.add_argument('--out',required=True);parser.add_argument('--translations',type=pathlib.Path)
    args=parser.parse_args();require(args.archive.stat().st_size<=LIMIT,'Archive too large');archive=Archive(args.archive.read_bytes());bank=TextBank(archive.unpack(args.entry));out=private_output(args.out)
    if args.translations:
        require(args.translations.stat().st_size<=16*1024*1024,'Translation document too large');document=json.loads(args.translations.read_text());out.write_bytes(archive.rebuild({args.entry:bank.translate(document)}))
    else:out.write_text(json.dumps(bank.template(),ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({'languages':bank.languages,'keys':bank.count,'variants':len(bank.blocks[0]['rows']),'translated_archive':bool(args.translations),'output_bytes':out.stat().st_size,'device_verified':False}))

if __name__=='__main__':main()
