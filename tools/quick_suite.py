#!/usr/bin/env python3
"""Quick Stage 1 check on one brain: the suite's tests run side by side as separate processes
(each with a few threads), which finishes far sooner on a laptop than one process running them
one after another. Prints one line per check and a summary.

Usage: python tools/quick_suite.py --seed 33 [--preset dev] [--set key=value ...] [--threads 2]
"""

import argparse
import os
import pathlib
import re
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = pathlib.Path(os.environ.get("NCM_EXE", ROOT / "build" / "cosmos_x1.exe"))

# (name, test arguments). Every test's exit code is 0 when it passes.
CHECKS = [
    ("recall", ["--test", "recall"]),
    ("capacity", ["--test", "capacity"]),
    ("efficiency", ["--test", "efficiency"]),
    ("wordpairs", ["--test", "pairload", "--store-ticks", "8"]),
    ("continual", ["--test", "continual"]),
    ("retention", ["--test", "retention"]),
    ("order", ["--test", "order"]),
]


def run(check, args, outdir):
    name, extra = check
    env = dict(os.environ, OMP_NUM_THREADS=str(args.threads))
    cmd = [str(EXE), "--preset", args.preset, "--seed", str(args.seed)] + extra
    for s in args.set:
        cmd += ["--set", s]
    t0 = time.time()
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, env=env)
    (outdir / f"{name}.txt").write_text(proc.stdout)
    why = [l.strip() for l in proc.stdout.splitlines() if re.search(r"FAIL|: NO|\(need .*\): NO", l)]
    return name, proc.returncode == 0, time.time() - t0, why[-1] if why else ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, required=True)
    ap.add_argument("--preset", default="dev")
    ap.add_argument("--set", action="append", default=[])
    ap.add_argument("--threads", type=int, default=2)
    ap.add_argument("--out", default=None)
    args = ap.parse_args()
    outdir = pathlib.Path(args.out or ROOT / "out" / f"quick_{args.seed}")
    outdir.mkdir(parents=True, exist_ok=True)
    t0 = time.time()
    with ThreadPoolExecutor(max_workers=len(CHECKS)) as pool:
        results = list(pool.map(lambda c: run(c, args, outdir), CHECKS))
    for name, ok, secs, why in results:
        print(f"  {name:<11} {'PASS' if ok else 'FAIL'}   ({secs / 60:.0f} min)" + ("" if ok else f"   {why[:150]}"), flush=True)
    passed = sum(ok for _, ok, _, _ in results)
    print(f"brain {args.seed}: {passed}/{len(results)} checks pass, {(time.time() - t0) / 60:.0f} min total; outputs in {outdir}")
    return 0 if passed == len(results) else 3


if __name__ == "__main__":
    sys.exit(main())
