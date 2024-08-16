#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcplxterm:lxtermlib.mk	1.1.3.1"
#ident  "$Header: lxtermlib.mk 1.1 91/07/09 $"

include $(LIBRULES)

#
#	lxtermlib.mk 1.1 90/04/06 lxtermlib:lxtermlib.mk
#

OWN = root
GRP = sys

LGTXTNM=_LIBC_TEXT

LOCALDEF = -DCM_N -DCM_GT -DCM_B -DCM_D
LIBTERM = libxtermlib.a
LIBOBJS = termcap.o tgoto.o tputs.o

.PRECIOUS: $(LIBTERM)

# standard targets
all: $(LIBTERM)

$(LIBTERM): $(LIBOBJS)
	$(AR) $(ARFLAGS) $(LIBTERM) $(LIBOBJS)

install: all
	 $(INS) -f $(USRLIB) -u $(OWN) -g $(GRP) -m 644 $(LIBTERM)
	-rm -f $(USRLIB)/libxtermcap.a
	-ln $(USRLIB)/libxtermlib.a $(USRLIB)/libxtermcap.a

clean:
	rm -f $(LIBOBJS)

clobber: clean
	rm -f $(LIBTERM)
