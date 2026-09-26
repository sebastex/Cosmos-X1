#!/usr/bin/env python3
"""Quality-diversity search (MAP-Elites) over the NCM's rule parameters (spec Section 5E).

Instead of chasing one best score, MAP-Elites keeps an archive: the best variant found for
each *kind* of behaviour. Here the behaviours are the Stage 1 requirements where Cosmos most
needs new ground: memory from streamed text, the order signal, and how strongly streamed text
reaches the deep fields. Parents are drawn from anywhere in the archive, so diverse stepping
stones stay alive and good traits can cross between niches.

Noise handling: every evaluation uses a fresh seed; a variant's fitness and behaviour are
averaged over all its evaluations, and a share of the budget re-tests archive members.

It reuses the evaluation (Stage 0 + the full memory suite) and mutation from evolve.py and
stops when a variant passes everything and holds on 3 unseen seeds.

Usage: python tools/map_elites.py [--evaluations N] [--parallel 2] [--preset tiny]
"""

import argparse
import json
import pathlib
import random
import sys
import time
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import evolve  # noqa: E402  (evaluation, mutation, parameter space)

ROOT = evolve.ROOT
ARCHIVE = ROOT / "tools" / "map_elites_archive.json"
MLOG = ROOT / "tools" / "map_elites_log.jsonl"

# Behaviour dimensions: (name, function of an evaluation record, bin edges)
DIMENSIONS = [
    ("streamed", lambda r: r["stage1"].get("streamed", -1.0), [0.0, 0.025, 0.05]),
    ("order", lambda r: r["stage1"].get("order_signal", -1.0), [0.0, 0.01, 0.02]),
    ("deep", lambda r: r["stage0"].get("deep_percent", 0.0), [15.0, 50.0]),
]


def bin_of(value, edges):
    return sum(1 for e in edges if value >= e)


def key_of(params):
    return json.dumps(params, sort_keys=True)


class Archive:
    def __init__(self):
        self.evals = {}   # params key -> list of evaluation records
        self.cells = {}   # cell tuple -> params key

    def record(self, params, r):
        self.evals.setdefault(key_of(params), []).append(r)

    def mean(self, k):
        rs = self.evals[k]
        fitness = sum(r["fitness"] for r in rs) / len(rs)
        behaviour = tuple(bin_of(sum(f(r) for r in rs) / len(rs), edges) for _, f, edges in DIMENSIONS)
        return fitness, behaviour

    def rebuild(self):
        """Place every variant in its (averaged) niche; keep the best per niche."""
        self.cells = {}
        for k in self.evals:
            fitness, cell = self.mean(k)
            cur = self.cells.get(cell)
            if cur is None or self.mean(cur)[0] < fitness:
                self.cells[cell] = k

    def members(self):
        return [json.loads(k) for k in self.cells.values()]

    def save(self):
        out = []
        for cell, k in sorted(self.cells.items()):
            fitness, _ = self.mean(k)
            out.append({"cell": dict(zip([d[0] for d in DIMENSIONS], cell)), "fitness": fitness,
                        "evaluations": len(self.evals[k]), "params": json.loads(k)})
        ARCHIVE.write_text(json.dumps(out, indent=2))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--evaluations", type=int, default=200)
    ap.add_argument("--parallel", type=int, default=2)
    ap.add_argument("--preset", default="tiny")
    ap.add_argument("--retest-share", type=float, default=0.2, help="share of evaluations spent re-testing archive members")
    ap.add_argument("--verify-seeds", type=int, default=3)
    ap.add_argument("--rng", type=int, default=11)
    args = ap.parse_args()

    import os
    evolve.THREADS = max(1, (os.cpu_count() or 2) // max(1, args.parallel))
    rng = random.Random(args.rng)
    archive = Archive()

    # Seed the archive with everything the earlier searches already evaluated.
    for path in (evolve.LOG, ROOT / "tools" / "evolve_log_round1.jsonl", MLOG):
        if path.exists():
            for line in path.read_text(encoding="utf-8").splitlines():
                if not line.strip():
                    continue
                e = json.loads(line)
                if e.get("verification") or "stage1" not in e or "streamed" not in e["stage1"]:
                    continue
                params = {k: e["params"].get(k, v[3]) for k, v in evolve.SPACE.items()}
                archive.record(params, e)
    archive.rebuild()
    print(f"archive seeded: {len(archive.evals)} variants, {len(archive.cells)} niches filled", flush=True)
    if not archive.cells:
        base = evolve.start_candidate()
        archive.cells[(0, 0, 0)] = key_of(base)
        archive.evals[key_of(base)] = []

    def propose():
        members = archive.members()
        if members and rng.random() < args.retest_share:
            return rng.choice(members)  # re-test an archive member on a new seed
        a = rng.choice(members)
        if len(members) > 1 and rng.random() < 0.5:
            a = evolve.crossover(a, rng.choice(members), rng)
        return evolve.mutate(a, rng, rng.choice((0.1, 0.2, 0.4)))

    done = 0
    while done < args.evaluations:
        batch = [propose() for _ in range(args.parallel)]
        seeds = [rng.randint(100, 10_000) for _ in batch]
        t0 = time.time()
        with ThreadPoolExecutor(max_workers=len(batch)) as pool:
            results = list(pool.map(lambda ps: evolve.evaluate(ps[0], args.preset, ps[1]), zip(batch, seeds)))
        for params, seed, r in zip(batch, seeds, results):
            r.update({"seed": seed, "params": params, "evaluation": done})
            with MLOG.open("a", encoding="utf-8") as f:
                f.write(json.dumps(r) + "\n")
            archive.record(params, r)
            done += 1
            s1 = r["stage1"]
            print(f"eval {done}: fitness {r['fitness']:.2f} all={r['passes_both']} checks={s1.get('checks_passed', 0)}/6 "
                  f"stream={s1.get('streamed', -1):+.3f} order={s1.get('order_signal', -1):+.3f} "
                  f"deep={r['stage0']['deep_percent']:.0f}%", flush=True)

            if r["passes_both"]:
                ok = True
                for _ in range(args.verify_seeds):
                    v = evolve.evaluate(params, args.preset, rng.randint(10_001, 20_000))
                    print(f"  verify: all={v['passes_both']} checks={v['stage1'].get('checks_passed', 0)}/6", flush=True)
                    if not v["passes_both"]:
                        ok = False
                        break
                if ok:
                    evolve.BEST.write_text(json.dumps({"params": params, "verified": True, "method": "map-elites"},
                                                      indent=2))
                    archive.rebuild()
                    archive.save()
                    print("VERIFIED: passes both stages on all verification seeds", flush=True)
                    return 0
        archive.rebuild()
        archive.save()
        best = max(archive.cells.values(), key=lambda k: archive.mean(k)[0])
        print(f"  niches filled {len(archive.cells)}/48, best average fitness {archive.mean(best)[0]:.2f} "
              f"({time.time() - t0:.0f}s per batch)", flush=True)

    print("evaluation budget used; archive in", ARCHIVE, flush=True)
    return 3


if __name__ == "__main__":
    sys.exit(main())
