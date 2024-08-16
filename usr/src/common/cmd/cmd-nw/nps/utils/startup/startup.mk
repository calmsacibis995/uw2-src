#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/startup/startup.mk	1.12"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nps/utils/startup/startup.mk,v 1.23 1994/09/23 17:54:43 vtag Exp $"

include $(CMDRULES)

TOP = ../../..

include $(TOP)/local.def

LINTFLAGS = $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

SRCS = startnps.c \
	statnps.c \
	stopnps.c \
	startsapd.c \
	stopsapd.c

STARTOBJS = startnps.o

STATOBJS = statnps.o

STOPOBJS = stopnps.o

LDLIBS = $(LIBUTIL)

all: startnps statnps stopnps startsapd stopsapd

install: all
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) startnps
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) statnps
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) stopnps
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) startsapd
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) stopsapd

startnps:	$(STARTOBJS)
	$(CC) -o $@ $(STARTOBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

statnps:	$(STATOBJS)
	$(CC) -o $@ $(STATOBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

stopnps:	$(STOPOBJS)
	$(CC) -o $@ $(STOPOBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS) -lgen

stopsapd:	stopsapd.o
	$(CC) -o $@ stopsapd.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

startsapd:	startsapd.o
	$(CC) -o $@ startsapd.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

startnps.o: startnps.c

statnps.o: statnps.c

stopnps.o: stopnps.c

stopsapd.o: stopsapd.c

startsapd.o: startsapd.c

clean:
	rm -f *.o
	rm -f *.ln

clobber: clean
	rm -f startnps statnps stopnps stopsapd startsapd lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done
