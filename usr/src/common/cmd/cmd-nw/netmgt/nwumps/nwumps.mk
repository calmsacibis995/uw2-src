#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/nwumps/nwumps.mk	1.11"
#****************************************************************************
#*                                                                          *
#*   Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992,    *
#*                 1993, 1994  Novell, Inc. All Rights Reserved.            *
#*                                                                          *
#****************************************************************************
#*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	    *
#*      The copyright notice above does not evidence any   	            *
#*      actual or intended publication of such source code.                 *
#****************************************************************************

include $(CMDRULES)

TOP = ../..

include $(TOP)/local.def

LOCALINC = -DSVR4 -DOS_UNIXWARE -I$(INC)/netmgt -I../include -I$(TOP)/nps/diagd

LINTFLAGS = $(GLOBALINC) $(LOCALINC)

SRCS =	nwudiag.c \
	nwuipx.c \
	nwuripsa.c \
	nwuspx.c \
	nwumpsd.c

OBJS =	nwudiag.o \
	nwuipx.o \
	nwuripsa.o \
	nwuspx.o \
	nwumpsd.o

LIBS = -lsmux -lsnmpio -lsnmp

INCLUDES = 

LDLIBS = $(LIBUTIL)

all: $(LIBNWCM) $(LIBNPS) nwumpsd

install: all
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) nwumpsd

nwumpsd: $(OBJS) 
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LDOPTIONS) $(LIBS) $(LDLIBS) $(SHLIBS) -lelf -lgen -lsocket -lnsl

clean:
	rm -f *.o

clobber: clean
	rm -f nwumpsd lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done
