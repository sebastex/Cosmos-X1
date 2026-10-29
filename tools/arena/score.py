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


def said_line(steps):
    """The words it says by itself, one per window, repeats joined."""
    out = []
    for st in steps:
        w = next(iter(st), None) if st and max(st.values()) > 0 else None
        if w and (not out or out[-1] != w):
            out.append(w)
    return out


def line_got(said, path):
    """How many words of the path it said in the right order."""
    k = 0
    for w in said:
        if k < len(path) and w == path[k]:
            k += 1
    return k


def judge(score, stm):
    """Each statement: what it expected before the last word, its surprise when the word came, how busy it stayed.
    It says false when its surprise is above its own middle surprise."""
    got = score.get('asks', [])
    rows = []
    for k, st in enumerate(stm):
        if 2 * k + 1 >= len(got):
            break
        pre, post = got[2 * k], got[2 * k + 1]
        sims = pre['sims']
        other = max(sims.get(c, -1.0) for c in ('trees', 'bushes', 'vines') if c != st['said'])
        rows.append(dict(st, expect=round(sims.get(st['said'], -1.0) - other, 4), turn=post.get('turn'),
                         busy=round(sum(post.get('busy', [])), 2), heard=post['last']))
    # The same ending word is compared only with itself (its echo is the same), so the middle is per word.
    mid = {}
    for w in {r['said'] for r in rows}:
        ts = sorted(r['turn'] for r in rows if r['said'] == w and r['turn'] is not None)
        mid[w] = (ts[(len(ts) - 1) // 2] + ts[len(ts) // 2]) / 2 if ts else 0
    for r in rows:
        r['answer'] = r['turn'] is None or r['turn'] <= mid[r['said']]  # True = it takes it as true
        r['right'] = r['answer'] == r['truth']
    return rows, {w: round(v, 4) for w, v in mid.items()}


def judge_summary(rows):
    out = {}
    for lv in sorted({r['level'] for r in rows}):
        rs = [r for r in rows if r['level'] == lv]
        # pairs: the same ending word, once where it is true and once where it is wrong
        both = [(t, f) for t in rs for f in rs if t['truth'] and not f['truth'] and t['said'] == f['said']
                and t['turn'] is not None and f['turn'] is not None]
        mean = lambda xs: round(sum(xs) / len(xs), 4) if xs else None
        out[lv] = dict(n=len(rs), right=round(sum(r['right'] for r in rs) / len(rs), 3),
                       wrong_more_surprising=round(sum(f['turn'] > t['turn'] for t, f in both) / max(1, len(both)), 3),
                       expected_true=round(sum(r['expect'] > 0 for r in rs if r['truth']) / max(1, sum(r['truth'] for r in rs)), 3),
                       surprise_true=mean([r['turn'] for r in rs if r['truth'] and r['turn'] is not None]),
                       surprise_false=mean([r['turn'] for r in rs if not r['truth'] and r['turn'] is not None]),
                       busy_true=mean([r['busy'] for r in rs if r['truth']]),
                       busy_false=mean([r['busy'] for r in rs if not r['truth']]))
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
        # The whole line: what it says by itself after the cue, and how much of the path is in it, in order.
        asks = [sc[0].get('asks', []), sc[1].get('asks', []) if len(sc) > 1 else []]
        for k, it in enumerate(p['items']):
            if 'path' not in it or k >= len(learner):
                continue
            said = said_line(asks[0][k].get('steps', []))
            tsaid = said_line(asks[1][k].get('steps', [])) if k < len(asks[1]) else []
            learner[k].update(path=it['path'], said=said, got=line_got(said, it['path']),
                              exact=said[:len(it['path'])] == it['path'])
            if k < len(twin):
                twin[k].update(said=tsaid, got=line_got(tsaid, it['path']))
            # right only when the whole line comes out in order and the twin does not do the same
            learner[k]['margin'] = round(learner[k]['got'] / len(it['path']) - 1 + 1e-3, 4) if learner[k]['got'] == len(it['path']) else -1.0
            tm[k] = (twin[k]['got'] / len(it['path']) - 1 + 1e-3) if k < len(twin) and twin[k]['got'] == len(it['path']) else -9
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
    elif kind == 'judge':
        sc = labscores(report)
        learner, mid = judge(sc[0], p['statements']) if sc else ([], 0)
        twin, tmid = judge(sc[1], p['statements']) if len(sc) > 1 else ([], 0)
        out.update(score=sum(r['right'] for r in learner) / max(1, len(p['statements'])), middle=mid,
                   levels=judge_summary(learner), twin_levels=judge_summary(twin) if twin else None,
                   twin_score=sum(r['right'] for r in twin) / max(1, len(p['statements'])) if twin else None,
                   learner=learner, twin=twin)
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
