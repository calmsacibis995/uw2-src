#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)tricord.cmds:tricord.cmds.mk	1.3"

include $(CMDRULES)

MAKEFILE = tricord.cmds.mk

LDLIBS = 

INITD = $(ETC)/init.d
STOPIMS0 = $(ETC)/rc0.d/K99ims
STOPIMS1 = $(ETC)/rc1.d/K99ims
STARTIMS = $(ETC)/rc2.d/S99ims
BINS = imsd
PROBEFILE = imsd.c


all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINS) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) $(BINS) $(MAKEARGS) ;\
		
	else \
		if [ ! -r $(BINS) ]; then \
			echo "ERROR: $(BINS) is missing" 1>&2 ;\
			false ;\
			break ;\
		fi \
	fi

$(BINS) : imsd.o
	$(CC) -O -o $@ $@.o $(LDLIBS)

imsd.o:	imsd.c \
	$(INC)/sys/ims.h \
	$(INC)/sys/imsd.h \
	$(INC)/sys/ims_mrp.h
 
install:	all
	$(INS) -f $(USRBIN) -m 0755 -u bin -g bin imsd
	$(INS) -f $(INITD) -m 0444 -u root -g sys ims
	rm -f $(STARTIMS) $(STOPIMS0) $(STOPIMS1)
	-ln $(INITD)/ims $(STARTIMS)
	-ln $(INITD)/ims $(STOPIMS0)
	-ln $(INITD)/ims $(STOPIMS1)

clean:
	rm -f *.o 

clobber: clean

