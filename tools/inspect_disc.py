#!/usr/bin/env python3
"""Inspect local/HTTP Xbox images and extract only Default.xbe into ignored storage.

Remote reads require exact byte-range responses. URLs and download tokens are never
included in reports. No complete-disc hash or game compatibility is implied.
"""
import argparse
import hashlib
import json
import os
import pathlib
import re
import struct
import sys
import tempfile
import urllib.parse
import urllib.request
from html.parser import HTMLParser

from inspect_xbe import inspect, Invalid
from prepare_game import safe_name

ROOT = pathlib.Path(__file__).resolve().parents[1]
MAGIC = b'MICROSOFT*XBOX*MEDIA'
BASES = (0, 0x18300000, 0x02080000)
MAX_READ = 64 * 1024 * 1024

class RangeImage:
    def __init__(self, url, opener=urllib.request.urlopen):
        if urllib.parse.urlsplit(url).scheme not in ('http', 'https'):
            raise Invalid('Only HTTP(S) image URLs are supported')
        self.url = url
        self.opener = opener
        self.size = None
        self.etag = None
        self.bytes_read = 0
        self.requests = 0
        self.read_at(0, 1)

    def read_at(self, offset, length):
        if offset < 0 or length < 0 or length > MAX_READ:
            raise Invalid('Unsafe image read extent')
        if self.size is not None and offset + length > self.size:
            raise Invalid('Image extent exceeds remote file')
        if not length:
            return b''
        if self.requests >= 1024:
            raise Invalid('Remote request limit reached')
        end = offset + length - 1
        headers = {'Range': f'bytes={offset}-{end}', 'Accept-Encoding': 'identity'}
        if self.etag:
            headers['If-Range'] = self.etag
        request = urllib.request.Request(self.url, headers=headers)
        with self.opener(request, timeout=90) as response:
            value = response.headers.get('Content-Range', '')
            match = re.fullmatch(r'bytes (\d+)-(\d+)/(\d+)', value)
            if response.status != 206 or not match:
                raise Invalid('Server must honor an exact bounded byte range')
            start, stop, total = map(int, match.groups())
            if start != offset or stop != end or total <= end:
                raise Invalid('Mismatched Content-Range')
            if self.size is not None and total != self.size:
                raise Invalid('Remote image size changed')
            tag = response.headers.get('ETag')
            if self.etag and tag and tag != self.etag:
                raise Invalid('Remote image ETag changed')
            data = response.read(length + 1)
            if len(data) != length:
                raise Invalid('Truncated or oversized byte-range response')
            self.size = total
            self.etag = tag or self.etag
            self.bytes_read += length
            self.requests += 1
            return data

class LocalImage:
    def __init__(self, path):
        self.path = pathlib.Path(path)
        if not self.path.is_file():
            raise Invalid('Image must be a regular file')
        self.size = self.path.stat().st_size
        self.bytes_read = 0
        self.requests = 0

    def read_at(self, offset, length):
        if offset < 0 or length < 0 or offset + length > self.size or length > MAX_READ:
            raise Invalid('Truncated image or unsafe extent')
        with self.path.open('rb') as source:
            source.seek(offset)
            data = source.read(length)
        if len(data) != length:
            raise Invalid('Image changed or was truncated')
        self.bytes_read += length
        self.requests += 1
        return data

class DownloadForm(HTMLParser):
    def __init__(self):
        super().__init__()
        self.action = None
        self.fields = {}

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == 'form':
            self.action = attrs.get('action')
        if tag == 'input' and 'name' in attrs:
            self.fields[attrs['name']] = attrs.get('value', '')

def resolve_drive(link, opener=urllib.request.urlopen):
    parsed = urllib.parse.urlsplit(link)
    if parsed.hostname not in ('drive.google.com', 'www.drive.google.com'):
        raise Invalid('Expected a Google Drive file link')
    match = re.search(r'/file/d/([A-Za-z0-9_-]+)', parsed.path)
    file_id = match.group(1) if match else urllib.parse.parse_qs(parsed.query).get('id', [''])[0]
    if not re.fullmatch(r'[A-Za-z0-9_-]+', file_id):
        raise Invalid('Missing Drive file ID')
    url = 'https://drive.google.com/uc?' + urllib.parse.urlencode({'export': 'download', 'id': file_id})
    request = urllib.request.Request(url, headers={'Range': 'bytes=0-0'})
    with opener(request, timeout=45) as response:
        if response.status == 206:
            return response.geturl()
        data = response.read(128 * 1024)
    form = DownloadForm()
    form.feed(data.decode('utf-8', errors='replace'))
    target = urllib.parse.urlsplit(form.action or '')
    if target.scheme != 'https' or target.hostname != 'drive.usercontent.google.com':
        raise Invalid('Drive file is not directly downloadable; check sharing permissions')
    if form.fields.get('id') != file_id:
        raise Invalid('Drive confirmation refers to another file')
    return form.action + '?' + urllib.parse.urlencode(form.fields)

