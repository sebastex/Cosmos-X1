"""Cycles: a grown brain (the ladder's 1000 words) learns 100 new words at a time (tools/cycle/lists.txt,
10 lists), while evolution tunes its settings on each list (tools/cycle/evo.py).

    python tools/cycle/cycle.py lesson LIST          the lesson for list LIST (1..10): each word alone, 8 times
    python tools/cycle/cycle.py test LIST            the test after list LIST, as an arena puzzle (json)
    python tools/cycle/cycle.py fitness score.json test.json   new-word recall, old-word recall, fitness (json)

The test shows (learning frozen) the list's 100 words and 100 words learned before (the ladder's and
earlier lists'), then cues every new word that a beginning of 3+ letters names among everything the
brain knows, and 50 old ones the same way. An answer is right when the cued word wins against all
200 shown. Fitness = half new-word recall + half old-word recall (learning without forgetting).
"""
import json
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
LADDER = os.path.join(HERE, '..', 'ladder')
sys.path.insert(0, LADDER)
import ladder  # noqa: E402

LISTS = 10
PER = 100


def lists():
    ws = [w.strip() for w in open(os.path.join(HERE, 'lists.txt')) if w.strip()]
    return [ws[i * PER:(i + 1) * PER] for i in range(LISTS)]


def known_before(k):
    """Every word learned before list k: the ladder's 1000, then lists 1..k-1."""
    out = list(ladder.words())
    for ws in lists()[:k - 1]:
        out += ws
    return out


def lesson(k):
    ops = [f'say:{w}:8:40' for w in lists()[k - 1]] + ['rest:300', f'report:list{k}']
    return ';'.join(ops)


def test(k):
    new = lists()[k - 1]
    old = known_before(k)
    pre = ladder.prefixes(old + new)  # a cue must name one word among everything known
    rng = random.Random(7000 + k)
    old_shown = rng.sample(old, PER)
    old_cueable = sorted(w for w in old_shown if pre[w])
    old_targets = rng.sample(old_cueable, min(50, len(old_cueable)))
    new_targets = sorted(w for w in new if pre[w])
    rng.shuffle(new_targets)
    shown = new + old_shown
    rng.shuffle(shown)
    targets = new_targets + old_targets
    rng.shuffle(targets)
    script = ['still:1', f'report:test{k}'] + [f'say:{w}:2:40' for w in shown] + ['rest:800']
    for w in targets:
        script += [f'cue:{pre[w]}:60', 'rest:150']
    script.append('still:0')
    items = [dict(cue=pre[w], word=w, group='new' if w in set(new) else 'old') for w in targets]
    return dict(id=f'cycle{k}', name=f'Cycle list {k}', kind='complete', twin=False, items=items,
                shown=len(shown), script=';'.join(script))


def fitness(score_path, puzzle_path):
    score = json.load(open(score_path))
    items = json.load(open(puzzle_path))['puzzles'][0]['items']
    group = {it['cue']: it['group'] for it in items}
    learner = score.get('learner', {})
    n = {'new': 0, 'old': 0}
    right = {'new': 0, 'old': 0}
    for cue, m in learner.items():
        g = group.get(cue)
        if not g:
            continue
        n[g] += 1
        right[g] += m > 0
    rec = {g: right[g] / n[g] if n[g] else 0.0 for g in n}
    return dict(new=round(rec['new'], 4), old=round(rec['old'], 4), n_new=n['new'], n_old=n['old'],
                fitness=round(0.5 * rec['new'] + 0.5 * rec['old'], 4))


def main():
    a = sys.argv[1:]
    if a[:1] == ['lesson']:
        print(lesson(int(a[1])))
    elif a[:1] == ['test']:
        print(json.dumps(dict(puzzles=[test(int(a[1]))])))  # the arena's puzzle-file form
    elif a[:1] == ['fitness']:
        print(json.dumps(fitness(a[1], a[2])))
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
