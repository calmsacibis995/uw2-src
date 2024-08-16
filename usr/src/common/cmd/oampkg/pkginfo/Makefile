#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)oampkg:common/cmd/oampkg/pkginfo/Makefile	1.6.7.5"
#ident "$Header: Makefile 1.2 91/04/10 $"

include $(CMDRULES)

SRCBASE=../
LINTLIBS=$(USRLIB)/llib-lpkg.ln $(USRLIB)/llib-ladm.ln
LDLIBS= -lpkg -ladm -lcmd -lgen

O_SHFILES=
O_CFILES=pkginfo pkgparam pkgname
OBJECTS=$(O_CFILES) $(O_SHFILES)

all: .check $(OBJECTS)
	@:

.check:	$(INC)/pkginfo.h
	rm -f $(O_CFILES)
	touch .check

.c:
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	chmod 775 $@

clean:
	rm -f .check
	
clobber: clean
	rm -f $(O_CFILES) $(O_SHFILES)

strip:
	$(STRIP) $(O_CFILES)

lintit:
	rm -f lint.out
	for file in $(O_CFILES) ;\
	do \
		$(LINT) $(DEFLIST) $$file.c $(LINTLIBS) >>lint.out ;\
	done

include $(SRCBASE)Install
