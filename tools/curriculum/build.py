"""Cosmos curriculum library: graded material written for Cosmos (lower-case words and spaces only).

    python tools/curriculum/build.py      # writes data/curriculum/*.txt and curriculum.json

Levels: words (themed), sentences (simple grammar), facts (categories, properties, causes),
questions with answers, short stories, and virtues (kindness, honesty, fairness, care; with the
good / bad judgement each teaches). Every line is one thing to say; words are never paired at random.
"""
import json
import os
import random

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, 'data', 'curriculum')

ANIMALS = {'cat': 'kitten', 'dog': 'puppy', 'cow': 'calf', 'duck': 'duckling', 'horse': 'foal', 'sheep': 'lamb',
           'goat': 'kid', 'bear': 'cub', 'lion': 'cub', 'pig': 'piglet', 'hen': 'chick', 'frog': 'tadpole'}
SOUNDS = {'cat': 'meows', 'dog': 'barks', 'cow': 'moos', 'duck': 'quacks', 'horse': 'neighs', 'sheep': 'bleats',
          'pig': 'oinks', 'hen': 'clucks', 'lion': 'roars', 'frog': 'croaks'}
CAN = {'fish': 'swim', 'bird': 'fly', 'dog': 'run', 'frog': 'jump', 'snake': 'slide', 'duck': 'swim', 'horse': 'run',
       'bee': 'fly', 'cat': 'climb', 'monkey': 'climb'}
CANNOT = {'fish': 'walk', 'dog': 'fly', 'cow': 'fly', 'snake': 'run', 'cat': 'fly', 'pig': 'fly', 'horse': 'fly'}
CATEGORY = {'animal': ['cat', 'dog', 'cow', 'horse', 'sheep', 'pig', 'goat', 'bear', 'lion', 'frog', 'duck', 'hen'],
            'fruit': ['apple', 'pear', 'plum', 'grape', 'lemon', 'peach', 'mango', 'cherry'],
            'color': ['red', 'blue', 'green', 'yellow', 'white', 'black', 'pink', 'brown'],
            'tool': ['hammer', 'saw', 'spade', 'drill', 'brush', 'knife'],
            'vehicle': ['car', 'bus', 'train', 'boat', 'truck', 'bike'],
            'drink': ['water', 'milk', 'juice', 'tea'],
            'place': ['house', 'school', 'park', 'farm', 'shop', 'beach']}
PROPERTY = [('fire', 'hot'), ('ice', 'cold'), ('snow', 'white'), ('grass', 'green'), ('sky', 'blue'), ('sun', 'bright'),
            ('night', 'dark'), ('stone', 'hard'), ('pillow', 'soft'), ('sugar', 'sweet'), ('lemon', 'sour'),
            ('water', 'wet'), ('sand', 'dry'), ('feather', 'light'), ('rock', 'heavy'), ('honey', 'sweet'),
            ('mouse', 'small'), ('whale', 'big'), ('turtle', 'slow'), ('cheetah', 'fast')]
CAUSE = [('rain', 'makes the grass wet'), ('fire', 'makes smoke'), ('sun', 'makes the day warm'),
         ('wind', 'moves the trees'), ('food', 'makes us strong'), ('sleep', 'makes us rested'),
         ('ice', 'makes the drink cold'), ('soap', 'makes hands clean'), ('a fall', 'can hurt'),
         ('a smile', 'makes friends happy'), ('a gift', 'makes people glad'), ('a lie', 'breaks trust')]
PEOPLE = ['mia', 'tom', 'sam', 'ana', 'leo', 'zoe']
THINGS = ['ball', 'book', 'apple', 'toy', 'hat', 'cup', 'kite', 'cake']
VERBS = ['sees', 'likes', 'has', 'finds', 'wants', 'holds']
ADJ = ['big', 'small', 'red', 'blue', 'new', 'old']
GROUPS = {  # more words to learn on their own (level 1), by theme
    'body': ['hand', 'foot', 'head', 'eye', 'ear', 'nose', 'mouth', 'arm', 'leg', 'hair'],
    'family': ['mom', 'dad', 'baby', 'sister', 'brother', 'friend', 'grandma', 'grandpa'],
    'food': ['bread', 'egg', 'rice', 'soup', 'cheese', 'cake', 'honey', 'jam'],
    'weather': ['rain', 'snow', 'wind', 'sun', 'cloud', 'storm', 'fog', 'ice'],
    'feeling': ['happy', 'sad', 'angry', 'scared', 'calm', 'tired', 'glad', 'proud'],
    'action': ['run', 'walk', 'jump', 'eat', 'drink', 'sleep', 'read', 'sing', 'play', 'help', 'share', 'give'],
    'number': ['one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine', 'ten'],
    'nature': ['tree', 'flower', 'river', 'hill', 'sea', 'rock', 'leaf', 'moon', 'star', 'sky'],
}
FEEL = [('a gift', 'happy'), ('a hug', 'happy'), ('a lost toy', 'sad'), ('a loud storm', 'scared'), ('a long day', 'tired'),
        ('a mean word', 'sad'), ('a kind word', 'glad'), ('a good job', 'proud')]
