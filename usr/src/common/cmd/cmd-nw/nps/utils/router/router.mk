#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/router/router.mk	1.9"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nps/utils/router/router.mk,v 1.14 1994/09/19 18:00:25 meb Exp $"

include $(CMDRULES)

TOP = ../../..

include $(TOP)/local.def

LINTFLAGS = $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

SRCS = drouter.c \
	resetroute.c

DROBJS = drouter.o

RROBJS = resetroute.o

LDLIBS = $(LIBUTIL)

all: drouter rrouter

install: all
	@-mkdir -p $(USRBIN) 2>/dev/null;exit 0
	$(INS) -f $(USRBIN) -m $(XMODE) -u $(OWN) -g $(GRP) drouter
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) rrouter

drouter:	$(DROBJS)
	$(CC) -o $@ $(DROBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

rrouter:	$(RROBJS)
	$(CC) -o $@ $(RROBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

drouter.o: drouter.c

resetroute.o: resetroute.c

clean:
	rm -f *.o
	rm -f *.ln

clobber: clean
	rm -f drouter rrouter lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done
