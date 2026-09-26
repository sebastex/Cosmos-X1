#!/usr/bin/env python3
"""Multi-size verification (scale invariance, spec Section 8A).

Runs Stage 0 (streamed text) and the full Stage 1 memory suite for one set of rule
parameters at several sizes and seeds, and reports pass/fail per size. A configuration
passes only if it passes at every size.

Sizes are presets (tiny, small, dev, full) or custom "FxSxL" (field, sheet, line), e.g. 10x8x16.

Usage:
  python tools/verify_multisize.py --params tools/evolve_best.json --sizes small,dev --seeds 1,2,3
  python tools/verify_multisize.py --sizes small,dev          (current built-in defaults)
"""

import argparse
import json
import pathlib
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = ROOT / "build" / "cosmos_x1.exe"
REPORT = ROOT / "tools" / "multisize_report.json"


def size_args(size):
    if "x" in size:
        f, s, l = size.split("x")
        return ["--preset", "tiny", "--set", f"field_dim={f}", "--set", f"sheet_dim={s}", "--set", f"line_len={l}"]
    return ["--preset", size]


def load_params(path):
    if not path:
        return {}
    data = json.loads(pathlib.Path(path).read_text())
    if isinstance(data, list):  # a MAP-Elites archive: take the fittest member
        data = max(data, key=lambda e: e.get("fitness", -1e9))
    return data.get("params", data)


def run(args, timeout=14400):
    out = subprocess.run([str(EXE)] + args, cwd=ROOT, capture_output=True, text=True, timeout=timeout)
    return out.returncode, out.stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--params", default="")
    ap.add_argument("--sizes", default="small,dev")
    ap.add_argument("--seeds", default="1,2,3")
    args = ap.parse_args()

    params = load_params(args.params)
    settings = []
    for k, v in params.items():
        settings += ["--set", f"{k}={v}"]
    sizes = [s.strip() for s in args.sizes.split(",") if s.strip()]
    seeds = [int(s) for s in args.seeds.split(",") if s.strip()]

    report = {"params": params, "results": []}
    all_pass = True
    for size in sizes:
        size_pass = True
        for seed in seeds:
            t0 = time.time()
            c0, o0 = run(size_args(size) + ["--ticks", "400", "--input-ticks", "150", "--snap", "5", "--quiet",
                                            "--seed", str(seed), "--out", "out/multisize"] + settings)
            c1, o1 = run(["--test", "suite", "--seed", str(seed)] + size_args(size) + settings)
            summary = [l.strip() for l in o1.splitlines() if l.strip().startswith("CP") and l.strip().endswith(("PASS", "FAIL"))]
            ok = c0 == 0 and c1 == 0
            size_pass = size_pass and ok
            report["results"].append({"size": size, "seed": seed, "stage0": c0 == 0, "suite": c1 == 0,
                                      "checks": summary, "seconds": round(time.time() - t0)})
            print(f"{size:>10} seed {seed}: stage0 {'PASS' if c0 == 0 else 'FAIL'}, suite {'PASS' if c1 == 0 else 'FAIL'} "
                  f"({time.time() - t0:.0f}s)", flush=True)
            for line in summary:
                print(f"{'':>14}{line}", flush=True)
        print(f"{size:>10}: {'PASS' if size_pass else 'FAIL'} at every seed" if size_pass else f"{size:>10}: FAIL", flush=True)
        all_pass = all_pass and size_pass
    report["pass"] = all_pass
    REPORT.write_text(json.dumps(report, indent=2))
    print(f"\nmulti-size verification: {'PASS' if all_pass else 'FAIL'} (report: {REPORT})", flush=True)
    return 0 if all_pass else 3


if __name__ == "__main__":
    sys.exit(main())
