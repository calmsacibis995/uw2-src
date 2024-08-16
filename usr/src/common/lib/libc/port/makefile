#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-port:makefile	1.17.6.7"
#
# makefile for libc/port
#
#
# The macro PROF is null by default, causing profiled object to be maintained.
# If profiled object is not desired, the reassignment PROF=@# should appear in
# the make command line.
#

include $(LIBRULES)

.SUFFIXES: .p .P
PROF=
NONPROF=
PIC=
SDEFLIST=

.MUTEX:	prof pic nonprof

all:	prof pic nonprof

nonprof:
	$(NONPROF)cd fmt; $(MAKE) $(IGN) nonprof PROF=@# PIC=@#
	$(NONPROF)cd gen; $(MAKE) $(IGN) nonprof PROF=@# PIC=@#
	$(NONPROF)cd re; $(MAKE) $(IGN) nonprof PROF=@# PIC=@#
	$(NONPROF)cd stdio; $(MAKE) $(IGN) nonprof PROF=@# PIC=@#
	$(NONPROF)cd str; $(MAKE) $(IGN) nonprof PROF=@# PIC=@#
	$(NONPROF)cd sys; $(MAKE) $(IGN) nonprof PROF=@# PIC=@#
prof:
	$(PROF)cd fmt; $(MAKE) $(IGN) prof NONPROF=@# PIC=@#
	$(PROF)cd gen; $(MAKE) $(IGN) prof NONPROF=@# PIC=@#
	$(PROF)cd re; $(MAKE) $(IGN) prof NONPROF=@# PIC=@#
	$(PROF)cd stdio; $(MAKE) $(IGN) prof NONPROF=@# PIC=@#
	$(PROF)cd str; $(MAKE) $(IGN) prof NONPROF=@# PIC=@#
	$(PROF)cd sys; $(MAKE) $(IGN) prof NONPROF=@# PIC=@#
pic:
	$(PIC)cd fmt; $(MAKE) $(IGN) pic PROF=@# NONPROF=@#
	$(PIC)cd gen; $(MAKE) $(IGN) pic PROF=@# NONPROF=@#
	$(PIC)cd re; $(MAKE) $(IGN) pic PROF=@# NONPROF=@#
	$(PIC)cd stdio; $(MAKE) $(IGN) pic PROF=@# NONPROF=@#
	$(PIC)cd str; $(MAKE) $(IGN) pic PROF=@# NONPROF=@#
	$(PIC)cd sys; $(MAKE) $(IGN) pic PROF=@# NONPROF=@#
	
clean:

clobber: clean
	-find . \( -name '*.o' -o -name '*.p' -o -name '*.P' \) \
		-print | xargs rm -f
	-rm -f gen/new_list.c
	-rm -f gen/errlst.c
