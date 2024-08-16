#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.

#ident	"@(#)pkgtools:cmd.mk	1.3"

include $(LIBRULES)

MAKEFILE =  libcmd.mk

LIBRARY  =  libcmd.a

OBJECTS	=  magic.o sum.o deflt.o getterm.o privname.o systbl.o sttyname.o \
	tp_ops.o

SOURCES	= $(OBJECTS:.o=.c)

all:	$(LIBRARY)


# local directory, nothing to copy

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(LIBRARY)

FRC:

.PRECIOUS:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)
