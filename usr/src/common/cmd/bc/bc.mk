#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bc:bc.mk	1.11.1.3"
#ident "$Header: bc.mk 1.2 91/03/19 $"
#	bc make file

include $(CMDRULES)
OWN=bin
GRP=bin
REL = current
LIST = lp
INSDIR = $(USRBIN)
INSLIB = $(USRLIB)
SOURCE = bc.y lib.b.data
FILES = bc.c

all: bc lib.b
	:

bc:	$(FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o bc $(FILES) $(LDLIBS) $(SHLIBS)

$(FILES):
	-$(YACC) bc.y && mv y.tab.c bc.x
	cp bc.x bc.c

lib.b:
	cp lib.b.data lib.b

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) bc
	$(INS) -f $(INSLIB) -m 0444 -u $(OWN) -g $(GRP) lib.b

build:	bldmk
	get -p $(CSID) s.bc.src $(REWIRE) | ntar -d $(RDIR) -g
	cd $(RDIR); $(YACC) bc.y; mv y.tab.c bc.x

bldmk:  ;  get -p $(MKSID) s.bc.mk > $(RDIR)/bc.mk

listing:
	pr bc.mk $(SOURCE) | $(LIST)
listmk: ;  pr bc.mk | $(LIST)

edit:
	get -e -p s.bc.src | ntar -g

delta:
	ntar -p $(SOURCE) > bc.src
	delta s.bc.src
	rm -f $(SOURCE)

mkedit:  ;  get -e s.bc.mk
mkdelta: ;  delta s.bc.mk

clean:
	:

clobber:	clean
	rm -f bc bc.c lib.b bc.x

delete:	clobber
	rm -f $(SOURCE) bc.x
