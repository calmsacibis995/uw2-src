#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mailx:svr3.mk	1.5.2.4"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# "@(#)svr3.mk	1.8 'attmail mail(1) command'"
#
# mailx -- a modified version of a University of California at Berkeley
#	mail program
#
# for standard Unix
#

# If system == SVR3 && !SUN4.1, use the following...
CPPDEFS = -DpreSVr4 -I$(HDR) -I$(CRX)/usr/include -I$(ROOT)/usr/include
CFLAGS  = -O $(CPPDEFS)
#CFLAGS  = -g $(CPPDEFS)
USRLIB= /usr/lib
MBINDIR=/usr/bin
USRBIN= /usr/bin
ETC= /etc
VAR= /var
USRSHARE= /usr/share
#LD_FLAGS = -s
LD_FLAGS =
LINT= lint
INS=	install

VERS	 = SVR3
DESTLIB = $(ROOT)/usr/lib/mailx
LDESTLIB = $(DESTLIB)
#LDFLAGS = -s
LDFLAGS =
LD_LIBS = -L$(ROOT)/usr/lib -lmail
SYMLINK = :
RCDIR = $(ROOT)/etc/mail
LRCDIR = /etc/mail

include comm.mk
