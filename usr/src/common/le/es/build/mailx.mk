#ident	"@(#)es_le:common/le/es/build/mailx.mk	1.4"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= es

SRCDIR  = ../runtime/usr/share/lib/mailx/$(LOCALE)
MLXDIR	= $(ROOT)/$(MACH)/usr/share/lib/mailx/$(LOCALE)

SRCS	= $(SRCDIR)/mailx.help $(SRCDIR)/mailx.help.~

all:

install: $(SRCS)
	[ -d $(MLXDIR) ] || mkdir -p $(MLXDIR)
	for i in $(SRCS);\
	do\
		$(INS) -f $(MLXDIR) $$i;\
	done

clean:

clobber:
