#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/lib/lib.mk	1.1"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/lib/lib.mk,v 1.4 1994/03/25 22:51:02 vtag Exp $"

include $(LIBRULES)

TOP = ../..

include $(TOP)/local.def

LOCALDEF = -DSVR4 -DLO_HI_MACH_TYPE

LOCALINC = -I../include

LINTFLAGS = $(GLOBALINC) $(LOCALINC)

LIBRARY = print

LIBDEST = .

SRCS = fdpoll.c \
	spxapi.c \
	spxtdr.c \
	lprapi.c

OBJS = $(SRCS:.c=.o)

all: $(LIBDEST)/lib$(LIBRARY).a

install: all

$(LIBDEST)/lib$(LIBRARY).a:	$(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) $(LDLIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f $(LIBDEST)/lib$(LIBRARY).a lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done
