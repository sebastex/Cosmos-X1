"""Write the arena v2 puzzles into tools/arena/puzzles.json (round-1 puzzles are kept as they are).

    python tools/arena/make_puzzles.py

Every new puzzle follows the same order a child would: each word is first learned on its own,
then the sentences that relate them (words are never paired at random), then questions.
A question is a cue followed by silence; the silence is compared with the answer and the other
choices (score.py, kind "ask"). Sentences are taught in rotating order so no one is always last.
"""
import json
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ROUND1 = ['complete', 'speak', 'sentence', 'choice', 'keep', 'sleep']
# What each puzzle stimulates, and what a low score says the brain is missing.
PROBES = {
    'complete': ('word memory', 'a word does not settle into its own lasting shape'),
    'complete50': ('room for many words', 'word shapes overlap and crowd each other out'),
    'speak': ('speech', 'it cannot turn a held shape back into the right sounds'),
    'sentence': ('order in time', 'it does not link one word to the next'),
    'choice': ('real choice', 'it cannot settle on one of two equal answers'),
    'keep': ('lasting memory', 'new learning writes over old learning'),
    'sleep': ('sleep that helps', 'sleep does not clean up or replay what matters'),
    'sleep50': ('sleep that helps (many words)', 'sleep does not untangle crowded words'),
    'next': ('order (first step)', 'it does not hold which item follows which'),
    'pattern': ('context, two back', 'it only knows the last word, not what came before it'),
    'category': ('grouping (what fits where)', 'it does not tie many things to one group word'),
    'analogy': ('relations held in sentences', 'it loses the subject by the time the answer is due'),
    'facts': ('facts', 'it does not tie a thing to its property'),
    'yesno': ('true or false', 'it cannot use the whole question to pick yes or no'),
    'virtue': ('good and bad (first step of virtue)', 'it does not tie acts to good or bad; next it needs a sense of approval'),
    'truefalse': ('understanding: noticing a wrong statement', 'a wrong ending does not surprise it more than a true one'),
    'truefalse_t': ('understanding, after a teacher said right and wrong', 'being told right and wrong does not change what surprises it'),
    'orchard': ('understanding: what grows where', 'it repeats what it heard but cannot work out where a fruit grows'),
}


def orchard_world():
    """What the orchard teaches: fruits taught straight, fruits known only by their kind, and the sentences."""
    direct = {'apples': 'trees', 'pears': 'trees', 'cherries': 'trees',
              'raspberries': 'bushes', 'blueberries': 'bushes', 'gooseberries': 'bushes',
              'grapes': 'vines', 'kiwis': 'vines', 'melons': 'vines'}
    kinds = {'peaches': ('stone fruits', 'trees'), 'apricots': ('stone fruits', 'trees'),
             'blackberries': ('berries', 'bushes'), 'currants': ('berries', 'bushes'),
             'pumpkins': ('gourds', 'vines'), 'cucumbers': ('gourds', 'vines')}
    sents = [f'{f} grow on {w}' for f, w in direct.items()]
    sents += ['stone fruits grow on trees', 'berries grow on bushes', 'gourds grow on vines']
    sents += [f'{f} are {k}' for f, (k, _) in kinds.items()]
    sents += ['milk comes from cows', 'eggs come from hens', 'honey comes from bees']  # what 'come from' means
    return direct, kinds, sents


def orchard():
    """The orchard: which fruit grows on a tree, a bush or a vine. Three levels of understanding:
    remember (taught directly), work it out (only 'X are berries' and 'berries grow on bushes' are
    taught), and same meaning in new words (asked 'come from' after learning 'grow on')."""
    direct, kinds, sents = orchard_world()
    where = ['trees', 'bushes', 'vines']
    items = [dict(level='remember', q=f'{f} grow on', a=w, choices=where) for f, w in direct.items()]
    items += [dict(level='work it out', q=f'{f} grow on', a=w, choices=where) for f, (_, w) in kinds.items()]
    items += [dict(level='new words', q=f'{f} come from', a=w, choices=where) for f, w in list(direct.items())[::2]]
    # Origin: hear only the fruit, nothing else; is its place the closest of the three?
    items += [dict(level='origin', q=f, a=w, choices=where) for f, w in direct.items()]
    # Turned around: hear only the place; which fruit belongs there? (only 'fruit grow on place' was taught)
    fr = list(direct)
    for k in range(3):
        row = [fr[k], fr[3 + k], fr[6 + k]]  # one tree, one bush and one vine fruit
        items += [dict(level='turned around', q=direct[f], a=f, choices=row) for f in row]
    random.Random(9).shuffle(items)
    vocab = sorted({w for t in sents for w in t.split()})
    qops = []
    for it in items:
        qops += [f"cue:{'_'.join(it['q'].split())}:80", 'rest:150']
    script = words_alone(vocab) + sentences(sents, 4) + ['rest:300'] + qops[:-1]
    return dict(id='orchard', name='The orchard', kind='quiz', twin=True, items=items,
                tests='Which fruit grows where (tree, bush or vine). Origin: hear only "apples", is "trees" closest. Turned around: hear only "bushes", which fruit. Remember: taught directly. Work it out: only "blackberries are berries" and "berries grow on bushes" were taught. New words: asked "apples come from" after learning "grow on".',
                script=';'.join(script))
def truefalse(taught=False):
    """True or false: after the orchard lessons it hears statements, half of them wrong
    ("apples grow on bushes"). It is never told which. Its answer is its own surprise: how far its
    pattern turns when the last word comes, against what it held a moment before. Above its own
    middle surprise = it says false. How long it stays busy afterwards is kept too (confusion,
    or the work of making sense)."""
    direct, kinds, sents = orchard_world()
    where = ['trees', 'bushes', 'vines']
    stm = []
    for k, (f, w) in enumerate(list(direct.items()) + [(f, w) for f, (_, w) in kinds.items()]):
        level = 'remember' if f in direct else 'work it out'
        wrong = [x for x in where if x != w][k % 2]
        stm += [dict(level=level, fruit=f, said=w, truth=True), dict(level=level, fruit=f, said=wrong, truth=False)]
    random.Random(11).shuffle(stm)
    vocab = sorted({w for t in sents for w in t.split()})
    ops = []
    for st in stm:
        ops += [f"cue:{st['fruit']}_grow_on:32", f"cue:{st['said']}:90", 'rest:120']
    script = words_alone(vocab) + sentences(sents, 4) + ['rest:300'] + ops[:-1]
    if taught:  # a teacher: whole statements, true and false, each followed by good or bad (no other learning)
        lesson = []
        direct_st = [(f, w) for f, w in direct.items()]
        for r in range(3):
            row = []
            for k, (f, w) in enumerate(direct_st):
                wrong = [x for x in where if x != w][(k + r) % 2]
                row += [(f, w, 'good'), (f, wrong, 'bad')]
            random.Random(20 + r).shuffle(row)
            for f, w, sig in row:
                lesson += [f"cue:{f}_grow_on_{w}:20", sig, 'rest:100']
        script = words_alone(vocab) + sentences(sents, 4) + ['rest:300'] + lesson + ['rest:300'] + ops[:-1]
        return dict(id='truefalse_t', name='True or false, after a teacher said right and wrong', kind='judge', twin=True,
                    statements=stm, tests='As truefalse, but first a teacher: the 9 taught fruits, each said once rightly (good) and once wrongly (bad), three rounds. The work-it-out fruits never get a teacher.',
                    script=';'.join(script))
    return dict(id='truefalse', name='True or false (surprise)', kind='judge', twin=True, statements=stm,
                tests='After the orchard lessons: 30 statements, half wrong. Does a wrong ending surprise it more than the true one?',
                script=';'.join(script))


# Taught harder: the same questions after better teaching (short steps first, pairs that differ by one
# word, more rounds). If a score rises it was a lesson; if it stays flat the brain is missing a part.
for _k in ('next', 'analogy', 'yesno', 'virtue'):
    PROBES[_k + '_t'] = (PROBES[_k][0] + ', taught harder', PROBES[_k][1] + ' (even with better teaching)')


def words_alone(words, reps=6, gap=40):
    return [f'say:{w}:{reps}:{gap}' for w in words]


