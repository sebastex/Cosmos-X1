#!/usr/bin/env python3
"""Health report for the evolutionary search: is it making real progress?

Reads tools/evolve_log.jsonl and prints, per generation:
  - best and mean fitness (progress)
  - the best value reached on each requirement (are the weak ones improving?)
  - parameter diversity (is the population still exploring or collapsing?)
  - elite re-test noise (does the same variant score similarly on a new seed?)
"""

import json
import math
import pathlib
import statistics
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
LOG = ROOT / "tools" / "evolve_log.jsonl"

CHECKS = [("recall", 0.05), ("capacity", 0.05), ("efficiency30", 0.05), ("streamed", 0.05),
          ("continual_old", 0.05), ("order_signal", 0.02)]


def main():
    if not LOG.exists():
        print("no log yet")
        return 1
    runs = [json.loads(l) for l in LOG.read_text(encoding="utf-8").splitlines() if l.strip()]
    main_runs = [r for r in runs if not r.get("verification")]
    gens = sorted({r["generation"] for r in main_runs})

    print("gen  n  best   mean  | best per check (bar)                                             | diversity | checks passed (best)")
    for g in gens:
        rs = [r for r in main_runs if r["generation"] == g]
        fit = [r["fitness"] for r in rs]
        per_check = []
        for key, bar in CHECKS:
            vals = [r["stage1"].get(key, -1.0) for r in rs]
            best = max(vals)
            per_check.append(f"{key[:8]} {best:+.3f}{'*' if best >= bar else ' '}")
        # Diversity: mean relative spread of continuous parameters across the generation.
        params = [r["params"] for r in rs]
        spreads = []
        for k in params[0]:
            vals = [float(p[k]) for p in params]
            mean = statistics.mean(vals)
            if len(vals) > 1 and abs(mean) > 1e-9:
                spreads.append(statistics.pstdev(vals) / abs(mean))
        diversity = statistics.mean(spreads) if spreads else 0.0
        best_checks = max(r["stage1"].get("checks_passed", 0) for r in rs)
        print(f"{g:>3} {len(rs):>2} {max(fit):6.2f} {statistics.mean(fit):6.2f} | {'  '.join(per_check)} | {diversity:8.2f}  | {best_checks}/6")

    # Noise: variants evaluated in more than one generation (elites) on different seeds.
    by_params = {}
    for r in main_runs:
        by_params.setdefault(json.dumps(r["params"], sort_keys=True), []).append(r["fitness"])
    repeated = [v for v in by_params.values() if len(v) > 1]
    if repeated:
        spreads = [max(v) - min(v) for v in repeated]
        print(f"\nre-tested variants: {len(repeated)}; fitness change on a new seed: "
              f"mean {statistics.mean(spreads):.2f}, max {max(spreads):.2f}")

    verif = [r for r in runs if r.get("verification")]
    if verif:
        ok = sum(1 for r in verif if r["passes_both"])
        print(f"verification runs: {len(verif)}, passed {ok}")

    best = max(main_runs, key=lambda r: r["fitness"])
    print(f"\nbest so far: generation {best['generation']}, fitness {best['fitness']:.2f}, "
          f"checks {best['stage1'].get('checks_passed', 0)}/6, deep {best['stage0']['deep_percent']:.0f}%")
    return 0


if __name__ == "__main__":
    sys.exit(main())
