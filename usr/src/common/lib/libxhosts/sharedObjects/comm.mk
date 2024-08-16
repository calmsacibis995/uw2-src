#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libhosts:sharedObjects/comm.mk	1.1"
all clean clobber lintit:
	cd system; make -f *.mk $@
	cd nis; make -f *.mk $@
	cd dns; make -f *.mk $@

install: $(ROOT)/$(MACH)/usr/lib/trees/hosts
	cd system; make -f *.mk $@
	cd nis; make -f *.mk $@
	cd dns; make -f *.mk $@

localinstall: /usr/lib/trees/hosts
	cd system; make -f *.mk $@
	cd nis; make -f *.mk $@
	cd dns; make -f *.mk $@

$(ROOT)/$(MACH)/usr/lib/trees/hosts:
	if [ ! -d $@ ] ; then mkdir -m 775 -p $@ ; fi
	if [ ! -d $@ ] ; then chown bin $@ ; fi
	if [ ! -d $@ ] ; then chgrp mail $@ ; fi

/usr/lib/trees/hosts:
	if [ ! -d $@ ] ; then mkdir -m 775 -p $@ ; fi
	if [ ! -d $@ ] ; then chown bin $@ ; fi
	if [ ! -d $@ ] ; then chgrp mail $@ ; fi
