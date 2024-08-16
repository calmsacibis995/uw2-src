#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/sapd/sapd.mk	1.10"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nps/sapd/sapd.mk,v 1.16 1994/05/09 20:24:04 mark Exp $"

include $(CMDRULES)

TOP = ../..

include $(TOP)/local.def

LINTFLAGS = $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

SRCS = sapd.c 

OBJS = sapd.o 

SRCSNUC = nucsapd.c

OBJSNUC = nucsapd.o

LDLIBS = $(LIBUTIL)

all: nucsapd sapd track

install: all
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) nucsapd
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) sapd
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) track

sapd:	$(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

nucsapd:    $(OBJSNUC)
	$(CC) -o $@ $(OBJSNUC) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

sapd.o: sapd.c

nucsapd.o: nucsapd.c

clean:
	rm -f *.o
	rm -f *.ln

clobber: clean
	rm -f nucsapd sapd track lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	echo '## lint output for sapd.c ##' >>lint.out  ; \
	$(LINT) $(LINTFLAGS) sapd.c $(LINTLIBS) >>lint.out; \
	echo '## lint output for nucsapd.c ##' >>lint.out  ; \
	$(LINT) $(LINTFLAGS) nucsapd.c $(LINTLIBS) >>lint.out 