def sentences(sents, rounds=4, reps=3, gap=60):
    ops = []
    for r in range(rounds):
        k = r % len(sents)
        for s in sents[k:] + sents[:k]:
            ops.append(f"seq:{','.join(s.split())}:{reps}:{gap}")
    return ops


def questions(qs, seed):
    qs = list(qs)
    random.Random(seed).shuffle(qs)
    ops = ['rest:300']
    for q, _ in qs:
        ops += [f"cue:{'_'.join(q.split())}:80", 'rest:150']
    return ops[:-1], [dict(q=q, a=a) for q, a in qs]


def ask_puzzle(pid, name, tests, sents, qs, choices, seed, rounds=4):
    vocab = sorted({w for s in sents for w in s.split()})
    qops, asks = questions(qs, seed)
    script = words_alone(vocab) + sentences(sents, rounds) + qops
    return dict(id=pid, name=name, tests=tests, kind='ask', twin=True, choices=choices, asks=asks,
                script=';'.join(script))


def word_puzzles():
    lib = json.load(open(os.path.join(ROOT, 'data', 'curriculum', 'curriculum.json')))
    pool = [w for w in lib['levels'][0]['items'] if 4 <= len(w) <= 7]
    random.Random(50).shuffle(pool)
    chosen = []
    for w in pool:  # 50 words, none a prefix-twin of a cue
        if len(chosen) < 50 and w not in chosen:
            chosen.append(w)
    cues = []
    for w in chosen:
        if len(w) >= 5 and len(cues) < 10:
            pre = w[:3]
            if sum(1 for v in chosen if v.startswith(pre)) == 1 and pre not in chosen:
                cues.append(pre)
    teach = words_alone(chosen, reps=8)
    cue_ops = [f'cue:{c}:60' for c in cues]
    return [
        dict(id='complete50', name='Finish the word (50)', kind='complete', twin=True,
             tests='50 words, each learned on its own; 10 of them cued by their first 3 letters. Does the silence bring back the right word, better than the untrained twin?',
             script=';'.join(teach + ['rest:300'] + cue_ops)),
        dict(id='sleep50', name='Sleep on 50 words', kind='complete', twin=True,
             tests='The same 50 words, then sleep (replay and unlearning), then the same 10 cues.',
             script=';'.join(teach + ['sleep:800'] + cue_ops)),
    ]


