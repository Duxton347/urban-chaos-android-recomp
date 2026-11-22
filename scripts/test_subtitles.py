#!/usr/bin/env python3
"""Cross-check independent Java/Python caption translators using authored fixtures only."""
import pathlib,sys,subprocess,tempfile,hashlib,os,shutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tests'),str(ROOT/'tools')]
from test_text_banks import authored_archive
from text_banks import Archive,TextBank
with tempfile.TemporaryDirectory(dir=ROOT/'build',prefix='captions-test-') as tmp:
    out=pathlib.Path(tmp);archive=authored_archive(text_last=True,duplicate=True);(out/'source.bin').write_bytes(archive);(out/'first.bin').write_bytes(authored_archive())
    bank=TextBank(Archive(archive).unpack('authored.exttxt'));doc=bank.template();doc['entries'][0]['translation']='Olá, mundo';doc['entries'][1]['translation']='Aperte \x02';expected=bank.translate(doc)
    java=pathlib.Path(os.environ.get('JAVA_HOME',ROOT/'.toolchain/jdk/jdk-17.0.17+10'))/'bin'
    if not (java/'javac').is_file():java=pathlib.Path(shutil.which('javac')).resolve().parent
    subprocess.run([str(java/'javac'),'-d',str(out),str(ROOT/'launcher/core/src/main/java/org/urbanchaos/launcher/localization/SubtitleArchive.java'),str(ROOT/'tests/SubtitleArchiveTest.java')],check=True)
    subprocess.run([str(java/'java'),'-cp',str(out),'SubtitleArchiveTest',str(out/'source.bin'),hashlib.sha256(bank.data).hexdigest(),str(out/'result.bin'),str(out/'first.bin')],check=True)
    result=Archive((out/'result.bin').read_bytes());assert result.unpack('authored.exttxt')==expected
    stored=Archive((out/'result.bin.stored').read_bytes())
    for a,b in zip(result.ordered,stored.ordered):
        assert a['name']==b['name'] and a['identifier']==b['identifier'] and a['raw']==b['raw']
        import zlib
        assert zlib.decompress(result.data[a['offset']:a['offset']+a['packed']])==zlib.decompress(stored.data[b['offset']:b['offset']+b['packed']])
    before=Archive(archive)
    for a,b in zip(before.ordered[:-1],result.ordered[:-1]):assert before.data[a['offset']:a['offset']+a['packed']]==result.data[b['offset']:b['offset']+b['packed']]
    print('Java/Python translated banks identical; other compressed payloads unchanged; stored cache preserves every raw resource')
