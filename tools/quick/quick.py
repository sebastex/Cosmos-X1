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


def mouth(n):
    """Speaking step 1: hear each word's beginning, then listen to the mouth (the Output field's far face)."""
    ws = words(n)
    pre = ladder.prefixes(ws)
    script = ['still:1']
    for w in ws:
        if pre[w]:
            script += ['rest:150', f'mouth:{pre[w]}:30', 'rest:150', f'talk:{pre[w]}:{len(w) - len(pre[w]) + 1}']
    return ';'.join(script)


def mouth_summary(n, result):
    """Per readout (exit lines = 'mouth', face sheet cells = 'mouth_sheet'): did it say a letter after the
    cue (some letter's fingerprint >= 30% shown), was the first such letter the word's next letter, or an echo."""
    ws = words(n)
    pre = ladder.prefixes(ws)
    nxt = {pre[w]: w[len(pre[w])] for w in ws if pre[w]}
    rows = [json.loads(l.split(' ', 1)[1]) for l in open(result) if l.startswith('MOUTH ')]
    if not rows:
        return {}
    out = {}
    for key, top, ov, act, lim in (('mouth', 'top', 'overlap', 'active', 0.3), ('mouth_sheet', 'top2', 'overlap2', 'active2', 0.0)):
        if top not in rows[0]:
            continue
        # exit lines: a letter counts once 30% of its lines show; face sheets: the best-matching letter, however faint
        first = [next((c for c, o in zip(r[top], r[ov]) if o > lim and c != '-'), None) for r in rows]
        out[key] = dict(cues=len(rows), spoke=round(sum(1 for f in first if f) / len(rows), 3),
                        next_letter=round(sum(1 for r, f in zip(rows, first) if f and f == nxt.get(r['cue'])) / len(rows), 3),
                        echo=round(sum(1 for r, f in zip(rows, first) if f and f == r['cue'][-1]) / len(rows), 3),
                        from_cue=round(sum(1 for r, f in zip(rows, first) if f and f in r['cue']) / len(rows), 3),
                        next_any=round(sum(1 for r in rows if nxt.get(r['cue']) in r[top]) / len(rows), 3),
                        lines_on=round(sum(sum(r[act]) / len(r[act]) for r in rows) / len(rows), 1),
                        best_overlap=round(sum(max(r[ov]) for r in rows) / len(rows), 3),
                        sample={r['cue']: r[top] for r in rows[:5]})
    talks = [json.loads(l.split(' ', 1)[1]) for l in open(result) if l.startswith('TALK ')]
    if talks:  # speaking step 3: does it spell the rest of the word on its own?
        full = {pre[w]: w for w in ws if pre[w]}
        lead, exact, first = [], 0, 0
        for t in talks:
            rest = full[t['cue']][len(t['cue']):]
            k = 0
            while k < min(len(rest), len(t['said'])) and rest[k] == t['said'][k]:
                k += 1
            lead.append(k / max(1, len(rest)))
            exact += int(t['said'] == rest)
            first += int(bool(t['said']) and t['said'][0] == rest[0])
        out['talk'] = dict(cues=len(talks), first_right=round(first / len(talks), 3), spelled=round(sum(lead) / len(lead), 3),
                           whole_words=round(exact / len(talks), 3), sample={t['cue']: t['said'] for t in talks[:8]})
    return out


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
    per_field = {'whole': [0] * head['fields'], 'beginning': [0] * head['fields']}
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
        for f in range(head['fields']):  # the same test on each field alone: what does each field remember?
            sl = slice(f * Vf * C3, (f + 1) * Vf * C3)
            cf, qf = Cn[:, sl], q[sl]
            sf = (cf @ qf) / ((np.linalg.norm(cf, axis=1) + 1e-12) * (np.linalg.norm(qf) + 1e-12))
            per_field[k][f] += int(sf[t] > np.delete(sf, t).max())
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
               recall_per_field={k: [round(v / max(1, right[k][1]), 3) for v in per_field[k]] for k in per_field},
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
    out.update(mouth_summary(n, result))
    # Integration (rough): do the four fields rise and fall together during the test (coupling), while
    # words stay distinct (differentiation)? coupling x differentiation; 0 = parts apart or all one blob.
    test_ticks = [t for t in range(m) if not learn[t]]
    if test_ticks:
        per = np.array([np.bincount(x[t].idx // (Vf * C3), weights=x[t].val, minlength=head['fields'])[:head['fields']] for t in test_ticks])
        if per.std(axis=0).min() > 0:
            cc = np.corrcoef(per.T)
            coupling = float(cc[~np.eye(len(cc), dtype=bool)].mean())
            out['integration'] = dict(coupling=round(coupling, 3), differentiation=round(1 - out['words_alike'], 3),
                                      score=round(max(0.0, coupling) * (1 - out['words_alike']), 3))
    return out


def main():
    a = sys.argv[1:]
    if a[:1] == ['lesson']:
        print(lesson(int(a[1])))
    elif a[:1] == ['test']:
        print(test(int(a[1]))[0])
    elif a[:1] == ['mouth']:
        print(mouth(int(a[1])))
    elif a[:1] == ['summary']:
        print(json.dumps(summary(int(a[1]), a[2], a[3])))
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
