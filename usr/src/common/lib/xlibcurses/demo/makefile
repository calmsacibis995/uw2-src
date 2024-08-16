#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)curses:common/lib/xlibcurses/demo/makefile	1.3.2.2"
#ident "$Header: makefile 1.4 91/06/27 $"
 include $(LIBRULES)
LDLIBS=	-lcurses

all: aliens rain menu worms

aliens:	aliens.c
	$(CC) $(CFLAGS) $(DEFLIST) aliens.c -o aliens $(LDFLAGS) $(LDLIBS) $(SHLIBIS)

rain: rain.c
	$(CC) $(CFLAGS) $(DEFLIST) rain.c -o rain $(LDFLAGS) $(LDLIBS) $(SHLIBS)

menu: menu.c
	$(CC) $(CFLAGS) $(DEFLIST) menu.c -o menu $(LDFLAGS) $(LDLIBS) $(SHLIBS)

worms: worms.c
	$(CC) $(CFLAGS) $(DEFLIST) worms.c -o worms $(LDFLAGS) $(LDLIBS) $(SHLIBS)
