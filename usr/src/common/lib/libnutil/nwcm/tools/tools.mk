#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/tools/tools.mk	1.11"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id: tools.mk,v 1.25 1994/09/08 18:32:16 vtag Exp $"

include $(LIBRULES)

include ../../local.def

MCC = $(HCC)

LOCALINC = \
	-DNWCM_FRIEND \
	-I../../nls/English \
	-I../../nls/English/nwnet \
	-I../../nls/English/netmgt \
	-I../../nls/English/nuc \
	-I../../nls/English/nprinter

TOP = ../..

LINTFLAGS = $(WORKINC) $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

LDLIBS = -lm

OBJS = scomp.o sc_parser.o sc_scanner.o
SRC2 = binbuild.c
OBJ2 = $(SRC2:.c=.o)

.MUTEX:	$(OBJS) $(OBJ2)

all: scomp $(OBJ2)

scomp: $(OBJS) 
	$(MCC) -o $@ $(OBJS)  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

scomp.o:	scomp.c \
	../../nls/English/nwnet/nwcmmsgs.h \
	./sc_parser.h \
	./sc_tune.h

scomp.o:	scomp.c
	$(MCC) -c $(CFLAGS) $(DEFLIST) scomp.c 

$(OBJ2): $(SRC2)
	$(MCC) -c $(CFLAGS) $(DEFLIST) $(SRC2)

install: all

clean:
	rm -f y.tab.? lex.yy.* sc_scanner.c *.o *.ln

clobber: clean
	rm -f scomp lint.out

lintit:
	-@rm -f lint.out ; \
	echo '## lint output for scomp.c ##' >>lint.out ; \
	$(LINT) $(LINTFLAGS) scomp.c $(LINTLIBS) >>lint.out
