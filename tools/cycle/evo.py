#!/usr/bin/env python3
"""Evolution over the cycles (tools/cycle/cycle.py): every lineage is one grown brain and its
settings; each generation, each lineage tries a few setting variations on its current 100-word list
(each learns the list from the lineage's brain, then is tested), keeps the best, and moves on to
the next list when the best stops improving.

    python tools/cycle/evo.py init                       write cycles/state.json from the lineages below
    python tools/cycle/evo.py propose                    this generation's jobs (json matrix for GitHub)
    python tools/cycle/evo.py collect RESULTS_DIR RUN    read the jobs' results, update the state;
                                                         prints "continue" or "stop"
    python tools/cycle/evo.py report                     the state as a short table

Each lineage keeps its own archive of every setting tried on its list (quality-diversity: the
next variations come from several of the best, not only the top one). Runs are exact (the same
brain and settings always give the same result), so a setting is never tested twice.
Graduation: no better fitness for PATIENCE generations, or MAX_GENS generations on one list; the
best candidate's brain becomes the lineage's brain for the next list.
"""
import json
import math
import pathlib
import random
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
STATE = ROOT / 'cycles' / 'state.json'
LOG = ROOT / 'cycles' / 'log.md'
PER_LINEAGE = 6    # candidates per lineage per generation
PATIENCE = 3       # generations without a better fitness before graduating
MAX_GENS = 8       # most generations on one list
TOP = 4            # variations are made from the best TOP of the archive

# The settings evolution may change: [low, high, scale]. 'log' = mutated on a log scale; 'lin' = on
# a linear scale; 'opt' = a part that can be switched on or off (0 = off, else in [low, high], log).
# Only settings that do not change how the brain's links are stored (a grown brain must load).
SPACE = {
    'learning_rate': [0.005, 0.1, 'log'],
    'plastic_budget': [1.0, 12.0, 'log'],
    'presynaptic_bound': [0.0, 1.0, 'lin'],
    'hetero_ltd': [0.0, 1.0, 'lin'],
    'covariance': [0.0, 1.0, 'lin'],
    'encoding_suppression': [0.5, 1.0, 'lin'],
    'consolidation_rate': [0.001, 0.2, 'log'],
    'consolidated_budget': [0.3, 4.0, 'log'],
    'order_gain': [1.0, 10.0, 'log'],
    'order_tau': [1.0, 8.0, 'log'],
    'inner_rate': [0.005, 0.3, 'log'],
    'prune': [1e-7, 1e-3, 'log'],
    'trace_floor': [1e-8, 1e-5, 'log'],
    'alloc_gain': [0.02, 0.3, 'opt'],
    'link_use': [0.02, 0.3, 'opt'],
    'pc_error': [0.1, 1.0, 'opt'],      # split lineages only (outward 4D links split off)
    'art_vigilance': [0.2, 0.9, 'opt'],  # split lineages only
}
SPLIT_ONLY = {'pc_error', 'art_vigilance'}

# The lineages: the best brains after the ladder's 1000 words (full test recall), each a different
# kind (no pruning, pruning at 1e-4, at 1e-5). 'run' = the run that saved the brain, 'artifact' its name.
LINEAGES = [
    dict(name='P5', variant='alefP5', seed=3004, run='42932875595', artifact='ladder-brain-alefP5-3004', split=False),
    dict(name='P', variant='alefP', seed=3004, run='42934483077', artifact='ladder-brain-alefP-3004', split=False),
    dict(name='EF', variant='alefEF', seed=3004, run='42742725584', artifact='ladder-brain-alefEF-3004', split=True),
]


def load():
    return json.loads(STATE.read_text())


def save(s):
    STATE.parent.mkdir(exist_ok=True)
    STATE.write_text(json.dumps(s, indent=1))


def key(params):
    return json.dumps({k: round(v, 9) for k, v in sorted(params.items())})


def args_of(params):
    return ' '.join(f'--set {k}={v:.9g}' for k, v in sorted(params.items()))


def clip(name, x):
    lo, hi, kind = SPACE[name]
    if kind == 'opt' and x <= 0.0:
        return 0.0
    return min(hi, max(lo, x))


def mutate(params, rng, split):
    c = dict(params)
    names = [n for n in SPACE if split or n not in SPLIT_ONLY]
    for n in rng.sample(names, k=max(1, round(len(names) * 0.25))):
        lo, hi, kind = SPACE[n]
        x = c.get(n, 0.0)
        if kind == 'opt':
            if x <= 0.0:  # off: switch on (somewhere in its range), or leave off
                c[n] = math.exp(rng.uniform(math.log(lo), math.log(hi))) if rng.random() < 0.5 else 0.0
            elif rng.random() < 0.25:
                c[n] = 0.0
            else:
                c[n] = clip(n, x * math.exp(rng.gauss(0.0, 0.4)))
        elif kind == 'log':
            c[n] = clip(n, max(x, lo) * math.exp(rng.gauss(0.0, 0.4)))
        else:
            c[n] = clip(n, x + rng.gauss(0.0, 0.15 * (hi - lo)))
    return c


