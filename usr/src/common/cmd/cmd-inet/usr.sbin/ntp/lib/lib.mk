#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/lib.mk	1.2"
#ident	"$Header: $"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#


include $(LIBRULES)

OWN=bin
GRP=bin

LOCALDEF=	-DSYSV
LOCALINC=	-I../include

OBJECTS=	atoint.o atolfp.o atouint.o auth12crypt.o authdecrypt.o \
		authdes.o authencrypt.o authkeys.o authparity.o \
		authreadkeys.o authusekey.o buftvtots.o caljulian.o \
		calleapwhen.o caltontp.o calyearstart.o clocktime.o dofptoa.o \
		dolfptoa.o emalloc.o fptoa.o fptoms.o getopt.o gettstamp.o \
		hextoint.o hextolfp.o humandate.o inttoa.o lfptoa.o lfptoms.o \
		lib_strbuf.o mfptoa.o mfptoms.o modetoa.o mstolfp.o \
		msutotsf.o ntoa.o ntohost.o numtoa.o numtohost.o octtoint.o \
		prettydate.o rand.o tsftomsu.o tstotv.o tvtoa.o tvtots.o \
		ufptoa.o ufptoms.o uglydate.o uinttoa.o ulfptoa.o ulfptoms.o \
		umfptoa.o umfptoms.o utvtoa.o

LIBRARY=libntp.a

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

install: all

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(LIBRARY)

#
# Header dependencies
#

