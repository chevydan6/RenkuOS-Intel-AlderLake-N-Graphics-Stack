#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Focused corruptions of actual compiled artifacts; no fabricated shader code."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent

class ArtifactChecks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='native-rcs-check-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'artifacts'
        shutil.copytree(HERE / 'artifacts', self.root)

    def run_checker(self):
        return subprocess.run([sys.executable, '-O', str(HERE / 'check_artifacts.py'),
                               str(self.root)], capture_output=True, text=True)

    def mutate(self, change):
        path = self.root / 'metadata.json'
        metadata = json.loads(path.read_text())
        change(metadata)
        path.write_text(json.dumps(metadata))

    def reject(self):
        result = self.run_checker()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn('FAIL:', result.stderr)

    def test_actual_artifacts(self):
        result = self.run_checker()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_position_vue_slot_out_of_range(self):
        self.mutate(lambda m: m['vs']['varying_to_slot'].__setitem__(0, 999))
        self.reject()

    def test_simd16_aliases_simd8(self):
        self.mutate(lambda m: m['fs'].__setitem__('prog_offset_16', 0))
        self.reject()

    def test_other_supported_profile_rejections(self):
        original = (self.root / 'metadata.json').read_bytes()
        mutations = {
            'scratch': lambda m: m['vs'].__setitem__('total_scratch', 1024),
            'relocation': lambda m: m['fs'].__setitem__('num_relocs', 1),
            'offset': lambda m: m['fs'].__setitem__('prog_offset_16', 4096),
        }
        for name, change in mutations.items():
            with self.subTest(name=name):
                (self.root / 'metadata.json').write_bytes(original)
                self.mutate(change)
                self.reject()
        (self.root / 'metadata.json').write_bytes(original)
        code = self.root / 'vs.bin'
        code.write_bytes(bytes(code.stat().st_size))
        self.reject()
        (self.root / 'metadata.json').unlink()
        self.reject()

if __name__ == '__main__':
    unittest.main()
