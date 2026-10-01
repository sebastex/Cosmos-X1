#!/usr/bin/env python3
"""Coordinator of the GitHub evolution around Cosmos Prime.

  propose  : read evolution/state.json, choose this generation's candidates (Prime as the
             reference, re-tested leaders, mutants of the leaders) and fresh brains (seeds);
             print the job matrix (candidate x brain x check) as JSON for GitHub Actions.
  collect  : read the jobs' result files, score every candidate (mean over all brains it has
             been tested on), update the state, the log and the monitoring report
             (evolution/monitor.md), and print whether to start another generation.

Scoring (per brain): checks passed (of 7) + 2 x true word-pair recall + 0.5 x how much of a
memory a 40% cue brings back (relative to the full cue) - 0.25 per memory recalled as another.
Leaders are re-tested on new brains every generation, so a lucky brain cannot carry a winner.
"""

import argparse
import json
import math
import pathlib
import random
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
EVO = ROOT / "evolution"
STATE = EVO / "state.json"
SPACE = json.loads((ROOT / "tools" / "evo" / "space.json").read_text())
SPACE.pop("_comment", None)
CHECKS = ["recall", "capacity", "efficiency", "wordpairs", "continual", "retention", "order", "monitor"]
SCORED = [c for c in CHECKS if c != "monitor"]
POP = 12          # candidates per generation (Prime + leaders + mutants)
LEADERS = 3       # best candidates re-tested on new brains each generation
SEEDS_PER_GEN = 2


def prime():
    return {k: v[3] for k, v in SPACE.items()}


def args_of(params):
    return " ".join(f"--set {k}={v}" for k, v in sorted(params.items()))


def mutate(params, rng, strength):
    c = dict(params)
    keys = list(SPACE)
    for k in rng.sample(keys, k=max(1, int(len(keys) * 0.25))):
        lo, hi, kind, _ = SPACE[k]
        v = c[k]
        if kind == "i":
            c[k] = int(min(hi, max(lo, v + rng.choice((-1, 1)))))
        elif lo > 0:
            c[k] = round(min(hi, max(lo, v * 2.0 ** rng.gauss(0.0, strength))), 5)
        else:  # ranges starting at 0 (switch-like strengths): additive steps
            c[k] = round(min(hi, max(lo, v + rng.gauss(0.0, strength * 0.5 * (hi - lo)))), 5)
    return c


def crossover(a, b, rng):
    return {k: (a[k] if rng.random() < 0.5 else b[k]) for k in SPACE}


def load():
    return json.loads(STATE.read_text()) if STATE.exists() else None


def save(state):
    EVO.mkdir(exist_ok=True)
    STATE.write_text(json.dumps(state, indent=1))


def ranked(state):
    c = [(cid, v) for cid, v in state["candidates"].items() if v["evals"]]
    return sorted(c, key=lambda kv: -kv[1]["fitness"])


