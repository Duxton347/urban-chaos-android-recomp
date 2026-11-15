"""Record Google's published package archive hashes (SHA-1) and sizes from repository XML."""
import urllib.request,xml.etree.ElementTree as E,json,pathlib
R=pathlib.Path(__file__).resolve().parents[1];url='https://dl.google.com/android/repository/repository2-1.xml'
with urllib.request.urlopen(url,timeout=60) as r:data=r.read()
root=E.fromstring(data);wanted={'platforms;android-34','build-tools;34.0.0','ndk;28.2.13676358','cmake;3.31.6'};found={}
for p in root:
 if p.tag.split('}')[-1]!='remotePackage' or p.get('path') not in wanted:continue
 local=E.parse(R/'.toolchain/android-sdk'/p.get('path').replace(';','/')/'package.xml').getroot()
 installed=next(x for x in local if x.tag.split('}')[-1]=='localPackage')
 def rev(e):return tuple(int(e.findtext('revision/'+k,'0')) for k in ('major','minor','micro'))
 if rev(p)!=rev(installed):continue
 archives=p.find('archives')
 if archives is None:continue
 for a in archives.findall('archive'):
  if a.findtext('host-os') not in (None,'linux'):continue
  c=a.find('complete')
  if c is None:continue
  found[p.get('path')]=dict(url='https://dl.google.com/android/repository/'+c.findtext('url'),size=int(c.findtext('size')),checksum_type=c.find('checksum').get('type','sha1'),checksum=c.findtext('checksum'))
(R/'docs/evidence/sdk-archives.json').write_text(json.dumps(dict(source=url,packages=found),indent=2)+'\n')
print(json.dumps(found,indent=2))
