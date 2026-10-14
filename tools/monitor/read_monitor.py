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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('file')
    ap.add_argument('--csv')
    ap.add_argument('--png')
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
    if vox is not None:
        counts = np.array([len(x) for x in vox])
        print(f'  voxel record: {counts.mean():.0f} voxels with activity per tick (max {counts.max()})')
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
