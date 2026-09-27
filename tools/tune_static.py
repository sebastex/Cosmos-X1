#!/usr/bin/env python3
"""Focused tuning of the borderline static-memory checks (CP3 capacity, CP4 efficiency,
CP6 continual learning) at a realistic size, one setting at a time around the current
configuration, each variant on several seeds. Prints a table and writes a JSON report.

Usage: python tools/tune_static.py [--preset small] [--seeds 1,2]
"""

import argparse
import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = ROOT / "build" / "cosmos_x1.exe"
REPORT = ROOT / "tools" / "tune_static_report.json"

# One change at a time around the current configuration.
VARIANTS = [
    {},
    {"learning_rate": 0.15}, {"learning_rate": 0.3},
    {"plastic_budget": 0.35}, {"plastic_budget": 0.7},
    {"channel_winners3": 2}, {"channel_winners3": 4},
    {"winners3": 2}, {"winners3": 4},
    {"fatigue_gain3": 0.4}, {"fatigue_gain3": 0.9},
    {"covariance": 0.5},
    {"order_gain": 0.2},
    {"encoding_suppression": 0.95},
]


def gain(label, out):
    m = re.search(re.escape(label) + r".*?gain ([+-][0-9.]+)", out)
    return float(m.group(1)) if m else -1.0


THREADS = 7  # two variants side by side, 7 threads each (measured best throughput)


def run(test, preset, seed, params):
    import os
    args = [str(EXE), "--test", test, "--preset", preset, "--seed", str(seed)]
    for k, v in params.items():
        args += ["--set", f"{k}={v}"]
    env = dict(os.environ, OMP_NUM_THREADS=str(THREADS))
    flags = 0x00004000 if sys.platform == "win32" else 0  # below-normal priority
    out = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=7200, env=env, creationflags=flags)
    return out.returncode == 0, out.stdout


def evaluate(params, preset, seeds):
    rows = []
    for seed in seeds:
        ok3, o3 = run("capacity", preset, seed, params)
        ok4, o4 = run("efficiency", preset, seed, params)
        ok6, o6 = run("continual", preset, seed, params)
        m = re.search(r"forgetting of old memories: ([+-][0-9.]+)", o6)
        rows.append({
            "seed": seed,
            "capacity": gain("8 memories:", o3), "capacity_pass": ok3,
            "efficiency30": gain("exposure 30 ticks:", o4), "efficiency_pass": ok4,
            "continual_old": gain("old memories, after new learning:", o6),
            "continual_new": gain("new memories:", o6),
            "forgetting": float(m.group(1)) if m else 1.0, "continual_pass": ok6,
        })
    passes = sum(r["capacity_pass"] + r["efficiency_pass"] + r["continual_pass"] for r in rows)
    score = sum(min(r["capacity"], 0.2) + min(r["efficiency30"], 0.2) + min(r["continual_old"], 0.2)
                - max(0.0, r["forgetting"] - 0.05) * 3 for r in rows) / len(rows)
    return {"params": params, "passes": passes, "of": 3 * len(rows), "score": score, "rows": rows}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preset", default="small")
    ap.add_argument("--seeds", default="1,2")
    args = ap.parse_args()
    seeds = [int(s) for s in args.seeds.split(",")]

    from concurrent.futures import ThreadPoolExecutor
    results = []
    pool = ThreadPoolExecutor(max_workers=2)
    futures = [pool.submit(evaluate, params, args.preset, seeds) for params in VARIANTS]
    for params, f in zip(VARIANTS, futures):
        r = f.result()  # reported in order, as each finishes
        results.append(r)
        REPORT.write_text(json.dumps(results, indent=2))
        rs = r["rows"]
        label = ", ".join(f"{k}={v}" for k, v in params.items()) or "current configuration"
        print(f"{label:<28} passes {r['passes']}/{r['of']}  score {r['score']:+.3f}  | "
              + "  ".join(f"s{x['seed']}: cap {x['capacity']:+.3f} eff {x['efficiency30']:+.3f} "
                          f"old {x['continual_old']:+.3f} forget {x['forgetting']:+.3f}" for x in rs), flush=True)
    best = max(results, key=lambda r: (r["passes"], r["score"]))
    print(f"\nbest single change: {best['params'] or 'none'} ({best['passes']}/{best['of']} passes, score {best['score']:+.3f})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
