"""Quick test: a fresh brain learns the ladder's first N words, then is tested (learning frozen).
Short enough to run many versions side by side; meant to find what breaks, not to rank close ones.

    python tools/quick/quick.py lesson N            the lesson (each word alone, 8 times; then a state report)
    python tools/quick/quick.py test N              the test: show the N words twice, cue each by its beginning
                                                    and by the whole word; then the levels audit (lesson and
                                                    recall mode) on same / lookalike / unrelated pairs
    python tools/quick/quick.py summary N RESULT MONITOR   one json line: recall, how alike words are, per field
"""
import itertools
import json
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'ladder'))
import ladder  # noqa: E402


def words(n):
    return list(ladder.words())[:n]


def pairs(n):
    ws = words(n)
    same = [(w, w) for w in ws[:4]]
    look = [(a, b) for a, b in itertools.combinations(ws, 2) if a[:2] == b[:2]][:5]
    far = [(ws[i], ws[-1 - i]) for i in range(3)]
    return same, look, far


def lesson(n):
    return ';'.join([f'say:{w}:8:40' for w in words(n)] + ['rest:300', 'report:lesson'])


def test(n):
    ws = words(n)
    rng = random.Random(5)
    shown = ws[:]
    rng.shuffle(shown)
    pre = ladder.prefixes(ws)
    script = ['still:1'] + [f'say:{w}:2:40' for w in shown] + ['rest:800']
    for w in ws:
        script += [f'cue:{w}:60', 'rest:150']
        if pre[w]:
            script += [f'cue:{pre[w]}:60', 'rest:150']
    same, look, far = pairs(n)
    spec = ','.join(f'{a}/{b}' for a, b in same + look + far)
    script += [f'levels:{spec}:8:1', f'levels:{spec}:8:0']
    script += [f'drive:{ws[0]}:2:1', f'drive:{ws[0]}:2:0', f'drive:{ws[0]}:8:0', 'still:0']
    items = [dict(cue=w, word=w) for w in ws] + [dict(cue=pre[w], word=w) for w in ws if pre[w]]
    return ';'.join(script), items


def summary(n, result, monitor):
    import numpy as np
    sys.path.insert(0, os.path.join(HERE, '..', 'monitor'))
    import read_monitor as rm
    _, items = test(n)
    head, r, vox = rm.read(monitor)
    m = len(r)
    C3 = head['C'][2]
    size = head['cells'][2] * C3
    Vf = head['cells'][2] // head['fields']
    x = [rm.Sparse.of(v, C3) for v in vox]
    heard, flags = r['heard'], r['flags']
    sound = (flags & 8) != 0
    learn = (flags & 16) != 0
    ev, i = [], 0
    while i < m:
        if sound[i] and heard[i] not in (0, 32):
            j, w = i, ''
            while j < m and sound[j] and heard[j] not in (0, 32):
                w += chr(heard[j])
                j += 1
            ev.append((w, i, j, bool(learn[i:j].any())))
            i = j
        else:
            i += 1
    D = lambda a, b: rm.dense(rm.Sparse.total(x[a:min(m, b)]), size)
    shapes = {}
    for w, a, b, l in ev:
        if l:
            shapes.setdefault(w, []).append(D(a, b + 3))
    ws = list(shapes)
    M = np.stack([np.mean(shapes[w][-2:], axis=0) for w in ws])
    bg = M.mean(0)
    Cn = M - bg
    nn = np.linalg.norm(Cn, axis=1) + 1e-12
    Mn = M / (np.linalg.norm(M, axis=1, keepdims=True) + 1e-12)
    S = Mn @ Mn.T
    np.fill_diagonal(S, np.nan)
    wi = {w: k for k, w in enumerate(ws)}
    target = {}
    for it in items:
        target.setdefault(it['cue'], it['word'])
    right = {'whole': [0, 0], 'beginning': [0, 0]}
    fields = np.zeros(head['fields'])
    for w, a, b, l in ev:
        if l or w not in target or target[w] not in wi:
            continue
        q = D(b + 3, b + 28)
        fields += [np.linalg.norm(q[f * Vf * C3:(f + 1) * Vf * C3]) for f in range(head['fields'])]
        q = q - bg
        s = (Cn @ q) / (nn * (np.linalg.norm(q) + 1e-12))
        t = wi[target[w]]
        k = 'whole' if w == target[w] else 'beginning'
        right[k][1] += 1
        right[k][0] += int(s[t] > np.delete(s, t).max())
    shape_fields = np.mean([[np.linalg.norm(M[k][f * Vf * C3:(f + 1) * Vf * C3]) for f in range(head['fields'])] for k in range(len(ws))], axis=0)
    levels = {'lesson': None, 'recall': None}
    same, look, far = pairs(n)
    rows = [json.loads(l.split(' ', 1)[1]) for l in open(result) if l.startswith('LEVELS ')]
    k = len(same) + len(look) + len(far)
    for name, part in (('lesson', rows[:k]), ('recall', rows[k:2 * k])):
        if len(part) < k:
            continue
        g = lambda rs: round(float(np.mean([r_['s3_after'] for r_ in rs])), 3)
        levels[name] = dict(same=g(part[:len(same)]), lookalike=g(part[len(same):len(same) + len(look)]), unrelated=g(part[-len(far):]),
                            fields=[round(float(np.mean([r_['fields_after'][f][1] for r_ in part])), 1) for f in range(4)])
    report = next((json.loads(l.split(' ', 1)[1]) for l in open(result) if l.startswith('BRAINREPORT ')), None)
    out = dict(recall_whole=round(right['whole'][0] / max(1, right['whole'][1]), 3),
               recall_beginning=round(right['beginning'][0] / max(1, right['beginning'][1]), 3),
               words_alike=round(float(np.nanmean(S)), 3), words_alike_max=round(float(np.nanmax(S)), 3),
               shape_per_field=[round(float(v), 1) for v in shape_fields],
               answer_per_field=[round(float(v / max(1, right['whole'][1] + right['beginning'][1])), 1) for v in fields],
               levels=levels)
    drives = [json.loads(l.split(' ', 1)[1]) for l in open(result) if l.startswith('DRIVE ')]
    if drives:  # where each field's input comes from (per firing voxel), heard in lesson mode, in recall, and later in recall
        out['drive'] = {f"{'lesson' if d['lesson'] else 'recall'}+{d['after']}": [{k: (round(v, 2) if isinstance(v, float) else v) for k, v in fd.items()}
                                                                              for fd in d['report']['fields']] for d in drives}
    if report:  # the lesson's state report: per field link fill and hubs (how much one source dominates)
        out['links'] = [dict(fill=round(f['fill'], 3), hubs=round(f['hubs'], 3), active3d=round(f['active3d'][0], 4)) for f in report['fields']]
    return out


def main():
    a = sys.argv[1:]
    if a[:1] == ['lesson']:
        print(lesson(int(a[1])))
    elif a[:1] == ['test']:
        print(test(int(a[1]))[0])
    elif a[:1] == ['summary']:
        print(json.dumps(summary(int(a[1]), a[2], a[3])))
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
