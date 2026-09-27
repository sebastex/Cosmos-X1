#!/usr/bin/env python3
"""Separates the two sources of seed-to-seed variation in the memory checks: the matrix's
random wiring (--seed) and the items (character fingerprints and cue thinning, codebook_seed).

Usage: python tools/noise_probe.py --params @file [--tests order,continual] [--preset small]
The params file holds "--set name=value ..." as written by the analysis scripts.
"""

import argparse
import os
import pathlib
import re
import statistics
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = ROOT / "build" / "cosmos_x1.exe"

METRICS = {
    "order": [("forward", r"cue of first item evokes second:.*?gain ([+-][0-9.]+)"),
              ("order", r"forward minus backward:.*?gain ([+-][0-9.]+)")],
    "continual": [("old_after", r"old memories, after new learning:.*?gain ([+-][0-9.]+)"),
                  ("new", r"new memories:.*?gain ([+-][0-9.]+)"),
                  ("forgetting", r"forgetting of old memories: ([+-][0-9.]+)")],
    "efficiency": [("eff30", r"exposure 30 ticks:.*?gain ([+-][0-9.]+)")],
    "streamed": [("streamed", r"streamed words:.*?gain ([+-][0-9.]+)")],
    "capacity": [("capacity", r"8 memories:.*?gain ([+-][0-9.]+)")],
}


def run(test, preset, seed, codebook, params):
    args = [str(EXE), "--test", test, "--preset", preset, "--seed", str(seed)] + params
    args += ["--set", f"codebook_seed={codebook}"]
    env = dict(os.environ, OMP_NUM_THREADS="7")
    out = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=3600, env=env).stdout
    vals = {}
    for name, pattern in METRICS[test]:
        m = re.search(pattern, out)
        vals[name] = float(m.group(1)) if m else float("nan")
    return vals


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--params", required=True)
    ap.add_argument("--tests", default="order,continual")
    ap.add_argument("--preset", default="small")
    ap.add_argument("--wiring", default="11,12,13,14")
    ap.add_argument("--items", default="21,22,23,24")
    ap.add_argument("--fixed-wiring", type=int, default=11)
    ap.add_argument("--fixed-items", type=int, default=99)
    args = ap.parse_args()
    text = pathlib.Path(args.params[1:]).read_text() if args.params.startswith("@") else args.params
    params = text.split()
    tests = args.tests.split(",")
    jobs = []
    for w in args.wiring.split(","):
        jobs.append(("wiring varies", int(w), args.fixed_items))
    for c in args.items.split(","):
        jobs.append(("items vary", args.fixed_wiring, int(c)))
    results = {}
    with ThreadPoolExecutor(max_workers=2) as pool:
        futures = {(label, w, c, t): pool.submit(run, t, args.preset, w, c, params)
                   for label, w, c in jobs for t in tests}
        for key, f in futures.items():
            results[key] = f.result()
    for t in tests:
        for label in ("wiring varies", "items vary"):
            rows = [(w, c, results[(label, w, c, t)]) for (lb, w, c, tt) in results if lb == label and tt == t]
            print(f"{t} / {label}:")
            for w, c, v in rows:
                print(f"   wiring {w:3d} items {c:3d}: " + "  ".join(f"{k} {x:+.3f}" for k, x in v.items()))
            for name, _ in METRICS[t]:
                xs = [v[name] for _, _, v in rows]
                print(f"   {name}: mean {statistics.mean(xs):+.3f}, spread (sd) {statistics.pstdev(xs):.3f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
