#!/usr/bin/env python3
"""Verify the internal consistency of a source host-kit ZIP without extracting it.

The embedded manifest is not a signature and cannot authenticate the publisher.
"""
import argparse
import hashlib
import json
import re
import stat
import sys
import zipfile
import zlib

PREFIX = 'TU102_Windows_Host_Test_Kit/'
MANIFEST = 'SOURCE_MANIFEST.json'
MAX_FILES = 10000
MAX_PATH = 1024
MAX_COMPONENT = 255
MAX_DEPTH = 32
MAX_MANIFEST = 4 * 1024 * 1024
MAX_FILE = 128 * 1024 * 1024
MAX_TOTAL = 512 * 1024 * 1024
DIGEST = re.compile(r'^[0-9a-f]{64}$')


class VerificationError(ValueError):
    pass


def check_path(path):
    if (not isinstance(path, str) or not path or len(path) > MAX_PATH or
            any(c in '\\<>"|?*' or ord(c) < 32 or ord(c) == 127 for c in path)):
        raise VerificationError('invalid path')
    parts = path.split('/')
    if (len(parts) > MAX_DEPTH or any(not p or p in ('.', '..') or
            len(p) > MAX_COMPONENT or ':' in p or p.endswith(('.', ' ')) for p in parts)):
        raise VerificationError('invalid path: ' + repr(path))
    # Avoid Windows device-name aliases when the archive is used on Windows.
    for p in parts:
        stem = p.split('.')[0].upper()
        if stem in {'CON', 'PRN', 'AUX', 'NUL'} or re.fullmatch(r'(COM|LPT)[1-9]', stem):
            raise VerificationError('reserved path: ' + repr(path))


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise VerificationError('duplicate manifest key: ' + repr(key))
        result[key] = value
    return result


def verify(path):
    with zipfile.ZipFile(path) as archive:
        entries = archive.infolist()
        if len(entries) > MAX_FILES + 1:
            raise VerificationError('too many entries')
        files = {}
        aliases = set()
        total = 0
        for entry in entries:
            name = entry.filename
            if name != entry.orig_filename:
                raise VerificationError('invalid ZIP entry name')
            if not name.startswith(PREFIX):
                raise VerificationError('entry outside kit: ' + repr(name))
            relative = name[len(PREFIX):]
            check_path(relative)
            if entry.is_dir():
                raise VerificationError('directory entry: ' + repr(name))
            mode = entry.external_attr >> 16
            if entry.create_system == 3 and stat.S_IFMT(mode) not in (0, stat.S_IFREG):
                raise VerificationError('non-regular entry: ' + repr(name))
            alias = relative.casefold()
            if alias in aliases:
                raise VerificationError('duplicate or case-colliding entry: ' + repr(name))
            aliases.add(alias)
            if entry.file_size > MAX_FILE:
                raise VerificationError('entry too large: ' + repr(name))
            total += entry.file_size
            if total > MAX_TOTAL:
                raise VerificationError('archive content too large')
            files[relative] = entry
        if MANIFEST not in files or files[MANIFEST].file_size > MAX_MANIFEST:
            raise VerificationError('missing or oversized manifest')
        try:
            manifest = json.loads(archive.read(files[MANIFEST]).decode('utf-8'),
                                  object_pairs_hook=unique_object)
        except (UnicodeError, json.JSONDecodeError) as exc:
            raise VerificationError('invalid manifest: ' + str(exc)) from exc
        if not isinstance(manifest, dict) or not manifest:
            raise VerificationError('manifest must be a nonempty object')
        manifest_aliases = set()
        for name, digest in manifest.items():
            check_path(name)
            if name.casefold() == MANIFEST.casefold() or name.casefold() in manifest_aliases:
                raise VerificationError('duplicate or reserved manifest path: ' + repr(name))
            manifest_aliases.add(name.casefold())
            if not isinstance(digest, str) or not DIGEST.fullmatch(digest):
                raise VerificationError('invalid SHA-256 for ' + repr(name))
        expected = set(manifest) | {MANIFEST}
        actual = set(files)
        if expected != actual:
            raise VerificationError('entry mismatch; missing: %s; undeclared: %s' %
                                    (sorted(expected - actual), sorted(actual - expected)))
        for name, expected_hash in manifest.items():
            digest = hashlib.sha256()
            with archive.open(files[name]) as stream:
                while True:
                    chunk = stream.read(1024 * 1024)
                    if not chunk:
                        break
                    digest.update(chunk)
            if digest.hexdigest() != expected_hash:
                raise VerificationError('SHA-256 mismatch: ' + name)
    return len(manifest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', help='source host-kit ZIP to verify')
    args = parser.parse_args()
    try:
        count = verify(args.archive)
    except (VerificationError, OSError, zipfile.BadZipFile, RuntimeError, NotImplementedError, EOFError, RecursionError, zlib.error) as exc:
        print('Verification failed: ' + str(exc), file=sys.stderr)
        return 1
    print('Verified %d files against the embedded manifest (publisher not authenticated).' % count)
    return 0


if __name__ == '__main__':
    sys.exit(main())
