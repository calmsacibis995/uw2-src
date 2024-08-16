#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libtelnet/libtelnet.mk	1.2"
#ident	"$Header: $"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#


include $(LIBRULES)
DEVINC1 = -U_REENTRANT

OWN=bin
GRP=bin

OBJECTS=   	auth.o encrypt.o genget.o misc.o setenv.o

LIBRARY=libtelnet.a

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

install: all
	$(INS) -f $(USRLIB) -m 0755 -u $(OWN) -g $(GRP) $(LIBRARY)

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(LIBRARY)

#
# Header dependencies
#


auth.o:		auth.c \
		encrypt.h \
		auth.h\
		misc-proto.h \
		$(FRC)

encrypt.o:	encrypt.c \
		encrypt.h \
		misc.h \
		$(FRC)

misc.o:		misc.c \
		misc.h \
		$(FRC)
