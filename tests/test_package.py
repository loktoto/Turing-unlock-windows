"""Offline tests for host-kit ZIP verification."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import warnings
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERIFIER = ROOT / 'scripts' / 'verify_host_kit.py'
PREFIX = 'TU102_Windows_Host_Test_Kit/'
CONTENT = b'hello source\n'
DIGEST = hashlib.sha256(CONTENT).hexdigest()


class VerifyKitTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / 'kit.zip'

    def make_kit(self, manifest=None, entries=None):
        if manifest is None:
            manifest = json.dumps({'host/example.c': DIGEST})
        if entries is None:
            entries = [('host/example.c', CONTENT)]
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', UserWarning)
            with zipfile.ZipFile(self.path, 'w', zipfile.ZIP_DEFLATED) as archive:
                archive.writestr(PREFIX + 'SOURCE_MANIFEST.json', manifest)
                for name, data in entries:
                    archive.writestr(PREFIX + name, data)

    def run_verify(self, valid, message):
        result = subprocess.run([sys.executable, str(VERIFIER), str(self.path)],
                                capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode == 0, valid, result.stderr)
        self.assertIn(message, result.stdout if valid else result.stderr)

    def test_valid_kit(self):
        self.make_kit()
        self.run_verify(True, 'Verified 1 files')

    def test_corrupted_content(self):
        self.make_kit(entries=[('host/example.c', b'changed')])
        self.run_verify(False, 'SHA-256 mismatch')

    def test_duplicate_entry(self):
        self.make_kit(entries=[('host/example.c', CONTENT), ('host/example.c', CONTENT)])
        self.run_verify(False, 'duplicate')

    def test_case_collision(self):
        self.make_kit(entries=[('host/example.c', CONTENT), ('HOST/example.c', CONTENT)])
        self.run_verify(False, 'case-colliding')

    def test_traversal(self):
        self.make_kit(entries=[('../escape', CONTENT)])
        self.run_verify(False, 'invalid path')

    def test_missing_entry(self):
        self.make_kit(entries=[])
        self.run_verify(False, 'missing')

    def test_undeclared_entry(self):
        self.make_kit(entries=[('host/example.c', CONTENT), ('extra', b'extra')])
        self.run_verify(False, 'undeclared')

    def test_invalid_manifests(self):
        for manifest in ('{', '[]', '{}', '{"x":"bad"}',
                         '{"x":"' + DIGEST + '","x":"' + DIGEST + '"}',
                         json.dumps({'../escape': DIGEST})):
            with self.subTest(manifest=manifest):
                self.make_kit(manifest=manifest)
                self.run_verify(False, 'Verification failed')

    def test_outside_prefix(self):
        self.make_kit()
        with zipfile.ZipFile(self.path, 'a') as archive:
            archive.writestr('outside', b'x')
        self.run_verify(False, 'outside kit')

    def test_bad_zip(self):
        self.path.write_bytes(b'not a zip')
        self.run_verify(False, 'Verification failed')


if __name__ == '__main__':
    unittest.main()
