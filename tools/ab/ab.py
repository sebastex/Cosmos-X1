"""A/B tests on GitHub (2026-11-26): each fix lives in its own branch behind a switch; this starts the same
word tests on any branch with any settings, on several brains at once (workflow tests.yml, one machine per
brain), keeps a ledger of what was started, and scores what has finished.

    python tools/ab/ab.py run REF NAME TEST [--seeds 3004,3005,3006] [--nobase] [-- --set k=v ...]
    python tools/ab/ab.py mem REF NAME [--words 100] [--seeds 3004,3005] [--nobase] [-- --set k=v ...]
    python tools/ab/ab.py score [WORD]       scores every finished run in the ledger (rows whose name has WORD)
    python tools/ab/ab.py script TEST        prints the script of a test

TEST: rep5 / rep30 / rep100 - words learnt as their own shapes (spread, shuffled practice), then each heard
      and said back; read30 - reading-aloud lessons (letters, then the shape), then each word heard, and
      read from its letters four ways: whole, first two + filler, filler + the rest, (and two never-learnt).
mem:  the quick memory test (tools/quick/quick.py via quick.yml): letters learnt as chains, recalled from cues.
BASE: the word settings every word test starts from (the best repeat version so far), unless --nobase.
"""
import json
import os
import random
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', '..')
sys.path.insert(0, os.path.join(HERE, '..', 'quick'))
import quick  # noqa: E402

LEDGER = os.path.join(ROOT, 'console', 'ab_ledger.jsonl')
BASE = ('--set word_shape=1 --set speak_soft=1 --set downward_gain=0.15 --set word_hold=16 --set word_say_min=0.05 '
        '--set word_say_margin=1.5 --set predict_surprise=1 --set presynaptic_bound=0.1')
NEW = ['lamp', 'river']
ROUNDS = 8


def first_order(words):
    w = list(words)
    random.Random(1).shuffle(w)
    return w


def lessons(words, op):
    out = ['lexicon:' + ','.join(quick.words(100) + NEW)]
    for r in range(1, ROUNDS + 1):
        p = list(words)
        random.Random(r).shuffle(p)  # a new order every pass, the same for every version
        out += [f'{op}:{w}:1:40' for w in p]
    return out


def script(test):
    if test.startswith('rep'):
        words = quick.words(100)[:int(test[3:])]
        t = ['still:1'] + [f'rest:150;repeat:{w}' for w in words + NEW]
        return ';'.join(lessons(words, 'say') + t)
    if test.startswith('read'):
        words = quick.words(100)[:int(test[4:])]
        t = ['still:1']
        for w in words:
            t += [f'rest:150;repeat:{w}:8:30', f'rest:150;repeat:{w}:8:30:read',
                  f'rest:150;repeat:{w[:2] + "q" * (len(w) - 2)}:8:30:read', f'rest:150;repeat:{"qq" + w[2:]}:8:30:read']
        for w in NEW:
            t += [f'rest:150;repeat:{w}:8:30', f'rest:150;repeat:{w}:8:30:read']
        return ';'.join(lessons(words, 'read') + t)
    if test.startswith('gauge'):  # gauges (no score): brain reports through the lesson, then drift while hearing
        words = quick.words(100)[:int(test[5:])]
        L = lessons(words, 'say')
        out, k = [L[0], 'drift:same:16', 'report:start'], 0
        for r in range(1, ROUNDS + 1):
            out += L[1 + (r - 1) * len(words):1 + r * len(words)]
            if r in (1, 4, 8):
                out.append(f'report:round{r}')
        return ';'.join(out + ['drift:same:16', 'drift:team:16', 'drift:trip:16'])
    raise SystemExit(f'unknown test {test}')


def gh(*args):
    r = subprocess.run(['gh', *args], capture_output=True, text=True, cwd=ROOT)
    if r.returncode:
        raise SystemExit(r.stderr.strip())
    return r.stdout


def record(entry):
    os.makedirs(os.path.dirname(LEDGER), exist_ok=True)
    with open(LEDGER, 'a') as f:
        f.write(json.dumps(entry) + '\n')
    print(json.dumps(entry))


def parse(argv):
    opts = {'seeds': '3004,3005,3006', 'words': '100', 'base': True}
    extra = []
    if '--' in argv:
        i = argv.index('--')
        argv, extra = argv[:i], argv[i + 1:]
    pos, k = [], 0
    while k < len(argv):
        a = argv[k]
        if a == '--nobase':
            opts['base'] = False
        elif a.startswith('--'):
            opts[a[2:]] = argv[k + 1]
            k += 1
        else:
            pos.append(a)
        k += 1
    return pos, opts, ' '.join(extra)


def run_id(out):
    m = re.search(r'/runs/(\d+)', out)
    return m.group(1) if m else None


def cmd_run(argv):
    (ref, name, test), o, extra = parse(argv)
    args = ' '.join(x for x in ['--test script --preset dev --variant prime', BASE if o['base'] else '', extra] if x)
    seeds = '[' + o['seeds'] + ']'
    out = gh('workflow', 'run', 'tests.yml', '--ref', ref, '-f', f'args={args}', '-f', f'seeds={seeds}',
             '-f', f'env=NCM_SCRIPT={script(test)}')
    record({'kind': 'word', 'ref': ref, 'name': name, 'test': test, 'args': args, 'seeds': o['seeds'], 'run': run_id(out)})


