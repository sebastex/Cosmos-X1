"""Slice by slice: does a grown brain recall in its deep slices what it cannot recall in the senses'
slice? Each slice is measured on its own scale (its shapes centred and compared only with
themselves), so a quiet slice counts as much as a loud one.

    python tools/ladder/probe.py script STAGE            the probe script and its items (json)
    python tools/ladder/probe.py score recording.ncmmon items.json

The script shows 100 learned words in two rounds (A, B) with learning frozen, then cues 48 of them by
their shortest unique beginning (as the ladder test). Per slice:
  id      round A matched to round B by likeness (can the slice alone tell the words apart?)
  recall  after a cue, is the silence in this slice more like the target word than like every
          other shown word? (chance about 1/100)
"""
import json
import os
import random
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'monitor'))
import ladder  # noqa: E402
import read_monitor as rm  # noqa: E402
from deep import events  # noqa: E402


def script(stage):
    t = ladder.test(stage)
    targets = [it['word'] for it in t['items']]
    pre = {it['word']: it['cue'] for it in t['items']}
    rng = random.Random(77)
    others = sorted(set(ladder.words()[:stage]) - set(targets))
    shown = targets + rng.sample(others, 100 - len(targets))
    rng.shuffle(shown)
    ops = ['still:1'] + [f'say:{w}:2:40' for w in shown] + ['rest:300'] + [f'say:{w}:2:40' for w in shown] + ['rest:800']
    for w in targets:
        ops += [f'cue:{pre[w]}:60', 'rest:150']
    ops.append('still:0')
    return dict(shown=shown, targets=targets, cues=[pre[w] for w in targets], script=';'.join(ops))


def score(rec, items):
    head, r, vox = rm.read(rec)
    C3, V, S = head['C'][2], head['cells'][2], head['fields']
    size, n = V * C3, len(r)
    per = size // S
    x = [rm.Sparse.of(v, C3) for v in vox]
    ev = events(r)
    shown = items['shown']
    nW = len(shown)
    learn = (r['flags'] & 16) != 0
    taught = [e for e in ev if learn[e['start']]]
    cues = [e for e in ev if not learn[e['start']]]
    A, B = taught[:2 * nW], taught[2 * nW:4 * nW]

    def shape(evs, w):
        parts = []
        for e in evs:
            if e['word'] == w:
                parts += x[e['start']:min(n, e['end'] + 3)]
        return rm.dense(rm.Sparse.total(parts), size)

    MA = np.array([shape(A, w) for w in shown])
    MB = np.array([shape(B, w) for w in shown])
    # Each cue's silence: 3..28 ticks after the cue's last letter (as the ladder's scores).
    by_cue = {}
    for k, e in enumerate(cues):
        nxt = cues[k + 1]['start'] if k + 1 < len(cues) else n
        if nxt - e['end'] < 20:
            continue
        a0 = min(n, e['end'] + 3)
        by_cue.setdefault(e['word'], rm.dense(rm.Sparse.total(x[a0:min(n, a0 + 25)]), size))
    out = dict(words=nW, cues=len(items['cues']), slices=[])
    idx = {w: i for i, w in enumerate(shown)}
    for s in range(S):
        sa, sb = MA[:, s * per:(s + 1) * per], MB[:, s * per:(s + 1) * per]
        mean = sb.mean(axis=0)
        ca, cb = sa - sa.mean(axis=0), sb - mean
        unit = lambda m: m / np.maximum(np.linalg.norm(m, axis=-1, keepdims=True), 1e-12)
        na, nb = unit(ca), unit(cb)
        ident = float(((na @ nb.T).argmax(axis=1) == np.arange(nW)).mean())
        right, margins, energy = 0, [], 0.0
        for cue, w in zip(items['cues'], items['targets']):
            if cue not in by_cue:
                continue
            sil = by_cue[cue][s * per:(s + 1) * per]
            energy += float((sil ** 2).sum())
            sims = nb @ unit(sil - mean)
            t = idx[w]
            other = np.max(np.delete(sims, t))
            margins.append(float(sims[t] - other))
            right += sims[t] > other
        out['slices'].append(dict(slice=s, id=round(ident, 3), recall=round(right / max(1, len(margins)), 3),
                                  margin=round(float(np.mean(margins)) if margins else 0.0, 4),
                                  word_energy=round(float((sb ** 2).sum()), 2), cue_energy=round(energy, 2)))
    # All slices together, each on its own scale (every slice's shapes scaled to unit norm first).
    return out


def main():
    if sys.argv[1] == 'script':
        print(json.dumps(script(int(sys.argv[2]))))
    else:
        print(json.dumps(score(sys.argv[2], json.load(open(sys.argv[3]))), indent=1))


if __name__ == '__main__':
    main()
