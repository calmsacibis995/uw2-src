#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)drf:prt_files/prt_files.mk	1.6"

include $(CMDRULES)

PFILES  = funcrc helpwin staticlist step1rc step2rc

INSDIR    = $(USRLIB)/drf

all: $(PFILES) 

helpwin:
	@ln -s $(PROTO)/desktop/menus/$@ $@

step1rc:
	@ln -s $(PROTO)/desktop/scripts/$@ $@

step2rc:
	@ln -s $(PROTO)/desktop/scripts/$@ $@

staticlist:
	@ln -s $(PROTO)/desktop/$@ $@

funcrc:
	@ln -s $(PROTO)/desktop/scripts/$@ $@

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(PFILES) ;\
	do \
		$(INS) -f $(INSDIR) $$i ;\
	done

clean:
	rm -f $(PFILES) 

clobber: clean
