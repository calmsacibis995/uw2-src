#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/re/makefile	1.3.3.5"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident "@(#)makefile	1.8 'attmail mail(1) command'"

LOCALINC = -I.
LOCALDEF = -D$(VERS) $(VERS2) -DPOSSTEP=64 -DMINSTATE=4
DEBUG=
OPTIMIZER= -O
REFLAGS= -v
CFLAGS= $(LOCALDEF) $(LOCALINC) $(REFLAGS) $(OPTIMIZER) $(DEBUG)

include $(CMDRULES)

LINT= lint -s

RESRC = bm.c cw.c eg.c egbr.c egcanon.c egcomp.c egcw.c egerror.c eglit.c \
	egmatch.c egparen.c egpos.c egstate.c re.c refile.c
REOBJS = $(RESRC:.c=.o)

LIBRE= ../libre.a

$(LIBRE): $(REOBJS)
	$(AR) $(ARFLAGS) $(LIBRE) $?

libre.a: $(LIBRE)

$(REOBJS): re.h lre.h io.h libc.h

lintit:
	$(LINT) $(LOCALDEF) $(LOCALINC) $(RESRC)

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(LIBRE)
