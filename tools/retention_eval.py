#!/usr/bin/env python3
"""Runs the CP6b retention test (8 memories, then 16 more) on many seeds, two at a time.

Usage: python tools/retention_eval.py --seeds 21-40 [--set name=value ...]
"""

import argparse
import os
import pathlib
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = ROOT / "build" / "cosmos_x1.exe"


def run(seed, sets, preset):
    args = [str(EXE), "--test", "retention", "--preset", preset, "--seed", str(seed)]
    for s in sets:
        args += ["--set", s]
    out = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=3600,
                         env=dict(os.environ, OMP_NUM_THREADS="7")).stdout
    m = re.search(r"after 16 new accuracy\s+(\d+)% margin ([+-][0-9.]+)", out)
    lost = re.search(r"margin lost ([+-][0-9.]+)", out)
    gain = re.search(r"gain over untrained ([+-][0-9.]+)", out)
    ok = "correctly: yes" in out and ": NO" not in out
    return seed, ok, (m.group(1) if m else "?"), (lost.group(1) if lost else "?"), (gain.group(1) if gain else "?")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", default="21-40")
    ap.add_argument("--preset", default="small")
    ap.add_argument("--set", action="append", default=[])
    args = ap.parse_args()
    a, b = (int(x) for x in args.seeds.split("-"))
    seeds = list(range(a, b + 1))
    passed = 0
    with ThreadPoolExecutor(max_workers=2) as pool:
        for seed, ok, acc, lost, gain in pool.map(lambda s: run(s, args.set, args.preset), seeds):
            passed += ok
            print(f"seed {seed}: {'PASS' if ok else 'FAIL'}  accuracy after 16 new {acc}%  margin lost {lost}  "
                  f"gain over untrained {gain}", flush=True)
    print(f"retention (8+16): {passed}/{len(seeds)} seeds pass", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
