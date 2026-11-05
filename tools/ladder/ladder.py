"""The word ladder: one brain learns 1000 words, each on its own, in stages of 50, 100, 200, 500, 1000,
and after every stage it is tested on words from every stage so far (does new learning crowd out
old?), with a full report of the brain's state (tools/ladder/ladder.yml runs it in 3-hour relays).

    python tools/ladder/ladder.py bounds              json: the op index where each stage ends
    python tools/ladder/ladder.py lesson FROM TO      the lesson ops FROM..TO (';'-joined)
    python tools/ladder/ladder.py test STAGE          the test after STAGE words, as an arena puzzle
    python tools/ladder/ladder.py row STAGE score.json report.txt result.txt   one growth-log row (json)
    python tools/ladder/ladder.py table growth.json   the ladder as a table

Words are heard alone (8 times, as in complete50); nothing is ever paired. The test first shows
up to 200 learned words with learning frozen (still:1, so their shapes are recorded but nothing
changes), then cues up to 48 of them by their shortest unique beginning; an untrained twin answers
the same cues as the baseline.
"""
import collections
import json
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
STAGES = [50, 100, 200, 500, 1000]


def words():
    ws = [w.strip() for w in open(os.path.join(HERE, 'words.txt')) if w.strip()]
    random.Random(1000).shuffle(ws)
    return ws


def plan():
    """Ops and the op index after each stage."""
    ws = words()
    ops, bounds, prev = [], [], 0
    for st in STAGES:
        for w in ws[prev:st]:
            ops.append(f'say:{w}:8:40')
        ops += ['rest:300', f'report:after{st}']
        bounds.append(len(ops))
        prev = st
    return ops, bounds


def prefixes(ws):
    """Each word's shortest beginning (3 letters or more) that no other word in the list shares and
    that is not itself a word; None if there is none."""
    out = {}
    allw = set(ws)
    for w in ws:
        for k in range(3, len(w)):
            p = w[:k]
            if p not in allw and sum(1 for v in ws if v.startswith(p)) == 1:
                out[w] = p
                break
        else:
            out[w] = None
    return out


def stage_of():
    ws = words()
    m, prev = {}, 0
    for st in STAGES:
        for w in ws[prev:st]:
            m[w] = st
        prev = st
    return m


def test(stage):
    ws = words()[:stage]
    pre = prefixes(words())
    so = stage_of()
    rng = random.Random(stage)
    by = collections.defaultdict(list)
    for w in ws:
        if pre[w]:
            by[so[w]].append(w)
    groups = sorted(by)
    per = max(1, 48 // len(groups))
    targets = []
    for g in groups:
        pool = sorted(by[g])
        targets += rng.sample(pool, min(per, len(pool)))
    others = sorted(set(ws) - set(targets))
    shown = targets + rng.sample(others, min(200 - len(targets), len(others)))
    rng.shuffle(shown)
    rng.shuffle(targets)
    script = ['still:1', f'report:test{stage}'] + [f'say:{w}:2:40' for w in shown] + ['rest:800']
    for w in targets:
        script += [f'cue:{pre[w]}:60', 'rest:150']
    script.append('still:0')
    items = [dict(cue=pre[w], word=w, stage=so[w]) for w in targets]
    return dict(id=f'ladder{stage}', name=f'Word ladder after {stage} words', kind='complete', twin=True,
                items=items, shown=len(shown), script=';'.join(script))


def labscores(report):
    out = []
    for line in open(report, encoding='utf-8', errors='replace'):
        if line.startswith('LABSCORE '):
            out.append(json.loads(line[len('LABSCORE '):]))
    return out


def row(stage, score_path, report, result):
    """One stage: recall by the stage each word was learned in, the twin, what wrongly won (magnets),
    how crowded the word shapes are, and the state of the learned links."""
    sc = json.load(open(score_path))
    t = test(stage)
    item = {it['cue']: it for it in t['items']}
    learner, twin = sc.get('learner', {}), sc.get('twin', {})
    by = collections.defaultdict(lambda: dict(n=0, right=0, twin_right=0))
    for cue, m in learner.items():
        it = item.get(cue)
        if not it:
            continue
        r = by[it['stage']]
        r['n'] += 1
        r['right'] += m > 0 and m > twin.get(cue, -9)
        r['twin_right'] += twin.get(cue, -9) > 0
    for r in by.values():
        r['score'] = round(r['right'] / r['n'], 3)
    labs = labscores(report)
    won = collections.Counter()
    if labs:
        for c in labs[0].get('cues', []):
            if c['cue'] in item and c.get('best') and c['best'] != item[c['cue']]['word']:
                won[c['best']] += 1
    brain = [json.loads(l[len('BRAINREPORT '):]) for l in open(result, encoding='utf-8', errors='replace')
             if l.startswith('BRAINREPORT ')]
    return dict(stage=stage, score=round(sc.get('score', 0), 3), gain=sc.get('gain'), cues=len(learner),
                by_stage={str(k): v for k, v in sorted(by.items())},
                wrong_winners=won.most_common(8),
                crowd=labs[0].get('crowd') if labs else None,
                crowd_twin=labs[1].get('crowd') if len(labs) > 1 else None,
                brain=brain[0] if brain else None)


def table(path):
    log = json.load(open(path))
    print(f"{log.get('variant')} seed {log.get('seed')}")
    print('stage  recall  by stage learned                      overlap  nearest  copies  dims/word  bg    fill   full   hubs')
    for r in log['stages']:
        c, b = r.get('crowd') or {}, r.get('brain') or {}
        f0 = (b.get('fields') or [{}])[0]
        bys = ' '.join(f"{k}:{v['score']:.2f}" for k, v in r['by_stage'].items())
        print(f"{r['stage']:5}  {r['score']:.2f}    {bys:38}  {c.get('pair_mean', 0):+.3f}   {c.get('nearest_mean', 0):.3f}    "
              f"{c.get('near_copies', 0):.2f}    {c.get('dims_per_word', 0):.3f}      {c.get('background_share', 0):.2f}  "
              f"{f0.get('fill', 0):.3f}  {f0.get('full', 0):.3f}  {f0.get('hubs', 0):.3f}")


def main():
    what = sys.argv[1]
    if what == 'bounds':
        print(json.dumps(plan()[1]))
    elif what == 'lesson':
        ops = plan()[0]
        print(';'.join(ops[int(sys.argv[2]):int(sys.argv[3])]))
    elif what == 'test':
        print(json.dumps(dict(puzzles=[test(int(sys.argv[2]))])))
    elif what == 'row':
        print(json.dumps(row(int(sys.argv[2]), sys.argv[3], sys.argv[4], sys.argv[5])))
    elif what == 'table':
        table(sys.argv[2])


if __name__ == '__main__':
    main()