def start_params(lineage):
    """The lineage's settings as they are in its variant (read once with cosmos_x1 --test settings)."""
    p = {}
    for n in SPACE:
        if n in SPLIT_ONLY and not lineage['split']:
            continue
        p[n] = float(lineage['values'].get(n, 0.0))
    if p.get('trace_floor', 0.0) <= 0.0:
        p['trace_floor'] = 1e-6  # the trace floor: learning 23x faster on grown brains
    return p


def init(values_path):
    vals = json.loads(pathlib.Path(values_path).read_text())  # {variant: {setting: value}}
    s = dict(generation=0, lineages=[])
    for L in LINEAGES:
        L = dict(L, values=vals[L['variant']])
        s['lineages'].append(dict(name=L['name'], variant=L['variant'], seed=L['seed'], split=L['split'],
                                  list=1, brain=dict(run=L['run'], artifact=L['artifact']),
                                  center=start_params(L), archive=[], gens_on_list=0, stale=0, best=None,
                                  history=[]))
    save(s)
    print(f'state written: {len(s["lineages"])} lineages, list 1')


def propose():
    s = load()
    rng = random.Random(1000 + s['generation'])
    jobs = []
    for L in s['lineages']:
        if L['list'] > 10:
            continue
        tried = {key(a['params']) for a in L['archive']}
        cands = []
        if not L['archive']:  # first generation on a list: the lineage's settings as they are
            cands.append(dict(L['center']))
        ranked = sorted(L['archive'], key=lambda a: -a['fitness'])[:TOP] or [dict(params=L['center'])]
        guard = 0
        while len(cands) < PER_LINEAGE and guard < 200:
            guard += 1
            base = rng.choice(ranked)['params']
            c = mutate(base, rng, L['split'])
            if key(c) not in tried and all(key(c) != key(x) for x in cands):
                cands.append(c)
        for i, c in enumerate(cands):
            jobs.append(dict(lineage=L['name'], variant=L['variant'], seed=L['seed'], list=L['list'],
                             cand=f"{L['name']}-g{s['generation']}-{i}", brain_run=L['brain']['run'],
                             brain_artifact=L['brain']['artifact'], params=key(c), args=args_of(c)))
    s['pending'] = jobs
    save(s)
    print(json.dumps(jobs))


def collect(results_dir, run_id):
    s = load()
    res = {}
    for f in pathlib.Path(results_dir).rglob('result-*.json'):
        r = json.loads(f.read_text())
        res[r['cand']] = r
    lines = [f"\n## Generation {s['generation']} (run {run_id})\n"]
    for L in s['lineages']:
        mine = [j for j in s.get('pending', []) if j['lineage'] == L['name']]
        if not mine:
            continue
        best_before = L['best']['fitness'] if L['best'] else -1.0
        for j in mine:
            r = res.get(j['cand'])
            if not r or 'fitness' not in r:
                lines.append(f"- {L['name']} {j['cand']}: no result (failed)")
                continue
            entry = dict(cand=j['cand'], params=json.loads(j['params']), fitness=r['fitness'], new=r['new'],
                         old=r['old'], brain=dict(run=str(run_id), artifact=f"cycle-brain-{j['cand']}"))
            L['archive'].append(entry)
            lines.append(f"- {L['name']} list {L['list']} {j['cand']}: fitness {r['fitness']:.3f} "
                         f"(new {r['new']:.2f}, old {r['old']:.2f})")
            if not L['best'] or entry['fitness'] > L['best']['fitness']:
                L['best'] = entry
        L['gens_on_list'] += 1
        L['stale'] = 0 if L['best'] and L['best']['fitness'] > best_before else L['stale'] + 1
        if L['best'] and (L['stale'] >= PATIENCE or L['gens_on_list'] >= MAX_GENS):
            b = L['best']
            L['history'].append(dict(list=L['list'], fitness=b['fitness'], new=b['new'], old=b['old'],
                                     params=b['params'], generations=L['gens_on_list'], brain=b['brain']))
            lines.append(f"- **{L['name']} graduates from list {L['list']}**: fitness {b['fitness']:.3f} "
                         f"(new {b['new']:.2f}, old {b['old']:.2f}) after {L['gens_on_list']} generations")
            L.update(list=L['list'] + 1, brain=b['brain'], center=b['params'], archive=[], gens_on_list=0,
                     stale=0, best=None)
    s['generation'] += 1
    s['pending'] = []
    save(s)
    LOG.parent.mkdir(exist_ok=True)
    with open(LOG, 'a') as f:
        f.write('\n'.join(lines) + '\n')
    print('\n'.join(lines))
    print('continue' if any(L['list'] <= 10 for L in s['lineages']) else 'stop')


def report():
    s = load()
    print(f"generation {s['generation']}")
    for L in s['lineages']:
        b = L['best']
        print(f"{L['name']:3} list {L['list']:2}  gens {L['gens_on_list']}  best "
              + (f"{b['fitness']:.3f} (new {b['new']:.2f}, old {b['old']:.2f})" if b else '-')
              + '  done: ' + ', '.join(f"{h['list']}:{h['fitness']:.2f}" for h in L['history']))


def main():
    a = sys.argv[1:]
    if a[:1] == ['init']:
        init(a[1])
    elif a[:1] == ['propose']:
        propose()
    elif a[:1] == ['collect']:
        collect(a[1], a[2])
    elif a[:1] == ['report']:
        report()
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
