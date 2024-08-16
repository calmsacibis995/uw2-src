#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/nwdiscover/nwdiscover.mk	1.5"
#ident	"$Id: nwdiscover.mk,v 1.12 1994/05/10 20:45:30 mark Exp $"

include $(CMDRULES)

TOP = ../..

include $(TOP)/local.def

LINTFLAGS = $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

LDLIBS = $(LIBUTIL)

INSDIR = $(USRSBIN)

SRCS = nwdiscover.c
OBJS = nwdiscover.o

all: nwdiscover


nwdiscover: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

nwdiscover.o: nwdiscover.c \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/dlpi.h \
	$(INC)/sys/dlpi_ether.h \
	$(INC)/sys/ipx_app.h

install: all
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) nwdiscover

clean:
	rm -f $(OBJS)
	rm -f *.ln
	
clobber: clean
	rm -f nwdiscover
	rm -f lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
        echo '## lint output for nwdiscover.c ##' >>lint.out ; \
	$(LINT) $(LINTFLAGS) nwdiscover.c $(LINTLIBS) >>lint.out
