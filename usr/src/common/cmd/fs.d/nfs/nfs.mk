#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nfs.cmds:nfs.mk	1.1.1.5"
#ident	"$Header: $"

#
# makefile for nfs.cmds
#
# These are the nfs specific subcommands for the generic distributed file
# system administration commands, along with many other nfs-specific
# administrative commands
#

include $(CMDRULES)

OWN = bin
GRP = bin
MSGDIR = $(USRLIB)/locale/C/MSGFILES

COMMANDS=automount biod bootpd dfmounts dfshares exportfs mount mountd nfsd share showmount umount unshare statd lockd nfsstat pcnfsd nfsping

install:
	@for i in $(COMMANDS);\
		do cd $$i;\
		echo "====> $(MAKE) -f $$i.mk $@" ;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@;\
		cd ..;\
	done;
	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) -m 0644 -u $(OWN) -g $(GRP) nfscmds.str

.DEFAULT:
	@for i in $(COMMANDS);\
		do cd $$i;\
		echo "====> $(MAKE) -f $$i.mk $@" ;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@;\
		cd ..;\
	done;
