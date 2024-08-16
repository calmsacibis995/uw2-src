#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/mipx/mipx.mk	1.3"

# 	Copyright (c) 1992 Univel(r)
#	All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Univel(r).
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.
include $(UTSRULES)

KBASE = ../..

MIPX = mipx.cf/Driver.o

OBJS	= mipx.o

all:	$(MIPX)

$(MIPX): $(OBJS) mipx.h
	$(LD) -r -o $(MIPX) $(OBJS)

#
# Configuration Section
#

install: all
	cd mipx.cf;       $(IDINSTALL) -R$(CONF) -M mipx

headinstall:

clean:
	-rm -f *.o $(MIPX)

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d mipx

#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)
