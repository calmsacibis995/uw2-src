#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/diagd/diagd.mk	1.6"
#ident	"$Id: diagd.mk,v 1.10 1994/05/10 20:44:39 mark Exp $"

include $(CMDRULES)

TOP = ../..

include $(TOP)/local.def

LINTFLAGS = $(GLOBALINC) $(LOCALINC) $(LOCALDEF) -c

SRCS = nwdiagd.c 

OBJS = nwdiagd.o 

LDLIBS = $(LIBUTIL)

all: nwdiagd

install: all
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) nwdiagd

nwdiagd: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

nwdiagd.o: nwdiagd.c

clean:
	rm -f *.o
	rm -f *.ln

clobber: clean
	rm -f nwdiagd lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	echo '## lint output for nwdiagd.c ##' >>lint.out  ; \
	$(LINT) $(LINTFLAGS) nwdiagd.c $(LINTLIBS) >>lint.out 
