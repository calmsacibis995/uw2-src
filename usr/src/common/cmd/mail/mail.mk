#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/mail.mk	1.47.1.21"
include $(CMDRULES)
SHELL=/usr/bin/sh

all clean clobber lintit:
	cd config; make -f *.mk $@
	cd mailalias; make -f *.mk $@
	cd mailrevalias; make -f *.mk $@
	cd mhs; make -f *.mk $@
	cd smtpd; make -f *.mk $@
	cd smtp; make -f *.mk $@
#	cd mail; make -f mail.mk $@
	make -f comm.mk $@
	cd mailproc; make -f mailproc.mk $@
	cd metamail; make -f metamail.mk $@
	cd pathrouter; make -f pathrouter.mk $@
	cd popper; make -f popper.mk $@

install: $(ETC)/mail $(USRBIN)
	cd config; make -f *.mk $@
	cd mailalias; make -f *.mk $@
	cd mailrevalias; make -f *.mk $@
	cd mhs; make -f *.mk $@
	cd smtpd; make -f *.mk $@
	cd smtp; make -f *.mk $@
#	cd mail; make -f mail.mk $@
	make -f comm.mk $@
	cd mailproc; make -f mailproc.mk $@
	cd metamail; make -f metamail.mk $@
	cd pathrouter; make -f pathrouter.mk $@
	cd popper; make -f popper.mk $@

localinstall: /usr/lib/mail/surrcmd
	cd config; make -f *.mk $@
	cd mailalias; make -f *.mk $@
	cd mailrevalias; make -f *.mk $@
	cd mhs; make -f *.mk $@
	cd smtpd; make -f *.mk $@
	cd smtp; make -f *.mk $@
#	cd mail; make -f comm.mk $@
	make -f comm.mk $@
	cd mailproc; make -f mailproc.mk $@
	cd metamail; make -f metamail.mk $@
	cd pathrouter; make -f pathrouter.mk $@
	cd popper; make -f popper.mk $@

/usr/lib/mail/surrcmd /usr/lib/mail $(ETC)/mail $(USRBIN):
	mkdir -p $@
	$(CH)chown bin $@
	$(CH)chgrp mail $@
	$(CH)chmod 0775 $@
