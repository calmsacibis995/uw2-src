#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/eqn/eqn.mk	1.3"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


#	eqn/neqn/eqnchar make file
#

include $(CMDRULES)

compile all:  eqn neqn

neqn:
	cd neqn.d; $(MAKE) -f neqn.mk neqn ROOT=$(ROOT) \
		LDFLAGS="$(LDFLAGS)"

eqn:
	cd eqn.d; $(MAKE) -f eqn.mk eqn  ROOT=$(ROOT) \
		LDFLAGS="$(LDFLAGS)"

install:
	cd eqnchar.d; $(MAKE) -f eqnchar.mk install ROOT=$(ROOT)
	cd neqn.d; $(MAKE) -f neqn.mk install ROOT=$(ROOT) LDFLAGS="$(LDFLAGS)"
	cd eqn.d; $(MAKE) -f eqn.mk install ROOT=$(ROOT) LDFLAGS="$(LDFLAGS)"


inseqnchar:
	cd eqnchar.d; $(MAKE) -f eqnchar.mk install ROOT=$(ROOT)

insneqn:
	cd neqn.d; $(MAKE) -f neqn.mk install ROOT=$(ROOT) LDFLAGS="$(LDFLAGS)"

inseqn:
	cd eqn.d; $(MAKE) -f eqn.mk install ROOT=$(ROOT) LDFLAGS="$(LDFLAGS)"

clean:
	cd neqn.d;  $(MAKE) -f neqn.mk clean
	cd eqn.d;  $(MAKE) -f eqn.mk clean

clobber:
	cd neqn.d;  $(MAKE) -f neqn.mk clobber
	cd eqn.d;  $(MAKE) -f eqn.mk clobber
