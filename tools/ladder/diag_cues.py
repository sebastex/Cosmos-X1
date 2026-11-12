"""Why cues fail: follows every cue of a test recording (NCM_MONITOR with voxels) step by step.

    python tools/ladder/diag_cues.py monitor/session0.ncmmon test.json [--out diag.json]

Uses the same shapes and answer window as the scorer (read_monitor.lab_scores: each word's mean
shape over its last 3 hearings, the background = the mean word shape, answer = the 25 ticks after a
cue, centred). For every cue it reports:
  strength   how strong the answer is: |answer| / |typical word shape| (raw, before centring)
  bg_cos     how alike the raw answer is to the background (a blurry average of all words)
  best_raw   the known word the raw answer is most like, and how much (another word pulling)
  target     the cued word's similarity, raw and centred; the margin as the scorer computes it
  course     the target's similarity and the activity in 5-tick steps for 150 ticks
It sorts failed cues into: died (answer < 0.25 of a word), blur (raw answer mostly background and no
word stands out), wrong word (the answer is clearly another word), weak (in between). And it shows
which word wins when the answer is silence (the scorer's default winner) and when it is pure background.
"""
import argparse
import collections
import json
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'monitor'))
import read_monitor as rm  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('monitor')
    ap.add_argument('test')
    ap.add_argument('--out', default='diag.json')
    ap.add_argument('--gap', type=int, default=0,
                    help='cues = the words heard after the first silence of GAP+ ticks (for cues heard in '
                         'encoding mode, which carry the learning flag); 0 = cues are the words heard without it')
    a = ap.parse_args()
    head, r, vox = rm.read(a.monitor)
    items = {it['cue']: it['word'] for it in json.load(open(a.test))['puzzles'][0]['items']}
    n = len(r)
    C3 = head['C'][2]
    size = head['cells'][2] * C3
    Vf = head['cells'][2] // head['fields']
    x = [rm.Sparse.of(v, C3) for v in vox]
    heard, flags = r['heard'], r['flags']
    sound = (flags & 8) != 0
    learning = (flags & 16) != 0
    events, i = [], 0
    while i < n:
        if sound[i] and heard[i] not in (0, 32):
            j, w = i, ''
            while j < n and sound[j] and heard[j] not in (0, 32):
                w += chr(heard[j])
                j += 1
            nxt = j
            while nxt < n and not (sound[nxt] and heard[nxt] not in (0, 32)):
                nxt += 1
            events.append(dict(word=w, start=i, end=j, next=nxt, learn=bool(learning[i:j].any())))
            i = j
        else:
            i += 1
    if a.gap:
        cut = next((k + 1 for k in range(len(events) - 1) if events[k + 1]['start'] - events[k]['end'] >= a.gap), len(events))
        taught, cues = events[:cut], events[cut:]
    else:
        taught = [e for e in events if e['learn']]
        cues = [e for e in events if not e['learn']]
    shapes = collections.defaultdict(list)
    for e in taught:
        shapes[e['word']].append(rm.Sparse.total(x[e['start']:min(n, e['end'] + 3)]))
    words = list(shapes)
    mean = {w: rm.dense(rm.Sparse.total(s[-3:]), size) for w, s in shapes.items()}
    M = np.stack([mean[w] for w in words])
    bg = M.mean(axis=0)
    Cn = M - bg
    Cn_norm = np.linalg.norm(Cn, axis=1)
    M_norm = np.linalg.norm(M, axis=1)
    word_norm = float(np.median(M_norm))

    def cos_all(vec, mat, mat_norm):
        nv = np.linalg.norm(vec)
        if nv == 0:
            return np.zeros(len(mat))
        return (mat @ vec) / (mat_norm * nv + 1e-12)

    def ccos(u, v):
        nu, nv = np.linalg.norm(u), np.linalg.norm(v)
        return float(u @ v / (nu * nv)) if nu > 0 and nv > 0 else 0.0

    # The scorer's default winners: the answer is silence (centred = -background), or nearly pure background.
    sil = cos_all(-bg, Cn, Cn_norm)
    default_silence = [words[k] for k in np.argsort(-sil)[:8]]
    slice_of = lambda vec: [float(np.linalg.norm(vec[f * Vf * C3:(f + 1) * Vf * C3])) for f in range(head['fields'])]
    word_slices = np.mean([slice_of(mean[w]) for w in words], axis=0)

    rows = []
    for e in cues:
        target = items.get(e['word'])
        if target is None or target not in mean:
            continue
        a0 = min(n, e['end'] + 3)
        a1 = min(n, a0 + 25)
        raw = rm.dense(rm.Sparse.total(x[a0:a1]), size)
        cen = raw - bg
        sims_c = cos_all(cen, Cn, Cn_norm)
        sims_r = cos_all(raw, M, M_norm)
        ti = words.index(target)
        others_c = np.delete(sims_c, ti)
        best_c = words[int(np.argmax(np.where(np.arange(len(words)) == ti, -9, sims_c)))]
        order_r = np.argsort(-sims_r)
        best_r = words[order_r[0]] if words[order_r[0]] != target else words[order_r[1]]
        course = []
        for t0 in range(e['end'], min(n, e['end'] + 150, e['next']), 5):
            v = rm.dense(rm.Sparse.total(x[t0:t0 + 5]), size)
            course.append(dict(t=t0 - e['end'], act=round(float(np.linalg.norm(v)), 2),
                               target=round(ccos(v, mean[target]), 3), bg=round(ccos(v, bg), 3)))
        cue_vec = rm.dense(rm.Sparse.total(x[e['start']:e['end'] + 3]), size)
        row = dict(cue=e['word'], target=target, strength=round(float(np.linalg.norm(raw)) / word_norm, 3),
                   bg_cos=round(ccos(raw, bg), 3), target_raw=round(float(sims_r[ti]), 3),
                   target_c=round(float(sims_c[ti]), 3), margin=round(float(sims_c[ti] - others_c.max()), 3),
                   best_c=best_c, best_c_sim=round(float(sims_c.max()), 3), best_raw=best_r,
                   best_raw_sim=round(float(sims_r[words.index(best_r)]), 3),
                   cue_target_raw=round(ccos(cue_vec, mean[target]), 3),
                   slices=[round(s / max(1e-9, w), 2) for s, w in zip(slice_of(raw), word_slices)], course=course)
        ok = row['margin'] > 0
        if ok:
            kind = 'right'
        elif row['strength'] < 0.25:
            kind = 'died'
        elif row['bg_cos'] > 0.8 and row['best_c_sim'] < 0.2:
            kind = 'blur'
        elif row['best_raw_sim'] > 0.5 and row['best_raw_sim'] > row['target_raw'] + 0.05:
            kind = 'wrong word'
        else:
            kind = 'weak'
        row['kind'] = kind
        rows.append(row)

    kinds = collections.Counter(r_['kind'] for r_ in rows)
    winners = collections.Counter(r_['best_c'] for r_ in rows if r_['kind'] != 'right')
    def med(k, sel):
        v = [r_[k] for r_ in rows if sel(r_)]
        return round(float(np.median(v)), 3) if v else None
    summary = dict(cues=len(rows), kinds=dict(kinds), wrong_winners=winners.most_common(10),
                   default_winners_if_silent=default_silence,
                   magnets_that_are_silence_defaults=[w for w, _ in winners.most_common(10) if w in default_silence[:5]],
                   strength=dict(right=med('strength', lambda r_: r_['kind'] == 'right'),
                                 failed=med('strength', lambda r_: r_['kind'] != 'right')),
                   bg_cos=dict(right=med('bg_cos', lambda r_: r_['kind'] == 'right'),
                               failed=med('bg_cos', lambda r_: r_['kind'] != 'right')),
                   cue_already_like_target=dict(right=med('cue_target_raw', lambda r_: r_['kind'] == 'right'),
                                                failed=med('cue_target_raw', lambda r_: r_['kind'] != 'right')),
                   slices_of_answer_vs_word=dict(
                       right=[round(float(np.median([r_['slices'][f] for r_ in rows if r_['kind'] == 'right'] or [0])), 2) for f in range(head['fields'])],
                       failed=[round(float(np.median([r_['slices'][f] for r_ in rows if r_['kind'] != 'right'] or [0])), 2) for f in range(head['fields'])]))
    # Time course: median target similarity and activity, right vs failed, in 5-tick steps.
    for name, sel in (('right', lambda r_: r_['kind'] == 'right'), ('failed', lambda r_: r_['kind'] != 'right')):
        sel_rows = [r_ for r_ in rows if sel(r_)]
        steps = min((len(r_['course']) for r_ in sel_rows), default=0)
        summary[f'course_{name}'] = [dict(t=sel_rows[0]['course'][k]['t'],
                                          act=round(float(np.median([r_['course'][k]['act'] for r_ in sel_rows])), 2),
                                          target=round(float(np.median([r_['course'][k]['target'] for r_ in sel_rows])), 3),
                                          bg=round(float(np.median([r_['course'][k]['bg'] for r_ in sel_rows])), 3))
                                     for k in range(min(steps, 30))]
    summary['typical_word_norm'] = round(word_norm, 2)
    json.dump(dict(summary=summary, cues=rows), open(a.out, 'w'), indent=1)
    print(json.dumps(summary, indent=1))


if __name__ == '__main__':
    main()
