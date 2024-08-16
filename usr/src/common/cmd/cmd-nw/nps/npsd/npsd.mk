#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/npsd/npsd.mk	1.8"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nps/npsd/npsd.mk,v 1.16 1994/05/10 20:45:05 mark Exp $"

include $(CMDRULES)

TOP = ../..

include $(TOP)/local.def

LINTFLAGS = $(WORKINC) $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

SRCS = dl.c \
	npsd.c \
	nvt.c

OBJS = dl.o \
	npsd.o \
	nvt.o

LDLIBS = $(LIBUTIL)

all: npsd

install: all
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) npsd

npsd:	$(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

npsd.o: npsd.c

clean:
	rm -f *.o
	rm -f *.ln

clobber: clean
	rm -f npsd lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done
