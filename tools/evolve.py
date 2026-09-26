#!/usr/bin/env python3
"""Evolutionary search over the NCM's rule parameters (spec Section 5E).

Each candidate is a set of rule parameters. It is scored against both Stage 0
(streamed text: spreads to all fields, stays sparse, settles) and Stage 1 (a partial
cue recalls the whole stored pattern better than an untrained twin). Every
generation uses a fresh random seed so no candidate can win on a lucky seed. A
candidate that passes both stages is re-checked on extra seeds; if it passes all of
them, the search stops and saves it.

Usage: python tools/evolve.py [--generations N] [--population N] [--preset small]
"""

import argparse
import json
import math
import pathlib
import random
import re
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = ROOT / "build" / "cosmos_x1.exe"
LOG = ROOT / "tools" / "evolve_log.jsonl"
BEST = ROOT / "tools" / "evolve_best.json"

# name: (low, high, kind, start). Start values = configuration A (memory passed 8/8).
SPACE = {
    "voxel_self": (0.1, 0.6, "f", 0.3),
    "voxel_neighbour": (0.02, 0.3, "f", 0.1),
    "long_range": (0.02, 0.4, "f", 0.15),
    "link4d": (0.05, 0.5, "f", 0.1),
    "link4d_backward": (0.0, 0.3, "f", 0.1),
    "upward_gain": (0.3, 4.0, "f", 1.0),
    "line_upward_gain": (0.3, 4.0, "f", 1.0),
    "normalize_upward": (0, 1, "b", 0),
    "downward_gain": (0.05, 1.0, "f", 0.3),
    "fatigue_gain2": (0.0, 1.5, "f", 0.6),
    "fatigue_gain3": (0.0, 1.5, "f", 0.6),
    "learning_rate": (0.02, 0.6, "f", 0.2),
    "plastic_budget": (0.1, 1.5, "f", 0.5),
    "encoding_suppression": (0.0, 1.0, "f", 0.8),
    "agc_rate": (0.0, 0.1, "f", 0.0),
    "agc_max": (1.0, 8.0, "f", 4.0),
    "channel_winners3": (2, 6, "i", 3),
    "winners3": (1, 6, "i", 3),
    "order_gain": (0.0, 2.0, "f", 0.5),
    "covariance": (0.0, 1.0, "f", 1.0),
}


def start_candidate():
    return {k: v[3] for k, v in SPACE.items()}


def mutate(cand, rng, strength):
    child = dict(cand)
    for name, (lo, hi, kind, _) in SPACE.items():
        if rng.random() > 0.35:  # mutate roughly a third of the parameters each time
            continue
        if kind == "b":
            child[name] = 1 - child[name]
        elif kind == "i":
            child[name] = int(min(hi, max(lo, child[name] + rng.choice((-1, 1)))))
        else:
            if lo > 0:
                value = child[name] * math.exp(rng.gauss(0.0, strength))
            else:
                value = child[name] + rng.gauss(0.0, strength * (hi - lo) * 0.5)
            child[name] = round(min(hi, max(lo, value)), 4)
    return child


def crossover(a, b, rng):
    return {k: (a[k] if rng.random() < 0.5 else b[k]) for k in SPACE}


def settings(cand):
    args = []
    for k, v in cand.items():
        args += ["--set", f"{k}={v}"]
    return args


def run(args, timeout=1800):
    try:
        out = subprocess.run([str(EXE)] + args, cwd=ROOT, capture_output=True, text=True, timeout=timeout)
        return out.returncode, out.stdout
    except subprocess.TimeoutExpired:
        return -1, ""


def stage0(cand, preset, seed):
    code, out = run(["--preset", preset, "--ticks", "400", "--input-ticks", "150", "--snap", "5", "--quiet",
                     "--seed", str(seed), "--out", "out/evo"] + settings(cand))
    flags = {
        "spread": bool(re.search(r"spread to Memory/Reasoning/Output: yes", out)),
        "sparse": bool(re.search(r"sparse .*?: yes", out)),
        "settles": bool(re.search(r"settles .*?: yes", out)),
    }
    rel = [float(x) for x in re.findall(r"(?:Memory|Reasoning|Output) (\d+)%", out)[:3]]
    deep = min(rel) if rel else 0.0
    return {"pass": code == 0, "deep_percent": deep, **flags}


def gain_after(label, out):
    m = re.search(re.escape(label) + r".*?gain ([+-][0-9.]+)", out)
    return float(m.group(1)) if m else -1.0


def stage1(cand, preset, seed):
    """The full Stage 1 memory suite (recall, capacity, efficiency, streamed text,
    continual learning, early order)."""
    code, out = run(["--test", "suite", "--preset", preset, "--seed", str(seed)] + settings(cand), timeout=5400)
    m = re.search(r"own ([0-9.]+) vs others ([0-9.]+) \(need", out)
    rel_own, rel_other = (float(m.group(1)), float(m.group(2))) if m else (0.0, 1.0)
    m = re.search(r"forgetting of old memories: ([+-][0-9.]+)", out)
    forgetting = float(m.group(1)) if m else 1.0
    return {
        "pass": code == 0,
        "reliability": rel_own,
        "reliability_other": rel_other,
        "recall": gain_after("specificity (own minus best other):", out),
        "capacity": gain_after("8 memories:", out),
        "efficiency30": gain_after("exposure 30 ticks:", out),
        "streamed": gain_after("streamed words:", out),
        "continual_old": gain_after("old memories, after new learning:", out),
        "continual_new": gain_after("new memories:", out),
        "forgetting": forgetting,
        "order_forward": gain_after("cue of first item evokes second:", out),
        "order_signal": gain_after("forward minus backward:", out),
        "checks_passed": len(re.findall(r"^\s+CP\d .*PASS$", out, re.M)),
        "gain": gain_after("specificity (own minus best other):", out),
    }


