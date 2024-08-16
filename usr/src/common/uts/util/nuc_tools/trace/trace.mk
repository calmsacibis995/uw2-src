#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-nuc:util/nuc_tools/trace/trace.mk	1.6"
#	Copyright (c) 1993 Novell, Inc.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	Novell, Inc.   	
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/util/nuc_tools/trace/trace.mk,v 1.7 1994/06/02 16:05:18 eric Exp $"

include $(UTSRULES)

KBASE    = ../../..
LINTDIR  = $(KBASE)/lintdir
DIR      = util/nuc_tools/trace

NVLT     = nvlt.cf/Driver.o
LFILE    = $(LINTDIR)/trace.ln

FILES    = trace.o kndt.o
SRCFILES = trace.c kndt.c
HFILES   = NVLT.h traceuser.h nwctrace.h
LFILES   = trace.ln

#
#	We must be compiled with -Kframe!
#
#	Add -DSYSPRO for Compaq SystemPro/XL and ProLiant
#	MP machines.
#
LOCALDEF = -Kframe -U_KMEM_STATS -U_KMEM_HIST


all:		$(NVLT)

install:	all
			(cd nvlt.cf ; $(IDINSTALL) -R$(CONF) -M NVLT)

$(NVLT):	$(FILES)
			$(LD) -r -o $(NVLT) $(FILES)

trace.o:	trace.c $(HFILES)

kndt.o:		kndt.c $(HFILES)

clean:
			-rm -f *.o $(LFILES) *.L $(NVLT)

clobber:	clean
			-$(IDINSTALL) -e -R$(CONF) -d NVLT

$(LINTDIR):
			-mkdir -p $@


tags:
			ctags $(SRCFILES) $(HFILES)

lintit:		$(LFILE)

$(LFILE):	$(LINTDIR) $(LFILES)
			-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
			for i in $(LFILES); do \
				cat $$i >> $(LFILE); \
				cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
			done


fnames:
			@for i in $(SRCFILES); do \
				echo $$i; \
			done


sysHeaders = nwctrace.h	\
			 traceuser.h


headinstall: $(sysHeaders)
			@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
			@for f in $(sysHeaders); \
			 do \
				$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
			 done

