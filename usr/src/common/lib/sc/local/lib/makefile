#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:local/lib/makefile	3.4" 
###############################################################################
#
# C++ Standard Components, Release 3.0.
#
# Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
# Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
#
# THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
# Laboratories, Inc.  The copyright notice above does not evidence
# any actual or intended publication of such source code.
#
###############################################################################

CC=/fs/seashore/chome/486/snowbird/bin/CC

# Set this to the flavor of your operating system.
# Set it to 0 for systems that are like UNIX System V, 
# such as Amdahl UTS and HP-UX.  Set it to 1 for systems 
# that are like Berkeley UNIX, such as SunOS.
BSD=0

# Here you should specify any special options you need
# to pass to the CC command.
CCFLAGS= 

############################################################
# You shouldn't have to change anything below this line
############################################################
SHELL=/bin/sh

all:	force
	@-echo "You must make one of {sc3.0,demos,clean,clobber}" 1>&2

sc3.0:	aux/fnewer force
	./aux/domake -a $(CC) $(BSD) "$(CCFLAGS)"

demos:	aux/fnewer force
	./aux/domake -d $(CC) $(BSD) "$(CCFLAGS)"

tests:	aux/fnewer force
	./aux/domake -T $(CC) $(BSD) "$(CCFLAGS)"

aux/fnewer:	aux/fnewer.c
	$(CC) $(CCFLAGS) -o aux/fnewer aux/fnewer.c

clean:	force
	./aux/domake -c $(CC) $(BSD) "$(CCFLAGS)"

clobber:	force
	./aux/domake -K $(CC) $(BSD) "$(CCFLAGS)"

cpio:	force
	cpio -oc <files >sc3.0.cpio

force:	

