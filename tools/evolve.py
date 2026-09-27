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
    # Added after the night-1 diagnosis (streamed input is amplitude-limited): output
    # normalization and normalized learning, plus the levers that make normalized activity settle.
    "output_sigma": (0.0, 0.6, "f", 0.0),
    "normalized_plasticity": (0, 1, "b", 0),
    "homeostasis3": (0.0005, 0.02, "f", 0.001),
    "fatigue_tau3": (5.0, 80.0, "f", 20.0),
}


# Firing regime (all-or-none output, found 2026-09-27): signals keep their strength between
# fields. Start = the settings that passed Stage 0 streaming at small seed 1; graded-regime
# levers that conflict with firing are left out.
FIRING_DROP = ("output_sigma", "normalized_plasticity", "agc_rate", "agc_max", "normalize_upward")
FIRING_EXTRA = {
    "fire_threshold2": (0.005, 0.3, "f", 0.02),
    "fire_threshold3": (0.05, 0.8, "f", 0.35),
    "modulator_tau": (0.0, 60.0, "f", 0.0),
}
FIRING_START = {"link4d": 0.5, "fatigue_gain2": 0.2, "fatigue_gain3": 0.05, "voxel_neighbour": 0.03,
                "long_range": 0.05, "voxel_self": 0.3, "covariance": 0.5, "encoding_suppression": 0.95}


def use_firing_regime():
    global SPACE, LOG, BEST
    space = {k: v for k, v in SPACE.items() if k not in FIRING_DROP}
    for k, v in FIRING_START.items():
        lo, hi, kind, _ = space[k]
        space[k] = (lo, hi, kind, v)
    space.update(FIRING_EXTRA)
    SPACE = space
    LOG = ROOT / "tools" / "evolve_firing_log.jsonl"
    BEST = ROOT / "tools" / "evolve_firing_best.json"


# Rate-coded regime (2026-09-27): a steep, saturating firing curve (fire_gain) amplifies the
# forward paths while every loop (self, neighbours, long-range, sheet loops, voxel-sheet loop,
# learned memory) is scaled down by the gain, so activity reaches all fields, stays input-driven
# and settles. Start = the first configuration to pass Stage 0, reliability and recall together
# (small, seed 1).
RATE_DROP = ("output_sigma", "normalized_plasticity", "agc_rate", "agc_max", "normalize_upward")
RATE_SPACE = {
    "fire_gain2": (1.5, 20.0, "f", 5.0),
    "fire_gain3": (1.5, 20.0, "f", 5.0),
    "fire_threshold2": (0.001, 0.1, "f", 0.01),
    "fire_threshold3": (0.001, 0.1, "f", 0.01),
    "link4d": (0.02, 0.5, "f", 0.2),
    "link4d_backward": (0.0, 0.1, "f", 0.02),
    "voxel_self": (0.01, 0.3, "f", 0.06),
    "voxel_neighbour": (0.003, 0.1, "f", 0.02),
    "long_range": (0.003, 0.15, "f", 0.03),
    "sheet_self": (0.01, 0.3, "f", 0.06),
    "sheet_neighbour": (0.002, 0.06, "f", 0.012),
    "downward_gain": (0.001, 0.1, "f", 0.012),
    "learning_rate": (0.001, 0.1, "f", 0.01),
    "plastic_budget": (0.1, 4.0, "f", 1.0),
    "covariance": (0.0, 1.0, "f", 0.5),
    "encoding_suppression": (0.0, 1.0, "f", 0.95),
    "modulator_tau": (0.0, 60.0, "f", 0.0),
    "order_tau": (0.0, 30.0, "f", 0.0),  # order timing window (added after generation 3)
    # Order needs a strong order term *and* a timing window (measured: order_gain 4 with
    # order_tau 5 passes the early order check; either alone does not), so the range is wider.
    "order_gain": (0.0, 8.0, "f", 0.5),
}


def use_rate_regime():
    global SPACE, LOG, BEST
    space = {k: v for k, v in SPACE.items() if k not in RATE_DROP}
    space.update(RATE_SPACE)
    SPACE = space
    LOG = ROOT / "tools" / "evolve_rate_log.jsonl"
    BEST = ROOT / "tools" / "evolve_rate_best.json"


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
            elif child[name] == 0 and rng.random() < 0.5:
                # An off switch (0) is otherwise only nudged back to 0 half the time: jump to a
                # random value in the lower part of the range instead.
                value = rng.uniform(lo, lo + 0.3 * (hi - lo))
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


THREADS = 0        # set from --threads; 0 = all cores (full speed)
COOLDOWN = 0.0     # set from --cooldown; seconds of rest between runs


