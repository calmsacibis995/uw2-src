#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mergeipx:merge_ipx.mk	1.2"

# 	Copyright (c) 1992 Univel(r)
#	All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Univel(r).
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

include $(CMDRULES)

OWN = bin
GRP = bin


all: binary 

binary: mipxadm

mipxadm:	mipxadm.c mpip_usr.h merge.h
	$(CC) -o mipxadm $(LCLINCLUDE) -DSVR4 mipxadm.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(ROOT)/$(MACH)/var/spool/merge_ipx ] || mkdir -p $(ROOT)/$(MACH)/var/spool/merge_ipx
	[ -d $(ROOT)/$(MACH)/usr/bin ] || mkdir -p $(ROOT)/$(MACH)/usr/bin
	$(INS) -f $(ROOT)/$(MACH)/usr/bin -u $(OWN) -g $(GRP) -m 0555 mipxadm
	if [ -f ipxtli.msg ] ;\
	then \
		$(INS) -f $(ROOT)/$(MACH)/usr/bin -u $(OWN) -g $(GRP) -m 0555 ipxtli.msg ;\
	fi
	if [ -f ipxtli.com ] ;\
	then \
		$(INS) -f $(ROOT)/$(MACH)/usr/bin -u $(OWN) -g $(GRP) -m 0555 ipxtli.com ;\
	fi
	if [ -f netx.exe ] ;\
	then \
		$(INS) -f $(ROOT)/$(MACH)/usr/bin -u $(OWN) -g $(GRP) -m 0555 netx.exe ;\
	fi



#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)
clean:
	-rm -f mipxadm.o

clobber:	clean
	-rm -f mipxadm