def index_image(image):
    base = None
    for candidate in BASES:
        offset = candidate + 32 * 2048
        if offset + 2048 > image.size:
            continue
        descriptor = image.read_at(offset, 2048)
        if descriptor[:20] == MAGIC:
            if descriptor[-20:] != MAGIC:
                raise Invalid('Truncated XDVDFS volume descriptor')
            base = candidate
            root, root_size = struct.unpack_from('<II', descriptor, 20)
            break
    if base is None:
        probe = image.read_at(0, min(image.size, 8 * 1024 * 1024)).upper()
        if b'PLAYSTATION' in probe or b'SYSTEM.CNF' in probe:
            raise Invalid('Imagem PS2 detectada: é necessário o disco Xbox com Default.xbe')
        raise Invalid('No supported XDVDFS partition found')
    entries, directories, paths = [], set(), set()

    def walk(sector, size, prefix='', depth=0):
        if depth > 64 or size > MAX_READ:
            raise Invalid('Directory depth or size limit exceeded')
        if not size:
            return
        if (sector, size) in directories:
            raise Invalid('Cyclic directory graph')
        directories.add((sector, size))
        data = image.read_at(base + sector * 2048, size)
        stack, visited = [0], set()
        while stack:
            offset = stack.pop()
            if offset in visited or offset + 14 > size:
                raise Invalid('Invalid XDVDFS directory tree')
            visited.add(offset)
            left, right, sec, length, attr, name_size = struct.unpack_from('<HHIIBB', data, offset)
            if not name_size or offset + 14 + name_size > size:
                raise Invalid('Truncated directory entry')
            name = safe_name(data[offset + 14:offset + 14 + name_size].decode('ascii'))
            path = prefix + name
            if path.casefold() in paths or len(paths) >= 250000:
                raise Invalid('Duplicate path or excessive directory entries')
            paths.add(path.casefold())
            for child in (left, right):
                if child:
                    stack.append(child * 4)
            if attr & 16:
                walk(sec, length, path + '/', depth + 1)
            else:
                at = base + sec * 2048
                if at + length > image.size:
                    raise Invalid('File extends beyond image')
                entries.append({'path': path, 'offset': at, 'size': length})
    walk(root, root_size)
    return {'format': 'XDVDFS', 'image_size': image.size, 'partition_base': base,
            'directory_count': len(directories), 'file_count': len(entries)}, entries

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--image')
    source.add_argument('--url-file', type=pathlib.Path, help='Private file containing a direct HTTP range URL')
    source.add_argument('--drive-link-file', type=pathlib.Path, help='Private file containing the shared Drive link')
    parser.add_argument('--extract-default', type=pathlib.Path, required=True)
    parser.add_argument('--report', type=pathlib.Path, required=True)
    args = parser.parse_args()
    try:
        destination = args.extract_default.resolve()
        if not any((ROOT / folder) in destination.parents for folder in ('private', 'build')):
            raise Invalid('Executable output must stay in ignored private/ or build/')
        if destination.exists():
            raise Invalid('Refusing to overwrite an existing executable')
        if args.report.resolve() == destination:
            raise Invalid('Report and executable destinations must differ')
        if args.image:
            image = LocalImage(args.image)
        else:
            url = args.url_file.read_text().strip() if args.url_file else resolve_drive(args.drive_link_file.read_text().strip())
            image = RangeImage(url)
        report, entries = index_image(image)
        default = [entry for entry in entries if entry['path'].casefold() == 'default.xbe']
        if len(default) != 1 or default[0]['size'] > MAX_READ:
            raise Invalid('One bounded root Default.xbe is required')
        destination.parent.mkdir(parents=True, exist_ok=True)
        entry = default[0]
        data = image.read_at(entry['offset'], entry['size'])
        temporary = None
        try:
            with tempfile.NamedTemporaryFile(dir=destination.parent, prefix='.xbe-check-', delete=False) as staged:
                temporary = pathlib.Path(staged.name)
                staged.write(data)
                staged.flush()
                os.fsync(staged.fileno())
            report['xbe'] = inspect(temporary)
            os.link(temporary, destination)  # atomic activation; refuses an existing destination
        finally:
            if temporary is not None:
                temporary.unlink(missing_ok=True)
        report.update(network_or_file_read_bytes=image.bytes_read, read_requests=image.requests,
                      full_image_sha256=None, game_qualified=False)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps({'format': report['format'], 'file_count': report['file_count'],
                          'xbe_sha256': report['xbe']['sha256'], 'candidate_match': report['xbe']['candidate_match'],
                          'game_qualified': False}, indent=2))
        return 0
    except (OSError, ValueError) as error:
        print(str(error), file=sys.stderr)
        return 2
if __name__ == '__main__':
    sys.exit(main())
