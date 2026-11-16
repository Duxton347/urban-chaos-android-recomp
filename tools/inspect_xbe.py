#!/usr/bin/env python3
"""Bounded structural Xbox XBE inspector; emits metadata, never section payloads."""
import argparse, hashlib, json, pathlib, struct, sys
CANDIDATE_SHA256='b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac'
CANDIDATE_SIZE=5283840
class Invalid(ValueError): pass

def inspect(path, expected_hash=None):
    path=pathlib.Path(path)
    if not path.is_file():raise Invalid('A origem XBE deve ser arquivo regular')
    if path.stat().st_size > 256*1024*1024: raise Invalid('XBE excede limite de 256 MiB')
    b=path.read_bytes(); size=len(b)
    def span(off,n):
        if off<0 or n<0 or off+n>size: raise Invalid(f'XBE truncado: intervalo {off:#x}+{n:#x}')
        return b[off:off+n]
    def u(off): return struct.unpack('<I',span(off,4))[0]
    if size<0x178 or b[:4]!=b'XBEH':raise Invalid('Arquivo inválido/truncado: assinatura XBEH ausente')
    sha=hashlib.sha256(b).hexdigest()
    if expected_hash and sha!=expected_hash:raise Invalid('SHA-256 incompatível com o executável esperado')
    base,hs,ims=u(0x104),u(0x108),u(0x10c)
    if hs<0x178 or hs>size or ims<hs or base+ims>0x100000000:raise Invalid('Limites de imagem/cabeçalho inválidos')
    def hdr(va,n):
        off=va-base
        if off<0 or off+n>hs:raise Invalid('Ponteiro fora do cabeçalho XBE')
        span(off,n);return off
    ns,st=u(0x11c),u(0x120)
    if not 0<ns<=4096:raise Invalid('Contagem de seções inválida')
    table=hdr(st,ns*56); sections=[]
    for i in range(ns):
        off=table+56*i;flags,va,vs,raw,rs,name=struct.unpack('<6I',span(off,24))
        if va<base or va+vs>base+ims or rs>vs:raise Invalid('Seção fora da imagem')
        span(raw,rs); no=hdr(name,1); end=b.find(b'\0',no,min(hs,no+256))
        if end<0:raise Invalid('Nome de seção não terminado')
        stored=span(off+36,20);actual=hashlib.sha1(struct.pack('<I',rs)+span(raw,rs)).digest()
        sections.append(dict(name=b[no:end].decode('ascii','replace'),flags=flags,va=va,virtual_size=vs,raw=raw,size=rs,stored_sha1=stored.hex(),actual_sha1=actual.hex(),digest_match=stored==actual))
    def offset(va,n):
        if base<=va and va+n<=base+hs:return hdr(va,n)
        for s in sections:
            if s['va']<=va and va+n<=s['va']+s['size']:return s['raw']+va-s['va']
        raise Invalid('Endereço virtual não mapeado em arquivo')
    decoded=None
    for kind,ek,tk in [('retail',0xa8fc57ab,0x5b6d40b6),('debug',0x94859d4b,0xefb1f152)]:
        ep,th=u(0x128)^ek,u(0x158)^tk
        if base<=ep<base+ims and base<=th<base+ims:decoded=(kind,ep,th);break
    if decoded is None:raise Invalid('Chave de entrada/thunk XBE desconhecida')
    kind,ep,th=decoded;offset(ep,1)
    co=hdr(u(0x118),12);cs=u(co)
    if cs<0x1d0:raise Invalid('Certificado XBE incompleto')
    hdr(u(0x118),cs)
    cert=dict(size=cs,timestamp=u(co+4),title_id=f'{u(co+8):08x}',region=u(co+0xa0),media=u(co+0x9c),disc_number=u(co+0xa8),version=u(co+0xac))
    libs=[];nl=u(0x160)
    if nl>4096:raise Invalid('Contagem de bibliotecas inválida')
    if nl:
        lo=hdr(u(0x164),nl*16)
        for i in range(nl):
            v=struct.unpack('<8s4H',span(lo+i*16,16));libs.append(dict(name=v[0].rstrip(b'\0').decode('ascii','replace'),major=v[1],minor=v[2],build=v[3],flags=v[4]))
    imports=[]
    for i in range(65536):
        val=u(offset(th+4*i,4))
        if val==0:break
        if not val&0x80000000:raise Invalid('Thunk kernel não ordinal')
        imports.append(val&0x7fffffff)
    else:raise Invalid('Tabela de importação sem terminador')
    return dict(format='XBE',size=size,sha256=sha,candidate_match=(sha==CANDIDATE_SHA256 and size==CANDIDATE_SIZE),kind=kind,base=base,image_size=ims,entry=ep,certificate=cert,sections=sections,libraries=libs,kernel_import_ordinals=imports,non_kernel_import_table=u(0x15c),compatibility='NOT_ESTABLISHED')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('xbe');p.add_argument('--expected-sha256');a=p.parse_args()
    try:print(json.dumps(inspect(a.xbe,a.expected_sha256),indent=2))
    except (OSError,ValueError) as e:print(str(e),file=sys.stderr);return 2
    return 0
if __name__=='__main__':sys.exit(main())
