#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)hcomp:ahcomp.mk	1.1"

include $(CMDRULES)

OWN = bin
GRP = bin

MAINS = hcomp

OBJECTS = hcomp.o wslib.o

SOURCES = hcomp.c wslib.c wslib.h

all: $(SOURCES) $(OBJECTS) $(MAINS)

hcomp.o: $(SOURCES)

$(MAINS): $(OBJECTS)
	$(HCC) -o $(MAINS) $(OBJECTS) -lw

$(SOURCES):
	@ln -s ${ROOT}/usr/src/${WORK}/cmd/winxksh/libwin/$@ $@

install: all
	@if [ ! -d $(USRBIN) ] ;\
	then \
		mkdir -p $(USRBIN) ;\
	fi
	$(INS) -f $(USRBIN) -m 0555 hcomp

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)
