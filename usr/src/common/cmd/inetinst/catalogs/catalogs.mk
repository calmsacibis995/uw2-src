#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)inetinst:catalogs/catalogs.mk	1.4"
#
# catalogs.mk
# @(#)inetinst:catalogs/catalogs.mk	1.4	7/15/94
#

include $(CMDRULES)

TARGET=inetinst.msg iserver.str installsrv.str

MSG_FILES = \
	inetinst_msgs.h 

all: $(TARGET)
	rm -f ../inetinst_msgs.h
	cp $(MSG_FILES) ../

install : all 
	[ -d "$(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES" ] || mkdir -p $(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES
	-$(CP) $(TARGET) $(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES

clean :
	rm -f inetinst.cat.m
	rm -f inetinst.cat
	rm -f iserver

clobber : clean
	-rm -f inetinst.msg ../inetinst_msgs.h iserver.str installsrv.str
	-rm -f $(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES/inetinst.msg
	-rm -f $(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES/iserver.str
	-rm -f $(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES/installsrv.str

inetinst.cat : inetinst.msg
	gencat inetinst.cat inetinst.msg

inetinst.msg : inetinst_msgs.h
	chmod 555 ./hdrtomsg ./cutmsgs.sh
	./hdrtomsg inetinst_msgs.h > inetinst.msg

iserver.str : ../IS_messages.sh
	rm -f iserver.msg iserver.str
	fgrep "iserver:" ../IS_messages.sh | cut -d'"' -f2 > iserver.str

installsrv.str : ../installsrv.sh
	rm -f installsrv.msg installsrv.str
	fgrep 'installsrv:' ../installsrv.sh | cut -d'"' -f2 > installsrv.str

iserver : iserver.str
	rm -f iserver
	mkmsgs iserver.str iserver
