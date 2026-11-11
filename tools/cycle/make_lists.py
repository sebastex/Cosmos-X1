"""Write tools/cycle/lists.txt: 1000 more everyday English words (letters only, 3 to 9 long), none of
them among the ladder's 1000 (tools/ladder/words.txt), in 10 lists of 100 (one line per word, the
lists in order). The brains that learned the ladder's 1000 words learn these 100 at a time.

    python tools/cycle/make_lists.py
"""
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))
OLD = os.path.join(HERE, '..', 'ladder', 'words.txt')
TEXT = """
apple banana cherry grape lemon melon orange peach pear plum berry mango olive onion carrot potato tomato pepper
garlic ginger honey butter cheese bread toast cake cookie pie soup salad rice pasta noodle sugar salt flour
cream milk juice coffee tea water wine beer bottle jar bowl plate spoon fork knife pan pot oven stove kettle
table chair sofa couch bench desk shelf drawer closet carpet curtain pillow blanket sheet mirror lamp candle
clock window ceiling roof wall stair ladder fence gate garden yard porch attic cellar garage barn shed tent
cabin castle palace tower bridge tunnel road street lane path trail highway corner square market shop store
bakery library museum theater cinema studio office factory farm field meadow forest jungle desert island
beach coast shore ocean sea lake river stream pond creek valley canyon cliff cave hill mountain volcano
glacier snow ice rain storm cloud thunder lightning wind breeze fog mist frost sunshine rainbow sky star moon
planet comet galaxy universe earth stone rock sand dust mud clay brick metal iron steel copper silver gold
diamond pearl crystal glass plastic paper cardboard wood leather cotton wool silk velvet rope string thread
needle button zipper pocket sleeve collar jacket coat shirt skirt dress pants shorts sock shoe boot sandal
hat cap scarf glove belt tie ring necklace bracelet earring watch wallet purse basket bucket barrel box crate
dog cat horse cow pig sheep goat chicken duck goose turkey rabbit mouse rat squirrel deer bear wolf fox lion
tiger leopard zebra giraffe elephant monkey gorilla camel kangaroo koala panda whale dolphin shark seal
penguin eagle hawk owl parrot crow pigeon sparrow swan frog toad snake lizard turtle crab lobster shrimp
octopus jellyfish snail worm spider ant bee wasp butterfly moth beetle mosquito fly grasshopper cricket
flower rose tulip daisy lily violet orchid sunflower tree oak pine maple willow palm bush grass leaf root
branch seed bark petal thorn moss weed vine hedge acorn mushroom pumpkin corn wheat bean pea cabbage lettuce
spinach celery cucumber radish turnip beet almond walnut peanut cashew raisin date fig coconut pineapple
kiwi lime apricot strawberry blueberry raspberry watermelon avocado doctor nurse teacher farmer baker
butcher painter singer dancer actor writer poet pilot sailor soldier police judge lawyer banker clerk
cashier waiter chef cook driver mechanic plumber carpenter builder miner hunter fisher shepherd gardener
student pupil professor scientist engineer inventor explorer traveler tourist stranger neighbor cousin
uncle aunt nephew niece grandson bride groom widow orphan hero villain giant dwarf wizard witch ghost
angel devil monster dragon king queen prince princess knight duke servant slave master captain chief
leader ruler emperor mayor senator minister priest monk nun saint prophet pirate thief robber spy guard
nose mouth lip tongue tooth teeth chin cheek ear forehead eyebrow eyelash neck shoulder elbow wrist palm
thumb knuckle chest belly back hip knee ankle heel toe skin bone muscle brain lung liver kidney stomach
throat vein sweat tear breath sneeze cough fever headache wound scar bruise pill bandage medicine vitamin
happy sad angry calm brave shy proud lazy busy clever silly funny gentle kind cruel rude polite honest
loyal jealous greedy curious nervous anxious excited bored tired sleepy hungry thirsty sick healthy
strong weak tall short fat thin young old new ancient modern fresh rotten clean dirty wet dry warm cool
cold freezing hot bright dim loud quiet soft hard smooth rough sharp blunt thick narrow wide deep shallow
heavy light empty full rich poor cheap expensive pretty ugly lovely gorgeous plain fancy simple complex
quick slow early late ready sweet sour bitter salty spicy tasty yummy crispy juicy creamy golden purple
pink brown gray orange yellow violet scarlet crimson turquoise beige ivory navy maroon olive amber
run walk jump hop skip climb crawl swim dive float sink fly glide slide roll spin twist bend stretch
kick throw catch toss grab hold squeeze push pull lift drop carry drag lean stand sit kneel lie sleep
wake dream yawn blink wink smile laugh giggle cry weep shout scream whisper sing hum whistle talk speak
chat argue joke tease praise thank greet hug kiss wave nod shake point touch tap knock scratch rub wash
bathe brush comb shave dress wear fold sew knit weave cut chop slice peel grate stir mix bake boil fry
roast grill toast pour fill spill sip taste chew swallow bite lick smell sniff listen watch stare glance
peek search hunt seek hide chase follow lead guide visit travel wander explore escape rescue protect
defend attack fight punch kick steal borrow lend owe pay spend save earn sell trade rent hire fire quit
plant water dig harvest gather pick collect sort count measure weigh guess solve plan arrange invent
paint draw sketch carve mold print type write spell read study learn teach train practice repeat review
remember recall wonder doubt trust hope wish pray promise forgive apologize complain obey refuse deny

anchor arrow axe badge balloon bandit banner barrel basin beacon beard bell blade blossom bolt bonnet
boulder bracket brake breadth brick bridle bristle broom bubble buckle bugle bundle burrow cactus canal
candy cannon canoe canvas canyon carriage cart cartoon cashew cassette catalog cattle cereal chalk chamber
channel chapel charcoal chariot chimney chisel cinnamon circus clam clover cobweb cockpit cocoa comb compass
cork cottage cradle crayon crown crumb crust cube cupboard cushion dagger dairy dart denim dew dial diary
dice dome donkey doorbell dough drum dune eagle easel elbow ember engine envelope eraser fable falcon feast
feather ferry fiddle flag flame flask fleet flint flute foam fountain freckle fudge funnel fur gadget
gallon garnet gem glider globe goblet goggles gong gravel gravy griddle grove gutter hammer hammock hamster
harbor harp hatchet hay helmet hinge hive hoof hook hoop horn hut igloo ink insect jacket jam jeans jewel
jigsaw jockey jug kayak kennel kernel kettle kitten kite knob label lace lagoon lantern lava lawn leash
lens lever lid locket locker lodge loom lotion lumber lute magnet mailbox mammal mantle marble mask mast
mattress meadow medal melody mermaid mitten moat mole mop mule mustard napkin nectar nest nickel noodle
nugget nutmeg oar oasis orbit ostrich otter oyster paddle pajamas pancake parcel parrot pasture patch
pebble pelican pendant pepper pickle piglet pillar pistol pitcher pixel plank platter plow plume pocket
pole pony popcorn porridge potion pouch prairie pretzel puddle pulley puppet puppy puzzle quarry quill
quilt rabbit raccoon raft rail raisin rake ranch razor reed reef ribbon rifle ripple robe rocket rooster
rudder rug saddle saffron sail sapling satchel sausage saw scale scarecrow scepter scooter scroll seashell
shovel shrub shutter sickle sieve signal skate skeleton sled sleigh slipper sloth smock snorkel soap socket
spade sparkle spatula spear sponge spool spout sprout squash stable stallion stamp stapler statue steam
stool stork stove straw stream stump submarine suitcase sundial swamp sweater sword syrup tablet tadpole
tambourine tank tassel teapot telescope temple thimble throne ticket timber toaster toffee tomb tongs
torch towel tractor trumpet tuba tulip tunic turban tusk tweezers twig umbrella unicorn urn valve vase
vest vessel violin visor vulture wagon waffle walrus wand warden wardrobe whisk whisker wig windmill
wreath yacht yarn yogurt yolk
abandon absorb accuse adore advise amaze annoy applaud arrest assist attach awake balance banish bounce
breathe bury calculate capture celebrate chuckle clap clatter coax collapse comfort confess conquer crack
crash crumble crush dangle dazzle decorate delight deliver depart deserve devour dodge drift drizzle drown
dust echo embrace erase evaporate examine exhale fasten fetch fidget flap flee flicker flip flutter forbid
freeze frighten gallop gargle gasp glare glitter gobble gossip grin groan growl grumble gulp hatch haul
hesitate hiccup hover howl hurry ignore imitate inhale inspect interrupt juggle kidnap launch leap linger
march marvel melt mend mumble murmur nibble obey paddle pause perch pinch plunge polish pounce pretend
prowl punish purr quarrel rattle relax rescue rinse roar rumble rush scatter scold scramble scribble
shiver shrug sigh sizzle slam slither smash sneak snore soak sob sparkle splash sprinkle sprint squeak
stagger stomp stroll stumble surrender swallow sway swing tackle tickle tiptoe topple tremble trot tumble
unfold vanish wail wobble wrap wrestle yell zoom
"""


def main():
    old = {w.strip() for w in open(OLD) if w.strip()}
    seen, out = set(), []
    for w in TEXT.split():
        w = w.lower()
        if not w.isalpha() or not 3 <= len(w) <= 9 or w in old or w in seen:
            continue
        seen.add(w)
        out.append(w)
    random.Random(2000).shuffle(out)
    if len(out) < 1000:
        raise SystemExit(f'only {len(out)} new words; add more to TEXT')
    with open(os.path.join(HERE, 'lists.txt'), 'w') as f:
        f.write('\n'.join(out[:1000]) + '\n')
    print(f'{len(out)} new words, wrote 1000 (10 lists of 100)')


if __name__ == '__main__':
    main()
