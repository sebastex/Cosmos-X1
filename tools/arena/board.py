"""Build the arena scoreboard data from GitHub arena runs.

    python tools/arena/board.py RUN_ID [RUN_ID ...] > board.json

Each run's arena-<variant> artifact (arena.json) is downloaded with gh and merged.
"""
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ORDER = ['alpha', 'beta', 'gamma', 'delta', 'epsilon', 'zeta', 'eta']


def main():
    puzzles = json.load(open(os.path.join(HERE, 'puzzles.json')))['puzzles']
    variants = {}
    for rid in sys.argv[1:]:
        with tempfile.TemporaryDirectory() as d:
            subprocess.run(['gh', 'run', 'download', rid, '-R', 'sebastex/Cosmos-X1', '-p', 'arena-*', '-D', d],
                           check=False, capture_output=True)
            for root, _, files in os.walk(d):
                for f in files:
                    if f == 'arena.json':
                        a = json.load(open(os.path.join(root, f)))
                        variants.setdefault(a['variant'], []).extend(a['scores'])
    board = dict(puzzles=[dict(id=p['id'], name=p['name'], tests=p['tests'], probes=p.get('probes', ''), if_fails=p.get('if_fails', ''), chance=round(1 / len(p['choices']), 3) if p.get('choices') else 0) for p in puzzles], variants=[])
    for v in sorted(variants, key=lambda x: ORDER.index(x) if x in ORDER else 99):
        rows = variants[v]
        per = {}
        for s in rows:
            per.setdefault(s['puzzle'], []).append(dict(seed=s['seed'], score=round(s['score'], 3),
                                                        detail={k: s[k] for k in s if k not in ('puzzle', 'name', 'seed', 'score')}))
        board['variants'].append(dict(name=v, puzzles=per))
    print(json.dumps(board))


if __name__ == '__main__':
    main()
