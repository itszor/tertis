/* Tertis (SICK - Merlin Hughes, Conrad Hughes and Andrew Block, 1993):
 * what libroger needs to know about the game.
 *
 * !Run sets Tertis$Dir (<Obey$Dir>.), runs Memory (a utility that asks for
 * 512K of application memory and 160K of screen memory) and then !RunImage
 * (an absolute at &8000). !RunImage checks that Tertis$Dir ends in "rt."
 * (the application is called !Tertis), selects MODE 13, decompresses
 * Data.StartPic onto the screen, then loads Data.TertisCode at &70000,
 * decompresses it to &267FC (../tools/unpack.py does the same) and jumps
 * to &749F4. The game is BBC BASIC V's assembler output (its source:
 * github.com/merlinorg/trts, a slightly later build than the disc's).
 *
 * The game decompresses Data.AllMods to &8100 (the offsets of nine voice
 * modules from there, then the modules: sampled instruments, each made by
 * the same Voice Generator 1.00) and inserts each where it is (OS_Module
 * 10); it kills them by title (OS_Module 4) when it quits.
 *
 * The translated images: the game (gen/, for &267FC), !RunImage (genload/)
 * and the voice modules (genmod1-9/, at their addresses). */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <roger.h>

extern const struct roger_image recomp_image, recompP_image;   /* gen{,load}/entries.c */
extern const struct roger_image recompM1_image, recompM2_image, recompM3_image, recompM4_image, recompM5_image,
    recompM6_image, recompM7_image, recompM8_image, recompM9_image;     /* genmod1-9/entries.c */

#define APP "../game/!Tertis"
static const char *const vars[] = {"Tertis$Dir", "!Tertis.", NULL};
static const char *const prefixes[] = {"<Obey$Dir>.", NULL};
/* TertisCode is compressed: !RunImage loads it at &70000 and decompresses
 * it to &267FC, the game image's base, and jumps to its entry, &749F4 */
static const struct roger_program programs[] = {{"TertisCode", &recomp_image, 0, 0x70000, 0x749F4}, {NULL, NULL, 0, 0, 0}};
/* the voice modules, in AllMods' order: in_memory, each at its address */
#define VOICE(name, image) {"AllMods " name, &image, 0, NULL, 1}
static const struct roger_module modules[] = {
    VOICE("ELDRUM", recompM1_image), VOICE("SATURAX", recompM2_image), VOICE("SLAP1", recompM3_image),
    VOICE("Slap2", recompM4_image), VOICE("SNARE", recompM5_image), VOICE("SYNTH", recompM6_image),
    VOICE("SYTHN1", recompM7_image), VOICE("TABLA", recompM8_image), VOICE("Thunk", recompM9_image),
    {NULL, NULL, 0, NULL, 0}};

/* a file in !Tertis, whatever ,load-exec suffix the extraction gave it */
static const char *in_app(const char *leaf)
{
    DIR *d = opendir(APP);
    struct dirent *e;
    char *path = NULL;
    while (d && !path && (e = readdir(d)))
        if (strcspn(e->d_name, ",") == strlen(leaf) && !strncasecmp(e->d_name, leaf, strlen(leaf))) {
            path = malloc(strlen(APP) + strlen(e->d_name) + 2);
            sprintf(path, "%s/%s", APP, e->d_name);
        }
    if (d) closedir(d);
    if (!path) { fprintf(stderr, "no %s/%s: build.sh extracts it\n", APP, leaf); exit(1); }
    return path;
}

/* Q on the title: the game puts its icon on the icon bar (a Wimp task:
 * Select on it plays again, its menu has Info, Help and Quit). With no
 * desktop to go back to, Q quits: it is told to (Message_Quit). */
static void wimp_idle(void)
{
    roger_wimp_quit();
}

/* Headless runs (--limit): Space on the title (a new game), the game's
 * keys (keypad 4 left, 6 right, 5 rotate, 0 drop), Escape (the game
 * ends: today's and the all-time high scores), Space to skip them, then Q
 * on the title (quit). Key numbers are INKEY's: Space 99, keypad 4 123,
 * 6 27, 5 124, 0 107, Escape 113, Q 17; times in centiseconds of emulated
 * time (2 million instructions a second). */
#define K(n, cs) {n, (cs) * 20000ull, (cs) * 20000ull + 160000}     /* (cs: centiseconds) */
static const struct roger_key keys[] = {
    K(99, 2000),
    K(123, 2200), K(124, 2300), K(123, 2400), K(107, 2500), K(27, 2600), K(27, 2700), K(124, 2800),
    K(107, 2900), K(124, 3000), K(123, 3100), K(107, 3200), K(27, 3300), K(124, 3400), K(107, 3500),
    K(123, 3600), K(123, 3700), K(107, 3800), K(124, 3900), K(27, 4000), K(107, 4100), K(124, 4200),
    K(123, 4300), K(107, 4400),
    K(113, 5000),
    K(99, 7000), K(99, 7400),
    K(17, 7800),
};

static struct roger_config config = {
    .title = "Tertis (recompiled)",
    .image = &recompP_image,
    .programs = programs,
    .modules = modules,
    .appdir = "../game",
    .dir_prefixes = prefixes,
    .variables = vars,
    .initial_mode = 12,                 /* the desktop's: !RunImage selects MODE 13 */
    .wimpslot = 512 * 1024,             /* as Memory asks for */
    .screen_size = 160 * 1024,
    /* code the analysis missed (none seen: the title, games from level 0
     * and 17, the high scores, the options, quitting) runs interpreted */
    .interpret_gaps = 1,
    .wimp_idle = wimp_idle,
    .keys = keys,
    .nkeys = sizeof keys / sizeof keys[0],
};

int main(int argc, char **argv)
{
    config.image_path = in_app("!RunImage");
    roger_main(argc, argv, &config);
}
