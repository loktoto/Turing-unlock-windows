#!/usr/bin/env python3
"""Package only source and docs, with a per-file integrity manifest."""
from pathlib import Path
import hashlib
import json
import sys
import zipfile
root = Path(__file__).resolve().parents[1]
output = Path(sys.argv[1]).resolve()
files = [root / n for n in ('CMakeLists.txt', 'LICENSE', 'README.md', 'UPSTREAM_LOCK.md', '.gitignore', '.gitattributes', 'CHANGELOG.md', 'CONTRIBUTING.md')]
for directory in ('host', 'tests', 'scripts', 'docs', '.github'):
    files.extend(p for p in (root / directory).rglob('*')
                 if p.is_file() and '__pycache__' not in p.parts and p.name != 'AGENT_HANDOVER.md')
manifest = {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(files)}
manifest_path = root / 'SOURCE_MANIFEST.json'
manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
files.append(manifest_path)
with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
    for p in sorted(files):
        archive.write(p, 'TU102_Windows_Host_Test_Kit/' + p.relative_to(root).as_posix())
with zipfile.ZipFile(output) as archive:
    assert archive.testzip() is None
    for name, digest in manifest.items():
        assert hashlib.sha256(archive.read('TU102_Windows_Host_Test_Kit/' + name)).hexdigest() == digest
print(hashlib.sha256(output.read_bytes()).hexdigest(), output)
