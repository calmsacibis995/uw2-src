#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/stats/stats.mk	1.10"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nps/utils/stats/stats.mk,v 1.15 1994/09/19 18:01:38 meb Exp $"

include $(CMDRULES)

TOP = ../../..

include $(TOP)/local.def

LINTFLAGS = $(WORKINC) $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

SRCS = spxinfo.c \
	ipxinfo.c \
	nwsaputil.c \
	ripinfo.c \
	sapinfo.c

TARGETS = spxinfo ipxinfo nwsaputil ripinfo nwsapinfo
TARGETS1 = nwsaputil

LDLIBS = $(LIBUTIL)

all: $(TARGETS)

install: all
	@for i in $(TARGETS) ; \
	do \
	$(INS) -f $(USRBIN) -m $(XMODE) -u $(OWN) -g $(GRP) $$i ; \
	done
	@for i in $(TARGETS1) ; \
	do \
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) $$i ; \
	done

spxinfo:	spxinfo.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

ipxinfo:	ipxinfo.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

nwsaputil:	nwsaputil.o
	$(CC) -o $@ $@.o $(LDOPTIONS) $(LDLIBS)

ripinfo:	ripinfo.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

nwsapinfo:	sapinfo.o
	$(CC) -o $@ sapinfo.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

sapinfo.o: sapinfo.c

ipxinfo.o: ipxinfo.c

ripinfo.o: ripinfo.c

spxinfo.o: spxinfo.c

nwsaputil.o: nwsaputil.c

clean:
	rm -f *.o
	rm -f *.ln

clobber: clean
	rm -f $(TARGETS) lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done
