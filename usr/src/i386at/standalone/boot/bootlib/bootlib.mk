#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Copyrighted as an unpublished work.
#	(c) Copyright 1989 INTERACTIVE Systems Corporation
#	All rights reserved.

#	RESTRICTED RIGHTS

#	These programs are supplied under a license.  They may be used,
#	disclosed, and/or copied only as permitted under such license
#	agreement.  Any copy must contain the above copyright notice and
#	this restricted rights notice.  Use, copying, and/or disclosure
#	of the programs is strictly prohibited unless otherwise provided
#	in the license agreement.

#ident	"@(#)stand:i386at/standalone/boot/bootlib/bootlib.mk	1.5"
#ident  "$Header: $"

include $(CMDRULES)

LOCALINC = -I ..

#
#	We do not wish to use all the capabilities of
#	the optimizer.  So, we do not use CFLAGS on
#	the compile lines.  Instead, we set our own
#	first-pass options in BOOTOPTIM and use it.

BOOTOPTIM = -W0,-Lb -K no_host


LIBOBJS = getfhdr.o blfile.o bfs.o sbfile.o sbfs.o btdcmp.o

all: bootlib.o 
	
bootlib.o: $(LIBOBJS)
	${LD} -r -o bootlib.o $(LIBOBJS)

#	Special Dependencies

$(LIBOBJS): FRC
	${CC} ${BOOTOPTIM} ${INCLIST} $(DEFLIST) -c $*.c

install: all 

clobber clean:
	-/bin/rm -f $(LIBOBJS) *.o

FRC: 
