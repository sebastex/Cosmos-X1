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
}


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
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
                          env=dict(os.environ, OMP_NUM_THREADS=str(os.cpu_count() or 2)))
    result = {"cand": a.cand, "seed": a.seed, "check": a.check, "args": a.args,
              "pass": proc.returncode == 0 if a.check != "monitor" else None,
              "minutes": round((time.time() - t0) / 60, 1), "exit": proc.returncode,
              "data": parse(a.check, proc.stdout)}
    if proc.returncode not in (0, 3) and a.check != "monitor":
        result["error"] = (proc.stderr or proc.stdout)[-500:]
    pathlib.Path(a.out).write_text(json.dumps(result))
    pathlib.Path(a.out).with_suffix(".txt").write_text(proc.stdout)
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    sys.exit(main())
