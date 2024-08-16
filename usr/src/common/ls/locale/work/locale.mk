#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)langsup:common/ls/locale/work/locale.mk	1.1"

include $(CMDRULES)

# LOCALE must be set on entry

SRCDIR  = $(HOMEDIR)/locale/$(LOCALE)
LCDIR	= $(PKGDIR)/usr/lib/locale/$(LOCALE)
LCXDIR	= $(PKGDIR)/usr/X/lib/locale/$(LOCALE)

CHRTBL	= ctype.text
COLLTBL	= dict.text
MONTBL	= money.text
TIMETBL	= LC_TIME
MSGTBL	= nl_lang.text
LOCDEF	= locale_def

TARGETS	= LC_CTYPE LC_NUMERIC LC_COLLATE LC_MONETARY LC_TIME \
	  Xopen_info $(LOCDEF)

all	: $(TARGETS)

install	: all
	@if [ ! -d $(LCDIR) ] ; then mkdir -p $(LCDIR) ; fi
	@if [ ! -d $(LCDIR)/LC_MESSAGES ] ; then mkdir $(LCDIR)/LC_MESSAGES; fi
	cp LC_CTYPE LC_NUMERIC LC_MONETARY LC_COLLATE LC_TIME $(LCDIR)
	cp Xopen_info $(LCDIR)/LC_MESSAGES
	@if [ -r $(LOCDEF) ] ; then cp $(LOCDEF) $(LCDIR)/$(LOCDEF) ; fi
	rm -f $(TARGETS)
	rm -f wctype.c
	rm -f $(LOCDEF)

LC_CTYPE LC_NUMERIC	:
	cp $(SRCDIR)/$(CHRTBL) .
	wchrtbl $(CHRTBL)
	rm -f $(CHRTBL)

LC_MONETARY	:
	cp $(SRCDIR)/$(MONTBL) .
	montbl $(MONTBL)
	rm -f $(MONTBL)

LC_COLLATE	:
	cp $(SRCDIR)/$(COLLTBL) .
	colltbl $(COLLTBL)
	rm -f $(COLLTBL)

LC_TIME	:
	cp $(SRCDIR)/$(TIMETBL) LC_TIME

$(LOCDEF):
	@if [ -r $(SRCDIR)/$(LOCDEF) ] ; then cp $(SRCDIR)/$(LOCDEF) . ; fi

Xopen_info	:
	mkmsgs -o $(SRCDIR)/$(MSGTBL) Xopen_info

clean	:
	rm -f $(TARGETS) ctype.c wctype.c Xopen_info

clobber	: clean