def clamp(x, lo=-2.0, hi=2.0):
    return max(lo, min(hi, x))


def score(s0, s1):
    """Every requirement counts. Each memory check contributes its gain relative to its bar
    (1.0 at the bar), so the search improves weak checks instead of maximizing one."""
    fitness = 0.0
    fitness += 1.0 * s0["spread"] + 1.0 * s0["sparse"] + 1.0 * s0["settles"]
    fitness += min(1.0, s0["deep_percent"] / 50.0)          # streamed text should reach deep fields
    fitness += 1.0 if (s1["reliability"] >= 0.5 and s1["reliability"] >= s1["reliability_other"] + 0.2) else 0.0
    for key in ("recall", "capacity", "efficiency30", "streamed"):
        fitness += clamp(s1[key] / 0.05)
    fitness += 0.5 * clamp(s1["continual_old"] / 0.05) + 0.5 * clamp(s1["continual_new"] / 0.05)
    fitness -= max(0.0, s1["forgetting"] - 0.05) * 10.0
    fitness += 0.5 * clamp(s1["order_forward"] / 0.02) + 0.5 * clamp(s1["order_signal"] / 0.02)
    fitness += 0.5 * s1["checks_passed"]
    return fitness


def evaluate(cand, preset, seed):
    s0 = stage0(cand, preset, seed)
    s1 = stage1(cand, preset, seed)
    return {"fitness": score(s0, s1), "passes_both": s0["pass"] and s1["pass"], "stage0": s0, "stage1": s1}


def log(entry):
    with LOG.open("a", encoding="utf-8") as f:
        f.write(json.dumps(entry) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--generations", type=int, default=12)
    ap.add_argument("--population", type=int, default=10)
    ap.add_argument("--elites", type=int, default=3)
    ap.add_argument("--preset", default="small")
    ap.add_argument("--verify-seeds", type=int, default=3)
    ap.add_argument("--rng", type=int, default=7)
    args = ap.parse_args()

    rng = random.Random(args.rng)
    base = start_candidate()
    population = [base] + [mutate(base, rng, 0.3) for _ in range(args.population - 1)]
    verified_seeds = set()

    for gen in range(args.generations):
        seed = rng.randint(100, 10_000)
        t0 = time.time()
        results = []
        for i, cand in enumerate(population):
            r = evaluate(cand, args.preset, seed)
            r.update({"generation": gen, "seed": seed, "index": i, "params": cand})
            log(r)
            results.append(r)
            s1 = r["stage1"]
            print(f"gen {gen} cand {i}: fitness {r['fitness']:.2f} all={r['passes_both']} checks={s1['checks_passed']}/6 "
                  f"deep={r['stage0']['deep_percent']:.0f}% recall={s1['recall']:+.3f} cap={s1['capacity']:+.3f} "
                  f"eff30={s1['efficiency30']:+.3f} stream={s1['streamed']:+.3f} "
                  f"old={s1['continual_old']:+.3f} order={s1['order_signal']:+.3f}", flush=True)

            if r["passes_both"]:
                # Re-check on fresh seeds before believing it.
                ok = True
                for _ in range(args.verify_seeds):
                    vseed = rng.randint(10_001, 20_000)
                    v = evaluate(cand, args.preset, vseed)
                    v.update({"generation": gen, "seed": vseed, "index": i, "params": cand, "verification": True})
                    log(v)
                    print(f"  verify seed {vseed}: all={v['passes_both']} checks={v['stage1']['checks_passed']}/6", flush=True)
                    if not v["passes_both"]:
                        ok = False
                        break
                    verified_seeds.add(vseed)
                if ok:
                    BEST.write_text(json.dumps({"params": cand, "generation": gen, "verified": True}, indent=2))
                    print("VERIFIED: passes both stages on all verification seeds", flush=True)
                    return 0

        results.sort(key=lambda r: r["fitness"], reverse=True)
        BEST.write_text(json.dumps({"params": results[0]["params"], "fitness": results[0]["fitness"],
                                    "generation": gen, "verified": False}, indent=2))
        print(f"gen {gen} done in {time.time() - t0:.0f}s, best fitness {results[0]['fitness']:.2f}", flush=True)

        elites = [r["params"] for r in results[: args.elites]]
        strength = max(0.08, 0.3 * (1.0 - gen / max(1, args.generations)))
        children = []
        while len(children) < args.population - len(elites):
            a, b = rng.sample(elites, 2) if len(elites) > 1 else (elites[0], elites[0])
            children.append(mutate(crossover(a, b, rng), rng, strength))
        population = elites + children

    print("no candidate verified within the generation limit; best so far in", BEST, flush=True)
    return 3


if __name__ == "__main__":
    sys.exit(main())
