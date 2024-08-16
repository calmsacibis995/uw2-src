#ident	"@(#)es_le:common/le/es/build/app.mk	1.8"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= es

APPSRC = ../runtime/usr/X/lib/locale/$(LOCALE)/app-defaults

APPFILES = Bitmap DesktopMgr Editres Mwm Spider Viewres XClipboard XFontSel \
	XLock XMdemos XMtravel XTerm Xfd Xmag Xman Xtetris Xtetris.bw \
	Xtetris.c moolit olam_msgs olpix_msgs olps_msgs \
	s_sampler xterm_msgs

LEOPTFILES = DtCalc

APPDIR = $(ROOT)/$(MACH)/usr/X/lib/locale/$(LOCALE)/app-defaults

LEOPT = $(ROOT)/$(MACH)/var/opt/$(LOCALE)le

all:

install: $(APPSRC)
	[ -d $(APPDIR) ] || mkdir -p $(APPDIR)
	for i in $(APPFILES) ;\
	do \
		$(INS) -f $(APPDIR) $(APPSRC)/$$i ;\
	done
	[ -d $(LEOPT) ] || mkdir -p $(LEOPT)
	for i in $(LEOPTFILES) ;\
	do \
		$(INS) -f $(LEOPT) $(APPSRC)/$$i ;\
	done
	
clean:

clobber:

