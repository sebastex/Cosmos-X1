#!/usr/bin/env python3
"""Fast grid over settings using the Stage 0 check only (spread, sparse, settles).

Usage: python tools/grid_stage0.py --grid @tools/grid_fire.json [--preset small] [--seed 1]
The grid file maps each setting name to a list of values; every combination is run.
"""

import argparse
import itertools
import json
import os
import pathlib
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = ROOT / "build" / "cosmos_x1.exe"
THREADS = 7
RELIABILITY = False


def run(params, preset, seed):
    args = [str(EXE), "--test", "stage0", "--preset", preset, "--seed", str(seed)]
    for k, v in params.items():
        args += ["--set", f"{k}={v}"]
    env = dict(os.environ, OMP_NUM_THREADS=str(THREADS))
    flags = 0x00004000 if sys.platform == "win32" else 0  # below-normal priority
    out = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=1800, env=env,
                         creationflags=flags).stdout
    spread = re.search(r"Memory (\d+)% Reasoning (\d+)% Output (\d+)%", out)
    settles = re.search(r"settles.*?: (yes|NO) \(3D activity at input end ([0-9.]+), end ([0-9.]+)", out)
    sparse = re.search(r"sparse.*?: (yes|NO) \(peak mean ([0-9.]+), peak Input active ([0-9.]+)%", out)
    rel = None
    if RELIABILITY:
        args = [str(EXE), "--test", "reliability", "--preset", preset, "--seed", str(seed)]
        for k, v in params.items():
            args += ["--set", f"{k}={v}"]
        rout = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=1800, env=env,
                              creationflags=flags).stdout
        m = re.search(r"own ([0-9.]+) vs others ([0-9.]+)", rout)
        rel = (float(m.group(1)), float(m.group(2))) if m else (0.0, 1.0)
    return {
        "params": params,
        "reliability": rel,
        "spread": [int(x) for x in spread.groups()] if spread else None,
        "settles": settles.group(1) == "yes" if settles else False,
        "end_activity": (float(settles.group(2)), float(settles.group(3))) if settles else None,
        "sparse": sparse.group(1) == "yes" if sparse else False,
        "input_active": float(sparse.group(3)) if sparse else None,
        "pass": len(re.findall(r":\s+yes", out)) >= 5 and (rel is None or (rel[0] >= 0.5 and rel[0] >= rel[1] + 0.2)),
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--grid", required=True)
    ap.add_argument("--preset", default="small")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--reliability", action="store_true", help="also run the held-input reliability check")
    ap.add_argument("--fixed", default="", help="JSON dict of settings applied to every combination")
    args = ap.parse_args()
    global RELIABILITY
    RELIABILITY = args.reliability
    fixed = json.loads(pathlib.Path(args.fixed[1:]).read_text() if args.fixed.startswith("@") else (args.fixed or "{}"))
    text = args.grid
    if text.startswith("@"):
        text = pathlib.Path(text[1:]).read_text(encoding="utf-8")
    grid = json.loads(text)
    if isinstance(grid, list):  # an explicit list of combinations
        names = sorted({k for g in grid for k in g})
        combos = [{**fixed, **g} for g in grid]
    else:
        names = list(grid)
        combos = [{**fixed, **dict(zip(names, vals))} for vals in itertools.product(*(grid[n] for n in names))]
    with ThreadPoolExecutor(max_workers=2) as pool:
        for r in pool.map(lambda p: run(p, args.preset, args.seed), combos):
            label = " ".join(f"{k}={v}" for k, v in r["params"].items() if k in names)
            print(f"{label:<70} spread {r['spread']} settles {'yes' if r['settles'] else 'no '} "
                  f"end {r['end_activity']} input {r['input_active']}% rel {r['reliability']} {'PASS' if r['pass'] else ''}",
                  flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