PLACES = {'cow': 'farm', 'fish': 'sea', 'bird': 'tree', 'bear': 'woods', 'child': 'school', 'car': 'road', 'boat': 'river'}


def sentences(rng):
    out = set()
    for a in ANIMALS:
        out.add(f'the {a} sees the {rng.choice(THINGS)}')
    for p in PEOPLE:
        for v in VERBS:
            out.add(f'{p} {v} the {rng.choice(ADJ)} {rng.choice(THINGS)}')
    for a, s in SOUNDS.items():
        out.add(f'the {a} {s}')
    for a, c in CAN.items():
        out.add(f'the {a} can {c}')
    for p in PEOPLE:
        for act in ['runs', 'walks', 'jumps', 'eats', 'sings', 'reads', 'plays', 'sleeps']:
            out.add(f'{p} {act}')
        for f in GROUPS['feeling']:
            out.add(f'{p} is {f}')
    for a, pl in PLACES.items():
        out.add(f'the {a} is in the {pl}')
    for w in GROUPS['weather']:
        out.add(f'the {w} is here')
    for a, b in zip(GROUPS['number'], GROUPS['number'][1:]):
        out.add(f'after {a} comes {b}')
    return sorted(out)


def facts():
    out = []
    for cat, members in CATEGORY.items():
        art = 'an' if cat[0] in 'aeiou' else 'a'
        for m in members:
            ma = 'an' if m[0] in 'aeiou' else 'a'
            out.append(f'{m} is a color' if cat == 'color' else f'{m} is a drink' if cat == 'drink' else f'{ma} {m} is {art} {cat}')
    for thing, prop in PROPERTY:
        out.append(f'{thing} is {prop}')
    for a, c in CAN.items():
        out.append(f'a {a} can {c}')
    for a, c in CANNOT.items():
        out.append(f'a {a} can not {c}')
    for a, b in ANIMALS.items():
        out.append(f'a baby {a} is a {b}')
    for c, e in CAUSE:
        out.append(f'{c} {e}')
    for c, f in FEEL:
        out.append(f'{c} makes you {f}')
    for a, pl in PLACES.items():
        out.append(f'a {a} lives in the {pl}' if a not in ('car', 'boat') else f'a {a} goes on the {pl}')
    return out


def questions():
    out = []
    for cat, members in CATEGORY.items():
        if cat == 'color':
            continue
        art = 'an' if cat[0] in 'aeiou' else 'a'
        for m in members:
            ma = 'an' if m[0] in 'aeiou' else 'a'
            out.append((f'what is {m}' if cat == 'drink' else f'what is {ma} {m}', f'{art} {cat}'))
    for thing, prop in PROPERTY:
        out.append((f'is {thing} {prop}', 'yes'))
    for a, c in CAN.items():
        out.append((f'can a {a} {c}', 'yes'))
    for a, c in CANNOT.items():
        out.append((f'can a {a} {c}', 'no'))
    for a, s in SOUNDS.items():
        out.append((f'what does a {a} say', s))
    for a, b in ANIMALS.items():
        out.append((f'what is a baby {a}', b))
    for c, f in FEEL:
        out.append((f'how does {c} make you feel', f))
    for a, b in zip(GROUPS['number'], GROUPS['number'][1:]):
        out.append((f'what comes after {a}', b))
    return out


def stories():
    return [
        ['mia has a red ball', 'the ball rolls away', 'tom finds the ball', 'tom gives the ball to mia', 'mia is happy'],
        ['sam is hungry', 'ana has two apples', 'ana gives sam one apple', 'sam says thank you', 'they are friends'],
        ['leo falls in the park', 'leo hurts his knee', 'zoe helps leo up', 'zoe brings water', 'leo feels better'],
        ['the cat is lost', 'the cat is cold and sad', 'mia finds the cat', 'mia takes the cat home', 'the cat is warm'],
        ['tom breaks a cup', 'tom tells the truth', 'mom is not angry', 'they fix the cup', 'tom feels good'],
        ['zoe has a new kite', 'sam has no kite', 'zoe shares her kite', 'they fly the kite together', 'both are happy'],
        ['the duck is in the pond', 'the duck can swim', 'the duck finds bread', 'the duck eats the bread', 'the duck is full'],
        ['it is cold', 'snow falls on the farm', 'the cow stays in the barn', 'the farmer brings hay', 'the cow is warm'],
    ]