def main():
    path = os.path.join(HERE, 'puzzles.json')
    old = json.load(open(path))
    keep = [p for p in old['puzzles'] if p['id'] in ROUND1]
    new = word_puzzles()
    # IQ-style: what comes next, which pattern continues, what fits where, analogies.
    new.append(ask_puzzle(
        'next', 'What comes next', 'Count from one to seven a few times; hear one number, does the silence bring the next one?',
        ['one two three four five six seven'],
        [('two', 'three'), ('four', 'five'), ('five', 'six'), ('six', 'seven')],
        ['one', 'two', 'three', 'four', 'five', 'six', 'seven'], 1, rounds=5))
    new.append(ask_puzzle(
        'pattern', 'Continue the pattern',
        'Hear red blue green and pink blue white. After "blue" the answer depends on what came before it, like a pattern test.',
        ['red blue green', 'pink blue white'],
        [('red blue', 'green'), ('pink blue', 'white'), ('pink blue', 'white'), ('red blue', 'green')],
        ['green', 'white'], 2, rounds=6))
    new.append(ask_puzzle(
        'category', 'What fits where', 'Learn which group each thing belongs to (cats are animals, apples are fruit...); asked "cats are", does it pick the right group?',
        ['cats are animals', 'dogs are animals', 'apples are fruit', 'pears are fruit', 'cars are vehicles', 'buses are vehicles'],
        [('cats are', 'animals'), ('apples are', 'fruit'), ('buses are', 'vehicles'), ('dogs are', 'animals'), ('pears are', 'fruit'), ('cars are', 'vehicles')],
        ['animals', 'fruit', 'vehicles'], 3))
    new.append(ask_puzzle(
        'analogy', 'Mother and baby', 'Learn the cat has a kitten, the dog has a puppy...; asked "the cow has a", does it bring calf (an analogy held in sentences)?',
        ['the cat has a kitten', 'the dog has a puppy', 'the cow has a calf', 'the hen has a chick'],
        [('the cat has a', 'kitten'), ('the dog has a', 'puppy'), ('the cow has a', 'calf'), ('the hen has a', 'chick')],
        ['kitten', 'puppy', 'calf', 'chick'], 4))
    # Facts and questions.
    new.append(ask_puzzle(
        'facts', 'Know the facts', 'Learn six facts (fire is hot, ice is cold...); asked "fire is", does it bring the right word?',
        ['fire is hot', 'ice is cold', 'snow is white', 'grass is green', 'sky is blue', 'sugar is sweet'],
        [('fire is', 'hot'), ('ice is', 'cold'), ('snow is', 'white'), ('grass is', 'green'), ('sky is', 'blue'), ('sugar is', 'sweet')],
        ['hot', 'cold', 'white', 'green', 'blue', 'sweet'], 5))
    new.append(ask_puzzle(
        'yesno', 'Yes or no', 'Learn questions with answers (is fire hot yes, is ice hot no...); asked one, does it answer yes or no correctly?',
        ['is fire hot yes', 'is ice hot no', 'is sun bright yes', 'is night bright no', 'is sugar sweet yes', 'is lemon sweet no'],
        [('is fire hot', 'yes'), ('is ice hot', 'no'), ('is sun bright', 'yes'), ('is night bright', 'no'), ('is sugar sweet', 'yes'), ('is lemon sweet', 'no')],
        ['yes', 'no'], 6))
    # Ethical virtue, first step: hearing which acts are good and which are bad.
    new.append(ask_puzzle(
        'virtue', 'Good or bad', 'Learn sharing is good, hitting is bad...; asked "sharing is", does it say good or bad correctly?',
        ['sharing is good', 'helping is good', 'caring is good', 'lying is bad', 'hitting is bad', 'stealing is bad'],
        [('sharing is', 'good'), ('lying is', 'bad'), ('helping is', 'good'), ('hitting is', 'bad'), ('caring is', 'good'), ('stealing is', 'bad')],
        ['good', 'bad'], 7))
    # (short steps taught first, then the full sentences, rounds of each)
    harder = {
        'next': (['one two', 'two three', 'three four', 'four five', 'five six', 'six seven'], ['one two three four five six seven'], 4, 6),
        'analogy': (['cat has kitten', 'dog has puppy', 'cow has calf', 'hen has chick'],
                    ['the cat has a kitten', 'the dog has a puppy', 'the cow has a calf', 'the hen has a chick'], 4, 6),
        'yesno': ([], ['is fire hot yes', 'is ice hot no', 'is sun bright yes', 'is night bright no', 'is sugar sweet yes', 'is lemon sweet no'], 0, 10),
        'virtue': ([], ['sharing is good', 'lying is bad', 'helping is good', 'hitting is bad', 'caring is good', 'stealing is bad'], 0, 10),
    }
    for p in list(new):
        if p['id'] in harder:
            short, full, r1, r2 = harder[p['id']]
            vocab = sorted({w for t in short + full for w in t.split()})
            qops = []
            for q in p['asks']:  # the same questions in the same order as the first version
                qops += [f"cue:{'_'.join(q['q'].split())}:80", 'rest:150']
            script = words_alone(vocab) + (sentences(short, r1) if short else []) + sentences(full, r2) + ['rest:300'] + qops[:-1]
            new.append(dict(id=p['id'] + '_t', name=p['name'] + ' (taught harder)', kind='ask', twin=True, choices=p['choices'],
                            asks=p['asks'], tests=p['tests'] + ' Taught harder: short steps first, pairs that differ by one word next to each other, more rounds.',
                            script=';'.join(script)))
    new.append(orchard())
    new.append(truefalse())
    new.append(truefalse(True))
    old['puzzles'] = keep + new
    for p in old['puzzles']:
        p['probes'], p['if_fails'] = PROBES[p['id']]
    json.dump(old, open(path, 'w'), indent=2)
    for p in new:
        print(p['id'], len(p['script'].split(';')), 'ops')


if __name__ == '__main__':
    main()
