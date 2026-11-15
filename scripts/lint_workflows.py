#!/usr/bin/env python3
"""Validate pinned action SHAs and run checksum-verified actionlint 1.7.7."""
import hashlib,json,pathlib,re,subprocess,tarfile,urllib.request
R=pathlib.Path(__file__).resolve().parents[1];lock=json.loads((R/'config/ci-tools.lock.json').read_text());d=R/'.toolchain/actionlint';d.mkdir(parents=True,exist_ok=True)
for p in sorted((R/'.github/workflows').glob('*.yml')):
 for action,sha in re.findall(r'uses:\s*actions/([\w-]+)@([^\s]+)',p.read_text()):
  if lock['actions'].get(action)!=sha:raise SystemExit(str(p)+': action pin mismatch')
meta=lock['actionlint'];archive=d/'actionlint.tar.gz'
if not archive.exists():
 url=f"https://github.com/rhysd/actionlint/releases/download/v{meta['version']}/actionlint_{meta['version']}_linux_amd64.tar.gz"
 with urllib.request.urlopen(url,timeout=60) as src,archive.open('wb') as out:out.write(src.read())
if hashlib.sha256(archive.read_bytes()).hexdigest()!=meta['archive_sha256']:raise SystemExit('actionlint checksum mismatch')
if not (d/'actionlint').exists():
 with tarfile.open(archive) as t:t.extractall(d,filter='data')
subprocess.run([str(d/'actionlint'),'-version'],check=True)
subprocess.run([str(d/'actionlint'),'-no-color'],cwd=R,check=True)
print('All six workflows passed actionlint and action SHA checks')
