"""Score one arena puzzle from its run: the result text (stdout of --test script) and the monitor report.

    python tools/arena/score.py PUZZLE_ID result.txt monitor/report.txt [puzzles.json] > score.json

Every score is in [0, 1] (1 = perfect) with the details behind it.
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def labscores(report):
    out = []
    for line in open(report, encoding='utf-8', errors='replace'):
        if line.startswith('LABSCORE '):
            out.append(json.loads(line[len('LABSCORE '):]))
    return out  # learner first, then the twin (if it ran)


def cue_margins(score):
    return {c['cue']: c['margin'] for c in score['cues']}


def ending_choice(score, endings):
    """For every cue that ended on 'the' after a sentence start: which ending was the silence most like."""
    rows = sorted(score['cues'], key=lambda c: c['tick'])
    picks = []
    for a, b in zip(rows, rows[1:]):
        if b['cue'] == 'the' and a['cue'] != 'the':
            sims = dict(b['top'])
            vals = {e: sims.get(e, -1.0) for e in endings}
            best = max(vals, key=vals.get)
            picks.append((a['cue'], best, vals))
    return picks


def ask_margins(score, asks, choices):
    """Each question in order: the answer's likeness minus the best other choice's (silence after the cue)."""
    got = score.get('asks', [])
    out = []
    for q, g in zip(asks, got):
        sims = g['sims']
        a = q['a']
        other = max((sims.get(c, -1.0) for c in choices if c != a), default=-1.0)
        out.append(dict(q=q['q'], a=a, heard=g['last'], margin=round(sims.get(a, -1.0) - other, 4),
                        pick=max(choices, key=lambda c: sims.get(c, -1.0))))
    return out


def quiz_margins(score, items):
    """Each quiz item in order: the answer's likeness minus the best other choice (all known words if none given)."""
    out = []
    for it, g in zip(items, score.get('asks', [])):
        sims = g['sims']
        pool = it['choices'] if it['choices'] else list(sims)
        other = max((sims.get(c, -1.0) for c in pool if c != it['a']), default=-1.0)
        out.append(dict(level=it['level'], q=it['q'], a=it['a'], margin=round(sims.get(it['a'], -1.0) - other, 4),
                        pick=max(pool, key=lambda c: sims.get(c, -1.0)) if pool else None))
    return out


def main():
    pid, result, report = sys.argv[1], sys.argv[2], sys.argv[3]
    pfile = sys.argv[4] if len(sys.argv) > 4 else os.path.join(HERE, 'puzzles.json')
    puzzles = {p['id']: p for p in json.load(open(pfile))['puzzles']}
    p = puzzles[pid]
    kind = p.get('kind', pid)
    out = dict(puzzle=pid, name=p['name'])
    # TWIN_FROM: an earlier score of the same puzzle whose untrained twin is reused (the twin never
    # learns, so it answers the same every time; the playground runs it once per lane).
    stored = json.load(open(os.environ['TWIN_FROM'])) if os.environ.get('TWIN_FROM') and os.path.exists(os.environ['TWIN_FROM']) else {}
    if kind == 'quiz':
        sc = labscores(report)
        learner = quiz_margins(sc[0], p['items']) if sc else []
        twin = quiz_margins(sc[1], p['items']) if len(sc) > 1 else []
        tm = [t['margin'] for t in twin] + [-9] * (len(learner) - len(twin))
        levels = {}
        for l, t in zip(learner, tm):
            r = levels.setdefault(l['level'], dict(n=0, right=0, picked=0))
            r['n'] += 1
            r['right'] += l['margin'] > 0 and l['margin'] > t
            r['picked'] += l['pick'] == l['a']
        for r in levels.values():
            r['score'] = round(r['right'] / r['n'], 3)
        out.update(score=sum(r['right'] for r in levels.values()) / max(1, len(p['items'])), levels=levels,
                   taught=p.get('taught'), learner=learner, twin=twin)
    elif kind == 'ask':
        sc = labscores(report)
        learner = ask_margins(sc[0], p['asks'], p['choices'])
        twin = ask_margins(sc[1], p['asks'], p['choices']) if len(sc) > 1 else stored.get('twin', [])
        tm = [t['margin'] for t in twin] + [-9] * (len(learner) - len(twin))
        right = [l['q'] for l, t in zip(learner, tm) if l['margin'] > 0 and l['margin'] > t]
        out.update(score=len(right) / max(1, len(p['asks'])), right=right,
                   picked=sum(l['pick'] == l['a'] for l in learner) / max(1, len(p['asks'])),
                   twin_picked=sum(t['pick'] == t['a'] for t in twin) / max(1, len(p['asks'])) if twin else None,
                   chance=round(1 / len(p['choices']), 3), learner=learner, twin=twin)
    elif kind in ('complete', 'keep', 'sleep'):
        sc = labscores(report)
        learner = cue_margins(sc[0])
        twin = cue_margins(sc[1]) if len(sc) > 1 else stored.get('twin', {})
        right = [c for c, m in learner.items() if m > 0 and m > twin.get(c, -9)]
        out.update(score=len(right) / max(1, len(learner)), right=right, learner=learner, twin=twin,
                   gain=round(sum(learner[c] - twin.get(c, 0) for c in learner) / max(1, len(learner)), 3))
    elif pid == 'speak':
        said = [l.split('spoke:', 1)[1].strip().strip('"') for l in open(result, encoding='utf-8', errors='replace')
                if 'spoke:' in l]
        items, good = [], 0
        for (cue, word), s in zip(p['words'].items(), said):
            rest = word[len(cue):]
            first = next((ch for ch in s if ch != '_'), '')
            hit = first != '' and first in rest[:2]
            good += hit
            items.append(dict(cue=cue, word=word, said=s, first=first, right=hit))
        out.update(score=good / max(1, len(items)), items=items)
    elif pid == 'sentence':
        sc = labscores(report)
        endings = [s[-1] for s in p['sentences']]
        starts = {s[-2] if s[-2] != 'the' else s[-3]: s[-1] for s in p['sentences']}  # the word before the final 'the'
        picks = ending_choice(sc[0], endings)
        twin = ending_choice(sc[1], endings) if len(sc) > 1 else []
        good = sum(1 for prev, best, _ in picks if starts.get(prev) == best)
        out.update(score=good / max(1, len(p['sentences'])), picks=[[a, b] for a, b, _ in picks],
                   twin=[[a, b] for a, b, _ in twin])
    elif pid == 'choice':
        sc = labscores(report)
        picks = [b for _, b, _ in ending_choice(sc[0], ['dog', 'bird'])]
        n = len(picks)
        dog = sum(1 for b in picks if b == 'dog')
        out.update(score=(2 * min(dog, n - dog) / n) if n else 0.0, picks=picks)
    print(json.dumps(out))


if __name__ == '__main__':
    main()
