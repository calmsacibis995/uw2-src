#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright	"%c%"

#ident	"@(#)zic:build_data/makefile	1.3"

include $(CMDRULES)

################################################################################

SOURCES = ../zic.c ../scheck.c ../ialloc.c
LSOURCES = zic.c scheck.c ialloc.c
LOBJECTS = zic.o scheck.o ialloc.o

# we want to have zic exec the yearistype which is one level up,
# not the one that may be on the native system
LPATH=..:$(PATH)

FILES = ../asia ../australasia ../europe ../etcetera ../northamerica \
	../pacificnew ../southamerica

all: zic

zic: $(SOURCES)
	sed -e 's/TZDIR/"."/' ../zic.c >zic.c
	cp ../scheck.c ../ialloc.c .
	$(HCC) -o zic $(LSOURCES)

install: all
	for i in $(FILES); do \
	    PATH=$(LPATH) ./zic $$i; done
	for i in `ls -d [A-Z]*`; do \
	    if [ -d $$i ] ; then \
		[ -d $(ETC)/TZ/$$i ] || \
		    mkdir -p $(ETC)/TZ/$$i; \
		for j in `ls $$i`; do \
		    $(INS) -f $(ETC)/TZ/$$i -m 644 $$i/$$j; done; \
	    else \
		$(INS) -f $(ETC)/TZ -m 644 $$i; \
	    fi; \
	done

clean:
	rm -f $(LSOURCES) $(LOBJECTS)

clobber: clean
	rm -rf zic [A-Z]*

lintit:

