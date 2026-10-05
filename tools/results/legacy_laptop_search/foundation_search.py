#!/usr/bin/env python3
"""Short evolutionary search over the foundation's dynamics settings.

Every candidate is scored at the small and dev sizes on a strong-input seed (22) and a
weak-input seed (38): steadiness under held input (similarity between consecutive 3D updates),
volume use (share of depth layers used by the deep fields), reliability and recall gain.
Settings not listed are the default (evolved) rule.

Usage: python tools/foundation_search.py [--generations 3] [--population 8]
"""

import argparse
import json
import os
import pathlib
import random
import re
import statistics as st
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = ROOT / "build" / "cosmos_x1.exe"
LOG = ROOT / "tools" / "foundation_log.jsonl"

SPACE = {  # name: (low, high, kind, start)
    "link4d_spread_gain": (2.0, 16.0, "f", 4.0),
    "agc_rate": (0.01, 0.2, "f", 0.05),
    "agc_relax_field": (0.0, 0.3, "f", 0.1),
    "fatigue_gain3": (0.0, 0.2, "f", 0.066),
    "fatigue_gain2": (0.2, 1.5, "f", 1.01),
    "winners3": (2, 4, "i", 2),
    "fire_gain3": (2.0, 8.0, "f", 4.36),
    "fire_threshold3": (0.01, 0.1, "f", 0.052),
    "input_depth_gain": (0.05, 0.4, "f", 0.2),
    "link4d": (0.1, 0.4, "f", 0.2),
    "voxel_self": (0.0, 0.1, "f", 0.0165),
}
CASES = [("small", 22), ("small", 38), ("dev", 22), ("dev", 38)]


def run(test, preset, seed, params):
    args = [str(EXE), "--test", test, "--preset", preset, "--seed", str(seed)]
    for k, v in params.items():
        args += ["--set", f"{k}={v}"]
    return subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=3600,
                          env=dict(os.environ, OMP_NUM_THREADS="7")).stdout


def evaluate_case(params, preset, seed):
    s = run("settle", preset, seed, params)
    m = re.search(r"consecutive-tick similarity, last 20 ticks:(.*)", s)
    sims = [float(x) for x in m.group(1).split()] if m else [0.0]
    steps = [x for x in sims if x < 0.999] or [1.0]  # only ticks where a 3D update happened
    steady = st.mean(steps)
    o = run("occupancy", preset, seed, params)
    layers = [int(a) / int(b) for a, b in re.findall(r"field [123]:.*?(\d+) of (\d+) layers", o)]
    volume = st.mean(layers) if layers else 0.0
    r = run("recall", preset, seed, params)
    rel = re.search(r"own ([0-9.]+) vs others ([0-9.]+) \(need", r)
    reliability = float(rel.group(1)) if rel else 0.0
    g = re.search(r"specificity \(own minus best other\):.*?gain ([+-][0-9.]+)", r)
    gain = float(g.group(1)) if g else -1.0
    return {"preset": preset, "seed": seed, "steady": steady, "volume": volume, "reliability": reliability,
            "recall": gain}


def score(cases):
    total = 0.0
    for c in cases:
        total += 2.0 * c["steady"] + c["volume"]
        total += 1.0 if c["reliability"] >= 0.5 else 2.0 * c["reliability"] - 1.0
        total += max(-2.0, min(2.0, c["recall"] / 0.05))
    return total / len(cases)


def mutate(p, rng, strength):
    c = dict(p)
    for k, (lo, hi, kind, _) in SPACE.items():
        if rng.random() > 0.4:
            continue
        if kind == "i":
            c[k] = int(min(hi, max(lo, c[k] + rng.choice((-1, 1)))))
        else:
            c[k] = round(min(hi, max(lo, c[k] * (2.0 ** rng.gauss(0.0, strength)))), 4)
    return c


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--generations", type=int, default=3)
    ap.add_argument("--population", type=int, default=8)
    ap.add_argument("--rng", type=int, default=5)
    args = ap.parse_args()
    rng = random.Random(args.rng)
    base = {k: v[3] for k, v in SPACE.items()}
    population = [base] + [mutate(base, rng, 0.6) for _ in range(args.population - 1)]
    best_hist = []
    for gen in range(args.generations):
        t0 = time.time()
        jobs = [(i, p, preset, seed) for i, p in enumerate(population) for preset, seed in CASES]
        with ThreadPoolExecutor(max_workers=2) as pool:
            outs = list(pool.map(lambda j: evaluate_case(j[1], j[2], j[3]), jobs))
        results = []
        for i, p in enumerate(population):
            cases = outs[i * len(CASES):(i + 1) * len(CASES)]
            r = {"generation": gen, "index": i, "params": p, "cases": cases, "fitness": score(cases)}
            results.append(r)
            with LOG.open("a", encoding="utf-8") as f:
                f.write(json.dumps(r) + "\n")
            print(f"gen {gen} cand {i}: fitness {r['fitness']:.2f} | " + "  ".join(
                f"{c['preset'][0]}{c['seed']}: steady {c['steady']:.2f} vol {c['volume']:.2f} rel {c['reliability']:.2f} "
                f"recall {c['recall']:+.3f}" for c in cases), flush=True)
        results.sort(key=lambda r: -r["fitness"])
        best_hist.append(results[0]["fitness"])
        print(f"gen {gen} done in {time.time() - t0:.0f}s, best fitness {results[0]['fitness']:.2f}: "
              f"{json.dumps(results[0]['params'])}", flush=True)
        elites = [r["params"] for r in results[:3]]
        population = elites + [mutate(rng.choice(elites), rng, 0.35) for _ in range(args.population - 3)]
    return 0


if __name__ == "__main__":
    sys.exit(main())
