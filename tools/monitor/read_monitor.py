"""Reads an activity-monitor file (NCM_MONITOR, see include/ncm/Monitor.hpp) and reports what
happened at every level: 1D line cells, 2D sheet cells, 3D voxels and the 4D cells.

    python tools/monitor/read_monitor.py monitor/session0.ncmmon [--csv out.csv] [--png out.png]

Prints a summary. --csv writes one row per recorded tick (totals per level); --png draws the
activity of each level over time and a map of every 4D cell's voxel activity over time
(needs matplotlib).
"""
import argparse
import struct
import sys

import numpy as np

LEVELS = ('1D line cells', '2D sheet cells', '3D voxels')


def read(path):
    with open(path, 'rb') as f:
        data = f.read()
    if data[:8] != b'NCMMON01':
        sys.exit('not a monitor file: ' + path)
    o = 8
    (version, N, SS, L, C1, C2, C3, fields, A, B, G, lattice, voxels, every) = struct.unpack_from('<14I', data, o)
    o += 14 * 4
    levels = struct.unpack_from('<3f', data, o)
    o += 12
    V, Q, P = struct.unpack_from('<3Q', data, o)
    o += 24
    head = dict(version=version, N=N, SS=SS, L=L, C=(C1, C2, C3), fields=fields, A=A, B=B, G=G,
                lattice=bool(lattice), voxels=bool(voxels), every=every, levels=levels, cells=(P, Q, V))
    rec = np.dtype([('tick', '<u8'), ('flags', 'u1'), ('heard', 'u1'), ('pad', '<u2'), ('mod', '<f4'),
                    ('a1', '<u4'), ('s1', '<f4'), ('a2', '<u4'), ('s2', '<f4'), ('a3', '<u4'), ('s3', '<f4'),
                    ('g', [('a1', '<u4'), ('s1', '<f4'), ('a2', '<u4'), ('s2', '<f4'), ('a3', '<u4'),
                           ('s3', '<f4'), ('d4', '<f4')], (G,))])
    if version >= 2:  # roots: stems, sheet cells in active / silent voxels, line cells in active / silent sheet cells
        extra = [('mtc', '<u8')] if version >= 3 else []  # microtubule reductions so far
        rec = np.dtype(rec.descr[:11] + [('stems', '<u4'), ('r2', '<u4'), ('o2', '<u4'), ('r1', '<u4'), ('o1', '<u4')] + extra +
                       rec.descr[11:])
    if not voxels:
        n = (len(data) - o) // rec.itemsize
        recs = np.frombuffer(data, rec, n, o)
        return head, recs, None
    rows, vox = [], []
    while o + rec.itemsize <= len(data):
        rows.append(np.frombuffer(data, rec, 1, o)[0])
        o += rec.itemsize
        (n,) = struct.unpack_from('<I', data, o)
        o += 4
        item = np.dtype([('v', '<u4'), ('ch', 'u1', (C3,))])
        vox.append(np.frombuffer(data, item, n, o))
        o += n * item.itemsize
    return head, np.array(rows, dtype=rec), vox


