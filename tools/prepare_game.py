#!/usr/bin/env python3
"""Stage a legitimate Xbox folder or XDVDFS image atomically. No overwrite of saves."""
import argparse, hashlib, json, os, pathlib, shutil, struct, sys, tempfile
from inspect_xbe import inspect, Invalid, CANDIDATE_SHA256
SECTOR=2048; MAGIC=b'MICROSOFT*XBOX*MEDIA'; BASES=(0,0x30600*SECTOR,0x4100*SECTOR)
def safe_name(name):
    if not name or name in ('.','..') or any(c in name for c in '/\\:\0') or any(ord(c)<32 for c in name):raise Invalid('Caminho inseguro na importação')
    return name

def image_entries(path):
    """Validate full directory graph before writing any extracted file."""
    size=path.stat().st_size; out=[];seen=set();names=set()
    with path.open('rb') as f:
        def read(off,n):
            if off<0 or n<0 or off+n>size:raise Invalid('Imagem XDVDFS truncada')
            f.seek(off);b=f.read(n)
            if len(b)!=n:raise Invalid('Leitura incompleta')
            return b
        base=None
        for candidate in BASES:
            off=candidate+32*SECTOR
            if off+SECTOR<=size and read(off,20)==MAGIC:
                desc=read(off,SECTOR)
                if desc[-20:]!=MAGIC:raise Invalid('Descritor XDVDFS incompleto')
                base=candidate;root,root_size=struct.unpack_from('<II',desc,20);break
        if base is None:
            probe=read(0,min(size,8*1024*1024))
            if b'PLAYSTATION' in probe.upper() or b'SYSTEM.CNF' in probe.upper():raise Invalid('Imagem PS2 detectada: é necessário o disco Xbox com Default.xbe')
            raise Invalid('Imagem não é XDVDFS/XISO suportada; ISO PS2 não contém o executável Xbox')
        def directory(sec,length,prefix,depth):
            if depth>64 or length>64*1024*1024:raise Invalid('Diretório XDVDFS excede limites')
            key=(sec,length)
            if key in seen:raise Invalid('Ciclo/alias de diretórios XDVDFS')
            seen.add(key);table=read(base+sec*SECTOR,length);nodes=set(); stack=[0] if length else []
            while stack:
                off=stack.pop()
                if off in nodes or off+14>length:raise Invalid('Árvore XDVDFS inválida')
                nodes.add(off)
                left,right,sector,n,attr,nlen=struct.unpack_from('<HHIIBB',table,off)
                if nlen==0 or off+14+nlen>length:raise Invalid('Entrada de diretório truncada')
                name=safe_name(table[off+14:off+14+nlen].decode('ascii','strict'));rel=prefix/name;k=rel.as_posix().casefold()
                if k in names:raise Invalid('Colisão de caminhos sem distinção de maiúsculas')
                names.add(k)
                if len(names)>250000:raise Invalid('Número excessivo de arquivos')
                for child in (left,right):
                    if child:stack.append(child*4)
                if attr&0x10:directory(sector,n,rel,depth+1)
                else:
                    at=base+sector*SECTOR
                    if at+n>size:raise Invalid('Arquivo fora da imagem/truncado')
                    out.append((rel,at,n))
        directory(root,root_size,pathlib.PurePosixPath(),0)
    return out

def prepare(source,dest,expected=None):
    source=pathlib.Path(source).absolute();dest=pathlib.Path(dest).absolute()
    if dest.exists():raise Invalid('Destino já existe; escolha nova instalação de staging')
    if source.is_symlink():raise Invalid('Origem simbólica recusada')
    entries=[]
    if source.is_dir():
        if source==dest or source in dest.parents:raise Invalid('Destino dentro da origem')
        keys=set()
        for root,dirs,files in os.walk(source,followlinks=False):
            for name in dirs+files:
                safe_name(name);p=pathlib.Path(root)/name
                if p.is_symlink():raise Invalid('Links simbólicos recusados')
                rel=p.relative_to(source);key=rel.as_posix().casefold()
                if key in keys:raise Invalid('Colisão de caminhos')
                keys.add(key)
                if p.is_file():entries.append((rel,p,p.stat().st_size))
                elif not p.is_dir():raise Invalid('Entrada especial recusada')
    else:
        if not source.is_file():raise Invalid('A origem deve ser pasta ou imagem regular')
        entries=image_entries(source)
    defaults=[e for e in entries if e[0].as_posix().casefold()=='default.xbe']
    if len(defaults)!=1:raise Invalid('Pasta Xbox deve conter exatamente um Default.xbe na raiz')
    total=sum(e[2] for e in entries);dest.parent.mkdir(parents=True,exist_ok=True)
    if shutil.disk_usage(dest.parent).free<total+64*1024*1024:raise Invalid('Espaço insuficiente')
    stage=pathlib.Path(tempfile.mkdtemp(prefix='.import-',dir=dest.parent))
    try:
        with (source.open('rb') if source.is_file() else open(os.devnull,'rb')) as image:
            for rel,start,n in entries:
                target=stage/rel;target.parent.mkdir(parents=True,exist_ok=True)
                src=start.open('rb') if isinstance(start,pathlib.Path) else image
                try:
                    if src is image:src.seek(start)
                    with target.open('xb') as out:
                        remain=n
                        while remain:
                            chunk=src.read(min(remain,1024*1024))
                            if not chunk:raise Invalid('Importação interrompida/truncada')
                            out.write(chunk);remain-=len(chunk)
                        out.flush();os.fsync(out.fileno())
                finally:
                    if src is not image:src.close()
        report=inspect(stage/defaults[0][0],expected)
        (stage/'installation.json').write_text(json.dumps(dict(xbe=report,files=len(entries),bytes=total),indent=2)+'\n')
        stage.rename(dest)
        return dict(status='IMPORTED_NOT_GAME_QUALIFIED',files=len(entries),bytes=total,sha256=report['sha256'])
    except BaseException:
        shutil.rmtree(stage);raise

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source');p.add_argument('destination');p.add_argument('--require-candidate',action='store_true');a=p.parse_args()
    try:print(json.dumps(prepare(a.source,a.destination,CANDIDATE_SHA256 if a.require_candidate else None),indent=2));return 0
    except (OSError,ValueError) as e:print(str(e),file=sys.stderr);return 2
if __name__=='__main__':sys.exit(main())