def propose(hours, restart):
    state = load()
    now = time.time()
    if state is None or restart:
        state = {"generation": 0, "deadline": now + hours * 3600, "started": now, "candidates": {}, "next_id": 1}
    # evolution/settings.json {"hours": H} changes the length of a running evolution
    # (counted from its start) without touching the state.
    settings = EVO / "settings.json"
    if settings.exists():
        h = json.loads(settings.read_text()).get("hours")
        if h:
            state["deadline"] = state["started"] + float(h) * 3600
    gen = state["generation"]
    rng = random.Random(1000 + gen)
    current = {}
    if "prime" not in state["candidates"]:
        state["candidates"]["prime"] = {"params": prime(), "parent": None, "born": gen, "evals": [], "fitness": 0.0}
    current["prime"] = state["candidates"]["prime"]["params"]
    leaders = [cid for cid, _ in ranked(state) if cid != "prime"][:LEADERS]
    for cid in leaders:
        current[cid] = state["candidates"][cid]["params"]
    pool = [cid for cid, _ in ranked(state)][:6] or ["prime"]
    while len(current) < POP:
        if len(pool) >= 2 and rng.random() < 0.3:
            a, b = rng.sample(pool, 2)
            child = mutate(crossover(state["candidates"][a]["params"], state["candidates"][b]["params"], rng), rng, 0.3)
            parent = f"{a}x{b}"
        else:
            a = rng.choice(pool)
            child = mutate(state["candidates"][a]["params"], rng, 0.5 if gen == 0 else 0.35)
            parent = a
        cid = f"c{state['next_id']}"
        state["next_id"] += 1
        state["candidates"][cid] = {"params": child, "parent": parent, "born": gen, "evals": [], "fitness": 0.0}
        current[cid] = child
    seeds = [2000 + SEEDS_PER_GEN * gen + i for i in range(SEEDS_PER_GEN)]
    state["current"] = list(current)
    state["seeds"] = seeds
    save(state)
    matrix = [{"cand": cid, "seed": s, "check": ch, "args": args_of(p)}
              for cid, p in current.items() for s in seeds for ch in CHECKS]
    # Big-load probe (64 words) for Prime and the re-tested leaders: where does memory break?
    matrix += [{"cand": cid, "seed": s, "check": "load64", "args": args_of(current[cid])}
               for cid in ["prime"] + leaders for s in seeds]
    print(json.dumps(matrix))


def score(results):
    """results: {check: result dict} for one candidate on one brain."""
    passes = sum(1 for c in SCORED if results.get(c, {}).get("pass"))
    tr = (results.get("wordpairs", {}).get("data", {}).get("true_recall") or 0.0) / 100.0
    mon = results.get("monitor", {}).get("data", {})
    reach = (mon.get("cue_reaches") or 0.0) / 100.0
    wrong = len(mon.get("wrong") or [])
    return passes + 2.0 * tr + 0.5 * reach - 0.25 * wrong, passes


def collect(results_dir):
    state = load()
    gen = state["generation"]
    files = sorted(pathlib.Path(results_dir).rglob("*.json"))
    by = {}
    errors = []
    for f in files:
        try:
            r = json.loads(f.read_text())
        except Exception:
            continue
        by.setdefault((r["cand"], r["seed"]), {})[r["check"]] = r
        if r.get("error"):
            errors.append(f"{r['cand']} brain {r['seed']} {r['check']}: {r['error'][-160:]}")
    log = EVO / "log.jsonl"
    with log.open("a", encoding="utf-8") as fh:
        for (cid, seed), res in by.items():
            fit, passes = score(res)
            cand = state["candidates"].get(cid)
            if cand is None:
                continue
            ev = {"gen": gen, "seed": seed, "fitness": round(fit, 3), "passes": passes,
                  "checks": {c: res[c].get("pass") for c in SCORED if c in res},
                  "true_recall": res.get("wordpairs", {}).get("data", {}).get("true_recall"),
                  "monitor": res.get("monitor", {}).get("data", {}),
                  "continual": res.get("continual", {}).get("data", {}),
                  "retention": res.get("retention", {}).get("data", {}),
                  "pairs": {k: v for k, v in res.get("wordpairs", {}).get("data", {}).items() if k != "mistakes"},
                  "load64": res.get("load64", {}).get("data", {}).get("stages"),
                  "minutes": round(sum(r.get("minutes", 0) for r in res.values()), 1),
                  "missing": [c for c in CHECKS if c not in res],
                  "params": cand["params"]}
            cand["evals"].append(ev)
            cand["fitness"] = sum(e["fitness"] for e in cand["evals"]) / len(cand["evals"])
            fh.write(json.dumps({"cand": cid, **ev}) + "\n")
    report(state, by, errors)
    best = ranked(state)[0]
    (EVO / "best.json").write_text(json.dumps({"cand": best[0], "fitness": best[1]["fitness"],
                                                "evals": len(best[1]["evals"]), "params": best[1]["params"]}, indent=1))
    state["generation"] = gen + 1
    save(state)
    remaining = state["deadline"] - time.time()
    print("continue" if remaining > 45 * 60 else "stop")