def run(args, timeout=1800):
    import os
    env = dict(os.environ)
    if THREADS > 0:
        env["OMP_NUM_THREADS"] = str(THREADS)
    flags = 0x00004000 if sys.platform == "win32" else 0  # BELOW_NORMAL_PRIORITY_CLASS
    try:
        out = subprocess.run([str(EXE)] + args, cwd=ROOT, capture_output=True, text=True, timeout=timeout,
                             env=env, creationflags=flags)
        result = out.returncode, out.stdout
    except subprocess.TimeoutExpired:
        result = -1, ""
    if COOLDOWN > 0:
        time.sleep(COOLDOWN)
    return result


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
    # --early-exit: the suite stops after its first test when recall is clearly worse than an
    # untrained matrix, so hopeless variants cost a quarter of a full run.
    code, out = run(["--test", "suite", "--early-exit", "--preset", preset, "--seed", str(seed)] + settings(cand),
                    timeout=5400)
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


def quick_recall(cand, preset, seed):
    """The 3-memory recall test alone (about a quarter of the full suite's cost)."""
    code, out = run(["--test", "recall", "--preset", preset, "--seed", str(seed)] + settings(cand))
    return gain_after("specificity (own minus best other):", out) if out else -1.0


def evaluate(cand, preset, seed):
    s0 = stage0(cand, preset, seed)
    # Early rejection: a variant whose basic recall is clearly worse than an untrained
    # matrix cannot pass the suite, so it is scored without running the rest.
    if REJECT_BELOW is not None:
        quick = quick_recall(cand, preset, seed)
        if quick < REJECT_BELOW:
            s1 = {"pass": False, "reliability": 0.0, "reliability_other": 1.0, "recall": quick,
                  "capacity": -1.0, "efficiency30": -1.0, "streamed": -1.0, "continual_old": -1.0,
                  "continual_new": -1.0, "forgetting": 1.0, "order_forward": -0.04, "order_signal": -0.04,
                  "checks_passed": 0, "gain": quick, "rejected_early": True}
            return {"fitness": score(s0, s1), "passes_both": False, "stage0": s0, "stage1": s1}
    s1 = stage1(cand, preset, seed)
    return {"fitness": score(s0, s1), "passes_both": s0["pass"] and s1["pass"], "stage0": s0, "stage1": s1}


