#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)regcmp:regcmp.mk	1.8.2.4"
#	regcmp make file

include $(CMDRULES)

OL = $(ROOT)/
SL = $(ROOT)/usr/src/cmd/regcmp
RDIR = $(SL)
INS = ../install/install.sh
REL = current
CSID = -r`gsid regcmp $(REL)`
MKSID = -r`gsid regcmp.mk $(REL)`
LIST = lp
INSDIR = $(CCSBIN)
LINK_MODE=
IFLAG = 
LOC_LDFLAGS = $(LDFLAGS) $(IFLAG)
SOURCE = regcmp.c

compile all: regcmp

regcmp:
	$(CC) $(CFLAGS) $(LOC_LDFLAGS) $(LINK_MODE) -o regcmp regcmp.c -lgen

install:	all
	cp regcmp regcmp.bak
	$(STRIP) regcmp
	/bin/sh $(INS) -f $(INSDIR) regcmp
	mv regcmp.bak regcmp

build:	bldmk
	get -p $(CSID) s.regcmp.c $(REWIRE) > $(RDIR)/regcmp.c
bldmk:  ;  get -p $(MKSID) s.regcmp.mk > $(RDIR)/regcmp.mk

listing:
	pr regcmp.mk $(SOURCE) | $(LIST)
listmk: ;  pr regcmp.mk | $(LIST)

edit:
	get -e s.regcmp.c

delta:
	delta s.regcmp.c

mkedit:  ;  get -e s.regcmp.mk
mkdelta: ;  delta s.regcmp.mk

clean:
	:

clobber:
	  rm -f regcmp

delete:	clobber
	rm -f $(SOURCE)