def report(state, by, errors):
    gen = state["generation"]
    lines = [f"# Evolution monitor (generation {gen} done, {max(0, state['deadline'] - time.time()) / 3600:.1f} h left)", ""]
    lines.append("Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.")
    lines.append("")
    lines.append("## Leaders")
    lines.append("| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |")
    lines.append("|---|---|---|---|---|---|")
    p0 = prime()
    for cid, v in ranked(state)[:8]:
        ev = v["evals"]
        passes = sum(e["passes"] for e in ev) / len(ev)
        trs = [e["true_recall"] for e in ev if e["true_recall"] is not None]
        tr = sum(trs) / len(trs) if trs else float("nan")
        diff = ", ".join(f"{k} {p0[k]}->{v['params'][k]}" for k in SPACE if v["params"][k] != p0[k]) or "(Prime)"
        lines.append(f"| {cid} | {v['fitness']:.2f} | {len(ev)} | {passes:.1f} | {tr:.0f} | {diff} |")
    lines.append("")
    lines.append("## Limitations seen this generation (all candidates)")
    fails = {c: [0, 0] for c in SCORED}
    wrongs, reach, mist = {}, [], {}
    for (cid, seed), res in by.items():
        for c in SCORED:
            if c in res and res[c].get("pass") is not None:
                fails[c][0] += 0 if res[c]["pass"] else 1
                fails[c][1] += 1
        mon = res.get("monitor", {}).get("data", {})
        for w in mon.get("wrong") or []:
            wrongs[w] = wrongs.get(w, 0) + 1
        if mon.get("cue_reaches") is not None:
            reach.append(mon["cue_reaches"])
        for m in res.get("wordpairs", {}).get("data", {}).get("mistakes") or []:
            mist[m] = mist.get(m, 0) + 1
    lines.append("| check | failed | of |")
    lines.append("|---|---|---|")
    for c, (f, n) in fails.items():
        lines.append(f"| {c} | {f} | {n} |")
    lines.append("")
    if reach:
        lines.append(f"- A 40% cue brings back on average {sum(reach) / len(reach):.0f}% of what the full letter brings (letters, 8 memories).")
    if wrongs:
        top = sorted(wrongs.items(), key=lambda kv: -kv[1])[:8]
        lines.append("- Letters recalled as another memory (letter>taken for: times): " + ", ".join(f"{k}: {v}" for k, v in top))
    if mist:
        top = sorted(mist.items(), key=lambda kv: -kv[1])[:8]
        lines.append("- Word pairs recalled wrongly (cue>recalled: times): " + ", ".join(f"{k}: {v}" for k, v in top))
    if errors:
        lines.append("- Errors / crashes:")
        lines += [f"  - {e}" for e in errors[:10]]
    lines.append("")
    lines += cumulative(state)
    (EVO / "monitor.md").write_text("\n".join(lines), encoding="utf-8")


def mean(xs):
    xs = [x for x in xs if x is not None]
    return sum(xs) / len(xs) if xs else float("nan")


def corr(xs, ys):
    pts = [(x, y) for x, y in zip(xs, ys) if x is not None and y is not None]
    if len(pts) < 8:
        return float("nan")
    mx, my = mean([p[0] for p in pts]), mean([p[1] for p in pts])
    sxy = sum((x - mx) * (y - my) for x, y in pts)
    sx = math.sqrt(sum((x - mx) ** 2 for x, _ in pts))
    sy = math.sqrt(sum((y - my) ** 2 for _, y in pts))
    return sxy / (sx * sy) if sx > 0 and sy > 0 else float("nan")


