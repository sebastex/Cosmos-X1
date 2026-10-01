#!/usr/bin/env python3
"""One evolution job: one Stage 1 check for one candidate on one brain (seed).

Writes a JSON file with the pass/fail result, the check's key numbers and monitoring data
(what went wrong inside), so the coordinator can score candidates and report limitations.

Usage: python tools/evo/job.py --check capacity --seed 101 --args "--set a=1 --set b=2" --out m.json
"""

import argparse
import json
import os
import pathlib
import re
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
EXE = pathlib.Path(os.environ.get("NCM_EXE", ROOT / "build" / ("cosmos_x1.exe" if os.name == "nt" else "cosmos_x1")))

CHECKS = {
    "recall": ["--test", "recall"],
    "capacity": ["--test", "capacity"],
    "efficiency": ["--test", "efficiency"],
    "wordpairs": ["--test", "pairload", "--store-ticks", "8"],
    "continual": ["--test", "continual"],
    "retention": ["--test", "retention"],
    "order": ["--test", "order"],
    "monitor": ["--test", "recalldetail"],  # diagnostics only, not scored
    "load64": ["--test", "pairload", "--store-ticks", "32"],  # big-load probe (32 pairs = 64 words), not scored
}
UNSCORED = {"monitor", "load64"}


def num(pattern, text, default=None, group=1):
    m = re.findall(pattern, text)
    if not m:
        return default
    v = m[-1] if isinstance(m[-1], str) else m[-1][group - 1]
    try:
        return float(v)
    except ValueError:
        return v


def parse(check, out):
    d = {}
    if check == "recall":
        d["gain"] = num(r"specificity \(own minus best other\):.*?gain ([+-][0-9.]+)", out)
        d["reliability"] = num(r"input-driven .*?own ([0-9.]+)", out)
    elif check == "capacity":
        d["gain"] = num(r"8 memories:.*?gain ([+-][0-9.]+)", out)
        d["all_right"] = bool(re.search(r"8 memories:.*identifies all: yes", out))
    elif check == "efficiency":
        d["gain30"] = num(r"exposure 30 ticks:.*?gain ([+-][0-9.]+)", out)
    elif check == "continual":
        d["forgetting"] = num(r"forgetting of old memories: ([+-][0-9.]+)", out)
        d["new_all_right"] = bool(re.search(r"new memories:.*identifies all: yes", out))
        # Why old memories got worse: wiped out (erasure) or pushed aside by the new ones (interference).
        d["erasure"] = num(r"diagnostic: erasure ([+-][0-9.]+)", out)
        d["interference"] = num(r"interference from new memories ([+-][0-9.]+)", out)
    elif check == "retention":
        d["margin_lost"] = num(r"margin lost ([+-][0-9.]+)", out)
        d["all_right"] = bool(re.search(r"all old memories recalled correctly: yes", out))
    elif check == "order":
        d["gain"] = num(r"forward minus backward:.*?gain ([+-][0-9.]+)", out)
    elif check == "wordpairs":
        d["true_recall"] = num(r"word pairs: true recall ([0-9.]+)%", out)
        d["untrained"] = num(r"vs untrained ([0-9.]+)%", out)
        wrong = re.findall(r"wrong \(cue>recalled\):(.*)", out)
        d["mistakes"] = wrong[-1].strip().split() if wrong else []
        # Per pair at the last stage: was the link stored, and was the partner recalled?
        blocks = out.split("per pair:")
        if len(blocks) > 1:
            rows = re.findall(r"^\s+\d+ \w+\s+[0-9.]+ / [0-9.]+ (yes|NO)\s+\| (yes|NO)", blocks[-1], re.M)
            d["stored_recalled"] = sum(1 for a, b in rows if a == "yes" and b == "yes")
            d["stored_not_recalled"] = sum(1 for a, b in rows if a == "yes" and b == "NO")
            d["not_stored"] = sum(1 for a, b in rows if a == "NO")
    elif check == "load64":
        stages = re.findall(r"TRUE RECALL \(partner heard alone by this same matrix\): ([0-9]+) of ([0-9]+) right", out)
        d["stages"] = {int(n) * 2: round(100.0 * int(r) / int(n), 1) for r, n in stages}  # words -> % recalled
    elif check == "monitor":
        learned = out.split("UNTRAINED")[0]
        d["cue_reaches"] = num(r"40% cue reaches ([0-9.]+)% of what the full letter brings", learned)
        m = re.search(r"mean\s+([0-9.]+)\s+([0-9.]+)\s+([0-9.]+)\s+([0-9.]+)", learned)
        if m:
            d["complete"], d["clean"], d["other"], d["size"] = (float(x) for x in m.groups())
        d["wrong"] = [f"{a}>{b}" for a, b in re.findall(r"^\s+(\w)\s.*\|\s+(\w)\s+<-WRONG", learned, re.M)]
    return d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", required=True, choices=list(CHECKS))
    ap.add_argument("--seed", type=int, required=True)
    ap.add_argument("--args", default="")
    ap.add_argument("--cand", default="")
    ap.add_argument("--preset", default="dev")
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    cmd = [str(EXE), "--preset", a.preset, "--seed", str(a.seed)] + CHECKS[a.check] + a.args.split()
    t0 = time.time()
    env = dict(os.environ, OMP_NUM_THREADS=str(os.cpu_count() or 2))
    if a.check == "load64":
        env["NCM_PAIR_FAST"] = "1"  # learning matrix only, tested at 16 and 32 pairs
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, env=env)
    result = {"cand": a.cand, "seed": a.seed, "check": a.check, "args": a.args,
              "pass": proc.returncode == 0 if a.check not in UNSCORED else None,
              "minutes": round((time.time() - t0) / 60, 1), "exit": proc.returncode,
              "data": parse(a.check, proc.stdout)}
    if proc.returncode not in (0, 3):
        result["error"] = (proc.stderr or proc.stdout)[-500:]
    pathlib.Path(a.out).write_text(json.dumps(result))
    pathlib.Path(a.out).with_suffix(".txt").write_text(proc.stdout)
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    sys.exit(main())
