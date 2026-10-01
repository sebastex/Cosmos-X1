#!/usr/bin/env python3
"""Baseline evaluation: runs Stage 0 and the full Stage 1 memory suite (no early exit) for a set
of variants on the same fresh seeds, two runs side by side, and reports per-check pass counts
and mean gains. Variants are partial settings applied on top of the default (evolved) rule.

Usage: python tools/baseline_eval.py --variants @tools/baseline_variants.json --seeds 21,22,23,24,25
"""

import argparse
import json
import os
import pathlib
import re
import statistics as st
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = pathlib.Path(os.environ["NCM_EXE"]) if "NCM_EXE" in os.environ else ROOT / "build" / "cosmos_x1.exe"
REPORT = ROOT / "tools" / "baseline_report.json"


def gain(label, out):
    m = re.search(re.escape(label) + r".*?gain ([+-][0-9.]+)", out)
    return float(m.group(1)) if m else float("nan")


def run(name, params, preset, seed, threads):
    sets = []
    for k, v in params.items():
        sets += ["--set", f"{k}={v}"]
    env = dict(os.environ, OMP_NUM_THREADS=str(threads))
    base = [str(EXE), "--preset", preset, "--seed", str(seed)] + sets
    s0 = subprocess.run(base + ["--test", "stage0", "--quiet", "--out", f"out/base_{name}_{seed}"], cwd=ROOT,
                        capture_output=True, text=True, env=env, timeout=3600).stdout
    s1 = subprocess.run(base + ["--test", "suite"], cwd=ROOT, capture_output=True, text=True, env=env,
                        timeout=7200).stdout
    (ROOT / "out").mkdir(exist_ok=True)
    (ROOT / "out" / f"suite_{name}_{preset}_{seed}.txt").write_text(s0 + "\n" + s1)
    m = re.search(r"forgetting of old memories: ([+-][0-9.]+)", s1)
    deep = [int(x) for x in re.findall(r"(?:Memory|Reasoning|Output) (\d+)%", s0)[:3]]
    return {
        "variant": name, "seed": seed,
        "stage0": len(re.findall(r":\s+yes", s0)) >= 5, "deep": min(deep) if deep else 0,
        "reliable": bool(re.search(r"input-driven .*: yes", s1)),
        "checks": {k: bool(re.search(r"^\s+" + re.escape(label) + r"\s+PASS", s1, re.M)) for k, label in (
            ("recall", "CP2 recall (3 memories)"), ("capacity", "CP3 capacity (8 memories)"),
            ("efficiency", "CP4 efficiency"), ("wordpairs", "CP5w word pairs (8)"),
            ("continual", "CP6 continual learning"), ("retention", "CP6b retention (8+16)"),
            ("order", "CP7 early order"))},
        "gains": {"recall": gain("specificity (own minus best other):", s1), "capacity": gain("8 memories:", s1),
                  "efficiency": gain("exposure 30 ticks:", s1), "wordpairs": float("nan"),
                  "forgetting": float(m.group(1)) if m else float("nan"),
                  "order": gain("forward minus backward:", s1)},
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--variants", required=True)
    ap.add_argument("--seeds", default="21,22,23,24,25")
    ap.add_argument("--preset", default="small")
    ap.add_argument("--parallel", type=int, default=2)
    ap.add_argument("--threads", type=int, default=7)
    args = ap.parse_args()
    text = pathlib.Path(args.variants[1:]).read_text() if args.variants.startswith("@") else args.variants
    variants = json.loads(text)  # {name: {setting: value}}
    seeds = [int(s) for s in args.seeds.split(",")]
    jobs = [(n, p, sd) for n, p in variants.items() for sd in seeds]
    results = []
    with ThreadPoolExecutor(max_workers=args.parallel) as pool:
        for r in pool.map(lambda j: run(j[0], j[1], args.preset, j[2], args.threads), jobs):
            results.append(r)
            REPORT.write_text(json.dumps(results, indent=1))
            print(f"{r['variant']:<22} seed {r['seed']}: stage0 {'ok' if r['stage0'] else 'FAIL'} deep {r['deep']}% "
                  + " ".join(f"{k} {'Y' if v else '-'}" for k, v in r["checks"].items()), flush=True)
    print("\n=== summary (passes out of %d seeds; mean gain) ===" % len(seeds))
    for n in variants:
        rs = [r for r in results if r["variant"] == n]
        s0 = sum(r["stage0"] for r in rs)
        parts = []
        for k in ("recall", "capacity", "efficiency", "wordpairs", "continual", "retention", "order"):
            g = "forgetting" if k == "continual" else k
            if g not in rs[0]["gains"]:
                parts.append(f"{k} {sum(r['checks'][k] for r in rs)}/{len(rs)}")
                continue
            vals = [r["gains"][g] for r in rs if r["gains"][g] == r["gains"][g]]
            parts.append(f"{k} {sum(r['checks'][k] for r in rs)}/{len(rs)} ({st.mean(vals):+.3f})" if vals else k)
        allsix = sum(all(r["checks"].values()) and r["stage0"] for r in rs)
        print(f"{n:<22} stage0 {s0}/{len(rs)} | " + "  ".join(parts) + f" | ALL {allsix}/{len(rs)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