REJECT_BELOW = None  # separate quick-recall rejection (off: the suite's --early-exit does this without duplicate work)


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
    ap.add_argument("--threads", type=int, default=0,
                    help="CPU threads per run (0 = all cores, split evenly across --parallel runs)")
    ap.add_argument("--parallel", type=int, default=2, help="variants evaluated side by side")
    ap.add_argument("--immigrants", type=int, default=2, help="strongly mutated explorers per generation")
    ap.add_argument("--cooldown", type=float, default=0.0, help="seconds of rest between runs")
    ap.add_argument("--reject-below", type=float, default=None,
                    help="extra quick-recall rejection threshold (off by default; the suite exits early itself)")
    ap.add_argument("--resume", action="store_true", help="continue from the best variants in the log")
    ap.add_argument("--seeds-per-gen", type=int, default=1,
                    help="seeds each variant is tested on per generation (fitness = their mean)")
    ap.add_argument("--inject", default="",
                    help="JSON list (or @file) of partial parameter dicts added to the first population")
    ap.add_argument("--regime", default="graded", choices=("graded", "firing", "rate"),
                    help="firing: search the all-or-none firing regime (own log and best file)")
    args = ap.parse_args()
    if args.regime == "firing":
        use_firing_regime()
    elif args.regime == "rate":
        use_rate_regime()

    global THREADS, COOLDOWN, REJECT_BELOW
    THREADS, COOLDOWN, REJECT_BELOW = args.threads, args.cooldown, args.reject_below
    if THREADS == 0 and args.parallel > 1:
        import os
        THREADS = max(1, (os.cpu_count() or 2) // args.parallel)

    rng = random.Random(args.rng)
    base = start_candidate()
    first_gen = 0
    population = None
    if args.resume and LOG.exists():
        entries = [json.loads(line) for line in LOG.read_text(encoding="utf-8").splitlines() if line.strip()]
        entries = [e for e in entries if not e.get("verification")]
        if entries:
            first_gen = max(e["generation"] for e in entries) + 1
            scores = {}
            for e in entries:
                scores.setdefault(json.dumps(e["params"], sort_keys=True), []).append(e["fitness"])
            ranked = [json.loads(k) for k, _ in sorted(scores.items(), key=lambda kv: sum(kv[1]) / len(kv[1]),
                                                        reverse=True)]
            ranked = [{k: p.get(k, SPACE[k][3]) for k in SPACE} for p in ranked]
            elites = ranked[: args.elites]
            population = list(elites)
            while len(population) < args.population:
                a, b = rng.sample(elites, 2) if len(elites) > 1 else (elites[0], elites[0])
                population.append(mutate(crossover(a, b, rng), rng, 0.2))
            print(f"resuming at generation {first_gen} from {len(entries)} logged runs", flush=True)
    if population is None:
        population = [base] + [mutate(base, rng, 0.3) for _ in range(args.population - 1)]
    if args.inject:
        text = pathlib.Path(args.inject[1:]).read_text(encoding="utf-8") if args.inject.startswith("@") else args.inject
        injected = [{k: d.get(k, population[0].get(k, SPACE[k][3])) for k in SPACE} for d in json.loads(text)]
        # Injected variants replace the last (most exploratory) members of the population.
        population = population[: max(0, len(population) - len(injected))] + injected
        print(f"injected {len(injected)} variants", flush=True)
    verified_seeds = set()

    from concurrent.futures import ThreadPoolExecutor

    for gen in range(first_gen, first_gen + args.generations):
        # Several seeds per generation (each changes the wiring and the items): measured, the
        # same variant's checks swing across seeds by more than the pass bars, so selection on
        # one seed chased luck. Every seed's result is logged; ranking uses their mean.
        seeds = [rng.randint(100, 10_000) for _ in range(max(1, args.seeds_per_gen))]
        seed = seeds[0]
        t0 = time.time()
        results = []
        # Evaluate variants side by side (measured: 2 at a time, 7 threads each, is ~1.36x the
        # throughput of one at a time with all 14 threads).
        jobs = [(c, sd) for c in population for sd in seeds]
        with ThreadPoolExecutor(max_workers=max(1, args.parallel)) as pool:
            flat = list(pool.map(lambda cs: evaluate(cs[0], args.preset, cs[1]), jobs))
        evaluated = []
        for i, cand in enumerate(population):
            rs = flat[i * len(seeds):(i + 1) * len(seeds)]
            for sd, one in zip(seeds, rs):
                one.update({"generation": gen, "seed": sd, "index": i, "params": cand})
                log(one)
            agg = dict(rs[0])
            agg["fitness"] = sum(x["fitness"] for x in rs) / len(rs)
            agg["passes_both"] = all(x["passes_both"] for x in rs)
            agg["stage1"] = dict(rs[0]["stage1"])
            agg["stage1"]["checks_passed"] = min(x["stage1"]["checks_passed"] for x in rs)
            evaluated.append(agg)
        for i, (cand, r) in enumerate(zip(population, evaluated)):
            results.append(r)
            s1 = r["stage1"]
            print(f"gen {gen} cand {i}: fitness {r['fitness']:.2f} all={r['passes_both']} checks(min over seeds)={s1['checks_passed']}/6 "
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

        # Rank by average fitness over every seed a variant has been tested on: survivors are
        # re-tested each generation, so their averages grow reliable and one lucky seed
        # cannot keep a weak variant alive.
        history = {}
        for line in LOG.read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            e = json.loads(line)
            if not e.get("verification"):
                history.setdefault(json.dumps(e["params"], sort_keys=True), []).append(e["fitness"])
        def mean_fitness(params):
            v = history.get(json.dumps(params, sort_keys=True), [])
            return sum(v) / len(v) if v else -1e9
        ranked = sorted({json.dumps(r["params"], sort_keys=True): r["params"] for r in results}.values(),
                        key=mean_fitness, reverse=True)
        BEST.write_text(json.dumps({"params": ranked[0], "mean_fitness": mean_fitness(ranked[0]),
                                    "evaluations": len(history.get(json.dumps(ranked[0], sort_keys=True), [])),
                                    "generation": gen, "verified": False}, indent=2))
        print(f"gen {gen} done in {time.time() - t0:.0f}s, best single fitness "
              f"{max(r['fitness'] for r in results):.2f}, best average {mean_fitness(ranked[0]):.2f}", flush=True)

        elites = ranked[: args.elites]
        strength = max(0.12, 0.3 * (1.0 - (gen - first_gen) / max(1, args.generations)))
        children = []
        # Immigrants: strongly mutated variants that keep the search exploring.
        for _ in range(min(args.immigrants, args.population - len(elites))):
            children.append(mutate(rng.choice(elites), rng, 0.5))
        while len(children) < args.population - len(elites):
            a, b = rng.sample(elites, 2) if len(elites) > 1 else (elites[0], elites[0])
            children.append(mutate(crossover(a, b, rng), rng, strength))
        population = elites + children

    print("no candidate verified within the generation limit; best so far in", BEST, flush=True)
    return 3


if __name__ == "__main__":
    sys.exit(main())