def virtues():
    good = ['helping is good', 'sharing is good', 'telling the truth is good', 'being kind is good', 'saying thank you is good',
            'saying sorry is good', 'being fair is good', 'listening is good', 'caring for animals is good', 'being gentle is good']
    bad = ['hurting is bad', 'lying is bad', 'stealing is bad', 'being mean is bad', 'hitting is bad',
           'breaking things on purpose is bad', 'cheating is bad', 'teasing is bad', 'being cruel to animals is bad', 'shouting at friends is bad']
    situations = [  # situation, kind action, consequence
        ('a friend is sad', 'you help your friend', 'your friend feels better'),
        ('a child falls down', 'you help the child up', 'the child is safe'),
        ('you have two apples', 'you share one apple', 'your friend is happy'),
        ('you break a cup', 'you tell the truth', 'people trust you'),
        ('a cat is hungry', 'you feed the cat', 'the cat is fed'),
        ('someone gives you a gift', 'you say thank you', 'they feel glad'),
        ('you bump into someone', 'you say sorry', 'they forgive you'),
        ('it is your turn to wait', 'you wait your turn', 'the game is fair'),
        ('a friend is talking', 'you listen', 'your friend feels heard'),
        ('a bird is hurt', 'you are gentle with the bird', 'the bird can heal'),
    ]
    judged = [  # action, judgement
        ('tom shares his cake', 'kind'), ('sam takes a toy from ana', 'unkind'), ('zoe tells the truth', 'honest'),
        ('leo lies to his mom', 'dishonest'), ('mia helps an old man', 'kind'), ('tom hits his friend', 'unkind'),
        ('ana waits her turn', 'fair'), ('sam cheats at the game', 'unfair'), ('zoe feeds the dog', 'caring'),
        ('leo kicks the dog', 'cruel'),
    ]
    return good, bad, situations, judged


def main():
    rng = random.Random(2026)
    os.makedirs(OUT, exist_ok=True)
    good, bad, situations, judged = virtues()
    lib = dict(
        about='Cosmos curriculum library (tools/curriculum/build.py). Lower-case words and spaces only.',
        levels=[
            dict(level=1, name='words', items=sorted({w for c in CATEGORY.values() for w in c} | set(ANIMALS) | set(ANIMALS.values())
                                                      | set(THINGS) | set(PEOPLE) | {p for p, _ in PROPERTY} | {q for _, q in PROPERTY}
                                                      | {w for g in GROUPS.values() for w in g})),
            dict(level=2, name='sentences', items=sentences(rng)),
            dict(level=3, name='facts', items=facts()),
            dict(level=4, name='questions', items=[dict(q=q, a=a) for q, a in questions()]),
            dict(level=5, name='stories', items=stories()),
            dict(level=6, name='virtues', good=good, bad=bad,
                 situations=[dict(situation=s, action=a, consequence=c) for s, a, c in situations],
                 judged=[dict(action=a, judgement=j) for a, j in judged]),
        ])
    json.dump(lib, open(os.path.join(OUT, 'curriculum.json'), 'w'), indent=1)
    for lv in lib['levels']:
        with open(os.path.join(OUT, f"{lv['level']}_{lv['name']}.txt"), 'w') as f:
            if lv['name'] == 'questions':
                for it in lv['items']:
                    f.write(f"{it['q']} | {it['a']}\n")
            elif lv['name'] == 'stories':
                for st in lv['items']:
                    f.write(' / '.join(st) + '\n')
            elif lv['name'] == 'virtues':
                for s in lv['good'] + lv['bad']:
                    f.write(s + '\n')
                for it in lv['situations']:
                    f.write(f"{it['situation']} / {it['action']} / {it['consequence']}\n")
                for it in lv['judged']:
                    f.write(f"{it['action']} / that is {it['judgement']}\n")
            else:
                for it in lv['items']:
                    f.write(it + '\n')
    print({lv['name']: len(lv.get('items', lv.get('good', []))) for lv in lib['levels']})


if __name__ == '__main__':
    main()
