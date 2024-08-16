#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/hostmibd/hostmibd.mk	1.9"
#ident "$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/hostmibd/hostmibd.mk,v 1.14 1994/05/24 18:39:16 cyang Exp $"
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

include ${CMDRULES}

TOP = ../..

include $(TOP)/local.def

LOCALINC=-I$(INC)/netmgt -DNETWARE -DSVR4 -D_KMEMUSER

LINTFLAGS = $(GLOBALINC) $(LOCALINC)

SRCS = hostmibd.c \
	hrsystem.c \
	hrstorage.c \
	hrdevice.c \
	metrics.c
#	peek.c
#	hrswrun.c \
#	hrswrunperf.c \
#	hrswinstalled.c \


OBJS = hostmibd.o \
	hrsystem.o \
	hrstorage.o \
	hrdevice.o \
	metrics.o
#	peek.o
#	hrswrun.o \
#	hrswrunperf.o \
#	hrswinstalled.o \


LDLIBS= -lmas -lsocket -lnsl -lsmux -lsnmpio -lsnmp -lelf -lgen

INCLUDES = 

all: hostmibd

install: all
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) hostmibd

hostmibd: $(OBJS) $(LIBS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LDOPTIONS) $(LIBS) $(LDLIBS) $(SHLIBS) 

clean:
	rm -f *.o

clobber: clean
	rm -f hostmibd lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done
