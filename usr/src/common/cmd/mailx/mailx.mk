#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mailx:mailx.mk	1.21.6.7"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

# "@(#)mx.mk	1.25 'attmail mail(1) command'"
#
# mailx -- a modified version of a University of California at Berkeley
#	mail program
#
# for standard Unix
#

# first, in case $CMDRULES is empty, we define
# the things that $CMDRULES is supposed to supply.
#CFLAGS= -g $(LOCALDEF) $(LOCALINC)
CFLAGS= -O $(LOCALDEF) $(LOCALINC)
USRLIB= /usr/lib
MBINDIR=/usr/bin
USRBIN= /usr/bin
ETC= /etc
VAR= /var
USRSHARE= /usr/share
LD_FLAGS = -s $(LDFLAGS) $(PERFLIBS)
LINT= lint -s
INS=	install

include $(CMDRULES)

# If system == SVR4ES, use the following...
VERS	 = SVR4ES
DESTLIB = $(USRSHARE)/lib/mailx/C
LDESTLIB = /usr/share/lib/mailx/C
LOCALINC=-Ihdr -I$(ROOT)/$(MACH)/usr/include
LOCALDEF=-Xa -D$(VERS)
LD_LIBS = -lmail -lw -lgen $(PERFLIBS)
RCDIR=$(ETC)/mail
LRCDIR=/etc/mail
SYMLINK = ln -s

include comm.mk
