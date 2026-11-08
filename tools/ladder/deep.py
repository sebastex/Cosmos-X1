"""Why do the deep slices carry so little of each word? Reads a monitor recording in which every
word was learned, then shown twice more with learning frozen (rounds A and B, as made by the
script in the docstring of main), and measures for each w-slice of the 4D lattice and for several
time windows after a word:

  energy     share of all firing in this slice
  voxels     voxels firing (mean per word)
  own        share of the slice's firing that is the word's own (not the shape all words share)
  id         can the slice alone tell which word it heard? (round A matched to round B by likeness;
             chance = 1 / words)
  copy       how much the slice's map of which words look alike follows slice 0's map (1 = it only
             repeats what the senses' slice already holds, 0 = its own arrangement)

    python tools/ladder/deep.py recording.ncmmon words.txt
"""
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'monitor'))
import read_monitor as rm  # noqa: E402


def events(r):
    heard, flags = r['heard'], r['flags']
    sound = (flags & 8) != 0
    out, i, n = [], 0, len(r)
    while i < n:
        if sound[i] and heard[i] not in (0, 32):
            j, w = i, ''
            while j < n and sound[j] and heard[j] not in (0, 32):
                w += chr(heard[j])
                j += 1
            out.append(dict(word=w, start=i, end=j))
            i = j
        else:
            i += 1
    return out


def main():
    head, r, vox = rm.read(sys.argv[1])
    words = [w.strip() for w in open(sys.argv[2]) if w.strip()]
    C3 = head['C'][2]
    V = head['cells'][2]
    S = head['fields']
    size = V * C3
    per = size // S
    x = [rm.Sparse.of(v, C3) for v in vox]
    ev = events(r)
    nW = len(words)
    shown = ev[-4 * nW:]  # round A (2 hearings per word), then round B
    A, B = shown[:2 * nW], shown[2 * nW:]
    windows = [('word', 0, 3), ('+0..10', 3, 13), ('+10..25', 13, 28), ('+25..40', 28, 43)]
    n = len(r)

    def shape(evs, w, a, b):
        parts = []
        for e in evs:
            if e['word'] == w:
                s0 = e['start'] if a == 0 else e['end'] + a - 3
                s1 = min(n, e['end'] + b)
                parts += x[s0:s1]
        return rm.dense(rm.Sparse.total(parts), size)

    print(f'{nW} words, {S} slices, {V} voxels x {C3} channels')
    print('window    slice  energy  voxels  own    id     copy')
    for name, a, b in windows:
        MA = np.array([shape(A, w, a, b) for w in words])
        MB = np.array([shape(B, w, a, b) for w in words])
        tot = (MA ** 2).sum()
        sims0 = None
        for s in range(S):
            sa, sb = MA[:, s * per:(s + 1) * per], MB[:, s * per:(s + 1) * per]
            energy = (sa ** 2).sum() / max(tot, 1e-12)
            vox_on = (sa.reshape(nW, -1, C3).max(axis=2) > 0).sum(axis=1).mean()
            ca, cb = sa - sa.mean(axis=0), sb - sb.mean(axis=0)
            own = (ca ** 2).sum() / max((sa ** 2).sum(), 1e-12)
            na = ca / np.maximum(np.linalg.norm(ca, axis=1, keepdims=True), 1e-12)
            nb = cb / np.maximum(np.linalg.norm(cb, axis=1, keepdims=True), 1e-12)
            m = na @ nb.T
            ident = float((m.argmax(axis=1) == np.arange(nW)).mean())
            sims = na @ na.T
            iu = np.triu_indices(nW, 1)
            if s == 0:
                sims0 = sims[iu]
                copy = 1.0
            else:
                v1, v2 = sims0, sims[iu]
                copy = float(np.corrcoef(v1, v2)[0, 1]) if v1.std() > 0 and v2.std() > 0 else 0.0
            print(f'{name:9} {s:5}  {energy:.3f}   {vox_on:6.0f}  {own:.3f}  {ident:.2f}   {copy:+.2f}')
    print(f'(id chance = {1 / nW:.2f})')


if __name__ == '__main__':
    main()