def cell_name(g, A):
    x = g % A
    y = (g // A) % A
    z = (g // (A * A)) % A
    w = g // (A * A * A)
    return f'({x},{y},{z},{w})'


def expressions(head, r):
    # 4D expression of every tick: for each 4D cell, the share of its voxels, sheet cells and line
    # cells that are active (each level scaled to the same weight, so the many line cells do not
    # drown the voxels).
    P, Q, V = head['cells']
    G = head['G']
    g = r['g']
    blocks = []
    for k, cells in ((3, V), (2, Q), (1, P)):
        b = g[f'a{k}'].astype(np.float32) / (cells / G)
        norm = np.linalg.norm(b, axis=1, keepdims=True)
        blocks.append(np.divide(b, norm, out=np.zeros_like(b), where=norm > 0))
    return np.concatenate(blocks, axis=1)


def cos(a, b):
    na, nb = np.linalg.norm(a), np.linalg.norm(b)
    return float(a @ b / (na * nb)) if na > 0 and nb > 0 else 0.0


class Sparse:
    # A whole-content pattern: every active voxel channel (index v * C3 + c) with its value.
    def __init__(self, idx, val):
        self.idx, self.val = idx, val
        self.norm = float(np.linalg.norm(val)) if len(val) else 0.0

    @staticmethod
    def of(items, C3):
        if items is None or len(items) == 0:
            return Sparse(np.zeros(0, np.int64), np.zeros(0, np.float32))
        idx = (items['v'].astype(np.int64)[:, None] * C3 + np.arange(C3)[None, :]).ravel()
        val = items['ch'].astype(np.float32).ravel() / 255.0
        keep = val > 0
        return Sparse(idx[keep], val[keep])

    @staticmethod
    def total(parts):
        parts = [p for p in parts if len(p.idx)]
        if not parts:
            return Sparse(np.zeros(0, np.int64), np.zeros(0, np.float32))
        idx = np.concatenate([p.idx for p in parts])
        val = np.concatenate([p.val for p in parts])
        u, inv = np.unique(idx, return_inverse=True)
        return Sparse(u, np.bincount(inv, weights=val).astype(np.float32))

    def cos(self, o):
        if self.norm == 0 or o.norm == 0:
            return 0.0
        _, ia, ib = np.intersect1d(self.idx, o.idx, assume_unique=True, return_indices=True)
        return float(self.val[ia] @ o.val[ib]) / (self.norm * o.norm)


def coherence(head, r, vox=None):
    # Coherence, not predictability: patterns hold together across levels, the same idea comes back
    # in a similar (never identical) shape, different ideas take different shapes, the silence after
    # a cue is built from known shapes, and activity flows smoothly from step to step.
    n = len(r)
    heard = r['heard']
    sound = (r['flags'] & 8) != 0
    print('  coherence (not predictability):')
    if head['version'] >= 2:
        a2, a1 = r['a2'].sum(), r['a1'].sum()
        print(f"    holds together across levels: {r['r2'].sum() / max(a2, 1) * 100:.0f}% of active sheet cells sit in an "
              f"active voxel, {r['r1'].sum() / max(a1, 1) * 100:.0f}% of active line cells in an active sheet cell of an active voxel")
    if vox is None:
        print('    (shapes need the voxel record: run with NCM_MONITOR_VOXELS=1; only the level check above)')
    # Words: letters heard in a row up to a space or a pause; shape = its ticks plus 3 (shapes show late).
    if vox is None:
        return
    x = [Sparse.of(v, head['C'][2]) for v in vox]
    words, i = [], 0
    while i < n:
        if sound[i] and heard[i] not in (0, 32):
            j, w = i, ''
            while j < n and sound[j] and heard[j] not in (0, 32):
                w += chr(heard[j])
                j += 1
            end = min(n, j + 3)
            words.append((w, Sparse.total(x[i:end])))
            i = j
        else:
            i += 1
    byword = {}
    for w, e in words:
        byword.setdefault(w, []).append(e)
    rep = [w for w, es in byword.items() if len(es) >= 2]
    if rep:
        same = []
        for w in rep:
            es = byword[w]
            same += [es[a].cos(es[a + 1]) for a in range(len(es) - 1)]
        means = {w: Sparse.total(es) for w, es in byword.items()}
        keys = list(means)[:150]
        other = [means[keys[a]].cos(means[keys[b]]) for a in range(len(keys)) for b in range(a + 1, len(keys))]
        s, o = float(np.mean(same)), float(np.mean(other)) if other else 0.0
        print(f'    same idea, similar shape: {s:.2f} (two hearings of the same word, {len(rep)} words)')
        print(f'    different ideas, different shapes: {o:.2f} (between different words; lower = more distinct)')
        print(f'    coherence = {s - o:+.2f}   variation (not a copy) = {1 - s:.2f}')
        # Silence after input: is what happens built from known shapes?
        after, since = [], 99
        for t in range(n):
            since = 0 if sound[t] else since + 1
            if 1 <= since <= 20 and x[t].norm > 0 and len(after) < 3000:
                after.append(max(means[k].cos(x[t]) for k in keys))
        if after:
            print(f'    built from what it knows: silence after input is {np.mean(after):.2f} alike its closest known word '
                  f'shape ({len(after)} ticks; compare with the untrained twin)')
    else:
        print('    (no word heard twice: no shape comparison)')
    steps = np.where((r['flags'] & 2) != 0)[0]
    flow = [x[steps[k]].cos(x[steps[k + 1]]) for k in range(min(len(steps) - 1, 20000))
            if x[steps[k]].norm > 0 and x[steps[k + 1]].norm > 0]
    if flow:
        print(f'    flows smoothly: successive 3D steps {np.mean(flow):.2f} alike (1 = frozen, 0 = random jumps)')


def story(head, r, vox, limit=400):
    # Observation lab: what every hearing of every word did, in order. Needs the voxel record.
    if vox is None:
        print('  (story needs the voxel record: NCM_MONITOR_VOXELS=1)')
        return
    n = len(r)
    heard = r['heard']
    sound = (r['flags'] & 8) != 0
    x = [Sparse.of(v, head['C'][2]) for v in vox]
    if head['version'] >= 3:
        m = r['mtc'].astype(np.int64)
        mtc = np.diff(m, prepend=m[0])
    else:
        mtc = np.zeros(n, np.int64)
    stems = r['stems'] if head['version'] >= 2 else np.zeros(n)
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
            events.append((w, i, j, nxt))
            i = j
        else:
            i += 1
    shapes, seen = {}, {}
    print('  story: every hearing (shape = activity while heard + 3 ticks; after = the silence that follows, up to 30 ticks)')
    print('    #  tick   word        voxels 4Dcells sheets  MT-during  like-last like-first | after: voxels MT  like-own  closest other')
    rows = 0
    for k, (w, a, b, nxt) in enumerate(events):
        end = min(n, b + 3)
        shape = Sparse.total(x[a:end])
        aft = slice(end, min(nxt, end + 30))
        after = Sparse.total(x[aft])
        prev = seen.get(w, [])
        like_last = shape.cos(prev[-1]) if prev else float('nan')
        like_first = shape.cos(prev[0]) if prev else float('nan')
        others = [(o, after.cos(e)) for o, e in shapes.items() if o != w]
        best = max(others, key=lambda t: t[1]) if others else ('-', 0.0)
        own = after.cos(shape)
        a3 = r['a3'][a:end]
        sheets = r['a2'][a:end]
        if rows < limit:
            print(f"   {k + 1:3d} {r['tick'][a]:6d} {w[:11]:11s} {a3.max():6d} {stems[a:end].max():6.0f} {sheets.max():6d}  {mtc[a:end].sum():8d}"
                  f"   {like_last:7.2f}  {like_first:7.2f}  |  {r['a3'][aft].mean() if aft.stop > aft.start else 0:6.1f} {mtc[aft].sum():5d}"
                  f"   {own:6.2f}   {best[0][:10]} {best[1]:.2f}")
            rows += 1
        seen.setdefault(w, []).append(shape)
        shapes[w] = Sparse.total(seen[w][-3:]) # a word's shape = its last 3 hearings
    print('  per word: how much each hearing is like the one before (the shape settling over repeats)')
    for w, es in seen.items():
        if len(es) < 2:
            continue
        curve = ' '.join(f'{es[k].cos(es[k - 1]):.2f}' for k in range(1, len(es)))
        print(f'    {w[:14]:14s} {len(es):3d} hearings: {curve}')
    # Long silences: what the brain does on its own (no input for 100+ ticks).
    run, start = 0, 0
    for t in range(n + 1):
        if t < n and not sound[t]:
            if run == 0:
                start = t
            run += 1
            continue
        if run >= 100:
            seg = slice(start, start + run)
            act = r['a3'][seg]
            alive = int(np.sum(act > 0))
            print(f"  silence at tick {r['tick'][start]} ({run} ticks): voxels active in {alive} ticks, last activity "
                  f"{(np.nonzero(act)[0].max() if alive else 0)} ticks in, MT reductions {mtc[seg].sum()}")
            if alive and shapes:
                late = Sparse.total(x[start + 20:start + run])
                if late.norm > 0:
                    m = max(shapes.items(), key=lambda kv: late.cos(kv[1]))
                    print(f'    after 20 ticks it is most like "{m[0]}" ({late.cos(m[1]):.2f})')
        run = 0


def dense(sp, size):
    v = np.zeros(size, np.float32)
    if len(sp.idx):
        v[sp.idx] = sp.val
    return v


def lab_scores(head, r, vox, window=25):
    """Stricter scores (independent review): shapes are centred (the mean word shape, the
    always-on background, is removed), cue results are own-minus-best margins against every other
    known word, after-windows have one fixed length, and a 4D cell counts as carrying activity only
    when at least 10% of its voxels fire. Prints a table and one LABSCORE json line."""
    import json
    if vox is None:
        return None
    n = len(r)
    heard = r['heard']
    flags = r['flags']
    sound = (flags & 8) != 0
    learning = (flags & 16) != 0
    size = head['cells'][2] * head['C'][2]
    x = [Sparse.of(v, head['C'][2]) for v in vox]
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
    taught = [e for e in events if e['learn']]
    cues = [e for e in events if not e['learn']]
    shapes = {}
    for e in taught:
        shapes.setdefault(e['word'], []).append(Sparse.total(x[e['start']:min(n, e['end'] + 3)]))
    words = list(shapes)
    if not words:
        return None
    mean = {w: dense(Sparse.total(s[-3:]), size) for w, s in shapes.items()}
    background = np.mean([mean[w] for w in words], axis=0)
    centred = {w: mean[w] - background for w in words}
    successor = {}
    for a, b2 in zip(taught, taught[1:]):
        if b2['start'] - a['end'] <= 3:  # heard right after, in the same phrase
            successor.setdefault(a['word'], []).append(b2['word'])

    def ccos(a, b2):
        na, nb = np.linalg.norm(a), np.linalg.norm(b2)
        return float(a @ b2 / (na * nb)) if na > 0 and nb > 0 else 0.0

    def after(e):
        a0 = min(n, e['end'] + 3)
        a1 = min(n, a0 + window)
        full = e['next'] >= a1  # nothing heard inside the window
        return dense(Sparse.total(x[a0:a1]), size) - background, full

    rows = []
    # Every question asked (a cue's last word, followed by silence): how alike the silence after it is to
    # every known word, so a puzzle can score any expected answer.
    asks = []
    for k, e in enumerate(cues):
        nxt = cues[k + 1]['start'] if k + 1 < len(cues) else n
        if nxt - e['end'] < 20:
            continue  # more of the same cue follows
        aft, _ = after(e)
        sims = {w: round(ccos(aft, centred[w]), 4) for w in words if w != e['word']}
        # The line it says by itself: the silence cut into short word-length windows, the best known word in each.
        steps = []
        for w0 in range(min(n, e['end'] + 1), min(n, e['end'] + 1 + 8 * 6, nxt), 6):
            win = dense(Sparse.total(x[w0:min(n, w0 + 6, nxt)]), size) - background
            ss = {w: ccos(win, centred[w]) for w in words if w != e['word']}
            steps.append({w: round(v, 4) for w, v in sorted(ss.items(), key=lambda kv: -kv[1])[:6]})
        # Surprise: how much the brain's pattern turned when this word came, against what it held in the
        # silence just before it (the previous ask, if it ended right before); and how much it stays busy
        # afterwards (raw activity in 10-tick slices), which may be confusion or the work of making sense.
        turn = None
        if asks and e['start'] - asks[-1]['_cend'] <= 40:
            turn = round(1.0 - ccos(aft, asks[-1]['_vec']), 4)
        busy = [round(float(np.linalg.norm(dense(Sparse.total(x[t0:min(n, t0 + 10, nxt)]), size))), 3)
                for t0 in range(e['end'], min(n, e['end'] + 80, nxt), 10)]
        asks.append(dict(last=e['word'], tick=int(r['tick'][e['start']]), sims=dict(sorted(sims.items(), key=lambda kv: -kv[1])),
                         steps=steps, turn=turn, busy=busy, jolt=round(float(r['mod'][e['start']:min(n, e['end'] + 3)].mean()), 4), _vec=aft, _cend=e['end']))
    print(f'  lab scores (centred shapes, after-window {window} ticks, margin = target minus best other known word):')
    for e in cues:
        cw = e['word']
        if cw in successor:
            target = max(set(successor[cw]), key=successor[cw].count)
        else:
            cand = [w for w in words if w.startswith(cw) and w != cw]
            target = cand[0] if len(cand) == 1 else None
        if target is None:
            continue
        aft, full = after(e)
        sims = {w: ccos(aft, centred[w]) for w in words if w != cw}
        best_other = max((s for w, s in sims.items() if w != target), default=0.0)
        best_word = max(sims, key=sims.get)
        top = sorted(sims.items(), key=lambda kv: -kv[1])[:8]
        rows.append(dict(cue=cw, target=target, own=round(sims.get(target, 0.0), 3), margin=round(sims.get(target, 0.0) - best_other, 3),
                         top=[[w, round(s, 3)] for w, s in top], tick=int(r['tick'][e['start']]),
                         best=best_word, full_window=full))
        print(f"    cue {cw:10s} -> target {target:8s} own {sims.get(target, 0.0):+.3f}  margin {sims.get(target, 0.0) - best_other:+.3f}  (most like {best_word}){'' if full else '  [window cut]'}")
    # Afterglow per hearing, equal windows only.
    glow = {}
    for e in taught:
        a0 = min(n, e['end'] + 3)
        a1 = a0 + window
        if e['next'] >= a1 and a1 <= n:
            glow.setdefault(e['word'], []).append(round(float(r['a3'][a0:a1].mean()), 2))
    # Strict 4D lighting: at least 10% of a 4D cell's voxels firing.
    G = head['G']
    per = head['cells'][2] // G
    strong = r['g']['a3'] >= max(1, per // 10)
    per_slice = G // head['fields']
    strict = [int(strong[:, s * per_slice:(s + 1) * per_slice].any(axis=0).sum()) for s in range(head['fields'])]
    at_once = [round(float(strong[sound][:, s * per_slice:(s + 1) * per_slice].sum(axis=1).mean()), 2) if sound.any() else 0.0
               for s in range(head['fields'])]
    print(f'    4D cells carrying activity (>= 10% of voxels firing), per slice from the senses: ever {strict}, at once during sound {at_once}')
    for a in asks:
        a.pop('_vec', None)
        a.pop('_cend', None)
    score = dict(cues=rows, asks=asks, glow=glow, strict4d=strict, strict4d_at_once=at_once,
                 mt=int(r['mtc'][-1]) if head['version'] >= 3 else 0)
    print('LABSCORE ' + json.dumps(score))
    return score


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('file')
    ap.add_argument('--csv')
    ap.add_argument('--png')
    ap.add_argument('--story', action='store_true', help='observation lab: what every hearing did')
    args = ap.parse_args()
    head, r, vox = read(args.file)
    P, Q, V = head['cells']
    G, A = head['G'], head['A']
    n = len(r)
    if n == 0:
        sys.exit('no ticks recorded')
    flags = r['flags']
    sound = (flags & 8) != 0
    print(f"{args.file}: {n} ticks recorded (ticks {r['tick'][0]}-{r['tick'][-1]}, every {head['every']}), "
          f"{'Pharmakon 4D lattice' if head['lattice'] else 'Prime fields'}, {G} 4D cells of "
          f"{head['B']}x{head['B']}x{head['B']} voxels")
    print(f"  steps: 2D {np.mean((flags & 1) != 0) * 100:.0f}% of ticks, 3D {np.mean((flags & 2) != 0) * 100:.0f}%, "
          f"4D {np.mean((flags & 4) != 0) * 100:.0f}%; sound {np.mean(sound) * 100:.0f}% of ticks, "
          f"learning {np.mean((flags & 16) != 0) * 100:.0f}%")
    total = (P, Q, V)
    print('  level            active share: mean (sound / silence)   peak     mean activity per cell')
    for k, name in enumerate(LEVELS):
        a = r[f'a{k + 1}'] / total[k]
        s = r[f's{k + 1}'] / total[k]
        snd = a[sound].mean() * 100 if sound.any() else float('nan')
        sil = a[~sound].mean() * 100 if (~sound).any() else float('nan')
        print(f'  {name:16s} {a.mean() * 100:6.2f}% ({snd:5.2f}% / {sil:5.2f}%)          {a.max() * 100:6.2f}%  {s.mean():.4f}')
    g = r['g']
    per = (P // G, Q // G, V // G)
    print('  4D cells (each holds all levels inside it):')
    for k, name in enumerate(LEVELS):
        act = g[f'a{k + 1}'] > 0
        busy = act.mean(axis=0)
        at_once = act.sum(axis=1)
        print(f'    {name:16s} ever active {np.sum(busy > 0)}/{G}; active at once: mean {at_once.mean():.1f}, '
              f'max {at_once.max()}; share of ticks a cell is active: median {np.median(busy) * 100:.0f}%, '
              f'max {busy.max() * 100:.0f}%')
    load = (g['a3'] / per[2]).mean(axis=0)
    top = np.argsort(-load)[:8]
    print('    busiest 4D cells (x,y,z,w: mean share of their voxels active): ' +
          ', '.join(f'{cell_name(i, A)} {load[i] * 100:.1f}%' for i in top))
    if head['lattice']:
        d4 = g['d4']
        print(f"    4D-lattice drive per 4D cell: mean {d4.mean():.4f}, max {d4.max():.4f}")
    # The 4D surface by distance from the senses: the 4D cells of each w-slice (w = 0 holds the senses), how often they light up and how
    # much 4D-lattice signal reaches them (does the signal arrive but not fire, or not arrive?).
    per_slice = G // head['fields']
    act = g['a3'] > 0
    print('  4D surface by distance from the senses (w = 0 is where the senses plug in): 4D cells ever lit | lit at once (mean, sound) | 4D signal arriving per cell (sound / silence) | voxels firing per lit cell')
    for w in range(head['fields']):
        sl = slice(w * per_slice, (w + 1) * per_slice)
        a = act[:, sl]
        lit_once = a.sum(axis=1)
        sig = g['d4'][:, sl].mean(axis=1) if head['lattice'] else np.zeros(n)
        vfire = g["a3"][:, sl].sum(axis=1)
        snd = lit_once[sound].mean() if sound.any() else 0.0
        print(f"    w={w}: {int(np.sum(a.any(axis=0))):3d}/{per_slice} | {snd:6.1f} | {sig[sound].mean() if sound.any() else 0:.4f} / {sig[~sound].mean() if (~sound).any() else 0:.4f}"
              f" | {(vfire[lit_once > 0] / lit_once[lit_once > 0]).mean() if np.any(lit_once > 0) else 0:.2f}")
    if head['version'] >= 3 and r['mtc'][-1] > 0:
        mtc = np.diff(r['mtc'].astype(np.int64), prepend=r['mtc'][0])
        print(f"  microtubules: {r['mtc'][-1] - r['mtc'][0]} objective reductions; per tick mean {mtc.mean():.1f} "
              f"(sound {mtc[sound].mean() if sound.any() else 0:.1f} / silence {mtc[~sound].mean() if (~sound).any() else 0:.1f}), "
              f"ticks with any {np.mean(mtc > 0) * 100:.0f}%")
    if head['version'] >= 2:
        st = r['stems'].astype(float)
        on = st > 0
        if on.any():
            v3 = r['a3'][on] / st[on]
            v2 = r['r2'][on] / np.maximum(r['a3'][on], 1)
            v1 = r['r1'][on] / np.maximum(r['r2'][on], 1)
            print('  roots of an idea (ticks with any active 4D cell):')
            print(f'    stems: {st[on].mean():.1f} active 4D cells; each holds {v3.mean():.1f} active voxels')
            print(f'    each active voxel holds {v2.mean():.2f} active sheet cells (of {head["SS"]}); '
                  f'each of those holds {v1.mean():.2f} active line cells (of {head["L"]})')
            print(f'    whole root per stem: {v3.mean():.1f} voxels -> {(r["r2"][on] / st[on]).mean():.1f} sheet cells '
                  f'-> {(r["r1"][on] / st[on]).mean():.1f} line cells')
            print(f'    cut roots: {r["o2"].mean():.0f} active sheet cells in silent voxels, '
                  f'{r["o1"].mean():.0f} active line cells in silent sheet cells (per tick)')
    if vox is not None:
        counts = np.array([len(x) for x in vox])
        print(f'  voxel record: {counts.mean():.0f} voxels with activity per tick (max {counts.max()})')
    coherence(head, r, vox)
    if args.story:
        story(head, r, vox)
        lab_scores(head, r, vox)
    if args.csv:
        with open(args.csv, 'w') as f:
            f.write('tick,sound,heard,step2d,step3d,step4d,modulator,active1d,sum1d,active2d,sum2d,active3d,sum3d,cells4d_active\n')
            cells_active = (g['a3'] > 0).sum(axis=1)
            for i in range(n):
                fl = int(r['flags'][i])
                ch = chr(r['heard'][i]) if r['heard'][i] else ''
                ch = {'': '', ' ': 'space', ',': 'comma'}.get(ch, ch)
                f.write(f"{r['tick'][i]},{fl >> 3 & 1},{ch},{fl & 1},{fl >> 1 & 1},{fl >> 2 & 1},{r['mod'][i]:.4f},"
                        f"{r['a1'][i]},{r['s1'][i]:.4f},{r['a2'][i]},{r['s2'][i]:.4f},{r['a3'][i]},{r['s3'][i]:.4f},"
                        f"{cells_active[i]}\n")
        print('  wrote ' + args.csv)
    if args.png:
        try:
            import matplotlib
        except ImportError:
            print('  (no picture: matplotlib is not installed)')
            return
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        t = r['tick']
        fig, ax = plt.subplots(5, 1, figsize=(14, 13), sharex=True,
                               gridspec_kw={'height_ratios': [1, 1, 1, 0.5, 3]})
        for k, name in enumerate(LEVELS):
            ax[k].plot(t, r[f'a{k + 1}'] / total[k] * 100, lw=0.6)
            ax[k].set_ylabel(name + '\n% active')
        ax[3].plot(t, (g['a3'] > 0).sum(axis=1), lw=0.6, color='tab:green')
        ax[3].set_ylabel('4D cells\nactive')
        ax[4].imshow((g['a3'] / per[2]).T, aspect='auto', interpolation='nearest', origin='lower',
                     extent=[t[0], t[-1], 0, G], cmap='magma')
        ax[4].set_ylabel('4D cell\n(voxels active)')
        ax[4].set_xlabel('tick')
        for a in ax[:3]:
            a.fill_between(t, 0, 1, where=sound, color='0.85', transform=a.get_xaxis_transform(), lw=0)
        fig.tight_layout()
        fig.savefig(args.png, dpi=110)
        print('  wrote ' + args.png)


if __name__ == '__main__':
    main()
