# Static recompilation of Tertis: the game (gen/: Data.TertisCode as
# !RunImage decompresses it to &267FC, ../tools/unpack.py), its loader
# !RunImage (genload/) and the nine voice modules the game decompresses from
# Data.AllMods and inserts where they are (genmod1-9/), from SuperSpider's
# analyse and gen, and game.c, linked with libroger (the RISC OS runtime).
ROGER   ?= $(HOME)/stuff/libroger
CC      ?= gcc
CFLAGS  ?= -O2 -g -w -fno-strict-aliasing $(if $(TRACE),-DRECOMP_TRACE)
GEN     := $(wildcard gen/code*.c) gen/entries.c
GENLOAD := $(wildcard genload/code*.c) genload/entries.c
# the voice modules: N:address (at &8100 + AllMods' offsets)
MODS    := 1:8128 2:95ac 3:11030 4:131b4 5:14538 6:15df4 7:1c174 8:1eff8 9:20c7c
GENMOD  := $(foreach m,$(MODS),$(wildcard genmod$(word 1,$(subst :, ,$(m)))/code*.c) \
             genmod$(word 1,$(subst :, ,$(m)))/entries.c)
OBJ     := $(GEN:.c=.o) $(GENLOAD:.c=.o) $(GENMOD:.c=.o) game.o
LIBS    := $(shell pkg-config --libs sdl3) -ljpeg -lm -lpthread
INC     := -I$(ROGER)/include

tertis: $(OBJ) $(ROGER)/libroger.a
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(ROGER)/libroger.a $(LIBS)

$(ROGER)/libroger.a: FORCE
	$(MAKE) -C $(ROGER) libroger.a

%.o: %.c $(ROGER)/include/roger.h $(ROGER)/include/roger_cpu.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

# Code/data analysis: SuperSpider, into ../analysis/<name>.json, then
# superspider gen. Each image's ../analysis/<name>.roots adds entry points
# to the analysis: "make roots" merges the gap entries a --trace-gaps run
# logged (untranslated.log, or LOG=...) into them, so the next "make regen"
# translates them.
SS      ?= $(HOME)/stuff/superspider
SSBIN   := $(SS)/_build/default/bin/main.exe
AN      := ../analysis
APP     := ../game/!Tertis
GAME    := ../unpacked/tertiscode
# the game's base and entry (!RunImage decompresses it to &267FC and jumps to
# &749F4), and its end (--extern for !RunImage's analysis)
BASE    := 267fc
ENTRY   := 749f4
GAME_END = $(shell printf %x $$((0x$(BASE) + $$(stat -c %s $(GAME)))))
LOADER  := $(firstword $(wildcard $(APP)/!RunImage,*) $(APP)/!RunImage)
LOG     ?= untranslated.log

# --roots for ../analysis/<name>.roots, if there is one
roots = $(if $(wildcard $(AN)/$(1).roots),--roots $(AN)/$(1).roots)

# $(call analyse,name,image,base[,options])
define analyse
	$(SSBIN) analyse "$(2)" $(3) $(4) --full $(call roots,$(1)) -o $(AN)/$(1).json
endef

regen:
	cd $(SS) && dune build
	python3 -I ../tools/unpack.py
	mkdir -p $(AN)
	$(call analyse,game,$(GAME),$(BASE),--exec $(ENTRY))
	$(call analyse,loader,$(LOADER),8000,--extern $(BASE)-$(GAME_END))
	for m in $(MODS); do n=$${m%:*}; a=$${m#*:}; \
	    $(SSBIN) analyse ../unpacked/mod$$n $$a --module --full -o $(AN)/mod$$n.json || exit 1; done
	rm -rf gen genload genmod*
	$(SSBIN) gen "$(GAME)" $(BASE) $(AN)/game.json --c gen --leaders=dispatch
	$(SSBIN) gen "$(LOADER)" 8000 $(AN)/loader.json --c genload --leaders=dispatch \
	    --prefix=P --tabprefix=recompP --files=1 --covtag=P
	for m in $(MODS); do n=$${m%:*}; a=$${m#*:}; \
	    $(SSBIN) gen ../unpacked/mod$$n $$a $(AN)/mod$$n.json --c genmod$$n --leaders=dispatch \
	        --prefix=M$$n --tabprefix=recompM$$n --files=1 --covtag=M$$n || exit 1; done

roots:
	python3 -I $(SS)/tools/gap_roots.py "$(GAME)" $(BASE) $(AN)/game.roots $(LOG)
	python3 -I $(SS)/tools/gap_roots.py "$(LOADER)" 8000 $(AN)/loader.roots $(LOG)

clean:
	rm -f gen/*.o genload/*.o genmod*/*.o game.o tertis

.PHONY: FORCE regen roots clean
