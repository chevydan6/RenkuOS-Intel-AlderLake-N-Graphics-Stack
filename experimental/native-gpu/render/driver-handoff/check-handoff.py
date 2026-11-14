#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Check the complete golden source transformation, including fallback behavior."""
import argparse
import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location('prepare_driver', Path(__file__).with_name('prepare-driver.py'))
prepare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prepare)


def check(original, candidate):
    if 'render->acknowledge_display_profile(' not in candidate:
        raise ValueError('missing display-profile handoff callback')
    if candidate != prepare.transform(original):
        raise ValueError('candidate differs outside the permitted two insertions')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--candidate', required=True, type=Path)
    args = parser.parse_args()
    check(args.source.read_text(), args.candidate.read_text())
    print('DRIVER_HANDOFF_GOLDEN_CHECK_PASSED')
