#!/usr/bin/env python3
"""Compile and exercise diagnostic recording/export using authored metadata only."""
import json
import os
import pathlib
import shutil
import subprocess
import tempfile
import zipfile
from prepare_runtime_overlay import ROOT, prepare

def records(directory):
    result=[]
    for path in sorted(directory.glob('runtime.*.jsonl')):
        assert path.stat().st_size <= 1024*1024
        for line in path.read_text().splitlines():
            item=json.loads(line)
            assert item['schema']==1
            assert isinstance(item['cause_known'],bool)
            assert item['seq']>0
            result.append(item)
    return result

def main():
    (ROOT/'build').mkdir(exist_ok=True)
    java_home=pathlib.Path(os.environ.get('JAVA_HOME',ROOT/'.toolchain/jdk/jdk-17.0.17+10'))
    javac,java=str(java_home/'bin/javac'),str(java_home/'bin/java')
    if not pathlib.Path(javac).is_file():
        javac,java=shutil.which('javac'),shutil.which('java')
    with tempfile.TemporaryDirectory(prefix='diagnostics-',dir=ROOT/'build') as temporary:
        out=pathlib.Path(temporary);overlay=out/'overlay';prepare(overlay)
        binary=out/'diagnostics_probe'
        progress_binary=out/'progress_probe'
        subprocess.run(['c++','-std=c++20','-O2','-DNDEBUG','-pthread','-I'+str(ROOT/'native'),
            str(ROOT/'tests/progress_fixture.cpp'),'-o',str(progress_binary)],check=True)
        subprocess.run([str(progress_binary),str(out/'progress')],check=True)
        progress_records=records(out/'progress')
        assert sum(item['code']=='UC-CPU-003' for item in progress_records)==64
        io=next(item['detail'] for item in progress_records if item['code']=='UC-DISC-002')
        assert io['bytes']==19 and io['short_reads']==1 and io['inflight']==0 and io['thread_slots_dropped']==7
        assert any(item['code']=='UC-GPU-005' and item['detail']['operation']==2 and item['detail']['calls']==1 for item in progress_records)
        subprocess.run(['c++','-std=c++20','-O2','-DNDEBUG','-pthread',
            '-I'+str(ROOT/'native'),'-I'+str(overlay/'runtime'),'-I'+str(ROOT/'vendor/runtime/runtime'),
            str(ROOT/'tests/diagnostics_probe.cpp'),str(overlay/'runtime/core/log.cpp'),'-o',str(binary)],check=True)
        native=out/'native'
        subprocess.run([str(binary),str(native)],check=True,capture_output=True,text=True)
        standard=records(native/'standard')
        codes={item['code'] for item in standard}
        assert {'UC-GPU-001','UC-GPU-002','UC-GPU-003','UC-GPU-004','UC-AUDIO-001','UC-AUDIO-002','UC-AUDIO-003','UC-CPU-001','UC-KERNEL-001','UC-DISC-001','UC-RUNTIME-002'}<=codes
        assert any(item['code']=='UC-GPU-001' and item['detail']=={'frame':1800,'backend':2,'headless':1} for item in standard)
        assert any(item['code']=='UC-GPU-002' and item['detail']=={'samples':64} for item in standard)
        assert any(item['code']=='UC-GPU-001' and item['detail']=={'frame':2400,'backend':2,'headless':0} for item in standard)
        assert any(item['code']=='UC-GPU-003' and item['detail']['vendor']==0x5143 and item['detail']['phase']==1 for item in standard)
        assert any(item['code']=='UC-GPU-004' and item['detail']=={'operation':4} for item in standard)
        assert 'UC-TRACE-001' not in codes
        assert any(item['code']=='UC-KERNEL-001' and item['detail']['ordinal']==232 for item in standard)
        assert all('SENTINEL_' not in json.dumps(item) for item in standard)
        detailed=records(native/'detailed')
        assert any(item['template']=='private debug %s' for item in detailed)
        assert all('SENTINEL_' not in json.dumps(item) for item in detailed)
        rotated=list((native/'rotated').glob('runtime.*.jsonl'))
        assert len(rotated)==4 and all(p.stat().st_size<=4096 for p in rotated)
        assert any(item['detail'].get('counter')==999 for item in records(native/'rotated'))
        parallel=records(native/'parallel')
        assert len(parallel)==162 and len({item['seq'] for item in parallel})==162
        assert not (native/'standard/session.open').exists()
        abrupt=native/'abrupt'
        result=subprocess.run([str(binary),str(abrupt),'--abrupt'],capture_output=True)
        assert result.returncode==17 and (abrupt/'session.open').is_file()
        assert records(abrupt)[-1]['code']=='UC-CPU-002'
        classes=out/'classes';classes.mkdir()
        core=ROOT/'launcher/core/src/main/java/org/urbanchaos/launcher/diagnostics/ReportSession.java'
        subprocess.run([javac,'-d',str(classes),str(core),str(ROOT/'launcher/core/src/main/java/org/urbanchaos/launcher/diagnostics/NativeReports.java'),str(ROOT/'tests/DiagnosticsTest.java'),str(ROOT/'tests/DiagnosticsJniTest.java')],check=True)
        subprocess.run([java,'-cp',str(classes),'DiagnosticsTest',str(out/'java'),str(native)],check=True)
        subprocess.run([javac,'-d',str(classes),str(ROOT/'launcher/core/src/main/java/org/urbanchaos/launcher/importer/ResumableCopy.java'),str(ROOT/'tests/ResumableCopyTest.java')],check=True)
        subprocess.run([java,'-cp',str(classes),'ResumableCopyTest',str(out)],check=True)
        with zipfile.ZipFile(out/'java/report-fixture.zip') as archive:
            assert all(not n.startswith('/') and '..' not in pathlib.PurePosixPath(n).parts for n in archive.namelist())
            for name in archive.namelist():
                data=archive.read(name)
                assert b'SENTINEL_' not in data
                if name.endswith('.json'):
                    json.loads(data)
                elif name.endswith('.jsonl'):
                    for line in data.splitlines():json.loads(line)
        jdk=pathlib.Path(javac).resolve().parent.parent
        library=out/'liburban_diagnostics_fixture.so'
        subprocess.run(['c++','-std=c++20','-O2','-DNDEBUG','-fPIC','-shared','-pthread',
            '-I'+str(jdk/'include'),'-I'+str(jdk/'include/linux'),
            '-I'+str(ROOT/'native'),'-I'+str(overlay/'runtime'),'-I'+str(ROOT/'vendor/runtime/runtime'),
            str(ROOT/'native/diagnostics/android_bridge.cpp'),str(ROOT/'tests/diagnostics_jni_fixture.cpp'),
            str(overlay/'runtime/core/log.cpp'),'-o',str(library)],check=True)
        subprocess.run([java,'-cp',str(classes),'DiagnosticsJniTest',str(library),str(out/'jni'),str(out/'jni.zip')],check=True,capture_output=True,text=True)
        with zipfile.ZipFile(out/'jni.zip') as archive:
            native_entries=[n for n in archive.namelist() if n.endswith('native/runtime.0.jsonl')]
            assert len(native_entries)==1
            items=[json.loads(line) for line in archive.read(native_entries[0]).splitlines()]
            assert any(item['code']=='UC-KERNEL-001' and item['detail']['ordinal']==233 for item in items)
            assert any(item['code']=='UC-SESSION-002' for item in items)
            assert all(b'SENTINEL_' not in archive.read(name) for name in archive.namelist())
        print(json.dumps({'status':'PASS','jni_bridge_executed':True,'game_data_used':False,'native_release_observer':True,
            'rotation':True,'concurrency':True,'abrupt_exit_persisted':True,'zip_json_validated':True,
            'private_payload_excluded':True,'android_device_tested':False}))
    return 0

if __name__=='__main__':
    raise SystemExit(main())
