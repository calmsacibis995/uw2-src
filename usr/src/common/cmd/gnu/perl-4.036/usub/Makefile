#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

SRC = ..
GLOBINCS = 
LOCINCS = 
LIBS = -lcurses -ltermlib `. $(SRC)/config.sh; echo $$libs`

curseperl: $(SRC)/uperl.o usersub.o curses.o
	cc $(SRC)/uperl.o usersub.o curses.o $(LIBS) -o curseperl

usersub.o: usersub.c
	cc -c -I$(SRC) $(GLOBINCS) -DDEBUGGING -g usersub.c

curses.o: curses.c
	cc -c -I$(SRC) $(GLOBINCS) -DDEBUGGING -g curses.c

curses.c: curses.mus
	mus curses.mus >curses.c
