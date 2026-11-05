"""Write tools/ladder/words.txt: 1000 everyday English words (letters only, 3 to 9 long), one per line.

    python tools/ladder/make_words.py
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
TEXT = """
about above accept across act action add address admit adult affect after again against age agency agent ago agree
ahead air all allow almost alone along already also always among amount analysis and animal another answer any
anyone appear apply approach area argue arm army around arrive art article artist ask assume attack attention
author away baby back bad bag ball bank bar base beat beautiful because become bed before begin behind believe
benefit best better between beyond big bill bird black blood blue board boat body book born both box boy break
bring brother budget build building business but buy call camera campaign can cancer car card care career carry
case catch cause cell center central century certain chair challenge chance change character charge check child
choice choose church city civil claim class clear close coach cold collection college color come common community
company compare computer concern condition conference consider contain continue control cost could country couple
course court cover create crime cultural culture cup current customer cut dark data daughter day dead deal death
debate decade decide decision deep defense degree democrat describe design despite detail determine develop
die difference different difficult dinner direction director discover discuss disease doctor dog door down draw
dream drive drop drug during each early east easy eat economic economy edge education effect effort eight either
election else employee end energy enjoy enough enter entire environment especially establish even evening event
ever every everybody everyone everything evidence exactly example executive exist expect experience expert explain
eye face fact factor fail fall family far fast father fear federal feel feeling few field fight figure fill film
final finally financial find fine finger finish fire firm first fish five floor fly focus follow food foot force
foreign forget form former forward four free friend from front full fund future game garden gas general generation
get girl give glass goal good government great green ground group grow growth guess gun guy hair half hand hang
happen happy hard have head health hear heart heat heavy help her here herself high him himself his history hit
hold home hope hospital hot hotel hour house how however huge human hundred husband idea identify image imagine
impact important improve include including increase indeed indicate individual industry information inside
instead institution interest interesting international interview into investment involve issue item its itself
job join just keep key kid kill kind kitchen know knowledge land language large last late later laugh law lawyer
lay lead leader learn least leave left leg legal less let letter level lie life light like likely line list
listen little live local long look lose loss lot love low machine magazine main maintain major majority make
man manage management manager many market marriage material matter may maybe mean measure media medical meet
meeting member memory mention message method middle might military million mind minute miss mission model modern
moment money month more morning most mother mouth move movement movie much music must myself name nation national
natural nature near nearly necessary need network never new news newspaper next nice night none nor north not note
nothing notice now number occur off offer office officer official often oil old once one only onto open operation
opportunity option order organization other others our out outside over own owner page pain painting paper parent
part participant particular particularly partner party pass past patient pattern pay peace people per perform
performance perhaps period person personal phone physical pick picture piece place plan plant play player point
police policy political politics poor popular population position positive possible power practice prepare present
president pressure pretty prevent price private probably problem process produce product production professional
professor program project property protect prove provide public pull purpose push put quality question quickly
quite race radio raise range rate rather reach read ready real reality realize really reason receive recent
recently recognize record red reduce reflect region relate relationship religious remain remember remove report
represent require research resource respond response rest result return reveal rich right rise risk road rock
role room rule run safe same save say scene school science scientist score sea season seat second section security
see seek seem sell send senior sense series serious serve service set seven several shake share she shoot short
shot should shoulder show side sign significant similar simple simply since sing single sister sit site situation
six size skill skin small smile social society soldier some somebody someone something sometimes son song soon sort
sound source south southern space speak special specific speech spend sport spring staff stage stand standard star
start state statement station stay step still stock stop store story strategy street strong structure student study
stuff style subject success successful such suddenly suffer suggest summer support sure surface system table take
talk task tax teach teacher team technology television tell ten tend term test than thank that the their them
themselves then theory there these they thing think third this those though thought thousand threat three through
throughout throw thus time today together tonight too top total tough toward town trade traditional training travel
treat treatment tree trial trip trouble true truth try turn two type under understand unit until upon use usually
value various very victim view violence visit voice vote wait walk wall want war watch water way weapon wear week
weight well west western what whatever when where whether which while white who whole whom whose why wide wife will
win wind window wish with within without woman wonder word work worker world worry would write writer wrong yard
yeah year yes yet you young your yourself apple banana cherry grape lemon melon orange peach pear plum berry bread
butter cheese cookie cream egg honey juice milk pizza rice salad soup sugar tea bean carrot corn onion potato tomato
cat cow duck fox goat horse lion mouse pig rabbit sheep tiger whale wolf bee ant frog snake owl crow eagle shark
river lake hill mountain forest island beach desert valley cloud rain snow storm sun moon sky wave stone sand leaf
flower grass root seed branch bush vine bed blanket pillow chair desk lamp clock mirror door roof floor stairs shelf
spoon fork knife plate bowl pot pan oven fridge sink towel soap brush comb shirt dress coat hat shoe sock glove belt
button pocket ring watch bag box basket bottle jar cup key lock rope wheel ball doll kite drum bell train truck bus
ship plane bike rocket road bridge tower castle farm barn tent garden park zoo market shop bank school library
museum bakery kitchen yellow purple brown pink gray silver golden round square soft loud quiet quick slow sweet
sour bitter salty warm cool wet dry clean dirty empty heavy light tall short thin thick young old brave kind gentle
angry calm proud shy sad glad tired hungry thirsty sleepy jump run swim climb dance sing laugh cry shout whisper
smile wave clap kick throw catch push pull carry lift drop open close build break fix paint draw cook bake wash
"""


def main():
    seen, out = set(), []
    for w in TEXT.split():
        if w.isalpha() and 3 <= len(w) <= 9 and w not in seen:
            seen.add(w)
            out.append(w)
    out = out[:1000]
    assert len(out) == 1000, len(out)
    open(os.path.join(HERE, 'words.txt'), 'w').write('\n'.join(out) + '\n')
    print(len(out), 'words')


if __name__ == '__main__':
    main()
