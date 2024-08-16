#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ypcmd:yppasswd/yppasswd.mk	1.1"
#ident  "$Header: $"

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#          All rights reserved.
# 

#	Copyright (c) 1992 Intel Corp.
#	  All Rights Reserved
#
#	INTEL CORPORATION PROPRIETARY INFORMATION
#
#	This software is supplied under the terms of a license
#	agreement or nondisclosure agreement with Intel Corpo-
#	ration and may not be copied or disclosed except in
#	accordance with the terms of that agreement.
#

include $(CMDRULES)

NETSVC = $(USRLIB)/netsvc
NETYP  = $(NETSVC)/yp
LOCALDEF=-DPORTMAP -DUSE_SHADOW -DUNIXWARE
LDLIBS=-lcrypt -lnsl -lgen
OBJS=yppasswd yppasswdd
YPPASSWDOBJ=yppasswd.o yppasswdxdr.o
YPPASSWDDOBJ=passwdd.o yppasswdxdr.o
INSTALL=install

all:	yppasswd rpc.yppasswdd

yppasswd:	$(YPPASSWDOBJ)
	$(CC) $(CFLAGS) -o $@ $(YPPASSWDOBJ) $(LDLIBS)

rpc.yppasswdd:	$(YPPASSWDDOBJ)
	$(CC) $(CFLAGS) -o $@ $(YPPASSWDDOBJ) $(LDLIBS)

passwdd.o:	passwdd.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/signal.h \
	$(INC)/rpc/rpc.h \
	$(INC)/netconfig.h\
	$(INC)/netdir.h \
	$(INC)/pwd.h \
	yppasswd.h \
	$(INC)/sys/file.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/resource.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/syslog.h \
	$(INC)/unistd.h \
	$(INC)/crypt.h \
	$(INC)/termios.h \
	$(INC)/shadow.h

yppasswd.o:	yppasswd.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/pwd.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/rpcsvc/ypclnt.h \
	yppasswd.h \
	$(INC)/sys/file.h \
	$(INC)/errno.h \
	$(INC)/netdir.h \
	$(INC)/netconfig.h \
	$(INC)/crypt.h

yppasswdxdr.o:	yppasswdxdr.c \
	$(INC)/rpc/rpc.h \
	yppasswd.h

install: all
	if [ ! -d $(NETYP) ] ; \
	then \
		mkdir $(NETYP) ; \
	fi ; \
	$(INSTALL) -f $(USRBIN) yppasswd
	$(INSTALL) -f $(NETYP) rpc.yppasswdd

clobber:	clean
	rm -f $(OBJS)

clean:	
	rm -f $(YPPASSWDOBJ) $(YPPASSWDDOBJ)

size:	$(OBJS)
	$(SIZE) $(OBJS)

strip:	$(OBJS)
	$(STRIP) $(OBJS)