def cumulative(state):
    """Everything learned so far, over all generations: what fails, why, and which settings matter."""
    evals = [(cid, e) for cid, v in state["candidates"].items() for e in v["evals"]]
    L = ["## Picture so far (all generations together)", ""]
    L.append(f"- Brain tests so far: {len(evals)} ({len(state['candidates'])} versions, generation {state['generation'] + 1}).")
    L.append("")
    L.append("| check | failed | of | Prime failed | Prime of |")
    L.append("|---|---|---|---|---|")
    for c in SCORED:
        f = sum(1 for _, e in evals if e["checks"].get(c) is False)
        n = sum(1 for _, e in evals if c in e["checks"])
        pf = sum(1 for cid, e in evals if cid == "prime" and e["checks"].get(c) is False)
        pn = sum(1 for cid, e in evals if cid == "prime" and c in e["checks"])
        L.append(f"| {c} | {f} | {n} | {pf} | {pn} |")
    L.append("")
    # Why old memories get worse (continual check): wiped out (erasure) or pushed aside (interference).
    er = [e.get("continual", {}).get("erasure") for _, e in evals]
    it = [e.get("continual", {}).get("interference") for _, e in evals]
    fails = [(e.get("continual", {}).get("erasure"), e.get("continual", {}).get("interference"))
             for _, e in evals if e["checks"].get("continual") is False]
    if any(x is not None for x in er):
        dom_i = sum(1 for a, b in fails if a is not None and b is not None and b > a)
        L.append(f"- Old memories getting worse (all tests): wiped out {mean(er):+.3f}, pushed aside by new memories {mean(it):+.3f} on average; "
                 f"of {len(fails)} failed 'keep old memories' tests, {dom_i} were mainly pushed aside (interference), "
                 f"{len(fails) - dom_i} mainly wiped out (erasure).")
    sr = sum(e.get("pairs", {}).get("stored_recalled") or 0 for _, e in evals)
    snr = sum(e.get("pairs", {}).get("stored_not_recalled") or 0 for _, e in evals)
    ns = sum(e.get("pairs", {}).get("not_stored") or 0 for _, e in evals)
    if sr + snr + ns:
        L.append(f"- Word pairs (8 pairs per test, all tests): stored and recalled {sr}, stored but NOT recalled {snr}, not stored {ns}.")
    loads = {}
    for cid, e in evals:
        for words, pct in (e.get("load64") or {}).items():
            loads.setdefault((cid, int(words)), []).append(pct)
    if loads:
        L.append("- Big-load probe (true word recall %, mean over brains): " + "; ".join(
            f"{cid} {w} words {mean(v):.0f}% (n={len(v)})" for (cid, w), v in sorted(loads.items())))
    L.append("")
    # Which settings matter: correlation of each setting with score, keeping old memories, holding 8.
    L.append("### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)")
    L.append("| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |")
    L.append("|---|---|---|---|")
    fit = [e["fitness"] for _, e in evals]
    keep = [(1 if e["checks"].get("continual") else 0) + (1 if e["checks"].get("retention") else 0) for _, e in evals]
    cap = [1 if e["checks"].get("capacity") else 0 for _, e in evals]
    rows = []
    for k in SPACE:
        xs = [e.get("params", state["candidates"][cid]["params"]).get(k) for cid, e in evals]
        rows.append((k, corr(xs, fit), corr(xs, keep), corr(xs, cap)))
    rows.sort(key=lambda r: -abs(r[1]) if r[1] == r[1] else 0)
    for k, a, b, c in rows:
        L.append(f"| {k} | {a:+.2f} | {b:+.2f} | {c:+.2f} |")
    L.append("")
    L.append("Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.")
    return L


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("propose")
    p.add_argument("--hours", type=float, default=12.0)
    p.add_argument("--restart", action="store_true")
    c = sub.add_parser("collect")
    c.add_argument("--results", required=True)
    a = ap.parse_args()
    if a.cmd == "propose":
        propose(a.hours, a.restart)
    else:
        collect(a.results)
    return 0


if __name__ == "__main__":
    sys.exit(main())