def cmd_mem(argv):
    (ref, name), o, extra = parse(argv)
    # the memory test learns letters, so the word settings are left out unless asked for
    args = extra
    runs = json.dumps([{'name': name, 'variant': 'prime', 'args': args}])
    for s in o['seeds'].split(','):
        out = gh('workflow', 'run', 'quick.yml', '--ref', ref, '-f', f'runs={runs}', '-f', f'words={o["words"]}', '-f', f'seed={s}')
        record({'kind': 'mem', 'ref': ref, 'name': name, 'test': f'mem{o["words"]}', 'args': args, 'seeds': s, 'run': run_id(out)})


def rows_of(path, key):
    out = []
    for line in open(path, errors='replace'):
        if line.startswith(key + ' '):
            out.append(json.loads(line.split(' ', 1)[1]))
    return out


def top(r):
    return r['plan'].split(' ')[0] if r['plan'] else ''


def score_word(test, path):
    if test.startswith('rep'):
        n = int(test[3:])
        words = quick.words(100)[:n]
        pos = {w: i for i, w in enumerate(first_order(words))}
        rows = rows_of(path, 'REPEAT')
        known = [r for r in rows if r['heard'] not in NEW]
        if len(known) < n:
            return None
        q = [0, 0, 0, 0]
        for r in known:
            if top(r) == r['heard']:
                q[min(3, pos[r['heard']] * 4 // n)] += 1
        return {'said': sum(r['said'] == r['heard'] for r in known), 'top': sum(top(r) == r['heard'] for r in known),
                'wrong': sum(bool(r['said']) and r['said'] != r['heard'] for r in known), 'quarters': q,
                'new_said': [r['said'] or '-' for r in rows if r['heard'] in NEW]}
    if test.startswith('gauge'):
        g = {}
        for r in rows_of(path, 'BRAINREPORT'):
            g[r['tag']] = {'fill': [round(f['fill'], 3) for f in r['fields']], 'full': [round(f['full'], 3) for f in r['fields']],
                           'theta': [t[0] for t in r.get('theta', [])], 'pre_room': r.get('pre_room'), 'scaled': round(r['scaled'], 1)}
        g['drift'] = [[d['word']] + [f[-1] for f in d['vs_tick4']] for d in rows_of(path, 'DRIFT')]
        return g or None
    n = int(test[4:])
    words = quick.words(100)[:n]
    hear = rows_of(path, 'REPEAT')
    read = rows_of(path, 'READ')
    if len(read) < 3 * n:
        return None
    s = {'hear': sum(r['said'] == r['heard'] for r in hear if r['heard'] in words)}
    for k, label in enumerate(['whole', 'first2', 'rest']):
        s[label] = sum(top(read[3 * i + k]) == w for i, w in enumerate(words))
    s['read_said'] = sum(read[3 * i]['said'] == w for i, w in enumerate(words))
    s['new_read'] = [r['said'] or '-' for r in read[3 * n:]]
    return s


def cmd_score(argv):
    want = argv[0] if argv else ''
    if not os.path.exists(LEDGER):
        raise SystemExit('no ledger yet')
    entries = [json.loads(l) for l in open(LEDGER) if l.strip()]
    for e in entries:
        if want and want not in e['name'] and want not in e['test']:
            continue
        rid = e['run']
        d = os.path.join(ROOT, 'console', 'ab', rid)
        st = json.loads(gh('run', 'view', rid, '--json', 'status,conclusion'))
        if st['status'] != 'completed':
            print(f"{e['test']:7} {e['name']:22} {e['ref']:22} run {rid}: {st['status']}")
            continue
        if not os.path.isdir(d):
            subprocess.run(['gh', 'run', 'download', rid, '-D', d], capture_output=True, cwd=ROOT)
        if e['kind'] == 'mem':
            vals = []
            for root, _, files in os.walk(d):
                for f in files:
                    if f.startswith('quick-') and f.endswith('.json'):
                        j = json.load(open(os.path.join(root, f)))
                        vals.append(f"whole {j.get('recall_whole')} begin {j.get('recall_beginning')}")
            print(f"{e['test']:7} {e['name']:22} {e['ref']:22} seed {e['seeds']}: {'; '.join(vals) or st['conclusion']}")
            continue
        for seed in e['seeds'].split(','):
            p = os.path.join(d, f'result-seed-{seed}', f'result_{seed}.txt')
            s = score_word(e['test'], p) if os.path.exists(p) else None
            print(f"{e['test']:7} {e['name']:22} {e['ref']:22} seed {seed}: {s if s else 'no result (' + st['conclusion'] + ')'}")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    c, rest = sys.argv[1], sys.argv[2:]
    if c == 'script':
        print(script(rest[0]))
    elif c == 'run':
        cmd_run(rest)
    elif c == 'mem':
        cmd_mem(rest)
    elif c == 'score':
        cmd_score(rest)
    else:
        raise SystemExit(__doc__)
