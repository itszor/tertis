Tertis (SICK, 1993) - static recompilation
==========================================

A recipe: this repository holds no game data. It rebuilds the game as a
native executable from your own copy, recognised by content (recipe.json),
so it can be shared without distributing the game.

    ./build.sh tertis.adf
    cd recomp && ./tertis

build.sh runs recipe.json's steps with SuperSpider's tools/weave.py
(~/stuff/superspider, or $SUPERSPIDER; $SS below): it extracts the disc,
runs !RunImage's own decompressor on the game and its voice modules
(tools/unpack.py), analyses and translates everything, and links it with
libroger (~/stuff/libroger, or make ROGER=...). Needs python3, OCaml/dune,
gcc and SDL3.

The input: the 800K ADFS disc of !Tertis 2.05 (sha256 ae813a32...). It is
also the tertis.adf in github.com/merlinorg/trts, byte for byte.

What it makes:

    extracted/disc        the disc, via weave.py
    game/!Tertis          libroger's appdir (../game)
    unpacked/tertiscode   tools/unpack.py: Data.TertisCode as !RunImage
                          decompresses it, at &267FC
    unpacked/allmods      Data.AllMods as the game decompresses it (to &8100),
                          and unpacked/mod1-9, the nine voice modules in it
    analysis/*.json       SuperSpider's analyses (game, loader, mod1-9)
    recomp/gen*/          their C translations
    recomp/tertis         the game

Committed: recipe.json, build.sh, tools/unpack.py, recomp/Makefile,
recomp/game.c, this file.

Playing
-------

    ./tertis                    as !Run: the SICK intro, the title, the game

The title (mouse or keys): PLAY or Space starts a game; ? the options
(keys, show next, rotation direction, skill level, the "amazing effect",
continuous, alternative graphics, save); X starts on another level, in
continuous mode; Q quits. In the game: keypad 4 left, 6 right, 5 rotate,
0 drop (as the options set them); f1 pause, f2 music, f3 sound effects,
f4 show next, f7/f8 volume down/up, Escape or f12 ends the game. The
initials for a high score are typed (Return ends them).

The high scores (Data.!HiScors) and the options (Data.!Keys) the game saves
go to recomp/savedata/, libroger's place for written files.

Quitting: Q on the title takes the original back to the desktop, with an
icon on the icon bar (Select on it plays again; its menu has Info, Help
and Quit). Without a desktop, game.c tells it to quit there and then
(Message_Quit), so Q ends the program. With --desktop the icon bar is real;
its Help item runs !Help, which isn't translated.

How the game is put together
----------------------------

Tertis is ARM code assembled by BBC BASIC V's assembler: its source (the
BASIC programs that assemble it, with their libraries) is on GitHub,
github.com/merlinorg/trts. The disc's build is not quite the source's last
state (the binaries differ in some 43,000 bytes), so the disc is what is
translated; the source is the reference for what the code does (labels,
the music player, the menus). There is no C to compile natively: a hybrid
would have nothing to put on the native side.

!Run sets Tertis$Dir, runs Memory (a utility: 512K of application memory,
160K of screen memory) and then !RunImage, an absolute at &8000 (the
source's Code.!RunImage). It checks that Tertis$Dir ends in "rt."
(the application must be called !Tertis), selects MODE 13, clears screen
memory, decompresses Data.StartPic (the SICK logo) onto the screen, then
loads Data.TertisCode at &70000, decompresses it to &267FC and jumps to
&749F4. The decompressor (ScorPress's, from the source's library) is a
coroutine: it returns NE with its resume address pushed on a stack of its
own in R1, and its caller resumes it with ldmneia r1!,{pc}.

TertisCode (&267FC-&82D08) is the game. It sets its stack to &8100, just
above &8000 (deep calls dip below it, into what was RISC OS's workspace),
reads the screen's address from RISC OS 2 and 3.1's zero page (&164, the
soft CAM map's first entry: the start of screen memory; &10C0, the VDU
drivers' ScreenStart) and the centisecond counter (MetroGnome, &10C),
decompresses Data.AllMods to &8100 - the offsets of nine voice modules,
then the modules, each a sampled instrument made by the same Voice
Generator 1.00 (ELDRUM, SATURAX, SLAP1, Slap2, SNARE, SYNTH, SYTHN1, TABLA,
Thunk) - and inserts each where it is (OS_Module 10). Its music is its own
sequencer, driven from EventV (vsync), scheduling notes on those voices.
Its pictures and sprites (Data.BindleOne, BindleTwo, FootBall) are
compressed the same way. Q on the title runs a small Wimp task (the
source's DeskWork) before it exits.

What's translated (recomp/Makefile, make regen): the game (gen/, for
&267FC, entered at &749F4), !RunImage (genload/) and the nine voice modules
(genmod1-9/, each for its address in AllMods). libroger selects the game's
translation when !RunImage loads TertisCode at &70000 and jumps to its
entry (roger_program.at, .entry), and inserts each module's translation
when the game does OS_Module 10 at its address (roger_module.in_memory).

Tested (headless, --trace-gaps): the intro, the title and its attract
screens, games from level 0 and from level 17 (continuous mode, X),
pause and the function keys, game over, the initials, today's and the
all-time high scores, the options screen, and Q - no untranslated code was
reached. The bonus levels (reached by clearing lines) weren't played; code
the analysis missed would run interpreted (interpret_gaps).

What SuperSpider and libroger needed
------------------------------------

SuperSpider:

- An unresolved jump with a pushed "return address" (the hand-linked call
  assumption) takes the word on top of the stack only if it looks like
  code. The decompressor's wrapper pushes r0-r12 and lr, with r0 the
  source buffer, before resuming the decompressor through ldmneia
  r1!,{pc}: the buffers (&3F71C, 118K of zeros; &53D34) were taken as
  code, 29,600 words of andeq r0,r0,r0 in one function that took gcc
  25 minutes.
- add pc,rN,rX,lsl #k with rN a known address and rX unknown: into a run
  of identical blocks starting at rN, each block a multiple of 2^k bytes
  (Tertis' sprite rows: 9-instruction blocks, entered at rN + 36n with rX
  made by add rX,rX,rX,lsl #3) - every block start and the code after the
  run. (The backwards form, sub pc,lr,rX into blocks ending at rN, was
  already there.)

libroger:

- OS_Module 10 (insert a module from memory, where it is): a configured
  module with in_memory set, at the address R1 gives.
- roger_program.entry: a program unpacked into place and entered part way
  through (TertisCode at &749F4), not at its base.
- RISC OS 2 and 3.1's zero page words: &164 (the soft CAM map's first
  entry: where screen memory starts), &10C0 (VDU ScreenStart, the
  drivers' bank) and &10C (MetroGnome, centiseconds), kept current on
  mode and bank changes and every centisecond.
- OS_BinaryToDecimal.
- OS_Byte 129 reading a key returns R1 = R2 (&FF on timeout, &1B on
  Escape), as RISC OS's kernel does; R1 was 0. The level entry tests R1
  for &FF and otherwise pops a word it pushed for a digit: every frame
  without a key leaked a word of stack until it returned through garbage.
- A --script with input of its own (keys, clicks...) replaces the game's
  headless scenario (roger_config.keys/clicks); one of screenshots only
  doesn't.

Not supported: !Help (an absolute that the icon bar menu runs, only with
--desktop) isn't translated.
