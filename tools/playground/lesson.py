"""The playground's endless lesson plan and its quizzes (tools/playground/playground.yml runs it).

    python tools/playground/lesson.py lesson OP COUNT > lesson.txt   # COUNT script steps from global step OP
    python tools/playground/lesson.py quiz OP STEP > quiz.json        # a quiz on what steps < OP taught

The plan goes through the curriculum (data/curriculum/curriculum.json) level by level, again and
again (each new cycle in a new order = practice):
  words (each on its own) -> sentences -> facts -> questions -> stories -> virtues,
with every batch reviewed once in the next one and a sleep after every few batches. A word is always
learned on its own before any sentence uses it; relations come only from sentences.
The quiz asks about things already taught: words from their first 3 letters, and the last word of
taught sentences, chosen among the words that end other sentences of the same kind ("what fits here").
"""
import json
import os
import random
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LEVELS = ['words', 'sentences', 'facts', 'questions', 'stories', 'virtues']


def curriculum():
    lib = json.load(open(os.path.join(ROOT, 'data', 'curriculum', 'curriculum.json')))
    lv = {l['name']: l for l in lib['levels']}
    items = dict(words=list(lv['words']['items']), sentences=list(lv['sentences']['items']),
                 facts=list(lv['facts']['items']),
                 questions=[f"{q['q']} {q['a']}" for q in lv['questions']['items']],
                 stories=[' | '.join(st) for st in lv['stories']['items']])
    v = lv['virtues']
    items['virtues'] = (v['good'] + v['bad'] + [f"{s['situation']} | {s['action']} | {s['consequence']}" for s in v['situations']]
                        + [f"{j['action']} that is {j['judgement']}" for j in v['judged']])
    return items


def say(w, n):
    return f'say:{w}:{n}:40'


def seq(sentence, n):
    return f"seq:{','.join(sentence.split())}:{n}:60"


def plan(upto):
    """Script steps 0..upto-1 of the endless plan, each with what it taught: (step, level, item or None)."""
    items = curriculum()
    out, known, cycle = [], set(), 0
    while len(out) < upto:
        rng = random.Random(1000 + cycle)
        for level in LEVELS:
            pool = list(items[level])
            if cycle:
                rng.shuffle(pool)
            size = 12 if level == 'words' else 10 if level != 'stories' else 2
            batches = [pool[k:k + size] for k in range(0, len(pool), size)]
            prev = []
            for b, batch in enumerate(batches):
                if level == 'words':
                    for w in batch:
                        out.append((say(w, 6 if cycle == 0 else 3), level, w))
                        known.add(w)
                    for w in prev:
                        out.append((say(w, 2), level, None))
                else:
                    for item in batch:  # new words first, each on its own
                        for w in item.replace('|', ' ').split():
                            if w not in known:
                                out.append((say(w, 6), 'words', w))
                                known.add(w)
                    for r in range(3):  # rotating order, so no item is always last
                        k = r % len(batch)
                        for item in batch[k:] + batch[:k]:
                            for part in item.split(' | '):
                                out.append((seq(part, 1), level, part if r == 2 else None))
                    for item in prev:
                        for part in item.split(' | '):
                            out.append((seq(part, 1), level, None))
                if b % 3 == 2:
                    out.append(('sleep:600', 'sleep', None))
                prev = batch
                out.append(('rest:200', 'rest', None))
        cycle += 1
    return out[:upto]


def quiz(op, step):
    """Questions about what steps < op taught, as an arena puzzle (score.py kind 'quiz')."""
    taught = plan(op)
    words = sorted({it for _, lv, it in taught if lv == 'words' and it})
    sents = {}
    for _, lv, it in taught:
        if lv not in ('words', 'sleep', 'rest') and it and len(it.split()) >= 2:
            sents.setdefault(lv, set()).add(it)
    rng = random.Random(step)
    items = []
    cands = [w for w in words if len(w) >= 5 and sum(1 for v in words if v.startswith(w[:3])) == 1 and w[:3] not in words]
    for w in rng.sample(cands, min(8, len(cands))):
        items.append(dict(level='words', q=w[:3], a=w, choices=None))
    allsents = [s for ss in sents.values() for s in ss]
    for lv in LEVELS[1:]:
        pool = sorted(sents.get(lv, []))
        rng.shuffle(pool)
        got = 0
        for s in pool:
            ws = s.split()
            frame, ans = ws[-2], ws[-1]
            others = sorted({t.split()[-1] for t in allsents if t.split()[-2] == frame and t.split()[-1] != ans})
            if not others:
                continue
            choices = [ans] + rng.sample(others, min(5, len(others)))
            items.append(dict(level=lv, q=' '.join(ws[:-1]), a=ans, choices=sorted(choices)))
            got += 1
            if got == 6:
                break
    refresh = sorted({w for it in items for w in ([it['a']] + (it['choices'] or []))})
    script = [say(w, 2) for w in refresh] + ['rest:800']
    for it in items:
        script += [f"cue:{'_'.join(it['q'].split())}:{60 if it['level'] == 'words' else 80}", 'rest:150']
    return dict(id='quiz', name=f'Playground quiz, run {step}', kind='quiz', twin=True, items=items,
                taught=dict(words=len(words), **{lv: len(v) for lv, v in sents.items()}), script=';'.join(script))


def main():
    what = sys.argv[1]
    if what == 'lesson':
        op, count = int(sys.argv[2]), int(sys.argv[3])
        steps = plan(op + count)[op:]
        print(';'.join(s for s, _, _ in steps))
    elif what == 'quiz':
        print(json.dumps(dict(puzzles=[quiz(int(sys.argv[2]), int(sys.argv[3]))])))
    elif what == 'where':  # what the plan is doing at a step
        op = int(sys.argv[2])
        s = plan(op + 1)[op]
        print(json.dumps(dict(step=op, does=s[0], level=s[1])))


if __name__ == '__main__':
    main()
