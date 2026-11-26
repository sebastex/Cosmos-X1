"""Leaderboard for the improvement loop: every version (ledger name) against the product scorecard, averaged over
its brains. Downloads finished runs into console/ab/<run> (cached). Columns:
  r10 r30 r100 = words said back right (mean over brains), wrong = wrong words said (rep100),
  rd = read whole word from its letters (read30, top), end = read from filler + the rest (the word's end),
  hear = heard and said back in the read test, mem = whole-word recall in the memory test (mem100).
    python tools/ab/board.py            (all)      python tools/ab/board.py --md   (markdown table)
"""
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ab  # noqa: E402

ROOT = ab.ROOT


def status(rid):
    r = subprocess.run(['gh', 'run', 'view', rid, '--json', 'status,conclusion'], capture_output=True, text=True, cwd=ROOT)
    try:
        return json.loads(r.stdout)
    except Exception:
        return {'status': 'unknown', 'conclusion': ''}


def main(md=False):
    entries = [json.loads(l) for l in open(ab.LEDGER) if l.strip()]
    board = {}
    pending = 0
    for e in entries:
        d = os.path.join(ROOT, 'console', 'ab', e['run'])
        done_flag = os.path.join(d, '.done')
        if not os.path.exists(done_flag):
            st = status(e['run'])
            if st['status'] != 'completed':
                pending += 1
                continue
            subprocess.run(['gh', 'run', 'download', e['run'], '-D', d], capture_output=True, cwd=ROOT)
            os.makedirs(d, exist_ok=True)
            open(done_flag, 'w').write(st.get('conclusion') or '')
        row = board.setdefault(e['name'], {})
        if e['kind'] == 'mem':
            for root, _, files in os.walk(d):
                for f in files:
                    if f.startswith('quick-') and f.endswith('.json'):
                        j = json.load(open(os.path.join(root, f)))
                        row.setdefault('mem', []).append(j.get('recall_whole'))
            continue
        for seed in e['seeds'].split(','):
            p = os.path.join(d, f'result-seed-{seed}', f'result_{seed}.txt')
            if not os.path.exists(p):
                continue
            s = ab.score_word(e['test'], p)
            if not s:
                continue
            t = e['test'].rstrip('s') if e['test'].endswith('s') and not e['test'].startswith('jung') else e['test']
            if t.startswith('rep'):
                row.setdefault('r' + t[3:], []).append(s['said'])
                if t == 'rep100':
                    row.setdefault('wrong', []).append(s['wrong'])
            elif t.startswith('read'):
                k = '' if t == 'read30' else t[4:]
                row.setdefault('rd' + k, []).append(s['whole'])
                row.setdefault('end' + k, []).append(s['rest'])
                row.setdefault('hear' + k, []).append(s['hear'])
    cols = ['r10', 'r30', 'r100', 'wrong', 'rd10', 'end10', 'rd', 'end', 'hear', 'mem']
    mean = lambda v: sum(v) / len(v) if v else None
    lines = []
    if md:
        lines.append('| version | ' + ' | '.join(cols) + ' |')
        lines.append('|' + '---|' * (len(cols) + 1))
    for name in sorted(board, key=lambda n: -(mean(board[n].get('r100', [])) or -1)):
        vals = []
        for c in cols:
            v = board[name].get(c, [])
            m = mean([x for x in v if x is not None])
            vals.append('' if m is None else (f'{m:.2f}' if c == 'mem' else f'{m:.1f}') + (f'({len(v)})' if len(v) and c != 'mem' and len(v) < 3 else ''))
        lines.append(('| ' + name + ' | ' + ' | '.join(vals) + ' |') if md else f'{name:16} ' + ' '.join(f'{c}={v:9}' for c, v in zip(cols, vals) if v))
    print('\n'.join(lines))
    print(f'pending runs: {pending}')


if __name__ == '__main__':
    main('--md' in sys.argv)
